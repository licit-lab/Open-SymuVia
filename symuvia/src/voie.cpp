#include "stdafx.h"
#include "voie.h"

#include "tuyau.h"
#include "frontiere.h"
#include "sortie.h"
#include "repartiteur.h"
#include "DiagFonda.h"
#include "Segment.h"
#include "vehicule.h"
#include "reseau.h"
#include "ControleurDeFeux.h"
#include "regulation/RegulationModule.h"
#include "regulation/RegulationBrique.h"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/shared_ptr.hpp>

using namespace std;

//================================================================
    LigneDeFeu::LigneDeFeu
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 21/06/2013
// Historique: 21/06/2013 (O.Tonck - Ipsis)
//             Création
//================================================================
(
)
{
    m_pControleur=NULL;
    m_pTuyAm=NULL;
    m_pTuyAv=NULL;
    m_dbPosition=0.0;
    nVAm=0;
    nVAv=0;
}

//================================================================
        Voie::Voie
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    m_pVoieGauche = NULL;
    m_pVoieDroite = NULL;
}

//================================================================
        Voie::Voie
//----------------------------------------------------------------
// Fonction  : Constructeur avec initialisation du numéro de voie
//             et largeur
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
        double  dbLargeur,
        double dbVitReg
):Troncon(dbVitReg, DBL_MAX, "")
{

    m_pVoieGauche = NULL;
    m_pVoieDroite = NULL;

    m_dbLargeur = dbLargeur;
}

// Gestion des possibilités de changement de voie vers cette voie
void Voie::SetTargetLaneForbidden(bool bIsVoieCibleInterdite)
{
    RegulationBrique * pBrique = m_pReseau->GetModuleRegulation()->getBriqueActive();
    m_bIsVoieCibleInterdite[pBrique] = bIsVoieCibleInterdite;
}

bool Voie::IsTargetLaneForbidden(SousTypeVehicule & sousType)
{
    bool bResult = false;

    // Si on a une interdiction globale (non liée à une brique), on renvoie true
    map<RegulationBrique*, bool>::iterator iter = m_bIsVoieCibleInterdite.find(NULL);
    if(iter != m_bIsVoieCibleInterdite.end()
        && iter->second)
    {
        bResult = true;
    }
    else
    {
        // Pour chaque brique pour lesquelles on a une consigne d'interdiction ...
        for(iter = m_bIsVoieCibleInterdite.begin(); iter != m_bIsVoieCibleInterdite.end() && !bResult; iter++)
        {
            if(iter->first != NULL)
            {
                // on interdit si le véhicule respecte cette consigne
                if(iter->second && sousType.checkRespect(iter->first))
                {
                    bResult = iter->second;
                }
            }
        }
    }

    return bResult;
}

void Voie::SetPullBackInInFrontOfGuidedVehicleForbidden(bool bIsRabattementDevantVehiculeGuideInterdit)
{
    RegulationBrique * pBrique = m_pReseau->GetModuleRegulation()->getBriqueActive();
    m_bIsRabattementDevantVehiculeGuideInterdit[pBrique] = bIsRabattementDevantVehiculeGuideInterdit;
}

bool Voie::IsPullBackInInFrontOfGuidedVehicleForbidden(SousTypeVehicule & sousType)
{
    bool bResult = false;

    // Si on a une interdiction globale (non liée à une brique), on renvoie true
    map<RegulationBrique*, bool>::iterator iter = m_bIsRabattementDevantVehiculeGuideInterdit.find(NULL);
    if(iter != m_bIsRabattementDevantVehiculeGuideInterdit.end()
        && iter->second)
    {
        bResult = true;
    }
    else
    {
        // Pour chaque brique pour lesquelles on a une consigne d'interdiction ...
        for(iter = m_bIsRabattementDevantVehiculeGuideInterdit.begin(); iter != m_bIsRabattementDevantVehiculeGuideInterdit.end() && !bResult; iter++)
        {
            if(iter->first != NULL)
            {
                // on interdit si le véhicule respecte cette consigne
                if(iter->second && sousType.checkRespect(iter->first))
                {
                    bResult = iter->second;
                }
            }
        }
    }

    return bResult;
}

//================================================================
        VoieMacro::VoieMacro
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
        m_pLstSegment   = NULL;
        m_pLstFrontiere = NULL; 
}

//================================================================
    VoieMacro::VoieMacro
//----------------------------------------------------------------
// Fonction  : Constructeur avec initialisation du numéro de voie
//             et largeur
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
        double  dbLargeur,
        double  dbVitReg
):Voie( dbLargeur, dbVitReg )
{
        m_pLstSegment   = NULL;
        m_pLstFrontiere = NULL;
}

//================================================================
    VoieMacro::~VoieMacro
//----------------------------------------------------------------
// Fonction  : Destructeur
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    SupprimeSegments();
}


//================================================================
    void VoieMacro::SupprimeSegments
//----------------------------------------------------------------
// Fonction  : Suppression et destruction de la liste des segments
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    int i;

    if(m_pLstFrontiere != NULL)
    {
        for( i=0; i< m_nNbCell+1; i++)
        {
            delete m_pLstFrontiere[i];
            m_pLstFrontiere[i] = NULL;
        }

        delete [] m_pLstFrontiere;
        m_pLstFrontiere = NULL;
    }

        if(m_pLstSegment != NULL)
        {
                for( i=0; i< m_nNbCell; i++)
                {
                        delete m_pLstSegment[i];
                        m_pLstSegment[i]=NULL;
                }
                delete [] m_pLstSegment;
                m_pLstSegment = NULL;
        }

}

//================================================================
      bool VoieMacro::Discretisation
//----------------------------------------------------------------
// Fonction  : Segmentation d'une voie en cellules
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    int nNbVoie
)
{
        SegmentMacro*           pSgm;
        int                     i;
        double                  dbX, dbY, dbZ;
        double                  dbAbsAmont,dbAbsAval;
        double                  dbOrdAmont,dbOrdAval;
        double                  dbHautAmont, dbHautAval;
        Frontiere*              pFrontiereAmont = NULL;
        Frontiere*              pFrontiereAval = NULL;
        int                     nNbPasTempsHist;

        // A supprimer après ajout heritage circulation
        char tmp[256];        

        // ménage
        SupprimeSegments();

        if(m_nNbCell==0)
                return false;

        dbX = (GetAbsAval()-GetAbsAmont())/m_nNbCell;
        dbY = (GetOrdAval()-GetOrdAmont())/m_nNbCell;
        dbZ = (GetHautAval()-GetHautAmont())/m_nNbCell;

        // création des listes
        m_pLstSegment   = new SegmentMacro*[m_nNbCell];
        m_pLstFrontiere = new Frontiere*[m_nNbCell+1];

        // Calcul du nombre de pas de temps à historiser  (permet d'obtenir la taille des tableaux
        // de la classe Frontiere
		TypeVehicule *pTV = m_pReseau->m_LstTypesVehicule[0];

        if( !pTV )
			return false;
        
        nNbPasTempsHist =   (int)ceil( pTV->GetVx() / - pTV->GetW() ) ;
        if ( nNbPasTempsHist == pTV->GetVx() / - pTV->GetW() )
            ((TuyauMacro*)GetParent())->SetCondVitAval(true);
        else
        {
            ((TuyauMacro*)GetParent())->SetCondVitAval(false);
            ((TuyauMacro*)GetParent())->SetCoeffAval( pTV->GetVx() / - pTV->GetW() );
        }        

        for( i=0; i<m_nNbCell; i++)
        {                    
                //_snprintf(tmp, sizeof(tmp), "%sC%d", m_strNom, i);   
				std::string sTmp = m_strLabel + "C" + to_string(i);

                dbAbsAmont= GetAbsAmont()+dbX*i;
                dbAbsAval = GetAbsAmont()+dbX*(i+1);
                dbOrdAmont= GetOrdAmont()+dbY*i;
                dbOrdAval = GetOrdAmont()+dbY*(i+1);
                dbHautAmont = GetHautAmont()+dbZ*i;
                dbHautAval = GetHautAmont()+dbZ*(i+1);

                if(pFrontiereAval)
                    pFrontiereAmont = pFrontiereAval;
                else
                {
                    pFrontiereAmont = new Frontiere(nNbPasTempsHist,pTV->GetVx());
                    m_pLstFrontiere[0] =  pFrontiereAmont;
                }

                pFrontiereAval = new Frontiere(nNbPasTempsHist, pTV->GetVx());
                m_pLstFrontiere[i+1] =  pFrontiereAval;

				int nInc;
				nInc = m_pReseau->IncLastIdSegment();
                pSgm=new SegmentMacro(sTmp , m_dbPasEspace,m_dbPasTemps, nNbVoie, pFrontiereAmont, pFrontiereAval, nInc, m_dbVitReg );
                m_pLstSegment[i] = pSgm;

                
                m_pLstSegment[i]->SetPropCom(m_pReseau, this, tmp, m_strRevetement);
                m_pLstSegment[i]->SetExtAmont(dbAbsAmont, dbOrdAmont, dbHautAmont);
                m_pLstSegment[i]->SetExtAval(dbAbsAval, dbOrdAval, dbHautAval );                

                if(i>0)
                {
                    m_pLstSegment[i]->setAmont(m_pLstSegment[i-1]);
                    ((SegmentMacro *)m_pLstSegment[i-1])->setAval(m_pLstSegment[i]);
                }
        }

        return true;
};

//================================================================
      void VoieMacro::InitVarSimu
//----------------------------------------------------------------
// Fonction  : Initialisation des variables de simulation
// Version du: 13/07/2007
// Historique: 13/07/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    for( int i=0; i<m_nNbCell; i++)
    {
        m_pLstSegment[i]->InitVarSimu();
    }
}

//================================================================
      bool VoieMacro::InitSimulation
//----------------------------------------------------------------
// Fonction  : Initialisation d'une simulation
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
        bool			bCalculAcoustique,
        bool            bSirane,
		std::string     strTuyau
)
{
        int     i;
        bool    bOk = true;

        for(i=0;i<m_nNbCell;i++)
        {
                bOk &= m_pLstSegment[i]->InitSimulation(bCalculAcoustique, bSirane, strTuyau);
        }

        return bOk;
}

//================================================================
      void VoieMacro::TrafficOutput
//----------------------------------------------------------------
// Fonction  : Ecriture des résulats du calcul du trafic le fichier
//             de sortie trafic / cellules
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    int i;

    for( i=0; i<m_nNbCell; i++)
    {
        m_pLstSegment[i]->TrafficOutput();
    }
}

//================================================================
      void VoieMacro::ComputeDemand
//----------------------------------------------------------------
// Fonction  : Calcul de la demande
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
	double dbInstSimu,
    double dbInstant  /*=0*/
)
{
    m_dbDemande = m_pLstSegment[m_nNbCell-1]->CalculDemandeVar();

}

//================================================================
        void VoieMacro::ComputeOffer
//----------------------------------------------------------------
// Fonction  : Calcul de la demande de l'offre
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double dbInstant
)
{
    m_dbOffre = m_pLstSegment[0]->CalculOffreVar();
}

//================================================================
        void VoieMacro::ComputeNoise
//----------------------------------------------------------------
// Fonction  : estimation du bruit
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
        Loi_emission*   Loi
)
{
        for(int i=0;i<m_nNbCell;i++)
        {
            m_pLstSegment[i]->Calcul_emission_cellule(Loi, m_strRevetement);
            m_pLstSegment[i]->EmissionOutput();
        }
}

//================================================================
        SegmentMacro* VoieMacro::GetSegment
//----------------------------------------------------------------
// Fonction  : Renvoie le segment de numéro passé en paramètre
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
        int nNum
)
{
        if (nNum>m_nNbCell)
	        return NULL;
	return m_pLstSegment[nNum-1];
}

//================================================================
      void VoieMacro::ComputeTrafficEx
//----------------------------------------------------------------
// Fonction  : Calcul des autres caractéristiques du réseau
// Version du: 03/07/2006
// Historique: 03/07/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double dbInstant
)
{
    int             i;
    TuyauMacro*     pParent;
    double          dbOffre;
    double          dbDemande;
    double          dbNx0tp;
    double          dbKx;
    double          dbQx;
    double          dbVitesse;

	TypeVehicule *pTV = m_pReseau->m_LstTypesVehicule[0];

    pParent = (TuyauMacro*)GetParent();
    if(!pParent)
        return;

    //----------------------------
    // CP de la premère frontière
    //----------------------------

    // Calcul de l'offre :

    // Kmax
    dbKx = pTV->GetKx();

    // Qmax
    dbQx = pTV->GetDebitMax();

    if( dbInstant > m_pLstFrontiere[1]->GetNbPasTempsHist() )  // avant, l'aval n'influence pas
    {
        // N( x0, t + dt ) = N( x0 + dx, t1) + Kmax * dx
        if(pParent->GetCondVitAval())
            dbNx0tp = m_pLstFrontiere[1]->GetLastN() +  dbKx * m_dbPasEspace;
        else
            dbNx0tp = (pParent->GetCoeffAval1()*m_pLstFrontiere[1]->GetLastN()) + (pParent->GetCoeffAval2()*m_pLstFrontiere[1]->GetBefLastN()) +  dbKx * m_dbPasEspace;

        // Offre = min ( N( x0, t + dt ) -  N( x0, t ) , Qmax * dt )
        dbOffre = min ( dbNx0tp - m_pLstFrontiere[0]->GetN(1), dbQx * m_dbPasTemps);
    }
    else
		dbOffre = DBL_MAX;

    // Calcul de la demande :
    dbDemande = m_dbDebitEntree;

    // On en déduit N ( x0, t + dt) :
    if (dbOffre < dbDemande)
        m_pLstFrontiere[0]->SetN( std::min<double>(dbNx0tp, dbQx * m_dbPasTemps + m_pLstFrontiere[0]->GetN(1) ) );
    else
        m_pLstFrontiere[0]->SetN( m_pLstFrontiere[0]->GetN(1) + dbDemande * m_dbPasTemps );

    // Calcul de la vitesse
    if( dbOffre < dbDemande )
    {
        if(pParent->GetCondVitAval())
            m_pLstFrontiere[0]->SetVit( m_pLstFrontiere[1]->GetLastVit() );
        else
            m_pLstFrontiere[0]->SetVit( (pParent->GetCoeffAval1()*m_pLstFrontiere[1]->GetLastVit()) + (pParent->GetCoeffAval2()*m_pLstFrontiere[1]->GetBefLastVit()) );
    }
    else
        m_pLstFrontiere[0]->SetVit(  pTV->GetVx() );

    // Calcul de l'accélération
    m_pLstFrontiere[0]->SetAcc( 0 );

    // ----------------------------
    // CP de la dernière frontière
    // ----------------------------

    // Calcul de l'offre :
    if(pParent)
    {
        if(pParent->get_Type_aval() == 'R')
            dbOffre = m_dbDebitSortie;
        else
        {
            dbOffre = *(((Sortie *)pParent->getConnectionAval())->GetLstCapacites()->GetVariationEx(dbInstant));            
        }
    }

    // Calcul de la demande :

    // Demande = min ( N( xs - dx, t+dt) - N(xs, t2) , Qmax * dt )
    if(pParent->GetCondVitAmont())
        dbDemande = min ( m_pLstFrontiere[m_nNbCell-1]->GetN(1) - m_pLstFrontiere[m_nNbCell]->GetN(1), dbQx * m_dbPasTemps);
    else
        dbDemande = min ( ( pParent->GetCoeffAmont1()*m_pLstFrontiere[m_nNbCell-1]->GetN(2) + pParent->GetCoeffAmont2()*m_pLstFrontiere[m_nNbCell-1]->GetN(1) ) - m_pLstFrontiere[m_nNbCell]->GetN(1), dbQx * m_dbPasTemps);

    // ----------------------------
    // On en déduit N ( xs, t + dt)
    // ----------------------------

    if (dbOffre < dbDemande)
        m_pLstFrontiere[m_nNbCell]->SetN( m_pLstFrontiere[m_nNbCell]->GetN(1) + dbOffre * m_dbPasTemps);
    else
    {
        if(pParent->GetCondVitAmont())
            m_pLstFrontiere[m_nNbCell]->SetN( std::min<double>(m_pLstFrontiere[m_nNbCell-1]->GetN(1), dbQx * m_dbPasTemps + m_pLstFrontiere[m_nNbCell]->GetN(1) ));
        else
            m_pLstFrontiere[m_nNbCell]->SetN( std::min<double>( ( pParent->GetCoeffAmont1()*m_pLstFrontiere[m_nNbCell-1]->GetN(2) + pParent->GetCoeffAmont2()*m_pLstFrontiere[m_nNbCell-1]->GetN(1) ), dbQx * m_dbPasTemps + m_pLstFrontiere[m_nNbCell]->GetN(1) ));
    }

    // ----------------------------
    // Calcul de la vitesse
    // ----------------------------
    if( dbOffre < dbDemande )
    {
        // Etat congestionné
        double dbVal = VAL_NONDEF;
        GetDiagFonda()->CalculVitEqDiagrOrig(dbVitesse, dbVal, dbOffre, false);
        m_pLstFrontiere[m_nNbCell]->SetVit( std::min<double>(dbVitesse, m_dbVitReg) );
    }
    else
    {
        if(pParent->GetCondVitAmont())
            m_pLstFrontiere[m_nNbCell]->SetVit( std::min<double>(m_pLstFrontiere[m_nNbCell-1]->GetVit(1), m_dbVitReg) );
        else
            m_pLstFrontiere[m_nNbCell]->SetVit( std::min<double>(pParent->GetCoeffAmont1()*m_pLstFrontiere[m_nNbCell-1]->GetVit(2) + pParent->GetCoeffAmont2()*m_pLstFrontiere[m_nNbCell-1]->GetVit(1), m_dbVitReg) );
    }

    // ----------------------------
    // Calcul de l'accélération
    // ----------------------------
        m_pLstFrontiere[m_nNbCell]->SetAcc( 0 );


    for( i=0; i<m_nNbCell; i++)
    {
        m_pLstSegment[i]->ComputeTrafficEx(dbInstant);
    }
}

//================================================================
      void VoieMacro::ComputeTraffic
//----------------------------------------------------------------
// Fonction  : Calcul de la quantité N à l'instant nInstant
// Version du: 29/06/2006
// Remarque  : Les cas du premier et dernier segment sont traités
//             à part
// Historique: 29/06/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double dbInstant
)
{
    int             i;
    double          dbNAmont, dbNAval;
    TuyauMacro*     pParent;

	TypeVehicule *pTV = m_pReseau->m_LstTypesVehicule[0];

    pParent = (TuyauMacro*)GetParent();
    if(!pParent)
        return;

    //  Décalage des coordonnées des variables caractéristiques du trafic pour
    // garder uniquement l'historique pour le calcul de l'instant courant
    for( i=0; i<m_nNbCell+1; i++)
    {
        m_pLstFrontiere[i]->DecalVarTrafic();
    }


    for( i=1; i<m_nNbCell; i++)

    {
        if(pParent->GetCondVitAmont())        // La condition sur la vitesse en amont est-elle respectée ?
            // N_amont = N(x-dx, t-dt)
            dbNAmont = m_pLstFrontiere[i-1]->GetN(1);
        else
            // N_amont = C_amont_1 * N(x-dx, t-dt) +  C_amont_2 * N(x-dx, t-dt)
            dbNAmont = pParent->GetCoeffAmont1()*m_pLstFrontiere[i-1]->GetN(2) + pParent->GetCoeffAmont2()*m_pLstFrontiere[i-1]->GetN(1);



        if( dbInstant > m_pLstFrontiere[i]->GetNbPasTempsHist() )
        {
            if(i<m_nNbCell)
            {
                if(pParent->GetCondVitAval())
                    dbNAval = m_pLstFrontiere[i+1]->GetLastN() + pTV->GetKx() * m_dbPasEspace;
                else
                    dbNAval = (pParent->GetCoeffAval1()*m_pLstFrontiere[i+1]->GetLastN() + pParent->GetCoeffAval2()*m_pLstFrontiere[i+1]->GetBefLastN() )+ pTV->GetKx() * m_dbPasEspace;
            }
        }

        else
			dbNAval = DBL_MAX;

        m_pLstFrontiere[i]->SetN( std::min<double>(dbNAmont, dbNAval) ) ;
       // Calcul de la vitesse
        if( dbNAmont < dbNAval )
        {
            if(pParent->GetCondVitAmont())
                m_pLstFrontiere[i]->SetVit ( std::min<double>(m_pLstFrontiere[i-1]->GetVit(1), m_dbVitReg) );
            else
                // V = C_amont_1 * V(x-dx, t-dt) +  C_amont_2 * V(x-dx, t-dt)
                m_pLstFrontiere[i]->SetVit ( std::min<double>(pParent->GetCoeffAmont1()*m_pLstFrontiere[i-1]->GetVit(2) + pParent->GetCoeffAmont2()*m_pLstFrontiere[i-1]->GetVit(1), m_dbVitReg) );
        }
        else
            if(pParent->GetCondVitAval())
                m_pLstFrontiere[i]->SetVit ( std::min<double>(m_pLstFrontiere[i+1]->GetLastVit(), m_dbVitReg) );
            else
                m_pLstFrontiere[i]->SetVit ( std::min<double>(pParent->GetCoeffAval1()*m_pLstFrontiere[i+1]->GetLastVit() + pParent->GetCoeffAval2()*m_pLstFrontiere[i+1]->GetBefLastVit(), m_dbVitReg));

        // Calcul de l'accélération
        m_pLstFrontiere[i]->SetAcc (0);
    }
}

//================================================================
    CDiagrammeFondamental* VoieMacro::GetDiagFonda
//----------------------------------------------------------------
// Fonction  :  Renvoie le pointeur sur le diagramme fondamental
//              de la voie et remet à jour le contexte
//              (c'est le diagramme du tuyau parent)
// Version du:  05/01/2007
// Historique:  05/01/2007 (C.Bécarie - Tinea )
//              Création
//================================================================
(
)
{
    if(!GetParent())
        return NULL;

    if(!((TuyauMacro*)GetParent())->GetDiagFonda())
        return NULL;

    // Mise à jour du contexte par le nom de l'objet appelant        
    ((TuyauMacro*)GetParent())->GetDiagFonda()->SetContexte(m_strLabel);
    return ((TuyauMacro*)GetParent())->GetDiagFonda();
}

//================================================================
        VoieMicro::VoieMicro
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    m_dbNbVehSASEntree = 0;
    m_dbNbVehSASSortie = 1;

    m_dbNextInstSortie[std::string()] = 0;

    m_pPrevVoie = NULL;

    m_pLstCellAcoustique = NULL;
    m_nNbCellAcoustique = 0;	

    m_bChgtVoieObligatoire = false;
    m_bChgtVoieObligatoireEch = false;

    m_nNbVeh = 0;
    m_dbPosLastVeh = UNDEF_POSITION;
}

//================================================================
    VoieMicro::VoieMicro
//----------------------------------------------------------------
// Fonction  : Constructeur avec initialisation du numéro de voie
//             et largeur
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
        double  dbLargeur,
        double  dbVitReg
):Voie( dbLargeur, dbVitReg )
{
    m_dbNbVehSASEntree = 0;
    m_dbNbVehSASSortie = 1;

    m_dbNextInstSortie[std::string()] = 0;

    m_pLstCellAcoustique = NULL;

    m_bChgtVoieObligatoire = false;  
    m_bChgtVoieObligatoireEch = false;

    m_dbNextInstSortie[std::string()] = 0;

    m_pPrevVoie = NULL;

    m_nNbCellAcoustique = 0;
	
    m_nNbVeh = 0;
    m_dbPosLastVeh = UNDEF_POSITION;
}

//================================================================
    VoieMicro::~VoieMicro
//----------------------------------------------------------------
// Fonction  : Destructeur
// Version du: 13/12/2006
// Historique: 13/12/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    SupprimeCellAcoustique();
}


//================================================================
    bool VoieMicro::IsChgtVoieObligatoire
//----------------------------------------------------------------
// Fonction  : IsChgtVoieObligatoire
// Version du: 29/09/2011
// Historique: 29/09/2011 (O.Tonck - Ipsis)
//             Création
//================================================================
(
    TypeVehicule* pTV
)
{
    bool result = m_bChgtVoieObligatoire;

    for(size_t i = 0; i < m_LstExceptionsTypesVehiculesChgtVoieObligatoire.size() && result; i++)
    {
        if(m_LstExceptionsTypesVehiculesChgtVoieObligatoire[i] == pTV)
        {
            result = false;
        }
    }

    return result;
}
    

//================================================================
    void VoieMicro::ComputeDemand
//----------------------------------------------------------------
// Fonction  : Calcul de la demande
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
	double dbInstSimu,		// Instant de simulation
    double dbInstant        // Permet de calculer une demande au cours
)                           // du pas de temps (à dbInstant (en s) du début du pas de temps)
{                           // 0 par défaut
    boost::shared_ptr<Vehicule> pVeh;
    double      dbS;
    double      dbSc;
    double      dbDemandeAccBornee;
    double      dbP;
    double      dbM1;
    double      dbVit;
    CDiagrammeFondamental   *pDF;

    // Existe-t-il des véhicules sur la voie considérée
    pVeh = m_pReseau->GetFirstVehiculePrecalc(this);

    if(pVeh.get() == NULL)
        m_dbDemande = 0;
    else
    // La demande est calculée en utilisant du premier véhicule
    // Cette demande est constante entre 2 sorties de véhicule
    {
        pDF = pVeh->GetDiagFonda();
        
        if( m_pVehLeader != pVeh )    // Pour garder la demande constante...
        {
            // Calcul de l'espacement
            dbS = GetParent()->GetLength() - pVeh->GetPos(1);

            if(dbInstant > 0)
            {
                dbS = GetParent()->GetLength() - (pVeh->GetPos(1) + (m_pVehLeader->GetVit(1)*dbInstant));
            }

            // Espacement critique
            dbSc = 1. / pDF->GetKCritique();

            if( dbS < dbSc)
                m_dbDemandeSortie = pDF->GetDebitMax();
            else
            {
                if( dbS > 1/ pDF->GetKMax() )
                {
                    //m_dbDemandeSortie = GetDiagFonda()->GetVitesseLibre()/dbS;
                    //m_dbDemandeSortie = pVeh->GetVit(1)/dbS;
                    double dbVitEqu;
					double dbTmp;
					dbTmp = 1/dbS;
					double dbVal;
					dbVal = VAL_NONDEF;
                    pDF->CalculVitEqDiagrOrig(dbVitEqu, dbTmp, dbVal, false);
                    m_dbDemandeSortie = dbVitEqu/dbS;
                }
                else
                    m_dbDemandeSortie = 0;
            }

            m_dbDemande = m_dbDemandeSortie;
            m_pVehLeader = pVeh;
        }

        if ( GetReseau()->IsAccBornee() )
        {
               // Calcul du débit en prenant en compe l'accélération bornée
                dbP = pDF->GetW();

                if(dbInstant == 0)
                {
                    dbInstant =  GetPasTemps();
                    dbVit = pVeh->GetVit(1);
                }
                else
                {
                    dbInstant =  GetPasTemps() - dbInstant;
                    dbVit = dbP + sqrt( (dbP - pVeh->GetVit(1))*( dbP-pVeh->GetVit(1) ) - 2*pVeh->GetAccMax(pVeh->GetVit(1))* dbP*dbInstant);
                    dbVit = std::min<double>(dbVit,pDF->GetVitesseLibre());
                    dbVit = std::min<double>(dbVit, ((Tuyau*)GetParent())->GetVitRegByTypeVeh(pVeh->GetType(), dbInstSimu, pVeh->GetPos(1), GetNum() ) );
                }

                dbM1 = (dbP - dbVit );
                dbM1 = dbM1*dbM1;
                dbM1 = dbM1 - (2. * dbP * pVeh->GetAccMax(pVeh->GetVit(1)) * dbInstant );
                dbM1 = sqrt(dbM1);
                dbM1 = dbM1 + (dbP - dbVit);
                dbM1 = (dbM1 / pVeh->GetAccMax(pVeh->GetVit(1))) - dbInstant;

                dbDemandeAccBornee = dbM1 * dbP * pDF->GetKMax();

                if(dbInstant>0)
                    dbDemandeAccBornee = dbDemandeAccBornee / dbInstant;
                else
                    dbDemandeAccBornee = m_dbDemandeSortie;


                if( m_dbDemandeSortie > dbDemandeAccBornee)
                    m_dbDemandeSortie =   dbDemandeAccBornee;

                m_dbDemande = m_dbDemandeSortie;                    

        }
    }
}

//================================================================
    void VoieMicro::ComputeOffer
//----------------------------------------------------------------
// Fonction  : Calcul de l'offre
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    double dbInstant        // Permet de calculer une offre au cours
)                           // du pas de temps (à dbInstent (en s) du début du pas de temps)
{                           // 0 par défaut

    boost::shared_ptr<Vehicule> pVeh1, pVeh2;
    double      dbS, dbS1, dbS2;
    double      dbSc;
    double      dbVitEqu;

    // Existe-t-il des véhicules sur la voie considérée
    pVeh1 = m_pReseau->GetLastVehicule(this);

    if(!pVeh1)
        m_dbOffre = GetParent()->GetDebitMax();
    else
    // L'offre est calculée en utilisant l'espace entre les 2 véhicules créés
    {
        {
            {
                // Calcul de l'espacement (il est calcule comme le max des espacements des 2 derniers
                // véhicules créés)

                //dbS1 = pVeh1->GetPos(1);
                dbS1 = pVeh1->GetPos(1) + pVeh1->GetVit(1)*GetPasTemps();

                if(dbInstant>0)
                    dbS1 = pVeh1->GetPos(1) + pVeh1->GetVit(1)*dbInstant;

                pVeh2 = m_pReseau->GetNearAvalVehicule(this, pVeh1->GetPos(1)+0.001);

                if(pVeh2 && pVeh2->GetType()==pVeh1->GetType() )
                {
                    // Deuxième condition (pVeh2->GetType()==pVeh1->GetType() )
                    // Rustine temporaire pour gérer la création de véhicule lorque les 2
                    // véhicules permettant de calculer l'offre sont de types différent
                    // A revoir par la suite !!!

                    dbS2 = pVeh2->GetPos(1) - pVeh1->GetPos(1);

                    if(dbInstant>0)
                        dbS2 = pVeh2->GetPos(1) + (pVeh2->GetVit(1)*dbInstant) - (pVeh1->GetPos(1) + (pVeh1->GetVit(1)*dbInstant) );
                }
                else
                    dbS2 =0;

                dbS = std::max<double>(dbS1, dbS2);

                // Espacement critique
                dbSc = 1. / pVeh1->GetDiagFonda()->GetKCritique();

                if( dbS >= dbSc)
                    m_dbOffre = pVeh1->GetDiagFonda()->GetDebitMax();
                else
                    if( dbS > 0 )
                    {
						double dbTmp = 1/dbS;
						double dbVal = VAL_NONDEF;
                        pVeh1->GetDiagFonda()->CalculVitEqDiagrOrig(dbVitEqu, dbTmp, dbVal, false);
                        m_dbOffre = std::max<double>(0,dbVitEqu / dbS);

                    }
                    else
                        m_dbOffre = 0;

            }
        }
    }
}

//================================================================
    void VoieMicro::ComputeTraffic
//----------------------------------------------------------------
// Fonction  : Calcul des variables du trafic pour un pas de temps
// Version du: 17/07/2006
// Historique: 17/07/2006 (C.Bécarie - Tinea)
//================================================================
(
    double dbInstant
)
{
    TuyauMicro* pTuyau;
    double      dbNbVehSASEntreeOld;
    double      dbNbVehSASSortieOld;
    double      dbInstSortie;
    boost::shared_ptr<Vehicule> pVehicule;
    double      dbVit;
    bool        bGestionMicro;
    Repartiteur*    pRep;
    Entree*     pOrg;
    bool        bCreation = false;    

    pTuyau = (TuyauMicro*)GetParent();

    pOrg =  (Entree*)((TuyauMicro*)GetParent())->getConnectionAmont();        

    // MAJ des SAS
    m_dbInstSASSortieVide = NONVAL_DOUBLE;
    m_dbInstSASEntreePlein = NONVAL_DOUBLE;

    // En aval

    // Recherche du véhicule leader
    pVehicule = m_pReseau->GetFirstVehiculePrecalc(this);

    if(pVehicule)
    {
        dbNbVehSASSortieOld = m_dbNbVehSASSortie;
        m_dbNbVehSASSortie -= GetDebitSortie() * m_dbPasTemps;

        // Calcul de la trajectoire du premier véhicule
        if (!( m_dbNbVehSASSortie > ZERO_DOUBLE))
        {
            // Recherche du type de gestion de la connection
            // Si répartiteur micro, les véhicules ne sont pas supprimés mais passent le répartiteur
            bGestionMicro = false;
            if (pTuyau->get_Type_aval() =='R')
            {
                pRep = (Repartiteur*)pTuyau->getConnectionAval();
                if(pRep->IsMicro())
                bGestionMicro = true;
            }

            // Le premier véhicule sort
            if(!bGestionMicro)
            {             
            }

            // Calcul de l'instant de sortie du véhicule
            dbInstSortie =  dbNbVehSASSortieOld / (dbNbVehSASSortieOld - m_dbNbVehSASSortie) * GetPasTemps();
            m_dbInstSASSortieVide = dbInstSortie;

            m_dbNbVehSASSortie = 1;

            // Calcul de la demande par rapport au nouveau véhicule leader
            ComputeDemand(dbInstant, dbInstSortie);

            if(!bGestionMicro)
            {
                /*m_pVehLeader = NULL;
                m_pVehLeader = m_pReseau->GetFirstVehicule(this);*/
            }

            // Si répartiteur, il faudra relancer les calculs d'écoulement car la demande va changer
            GetReseau()->SetRelancerCalculRepartiteur();

            // Attribution de la vitesse de sortie pour la voie
            if( m_pReseau->IsAccBornee() )
            {
                dbVit = pVehicule->GetVitMax();
                if( pVehicule->GetVit(1) < std::min<double>(((Tuyau*)GetParent())->GetVitRegByTypeVeh(pVehicule->GetType(), dbInstant, pVehicule->GetPos(1), GetNum()), pVehicule->GetDiagFonda()->GetVitesseLibre() ) )
                {
                    dbVit = pVehicule->GetDiagFonda()->GetW() + sqrt( (pVehicule->GetDiagFonda()->GetW()-pVehicule->GetVit(1))*(pVehicule->GetDiagFonda()->GetW()-pVehicule->GetVit(1)) - 2*pVehicule->GetAccMax(pVehicule->GetVit(1))*dbInstSortie*pVehicule->GetDiagFonda()->GetW());
                }
                m_dbVitMaxSortie = std::min<double>(dbVit, pVehicule->GetDiagFonda()->GetVitesseLibre() );                
            }
            else
            {
                m_dbVitMaxSortie = pVehicule->GetDiagFonda()->GetVitesseLibre();                
            }
            // Vitesse du véhicule limitée à la vitesse réglementaire
            m_dbVitMaxSortie = std::min<double>( m_dbVitMaxSortie, ((Tuyau*)GetParent())->GetVitRegByTypeVeh(pVehicule->GetType(), dbInstant, pVehicule->GetPos(1), GetNum()) );
        }
    }

    // En amont
    dbNbVehSASEntreeOld = m_dbNbVehSASEntree;
    m_dbNbVehSASEntree += GetDebitEntree() * m_dbPasTemps;

    // Création des véhicules
 /*   if ( m_dbNbVehSASEntree >= 1 || fabs(m_dbNbVehSASEntree-1)<ZERO_DOUBLE   )
    {
        double  dbVitMaxEntree;

        // Recherche du type de gestion de la connection
        // Si répartiteur micro, les véhicules ne sont pas supprimés mais passent le répartiteur
        bGestionMicro = false;
        if (pTuyau->get_Type_amont() =='R')
        {
            pRep = (Repartiteur*)pTuyau->getConnectionAmont();
            if(pRep->IsMicro())
                bGestionMicro = true;
        }

        // Si répartiteur en amont, il faut récupérer la vitesse de sortie de celui-ci (au début du pas de temps)
        if (pTuyau->get_Type_amont() =='R')
            dbVitMaxEntree = ((Repartiteur *)pTuyau->getConnectionAmont())->GetVitesseMaxSortie(0, 0, dbInstant);
        else
            dbVitMaxEntree = GetParent()->GetVitesseMax();        

        // Calcul de l'instant de la création au cours du pas de temps
        if( GetDebitEntree() )
            dbInstCreation = (1-dbNbVehSASEntreeOld) / GetDebitEntree();
        m_dbNbVehSASEntree = m_dbNbVehSASEntree-1;

        if( fabs(m_dbNbVehSASEntree-1)<ZERO_DOUBLE)     // Pour éviter les erreurs d'arrondis
            dbInstCreation = GetPasTemps();

        m_dbInstSASEntreePlein = dbInstCreation;

        // Calcul de l'offre par rapport à l'avant-dernier véhicule créé
        ComputeOffer(dbInstCreation);

        // Si répartiteur, il faut relancer les calculs d'écoulement car l'offre va changer
        GetReseau()->SetRelancerCalculRepartiteur();

       dbVitMaxEntree = min(dbVitMaxEntree, GetParent()->GetVitesseMax() );

       if(!bGestionMicro)
       {
            m_pVehLast = m_pReseau->GetLastVehicule(this);

            TypeVehicule* pTV = ((TuyauMicro*)GetParent())->CalculTypeNewVehicule(dbInstant, GetNum() );            

            if(pTV)
            {
                dbVitMaxEntree = min( GetParent()->GetVitRegByTypeVeh(pTV, dbInstant), dbVitMaxEntree);

                pVehicule = new Vehicule( m_pReseau->IncLastIdVeh(), dbVitMaxEntree, pTV, GetPasTemps() );
                pVehicule->SetVoie(this);
                pVehicule->SetTuyau((TuyauMicro*)GetParent());
                pVehicule->SetInstantCreation(dbInstCreation);

				// Agressivité
				if(m_pReseau->IsProcAgressivite())
					pVehicule->SetAgressif(pOrg->IsAgressif(pTV));

				// Mise à jour de la liste des véhicules du doc XML				
				System::String ^ssType = gcnew System::String(pVehicule->GetType()->GetLibelle());
                System::String ^ssEntree = gcnew System::String(pOrg->getNom());

                String ^ssVoieEntree;
                if( m_pReseau->IsTraceRoute() )
                    ssVoieEntree = gcnew System::String( GetLabel() );
                else
                 ssVoieEntree = gcnew System::String("");

                m_pReseau->m_XmlDocTrafic->AddVehicule( pVehicule->GetID(), "", ssType, pVehicule->GetDiagFonda()->GetKMax(), pVehicule->GetDiagFonda()->GetVitesseLibre(), pVehicule->GetDiagFonda()->GetW(), ssEntree, dbInstant - GetPasTemps() + dbInstCreation, ssVoieEntree, pVehicule->IsAgressif() );

				delete(ssType);				
                delete(ssEntree);  
                delete(ssVoieEntree);

                // Ajout dans la liste des véhicules du réseau
                m_pReseau->AddVehicule(pVehicule);

                pOrg =  (Entree*)((TuyauMicro*)GetParent())->getConnectionAmont();
                pVehicule->SetOrigine(pOrg);
                pVehicule->SetHeureEntree( dbInstant );

                // Calcule de sa destination si besoin
                if( m_pReseau->IsUsedODMatrix() )
                {                                        
					pDst = pOrg->GetDestination(dbInstant, GetNum() );
                    pVehicule->SetDestination(pDst);

                    // Gestion des itinéraires ?
                    if(m_pReseau->IsCptItineraire())
                    {
                        m_pReseau->CalculeItineraire(pOrg, pDst, pVehicule->GetItineraire(), pVehicule->GetType() );
                    }
                }
             }

            // Incrémentation du compteur pour les entrées
            pTuyau = (TuyauMicro*)GetParent();
            if( pTuyau->get_Type_amont() == 'E'  )              
                ((Entree*)(pTuyau->getConnectionAmont()))->IncNbVehicule(dbInstant);

            // Si premier véhicule créé sur la voie, il faut recalculer la demande et le SAS de la sortie associée
            if(!m_pVehLeader)
            {
                ComputeDemand(dbInstant);
                m_dbInstSASSortieVide = dbInstCreation;
            }
        }
    }*/

}

//================================================================
    void VoieMicro::EndComputeTraffic
//----------------------------------------------------------------
// Fonction  : Fin du calcul des variables du trafic pour un pas de temps
// Version du: 01/09/2006
// Historique: 01/09/2006 (C.Bécarie - Tinea)
//================================================================
(
    double dbInstant
)
{
    // MAJ des SAS

    // En aval
    if(m_dbInstSASSortieVide!=NONVAL_DOUBLE)
    {
        m_dbNbVehSASSortie = m_dbNbVehSASSortie - GetDebitSortie()*(m_dbPasTemps-m_dbInstSASSortieVide);
    }
}

//================================================================
    int Voie::GetNum
//----------------------------------------------------------------
// Fonction  : Retourne le numéro de la voie
// Version du: 12/09/2006
// Historique: 12/09/2006 (C.Bécarie - Tinea)
//================================================================
(
)
{
    Tuyau*  pTuyau;

    pTuyau = (Tuyau*)m_pParent;

    if(!pTuyau)
        return -1;

    for(int i=0; i<pTuyau->getNbVoiesDis();i++)
    {
        if ( pTuyau->GetLstLanes()[i] == this )
            return i;
    }
    return -1;
}

//================================================================
      bool VoieMicro::Discretisation
//----------------------------------------------------------------
// Fonction  : Segmentation d'une voie en cellules
// Remarques : dans le cas d'une voie micro, ce sont des cellules
//              acoustiques
// Version du: 13/10/20006
// Historique: 13/10/20006 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    int nNbVoie
)
{
    Segment*                pCell;
    int                     i;
    double                  dbX, dbY, dbZ;
    double                  dbAbsAmont,dbAbsAval;
    double                  dbOrdAmont,dbOrdAval;
    double                  dbHautAmont,dbHautAval;
    int                     nNbCell;

    // ménage
    SupprimeCellAcoustique();

	if( ((TuyauMicro*)GetParent())->GetNbCellAcoustique() == 0 && ((TuyauMicro*)GetParent())->GetAcousticCellLength()<=0 )
		return false;

    nNbCell = ((TuyauMicro*)GetParent())->GetNbCellAcoustique();

    if(nNbCell==0)
	{
		nNbCell = (int)ceil(((TuyauMicro*)GetParent())->GetLength() / ((TuyauMicro*)GetParent())->GetAcousticCellLength());
		((TuyauMicro*)GetParent())->SetNbCellAcoustique(nNbCell);		
	}
        

    dbX = (GetAbsAval()-GetAbsAmont())/nNbCell;
    dbY = (GetOrdAval()-GetOrdAmont())/nNbCell;
    dbZ = (GetHautAval()-GetHautAmont())/nNbCell;

    // création des listes
    m_pLstCellAcoustique   = new Segment*[nNbCell];
    m_nNbCellAcoustique = nNbCell;
	m_nNbCell =  nNbCell;

    for( i=0; i<nNbCell; i++)
    {                                
		std::string sTmp = m_strLabel + "C" + to_string(i);

        dbAbsAmont= GetAbsAmont()+dbX*i;
        dbAbsAval = GetAbsAmont()+dbX*(i+1);
        dbOrdAmont= GetOrdAmont()+dbY*i;
        dbOrdAval = GetOrdAmont()+dbY*(i+1);
        dbHautAmont= GetHautAmont()+dbZ*i;
        dbHautAval = GetHautAmont()+dbZ*(i+1);

        pCell = new Segment(m_pReseau->IncLastIdSegment(), m_dbVitReg);
        m_pLstCellAcoustique[i] = pCell;

        m_pLstCellAcoustique[i]->SetPropCom(m_pReseau, this, sTmp, m_strRevetement);
        m_pLstCellAcoustique[i]->SetExtAmont(dbAbsAmont, dbOrdAmont, dbHautAmont);
        m_pLstCellAcoustique[i]->SetExtAval(dbAbsAval, dbOrdAval, dbHautAval);

        m_pLstCellAcoustique[i]->CalculeLongueur(0);
    }

    return true;
};

//================================================================
    void VoieMicro::SupprimeCellAcoustique
//----------------------------------------------------------------
// Fonction  : Suppression et destruction de la liste des cellules
//             acoustqiues
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    int i;

    if(m_pLstCellAcoustique != NULL)
    {
        for( i=0; i< m_nNbCellAcoustique; i++)
        {
            delete m_pLstCellAcoustique[i];
            m_pLstCellAcoustique[i]=NULL;
        }
        delete [] m_pLstCellAcoustique;
        m_pLstCellAcoustique = NULL;
    }
}

//================================================================
    void VoieMicro::InitAcousticVariables
//----------------------------------------------------------------
// Fonction  : Initialisation des variables acoustiques à chaque
//             pas de temps
// Version du: 17/10/2006
// Historique: 17/10/2006 (C.Bécarie - Tinea)
//================================================================
(
)
{
    if(m_pLstCellAcoustique)
        for(int i=0; i<m_nNbCell; i++)
            m_pLstCellAcoustique[i]->InitAcousticVariables();
}

//================================================================
        void VoieMicro::EmissionOutput
//----------------------------------------------------------------
// Fonction  : Sortie des émissions acoustiques
// Version du: 23/10/2006
// Historique: 23/10/2006 (C.Bécarie - Tinea)
//             Création
//================================================================
(    
)
{
    for(int i=0;i<m_nNbCell;i++)
    {
        m_pLstCellAcoustique[i]->EmissionOutput();
    }
}

//================================================================
      bool VoieMicro::InitSimulation
//----------------------------------------------------------------
// Fonction  : Initialisation d'une simulation
// Version du: 19/11/2004
// Historique: 19/11/2004 (C.Bécarie - Tinea)
//             Création
//================================================================
(
        bool			bCalculAcoustique,
        bool            bSirane,
		std::string     strTuyau
)
{
    int     i;
    bool    bOk = true;

    if(m_pLstCellAcoustique)
    {
		for(i=0;i<m_nNbCellAcoustique;i++)
        {
                bOk &= m_pLstCellAcoustique[i]->InitSimulation(bCalculAcoustique, bSirane, strTuyau);
        }
    }

    // Calcul du premier instant de sortie
    Tuyau *pTAval;
    SymuViaTripNode *pDest;
    pTAval = (Tuyau*)GetParent();

    if(pTAval->get_Type_aval() =='S' || pTAval->get_Type_aval() =='P')
    {
        pDest = m_pReseau->GetDestinationSymuViaTripNode(pTAval->getConnectionAval());

        m_dbNextInstSortie[std::string()] = pDest->GetNextEnterInstant( pTAval->getNbVoiesDis(), 0, m_pReseau->GetTimeStep(), m_pReseau->GetTimeStep(), std::string() );
    }

    m_dbNbVehSASEntree = 0;
    m_dbNbVehSASSortie = 1;

    m_pVehLeader.reset(); 

    return bOk;
}


	  // Met à jour le nombre de véhicules présents sur la voie au début du pas de temps
	  void VoieMicro::UpdateNbVeh()
	  {
		  vector<boost::shared_ptr<Vehicule>>::iterator itVeh;		  

		  m_nNbVeh = 0;
		  m_dbPosLastVeh = m_dbLongueur;

		  for(itVeh = m_pReseau->m_LstVehicles.begin(); itVeh != m_pReseau->m_LstVehicles.end(); itVeh++)
			  if( (*itVeh)->GetVoie(0) == this )
			  {
				  m_nNbVeh++;
				  if( (*itVeh)->GetPos(0) < m_dbPosLastVeh )
					m_dbPosLastVeh = (*itVeh)->GetPos(0);
			  }
	  }

double	VoieMicro::GetLongueurEx(TypeVehicule * pTV)
{
	if( IsChgtVoieObligatoire(pTV) )
		return (m_dbLongueur - m_pReseau->GetChgtVoieGhostBevelLength());

	return m_dbLongueur;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void VoieIndex::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void VoieIndex::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void VoieIndex::serialize(Archive& ar, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(m_sTuyauID);
	ar & BOOST_SERIALIZATION_NVP(m_nNumVoie);
}

template void LigneDeFeu::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void LigneDeFeu::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void LigneDeFeu::serialize(Archive& ar, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(m_dbPosition);
	ar & BOOST_SERIALIZATION_NVP(m_pControleur);
    ar & BOOST_SERIALIZATION_NVP(m_pTuyAm);
	ar & BOOST_SERIALIZATION_NVP(m_pTuyAv);
    ar & BOOST_SERIALIZATION_NVP(nVAm);
	ar & BOOST_SERIALIZATION_NVP(nVAv);
}

template void Voie::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Voie::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Voie::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Troncon);

    ar & BOOST_SERIALIZATION_NVP(m_pVoieGauche);
    ar & BOOST_SERIALIZATION_NVP(m_pVoieDroite);
    ar & BOOST_SERIALIZATION_NVP(m_dbLargeur);

    ar & BOOST_SERIALIZATION_NVP(m_bIsVoieCibleInterdite);
    ar & BOOST_SERIALIZATION_NVP(m_bIsRabattementDevantVehiculeGuideInterdit);

    ar & BOOST_SERIALIZATION_NVP(m_LstLignesFeux);
}

template void VoieMacro::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void VoieMacro::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void VoieMacro::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Voie);

    int nbSegment = m_nNbCell;
    SerialiseTab<Archive, SegmentMacro*>(ar, "m_pLstSegment", m_pLstSegment, nbSegment);
    int nbFrontiere = m_nNbCell+1;
    SerialiseTab<Archive, Frontiere*>(ar, "m_pLstFrontiere", m_pLstFrontiere, nbFrontiere);
}

template void VoieMicro::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void VoieMicro::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void VoieMicro::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Voie);

    SerialiseTab<Archive, Segment*>(ar, "m_pLstCellAcoustique", m_pLstCellAcoustique, m_nNbCellAcoustique);

    ar & BOOST_SERIALIZATION_NVP(m_bChgtVoieObligatoire);
    ar & BOOST_SERIALIZATION_NVP(m_bChgtVoieObligatoireEch);
    ar & BOOST_SERIALIZATION_NVP(m_LstExceptionsTypesVehiculesChgtVoieObligatoire);
    ar & BOOST_SERIALIZATION_NVP(m_pPrevVoie);

    ar & BOOST_SERIALIZATION_NVP(m_dbNextInstSortie);

    ar & BOOST_SERIALIZATION_NVP(m_dbNbVehSASEntree);
    ar & BOOST_SERIALIZATION_NVP(m_dbNbVehSASSortie);
    ar & BOOST_SERIALIZATION_NVP(m_dbInstSASEntreePlein);
    ar & BOOST_SERIALIZATION_NVP(m_dbInstSASSortieVide);

    ar & BOOST_SERIALIZATION_NVP(m_dbInstSASSortieVide);
    ar & BOOST_SERIALIZATION_NVP(m_dbDemandeSortie);
    ar & BOOST_SERIALIZATION_NVP(m_pVehLeader);
    ar & BOOST_SERIALIZATION_NVP(m_pVehLast);
    ar & BOOST_SERIALIZATION_NVP(m_nNbVeh);
    ar & BOOST_SERIALIZATION_NVP(m_dbPosLastVeh);

    ar & BOOST_SERIALIZATION_NVP(m_LstVehiculesAnt);
}