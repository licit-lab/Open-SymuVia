#include "stdafx.h"
#include "Segment.h"

#include "frontiere.h"
#include "reseau.h"
#include "DocAcoustique.h"
#include "voie.h"
#include "tuyau.h"
#include "SystemUtil.h"
#include "TraceDocTrafic.h"
#include "vehicule.h"

#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/vector.hpp>

using namespace std;

const double SegmentMacro::EPSILON_01 = 1.0E-3;

//================================================================
    Segment::Segment
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 23/10/2006
// Historique: 23/10/2006 (C.Bécarie - Tinea)
//             Reprise
//================================================================
(
)
{
    // remarque : on initialise ces tableaux dans tous les cas 
    // pour ne pas avoir de pointeur null qui fait planter la serialisation
    // remarque 2 : un peu nul point de vue conso mémoire sur des grands réseaux ? à reprendre ?
    Lw_cellule      = new double[Loi_emission::Nb_Octaves+1];

    W_cellule       = new double[Loi_emission::Nb_Octaves+1];

    InitAcousticVariables();
}

//================================================================
    Segment::Segment
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 13/11/2006
// Historique: 13/11/2006 (C.Bécarie - Tinea)
//             Ajout de la gestion d'un ID des segments
//================================================================
(
    int     nID,
    double  dbVitReg
    ):Troncon(dbVitReg,DBL_MAX,"")
{
    m_nID = nID;

    Lw_cellule      = new double[Loi_emission::Nb_Octaves+1];
    
    W_cellule       = new double[Loi_emission::Nb_Octaves+1];

    InitAcousticVariables();
}

//================================================================
    Segment::~Segment
//----------------------------------------------------------------
// Fonction  : Destructeur
// Version du: 23/10/2006
// Historique: 23/10/2006 (C.Bécarie - Tinea)
//             Reprise
//================================================================
(
)
{
    delete [] Lw_cellule;
    Lw_cellule = NULL;

    delete [] W_cellule;
    W_cellule = NULL;
}

//================================================================
    void Segment::EmissionOutput
//----------------------------------------------------------------
// Fonction  : Sortie des calculs acoustique
// Version du: 12/10/2006
// Historique: 12/10/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(        
)
{
    for(int i=0; i<=Loi_emission::Nb_Octaves; i++)
    {
        // Le calcul s'est effectué en puissance, on le transforme en niveau de puissance
        if(W_cellule)
        {
            if( W_cellule[i] > 0)
            {
                Lw_cellule[i] = 10 * log10 (W_cellule[i] / pow((double)10, (double)-12) );
            }
            else
                Lw_cellule[i] = 0;
        }
    }    

	double *dbTest;
    bool bOK =  false;

	dbTest = new double[Loi_emission::Nb_Octaves+1];
    for(int i=0; i<=Loi_emission::Nb_Octaves; i++)
    {
        dbTest[i] = Lw_cellule[i];
        if(Lw_cellule[i]>0.000001)
            bOK = true;
    }
    if(bOK)
    {
		switch(m_pReseau->GetSymMode())
		{
		case Reseau::SYM_MODE_FULL:
			if(m_pReseau->IsXmlOutput())
				m_pReseau->m_XmlDocAcoustique->AddCellSimu( m_nID, dbTest);
			break;
		case Reseau::SYM_MODE_STEP_EVE:
        case Reseau::SYM_MODE_STEP_JSON:
		case Reseau::SYM_MODE_STEP_XML:
			std::string sTuyau = GetReseau()->GetLibelleTronconEve( (Tuyau*)GetParent()->GetParent(), ((VoieMicro*)GetParent())->GetNum() );
			m_pReseau->m_XmlDocAcoustique->AddCellSimuEx( m_nID, dbTest, sTuyau, GetAbsAmont(), GetOrdAmont(), GetHautAmont(), GetAbsAval(), GetOrdAval(), GetHautAval() );
			break;
		}
    }

	delete [] dbTest;
}

//================================================================
    void Segment::InitAcousticVariables
//----------------------------------------------------------------
// Fonction  : Initialisation des variables acoustiques à chaque
//             pas de temps
// Version du: 17/10/2006
// Historique: 17/10/2006 (C.Bécarie - Tinea)
//================================================================
(
)
{
    for (int i = 0; i <= Loi_emission::Nb_Octaves; i++)
    {
        W_cellule[i] = 0;
        Lw_cellule[i] = 0;
    }
}

//================================================================
    void Segment::AddPuissanceAcoustique
//----------------------------------------------------------------
// Fonction  : Ajoute la puissance acoustique d'une véhicule à celle
//             de la cellule
// Version du: 17/10/2006
// Historique: 17/10/2006 (C.Bécarie - Tinea)            
//================================================================
(
    double* dbPuissanceAcoustique    
)
{
    VoieMicro*  pVoie;
    TuyauMicro* pTuyau;

    pVoie = (VoieMicro*)GetParent();
    if(!pVoie)
        return;

    pTuyau = (TuyauMicro*)pVoie->GetParent();
    if(!pTuyau)
        return;
        
    for(int i=0; i<=Loi_emission::Nb_Octaves; i++)
        W_cellule[i] += dbPuissanceAcoustique[i] / (GetLength());
}


//---------------------------------------------------------------------------
// COnstructeurs, destructeurs et assimilés
//---------------------------------------------------------------------------

// Destructeur
SegmentMacro::~SegmentMacro()
{
        cell_amont=NULL;
        cell_aval=NULL;

         m_pFrontiereAmont = NULL;
         m_pFrontiereAval = NULL;
}

// Constructeur par défaut
SegmentMacro::SegmentMacro()
{
        cell_amont=NULL;
        cell_aval=NULL;

        m_pFrontiereAmont   = NULL;
        m_pFrontiereAval    = NULL;

};

// Constructeur normal
SegmentMacro::SegmentMacro
(
	std::string	sLabel,
    double  _pasespace,
    double  _pastemps,
    int     _nbVoies,    
    Frontiere *pFAmont,
    Frontiere *pFAval,
    int     &nLastID,
    double  dbVitReg
):Segment(nLastID, dbVitReg)
{
 //constructeur appelé dans la segmentation
        m_strLabel = sLabel;

        m_dbPasEspace=_pasespace;
        m_dbPasTemps=_pastemps;
        Nb_Voies=_nbVoies;

        m_pFrontiereAmont   = NULL;
        m_pFrontiereAval    = NULL;

        cell_amont=NULL;
        cell_aval=NULL;

        //Lw_cellule      = new double[Loi_emission::Nb_Octaves+1];
      
        m_pFrontiereAmont = pFAmont;
        m_pFrontiereAval = pFAval;
}

//---------------------------------------------------------------------------
// Fonctions de simulation des segments
//---------------------------------------------------------------------------

//  Initialisation de la simulation
bool Segment::InitSimulation(bool bCalculAcoustique, bool bSirane, std::string strTuyau)
{
    Tuyau*  pTuyau;

    pTuyau = (Tuyau*)GetParent()->GetParent();
    if(!pTuyau)
        return false;

    // Mise à jour des documents XML
    if(!bCalculAcoustique )
    {	    	    
        if(pTuyau->IsMacro())
        {
            std::deque< TimeVariation<TraceDocTrafic> >::iterator itXmlDocTrafic;
            for( itXmlDocTrafic = m_pReseau->m_xmlDocTrafics.begin(); itXmlDocTrafic!= m_pReseau->m_xmlDocTrafics.end(); ++itXmlDocTrafic)
            {
                itXmlDocTrafic->m_pData->AddCellule( m_nID, m_strLabel, strTuyau, GetAbsAmont(), GetOrdAmont(), GetHautAmont(), GetAbsAval(),GetOrdAval(), GetHautAval() );	    
            }
        }
    }
    else
    {
        m_pReseau->m_XmlDocAcoustique->AddCellule( m_nID, m_strLabel, strTuyau, GetAbsAmont(), GetOrdAmont(), GetHautAmont(), GetAbsAval(),GetOrdAval(), GetHautAval());
    }

    return true;
}


// Initialisation de la simulation pour Sirane (production de la définition des cellules)
void Segment::InitSimulationSirane(ePhaseCelluleSirane iPhase)
{
    std::vector<Point*> lstPts;
    lstPts.push_back(GetExtAmont());
    for(size_t i = 0 ; i < m_LstPtsInternes.size(); i++)
    {
        lstPts.push_back(m_LstPtsInternes[i]);
    }
    lstPts.push_back(GetExtAval());
    m_pReseau->m_XmlDocSirane->AddCellule(m_strLabel, CELL_BRIN, lstPts, iPhase);
}

//---------------------------------------------------------------------------
// Fonctions de calcul des émissions acoustiques
//---------------------------------------------------------------------------

// Calcul des émissions acoustiques d'un véhicule de la cellule
void SegmentMacro::Calcul_emission_vehicule(Loi_emission* Loi, std::string typ_rev)
{
    int     taille  = Loi_emission::Nb_Octaves + 1 ;
    double  dbVit;
    double  dbAcc, dbAccAnt;
    double  dbVitAnt;
    bool    bAcc;
    int     nVitNew, nVitOld;

    dbVit    = (m_pFrontiereAmont->GetVit(0)+ m_pFrontiereAval->GetVit(0))/2.;
    dbVitAnt = (m_pFrontiereAmont->GetVit(1)+ m_pFrontiereAval->GetVit(1))/2.;

    dbAcc    = (m_pFrontiereAmont->GetAcc(0)+ m_pFrontiereAval->GetAcc(0))/2.;
    dbAccAnt = (m_pFrontiereAmont->GetAcc(1)+ m_pFrontiereAval->GetAcc(1))/2.;

    bAcc = (dbAcc >= 1 && dbAccAnt >= 1) || (dbAcc <= -0.2 && dbAccAnt >= -0.2) || ( (dbAcc > -0.2 && dbAccAnt < 1) && (dbAcc > -0.2 && dbAccAnt < 1) );

    nVitNew = (int)(dbVit *3.6);
    if( dbVit*3.6 > 20)
    {
        if (div(nVitNew,2).rem > 0)
            nVitNew--;
    }

    nVitOld = (int)(dbVitAnt *3.6);
    if( dbVitAnt*3.6 > 20)
    {
        if (div(nVitOld,2).rem > 0)
                nVitOld--;
    }

    Loi->CalculEmissionBruit(W_cellule, dbVit, dbAcc, typ_rev, "VL");

}// Fin de la méthode void SegmentMacro::Calcul_emission_vehicule


// Calcul de l'émission acoustique du segment
void SegmentMacro::Calcul_emission_cellule(Loi_emission* Loi, std::string typ_rev)
{
    double  dbConcAnt;

    if( fabs(m_pFrontiereAmont->GetN(1) - m_pFrontiereAval->GetN(1)) < ZERO_DOUBLE )
        dbConcAnt = 0;
    else
        dbConcAnt =  ( m_pFrontiereAmont->GetN(1) - m_pFrontiereAval->GetN(1) ) / m_dbPasEspace;

    if(dbConcAnt==0)   // pour eviter une erreur
    {
        for (int i=Loi_emission::Nb_Octaves; i>=0; i--)    // la boucle a lieu avec i-- pour n'appeler getNb_Octave qu'une seule fois

                        Lw_cellule[i]=0;

        }
        else
        {
            Calcul_emission_vehicule(Loi, typ_rev);
                double log_conc_rel=10*log10(dbConcAnt);       // pour eviter le calcul successif de log
                for (int i=Loi_emission::Nb_Octaves; i>=0; i--)    // la boucle a lieu avec i-- pour n'appeler getNb_Octave qu'une seule fois
                {
                        Lw_cellule[i]=10 * log10 (W_cellule[i] / pow((double)10, (double)-12) )+log_conc_rel;

                }
        }
}

//================================================================
    void SegmentMacro::EmissionOutput
//----------------------------------------------------------------
// Fonction  : Sortie des calculs acoustique
// Version du: 12/10/2006
// Historique: 12/10/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(    
)
{
    // Sortie XML
    bool bOK =  false;
	double * dbTest;

	dbTest = new double[Loi_emission::Nb_Octaves+1];
    for(int i=0; i<=Loi_emission::Nb_Octaves; i++)
    {
        dbTest[i] = Lw_cellule[i];
        if(Lw_cellule[i]>0.000001)
            bOK = true;
    }

    if(bOK)
        m_pReseau->m_XmlDocAcoustique->AddCellSimu( m_nID, dbTest);

    delete [] dbTest;
}


//---------------------------------------------------------------------------
// Fonctions d'affectation des variables
//---------------------------------------------------------------------------

// Affecte pointeur sur segment amont
void SegmentMacro::setAmont(SegmentMacro *s)
{   cell_amont=s;  }

// Affecte pointeur sur segment aval
void SegmentMacro::setAval(SegmentMacro *s)
{   cell_aval=s;   }


//---------------------------------------------------------------------------
// Fonctions de renvoi des informations du segment
//---------------------------------------------------------------------------


double SegmentMacro::getConc  (void)
{  //retourne la concentration
    //return concentration;
    return m_dbConc;
}

double SegmentMacro::getNbVoie()
{
        return Nb_Voies;
}

SegmentMacro* SegmentMacro::getSegmentAmont  (void)
{  //retourne le segment en amont
 return cell_amont;
}

SegmentMacro* SegmentMacro::getSegmentAval  (void)
{  //retourne le segment en aval
  return cell_aval;
}

//---------------------------------------------------------------------------
// Fonctions utilitaires
//---------------------------------------------------------------------------

// Ecriture des fichiers de sortie trafic
void SegmentMacro::TrafficOutput()
{
	std::string ssTuyau = m_pReseau->GetLibelleTronconEve( (Tuyau*)GetParent()->GetParent(), 1);
	std::string ssLib = GetLabel();
    std::deque< TimeVariation<TraceDocTrafic> >::iterator itXmlDocTrafic;

    // Ecriture du résultat (N, Vit et Acc) pour la frontière amont de la première cellule
    //if(!cell_amont)
    {
        //memset(strNom,0x0,sizeof(strNom));
        //strncat(strNom, m_strNom, strlen(m_strNom)-1);
        //strcat(strNom, "*" );
        //Reseau::FicTraficCel<<strNom<<" "<<"*"<<" "<<"*"<<" "<<m_pFrontiereAmont->GetVit(0)<<" "
        //        <<m_pFrontiereAmont->GetAcc(0)<<" "<<m_pFrontiereAmont->GetN(0)<<endl;
    }

    //Reseau::FicTraficCel<<m_strNom<<" "<<m_dbConc<<" "<<m_dbDebit<<" "<<m_pFrontiereAval->GetVit(0)<<" "
      //          <<m_pFrontiereAval->GetAcc(0)<<" "<<m_pFrontiereAval->GetN(0)<<endl;

	// Ecriture des fichiers résultat
    for( itXmlDocTrafic = m_pReseau->m_xmlDocTrafics.begin(); itXmlDocTrafic!= m_pReseau->m_xmlDocTrafics.end(); ++itXmlDocTrafic )
    {
		itXmlDocTrafic->m_pData->AddCellSimu(m_nID, m_dbConc, m_dbDebit, m_pFrontiereAmont->GetVit(0), m_pFrontiereAmont->GetAcc(0), m_pFrontiereAmont->GetN(0), m_pFrontiereAval->GetVit(0), m_pFrontiereAval->GetAcc(0), m_pFrontiereAval->GetN(0)
            , ssLib, ssTuyau, GetAbsAmont(), GetOrdAmont(), GetHautAmont(), GetAbsAval(),GetOrdAval(), GetHautAval() );	
    }
}

//================================================================
    void SegmentMacro::AddVoisinAmont
//----------------------------------------------------------------
// Fonction  : Ajoute un voisin à la liste des voisins amont
// Version du: 07/01/2005
// Historique: 07/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    SegmentMacro*   pSegment
)
{
    std::deque <Voisin>::iterator cur, debut, fin;
    Voisin  v;

    // On vérifie si le segment est déjà dans la liste
    debut   = m_LstVoisinsAmont.begin();
    fin     = m_LstVoisinsAmont.end();

    for (cur=debut;cur!=fin;cur++)
    {
        if( cur->pSegment == pSegment )
            return;
    }

    // Sinon on l'ajoute
    v.pSegment              = pSegment;
    v.dbDebit               = 0;
    v.dbDemande             = 0;
    v.dbDebitInjecteCumule  = 0;

    m_LstVoisinsAmont.push_back(v);
}

//================================================================
    void SegmentMacro::AddVoisinAval
//----------------------------------------------------------------
// Fonction  : Ajoute un voisin à la liste des voisins aval
// Version du: 07/01/2005
// Historique: 07/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    SegmentMacro *   pSegment,
    double      dbDebit
)
{
    std::deque <Voisin>::iterator cur, debut, fin;
    Voisin  v;

    // On vérifie si le segment est déjà dans la liste
    debut   = m_LstVoisinsAval.begin();
    fin     = m_LstVoisinsAval.end();

    for (cur=debut;cur!=fin;cur++)
    {
        if( cur->pSegment == pSegment )
        {
            cur->dbDebit = dbDebit;
            return;
        }
    }

    // Sinon on l'ajoute
    v.pSegment              = pSegment;
    v.dbDebit               = dbDebit;
    v.dbDemande             = 0;
    v.dbDebitInjecteCumule  = 0;

    m_LstVoisinsAval.push_back(v);
}

//================================================================
    double SegmentMacro::GetDebitInjecteTo
//----------------------------------------------------------------
// Fonction  : Renvoie le débit injecté au segment passé en paramètre
//             par le segment courant
// Version du: 07/01/2005
// Historique: 07/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    SegmentMacro         *pSegment,
    bool            bAnt        /* = false */       // Indique si on renvoie le débit du pas de temps
                                                    // précédent
)
{
    std::deque <Voisin>::iterator cur, debut, fin;

    // Recherche du segment dans la liste des segments aval
    debut   = m_LstVoisinsAval.begin();
    fin     = m_LstVoisinsAval.end();

    for (cur=debut;cur!=fin;cur++)
    {
        if( cur->pSegment == pSegment )
        {
            if(bAnt)
                return cur->dbDebitAnt;
            else
                return cur->dbDebit;
        }
    }

    // Segment non trouvé, on retourne un débit nul
    return 0;
}

//================================================================
    double SegmentMacro::GetDebitEquiTo
//----------------------------------------------------------------
// Fonction  : Renvoie le débit injecté au segment passé en paramètre
//             par le segment courant
// Version du: 07/01/2005
// Historique: 07/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    SegmentMacro         *pSegment
)
{
    std::deque <Voisin>::iterator cur, debut, fin;

    // Recherche du segment dans la liste des segments aval
    debut   = m_LstVoisinsAval.begin();
    fin     = m_LstVoisinsAval.end();

    for (cur=debut;cur!=fin;cur++)
    {
        if( cur->pSegment == pSegment )
                return cur->dbDebitEqui;
    }

    // Segment non trouvé, on retourne un débit nul
    return 0;
}


//================================================================
    void SegmentMacro::SetDebitInjecteTo
//----------------------------------------------------------------
// Fonction  : Affecte le débit injecté au segment passé en paramètre
//             par le segment courant
// Version du: 10/01/2005
// Historique: 10/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    SegmentMacro      *  pSegment,
    double          dbDebit,
    bool            bMAJAnt     /* = true */    // Indique si il faut mettre à jour
                                                // le débit antérieur
)
{
    std::deque <Voisin>::iterator cur, debut, fin;

    // Recherche du segment dans la liste des segments aval
    debut   = m_LstVoisinsAval.begin();
    fin     = m_LstVoisinsAval.end();

    for (cur=debut;cur!=fin;cur++)
    {
        if( cur->pSegment == pSegment )
        {
            if( bMAJAnt )
                cur->dbDebitAnt = cur->dbDebit;
            cur->dbDebit = dbDebit;
            cur->dbDebitInjecteCumule += dbDebit;
        }
    }
}

//================================================================
    void SegmentMacro::SetDebitEquiTo
//----------------------------------------------------------------
// Fonction  : Affecte le débit injecté au segment passé en paramètre
//             par le segment courant
// Version du: 10/01/2005
// Historique: 10/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    SegmentMacro      *  pSegment,
    double          dbDebit,
    bool            bAdd        /*=false*/
)
{
    std::deque <Voisin>::iterator cur, debut, fin;

    // Recherche du segment dans la liste des segments aval
    debut   = m_LstVoisinsAval.begin();
    fin     = m_LstVoisinsAval.end();

    for (cur=debut;cur!=fin;cur++)
    {
        if( cur->pSegment == pSegment )
        {
            if(bAdd)
                cur->dbDebitEqui += dbDebit;
            else
                cur->dbDebitEqui = dbDebit;
        }
    }
}

//================================================================
    double SegmentMacro::GetDebitInjecteCumuleTo
//----------------------------------------------------------------
// Fonction  : Renvoie le débit injecté cumulé au segment passé en paramètre
//             par le segment courant
// Version du: 07/01/2005
// Historique: 07/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    SegmentMacro         *pSegment
)
{
    std::deque <Voisin>::iterator cur, debut, fin;

    // Recherche du segment dans la liste des segments aval
    debut   = m_LstVoisinsAval.begin();
    fin     = m_LstVoisinsAval.end();

    for (cur=debut;cur!=fin;cur++)
    {
        if( cur->pSegment == pSegment )
            return cur->dbDebitInjecteCumule;
    }

    // Segment non trouvé, on retourne un débit nul
    return 0;
}

//================================================================
    void SegmentMacro::SetDebitInjecteCumuleTo
//----------------------------------------------------------------
// Fonction  : Affecte le débit injecté cumule au segment passé en paramètre
//             par le segment courant
// Version du: 10/01/2005
// Historique: 10/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    SegmentMacro      *pSegment,
    double       dbDebit
)
{
    std::deque <Voisin>::iterator cur, debut, fin;

    // Recherche du segment dans la liste des segments aval
    debut   = m_LstVoisinsAval.begin();
    fin     = m_LstVoisinsAval.end();

    for (cur=debut;cur!=fin;cur++)
    {
        if( cur->pSegment == pSegment )
            cur->dbDebitInjecteCumule = dbDebit;
    }
}

//================================================================
    void SegmentMacro::SetDemandeTo
//----------------------------------------------------------------
// Fonction  : Affecte la demande du segment courant pour
//             le segment passé en paramètre
// Version du: 10/01/2005
// Historique: 10/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    SegmentMacro      *pSegment,
    double       dbDemande
)
{
    std::deque <Voisin>::iterator cur, debut, fin;

    // Recherche du segment dans la liste des segments aval
    debut   = m_LstVoisinsAval.begin();
    fin     = m_LstVoisinsAval.end();

    for (cur=debut;cur!=fin;cur++)
    {
        if( cur->pSegment == pSegment )
            cur->dbDemande = dbDemande;
    }
}

//================================================================
    double SegmentMacro::GetDemandeTo
//----------------------------------------------------------------
// Fonction  : Retourne la demande du segment courant pour le
//             segment voisin passé en paramètre
// Version du: 18/01/2005
// Historique: 18/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    SegmentMacro      *pSegment
)
{
    std::deque <Voisin>::iterator cur, debut, fin;

    // Recherche du segment dans la liste des segments aval
    debut   = m_LstVoisinsAval.begin();
    fin     = m_LstVoisinsAval.end();

    for (cur=debut;cur!=fin;cur++)
    {
        if( cur->pSegment == pSegment )
            return cur->dbDemande;
    }
    
    return 0;
}

//================================================================
    double SegmentMacro::GetDemandeFrom
//----------------------------------------------------------------
// Fonction  : Retourne la demande du segment passé en paramètre
//             pour le segment courant
// Version du: 18/01/2005
// Historique: 18/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    SegmentMacro      *pSegment
)
{
    std::deque <Voisin>::iterator cur, debut, fin;

    // Recherche du segment dans la liste des segments amont
    debut   = m_LstVoisinsAmont.begin();
    fin     = m_LstVoisinsAmont.end();

    for (cur=debut;cur!=fin;cur++)
    {
        if( cur->pSegment == pSegment )
            return cur->pSegment->GetDemandeTo(this);
    }
    
    return 0;
}

//================================================================
    double SegmentMacro::GetDebitRecuFrom
//----------------------------------------------------------------
// Fonction  : Renvoie le débit reçu par le segment courant et
//             provenant du segment passé en paramètre
// Version du: 07/01/2005
// Historique: 07/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    SegmentMacro    *pSegment,
     bool bAnt  /* = false */    // Indique si on renvoie le débit du pas de temps
                                // précédent
)
{
    std::deque <Voisin>::iterator cur, debut, fin;

    // Recherche du segment dans la liste des segments amont
    debut   = m_LstVoisinsAmont.begin();
    fin     = m_LstVoisinsAmont.end();

    for (cur=debut;cur!=fin;cur++)
    {
        if( cur->pSegment == pSegment )
            return pSegment->GetDebitInjecteTo(this, bAnt);
    }

    // Segment non trouvé, on retourne un débit nul
    return 0;
}

//================================================================
    double SegmentMacro::GetDebitCumuleRecuFrom
//----------------------------------------------------------------
// Fonction  : Renvoie le débit cumulé reçu par le segment courant et
//             provenant du segment passé en paramètre
// Version du: 07/01/2005
// Historique: 07/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    SegmentMacro    *pSegment
)
{
    std::deque <Voisin>::iterator cur, debut, fin;

    // Recherche du segment dans la liste des segments amont
    debut   = m_LstVoisinsAmont.begin();
    fin     = m_LstVoisinsAmont.end();

    for (cur=debut;cur!=fin;cur++)
    {
        if( cur->pSegment == pSegment )
            return pSegment->GetDebitInjecteCumuleTo(this);
    }

    // Segment non trouvé, on retourne un débit nul
    return 0;
}

//================================================================
    double SegmentMacro::GetSommeDebitInjecte
//----------------------------------------------------------------
// Fonction  : Renvoie la somme des débits injectés par le segment
//             courant vers les segments en aval
// Version du: 07/01/2005
// Historique: 07/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    double  dbDebit;

    std::deque <Voisin>::iterator cur, debut, fin;

    dbDebit = 0;

    // Parcours de la liste des segments en aval
    debut   = m_LstVoisinsAval.begin();
    fin     = m_LstVoisinsAval.end();

    for (cur=debut;cur!=fin;cur++)
    {
        dbDebit += cur->dbDebit;
    }

    return dbDebit;
}

//================================================================
    double SegmentMacro::GetSommeDebitRecu
//----------------------------------------------------------------
// Fonction  : Renvoie la somme des débits reçus par le segment
//             courant et provenant des segments en amont
// Version du: 07/01/2005
// Historique: 07/01/2005 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    bool bAnt  /* = false */    // Indique si on renvoie le débit du pas de temps
                                // précédent
)
{
    double  dbDebit;

    std::deque <Voisin>::iterator cur, debut, fin;

    dbDebit = 0;

    // Parcours de la liste des segments en aval
    debut   = m_LstVoisinsAmont.begin();
    fin     = m_LstVoisinsAmont.end();

    for (cur=debut;cur!=fin;cur++)
        if(cur->pSegment)
        {
            dbDebit += GetDebitRecuFrom(cur->pSegment, bAnt);
        }
    return dbDebit;
}


//================================================================
    void SegmentMacro::ComputeTrafficEx
//----------------------------------------------------------------
// Fonction  :  Calcule les autres variables caractéristiques du trafic
//              à partir de la quantité N
// Version du:  03/07/2006
// Historique:  03/07/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    double /*nInstant*/
)
{
    
    // Débit de sortie de la cellule
    if( fabs(m_pFrontiereAval->GetN(0) - m_pFrontiereAval->GetN(1)) < ZERO_DOUBLE )
        m_dbDebit = 0;
    else
        m_dbDebit =  (m_pFrontiereAval->GetN(0) - m_pFrontiereAval->GetN(1)) / m_dbPasTemps;

    // Concentration (calculée à la fin du pas de temps)
    if( fabs(m_pFrontiereAmont->GetN(0) - m_pFrontiereAval->GetN(0)) < ZERO_DOUBLE )
        m_dbConc = 0;
    else
        m_dbConc =  ( m_pFrontiereAmont->GetN(0) - m_pFrontiereAval->GetN(0) ) / m_dbPasEspace;
}

//================================================================
    double SegmentMacro::CalculDemandeVar
//----------------------------------------------------------------
// Fonction  :  Calcule de la demande dans le cas de la méthode
//              variationnelle
// Version du:  03/07/2006
// Historique:  03/07/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
)
{
    double  dbDemande;
    double  dbQx;

	TypeVehicule *pTV = m_pReseau->m_LstTypesVehicule[0];

    // Qmax
    dbQx = pTV->GetDebitMax();

    dbDemande = min ( m_pFrontiereAmont->GetN(1) - m_pFrontiereAval->GetN(1), dbQx * m_dbPasTemps);
    SetDemandeTo(cell_aval, dbDemande);

    return dbDemande;
}

//================================================================
    double SegmentMacro::CalculOffreVar
//----------------------------------------------------------------
// Fonction  :  Calcule de l'offre dans le cas de la méthode
//              variationnelle
// Version du:  03/07/2006
// Historique:  03/07/2006 (C.Bécarie - Tinea)
//              Création
//================================================================
(
)
{
    double dbOffre;    
    double dbNx0tp;
    double dbKx;
    double dbQx;

	TypeVehicule *pTV = m_pReseau->m_LstTypesVehicule[0];
    
    // Kmax
    dbKx = pTV->GetKx();

    // Qmax
    dbQx = pTV->GetDebitMax();

    // N( x0, t + dt ) = N( x0 + dx, t1) + Kmax * dx
    dbNx0tp = m_pFrontiereAval->GetLastN() +  dbKx * m_dbPasEspace;

    // Offre = min ( N( x0, t + dt ) -  N( x0, t ) , Qmax * dt )
    dbOffre = min ( dbNx0tp - m_pFrontiereAmont->GetN(1), dbQx * m_dbPasTemps);

    return dbOffre;
}

//================================================================
    CDiagrammeFondamental* SegmentMacro::GetDiagFonda
//----------------------------------------------------------------
// Fonction  :  Renvoie le pointeur sur le diagramme fondamental
//              du tuyau et remet à jour le contexte
//              (c'est le diagramme du tuyau grand-parent)
// Version du:  05/01/2007
// Historique:  05/01/2007 (C.Bécarie - Tinea )
//              Création
//================================================================
(
)
{
    if(!GetParent())
        return NULL;

    return ((VoieMacro*)GetParent())->GetDiagFonda();
}

//================================================================
    void SegmentMacro::InitVarSimu
//----------------------------------------------------------------
// Fonction  :  Initialise les variables de simulation
// Version du:  13/07/2007
// Historique:  13/07/2007 (C.Bécarie - Tinea )
//              Création
//================================================================
(
)
{
	TypeVehicule *pTV = m_pReseau->m_LstTypesVehicule[0];

    m_dbConc = 0;
    m_dbDebit = 0;

    if(m_pFrontiereAmont)
        m_pFrontiereAmont->InitSimu( pTV->GetVx() );

    if(m_pFrontiereAval)
       m_pFrontiereAval->InitSimu( pTV->GetVx() );
}



//================================================================
    void Segment::AddSpeedContribution
//----------------------------------------------------------------
// Fonction  :  Ajoute une contribution à la cellule Sirane
// Version du:  10/04/2012
// Historique:  10/04/2012 (O.Tonck - IPSIS)
//              Création
//================================================================
(
    TypeVehicule * pTypeVeh,
    double dbVitStart,
    double dbVitEnd,
    double dbPasDeTemps
)
{
    // on interpole la vitesse sur le pas de temps pour répartir au mieux
    // le temps dans les différentes plages. On travaille en kilomètres par heure
    double dbVitMin = min(dbVitStart, dbVitEnd);
    dbVitMin *= 3.6;
    double dbVitMax = max(dbVitStart, dbVitEnd);
    dbVitMax *= 3.6;

    double dbPente = (dbVitMax-dbVitMin) / dbPasDeTemps;

    double  dbMinBound, dbMaxBound;
    int     nPlage = GetSpeedRange(dbVitMin);
    GetSpeedBounds(nPlage, dbMinBound, dbMaxBound);

    // Si pas de variation de vitesse, on affecte tout le pas de temps à la plage de vitesse correspondante.
    if(dbPente  == 0)
    {
        AddSpeedContribution(pTypeVeh, nPlage, dbPasDeTemps);
    }
    else
    {
        while(dbVitMin < dbVitMax)
        {
            // affectation de la bonne durée à la plage de vitesse courante
            double dbDureeDansPlage = (min(dbVitMax, dbMaxBound)-dbVitMin) / dbPente;
            AddSpeedContribution(pTypeVeh, nPlage, dbDureeDansPlage);

            // préparation de l'itération suivante
            dbVitMin = min(dbVitMax, dbMaxBound);
            nPlage++;
            //nPlage = GetSpeedRange(dbVitMin);
            GetSpeedBounds(nPlage, dbMinBound, dbMaxBound);
        }
    }
}

//================================================================
    int    Segment::GetSpeedRange
//----------------------------------------------------------------
// Fonction  :  Donne les bornes min et max de la plage de vitesse
// dont l'index est donné en paramètre
// Version du:  10/04/2012
// Historique:  10/04/2012 (O.Tonck - IPSIS)
//              Création
//================================================================        
(
    double dbVit
)
{
    int result;

    if(dbVit <= 0)
    {
        result = 0;
    }
    else
    {
        // Plage particulière de 0 à 1 (les deux exclus)
        if(dbVit>0 && dbVit<1)
        {
            result = 1;
        }
        else
        {
            int i = (int)floor(dbVit-1.0);
            if(i % 2 == 0)
            {
                result = 2+i/2;
            }
            else
            {
                result = 2+(i-1)/2;
            }
        }
    }

    return result;
}

//================================================================
    void Segment::GetSpeedBounds
//----------------------------------------------------------------
// Fonction  :  Calcule les bornes inférieure et supérieure 
// (en km/h) de la plage dont l'index est passé en paramètre
// Version du:  10/04/2012
// Historique:  10/04/2012 (O.Tonck - IPSIS)
//              Création
//================================================================
(
    int     nPlage,
    double& dbVitMin,
    double& dbVitMax
)
{
    if(nPlage == 0)
    {
        dbVitMin = 0;
        dbVitMax = 0;
    }
    else if(nPlage == 1)
    {
        dbVitMin = 0;
        dbVitMax = 1;
    }
    else
    {
        dbVitMin = 1+(nPlage-2)*2;
        dbVitMax = dbVitMin+2;
    }
}

//================================================================
    void Segment::AddSpeedContribution
//----------------------------------------------------------------
// Fonction  :  Ajoute une durée à la plage de vitesse dont
// l'index est précisé en paramètre
// Version du:  10/04/2012
// Historique:  10/04/2012 (O.Tonck - IPSIS)
//              Création
//================================================================
(
    TypeVehicule * pType,
    int     nPlage,
    double  dbDuree
)
{
    std::vector<double > & lstCumuls = m_LstCumulsSirane[pType];
    // on agrandi automatiquement la liste des plages si nécessaire
    while((int)lstCumuls.size() <= nPlage)
    {
        lstCumuls.push_back(0.0);
    }

    lstCumuls[nPlage] += dbDuree;
}

//================================================================
    void Segment::SortieSirane
//----------------------------------------------------------------
// Fonction  :  Produit la sortie de la cellule sirane
// Version du:  10/04/2012
// Historique:  10/04/2012 (O.Tonck - IPSIS)
//              Création
//================================================================
(
)
{
    m_pReseau->m_XmlDocSirane->AddCellSimu(GetLabel());

    // Pour tous les types de véhicules
    std::map<TypeVehicule*, std::vector<double> >::iterator iter;
    for(iter = m_LstCumulsSirane.begin(); iter != m_LstCumulsSirane.end(); iter++)
    {
        // On constitue une liste de plages avec vitesse min, max et durée cumulée
        // pour lesquelles la durée n'est pas nulle
        std::vector<double> LstVitMin;
        std::vector<double> LstVitMax;
        std::vector<double> LstDuree;
        double minBound, maxBound;
        for(size_t i = 0; i < iter->second.size(); i++)
        {
            if(iter->second[i] > 0)
            {
                LstDuree.push_back(iter->second[i]);
                GetSpeedBounds((int)i, minBound, maxBound);
                LstVitMin.push_back(minBound);
                LstVitMax.push_back(maxBound);
            }
        }
    
        m_pReseau->m_XmlDocSirane->AddCellSimuForType(iter->first->GetLabel(), LstVitMin, LstVitMax, LstDuree);
    }
}

//================================================================
    void Segment::InitVariablesSirane
//----------------------------------------------------------------
// Fonction  :  Remet à zéro les variables Sirane pour nouvelle
// période d'agrégation
// Historique:  10/04/2012 (O.Tonck - IPSIS)
//              Création
//================================================================
(
)
{
    m_LstCumulsSirane.clear();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void Voisin::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Voisin::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Voisin::serialize(Archive& ar, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(pSegment);
    ar & BOOST_SERIALIZATION_NVP(dbDebit);
    ar & BOOST_SERIALIZATION_NVP(dbDebitEqui);
    ar & BOOST_SERIALIZATION_NVP(dbDemande);
    ar & BOOST_SERIALIZATION_NVP(dbDebitInjecteCumule);
    ar & BOOST_SERIALIZATION_NVP(dbDebitAnt);
}

template void Segment::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Segment::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);
    
template<class Archive>
void Segment::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Troncon);

    ar & BOOST_SERIALIZATION_NVP(m_nID);
    ar & BOOST_SERIALIZATION_NVP(m_LstCumulsSirane);
    if(Archive::is_loading::value)
    {
        if(Lw_cellule!=NULL)
        {
            delete [] Lw_cellule;
        }
        if(W_cellule!=NULL)
        {
            delete [] W_cellule;
        }
        Lw_cellule = new double[Loi_emission::Nb_Octaves+1];
        W_cellule = new double[Loi_emission::Nb_Octaves+1];
    }
    for(int i = 0; i < Loi_emission::Nb_Octaves+1; i++)
    {
        ar & BOOST_SERIALIZATION_NVP2("Lw_cellule", Lw_cellule[i]);
        ar & BOOST_SERIALIZATION_NVP2("W_cellule", W_cellule[i]);
    }
}
    
template void SegmentMacro::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void SegmentMacro::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void SegmentMacro::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Segment);

	ar & BOOST_SERIALIZATION_NVP(Nb_Voies);

	ar & BOOST_SERIALIZATION_NVP(cell_aval);
    ar & BOOST_SERIALIZATION_NVP(cell_amont);

    ar & BOOST_SERIALIZATION_NVP(m_pFrontiereAmont);
    ar & BOOST_SERIALIZATION_NVP(m_pFrontiereAval);

    ar & BOOST_SERIALIZATION_NVP(m_dbConc);
    ar & BOOST_SERIALIZATION_NVP(m_dbDebit);

    ar & BOOST_SERIALIZATION_NVP(m_LstVoisinsAmont);
    ar & BOOST_SERIALIZATION_NVP(m_LstVoisinsAval);
}