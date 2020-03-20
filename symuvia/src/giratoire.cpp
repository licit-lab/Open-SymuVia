#include "stdafx.h"
#include "giratoire.h"

#include "divergent.h"
#include "reseau.h"
#include "RandManager.h"
#include "convergent.h"
#include "tuyau.h"
#include "voie.h"
#include "vehicule.h"
#include "SystemUtil.h"

#include <boost/make_shared.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/shared_ptr.hpp>

using namespace std;

//================================================================
    Giratoire::Giratoire
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 20/11/2007
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{   
} 

//================================================================
    Giratoire::Giratoire
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 20/11/2007
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    char    *strID, 
    double  dbVitMax, 
    std::string strRevetement,
    double  dbTagreg, 
    double  dbGamma, 
    double  dbMu,
    int     nVoie,
    double  dbLargeurVoie, 
    char    cType, 
    int     nm,
    double  dbBeta,
    double  dbBetaInt,
    bool    bTraversees,
    Reseau  *pReseau
    ):BriqueDeConnexion(strID, pReseau, 'G', bTraversees)
{   
#pragma warning( disable : 4996 )
    strcpy(m_strNom, strID);
#pragma warning( disable : 4996 )

    m_dbVitMax = dbVitMax;
    m_strRevetement = strRevetement;
    
    m_dbTagreg = dbTagreg;
    m_dbGamma = dbGamma;
    m_dbMu = dbMu;
    m_nVoie = nVoie;
    m_dbLargeurVoie = dbLargeurVoie;
    m_cType = cType;
    m_dbBeta = dbBeta;
    m_dbBetaInt = dbBetaInt;


    m_pReseau = pReseau;
} 

//================================================================
    Giratoire::~Giratoire
//----------------------------------------------------------------
// Fonction  : Destructeur
// Version du: 16/09/2008
// Historique: 16/09/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{  
    // Nettoyage des groupes de traversées
    for(size_t i = 0; i < m_LstGrpTraEntree.size(); i++)
    {
        delete m_LstGrpTraEntree[i].pGrpTra;
    }
    for(size_t i = 0; i < m_LstGrpTraSortie.size(); i++)
    {
        delete m_LstGrpTraSortie[i].pGrpTra;
    }
} 

//================================================================
    void Giratoire::Init
//----------------------------------------------------------------
// Fonction  : Initialisation du giratoire (construction des tronçons
// de l'anneau)
// Version du: 20/11/2007
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{    
    TuyauMicro  *pT, *pTNext, *pTIP, *pTIAv;    
    Convergent  *pCvgt;
    Divergent   *pDvgt;    
    double dbExtAmX, dbExtAmY, dbExtAvX, dbExtAvY, dbExtAmZ, dbExtAvZ;
    std::deque<TuyauMicro*> LstTI;                  // Liste des tuyaux internes créés
    std::string strTmp;
    
    for(int i=0; i<(int)m_LstTAmAv.size();i++)
    {
        pT = m_LstTAmAv[i];

        if(i+1 <(int)m_LstTAmAv.size())        
            pTNext = m_LstTAmAv[i+1];
        else
            pTNext = m_LstTAmAv[0];

        // Création du tuyau interne en aval		
        if( pT->GetBriqueAval() == this)
        {
            dbExtAmX = pT->GetAbsAval();
            dbExtAmY = pT->GetOrdAval();
            dbExtAmZ = pT->GetHautAval();
        }
        else
        {
            dbExtAmX = pT->GetAbsAmont();
            dbExtAmY = pT->GetOrdAmont();
            dbExtAmZ = pT->GetHautAmont();
        }

        if( pTNext->GetBriqueAmont() == this)
        {
            dbExtAvX = pTNext->GetAbsAmont();
            dbExtAvY = pTNext->GetOrdAmont();
            dbExtAvZ = pTNext->GetHautAmont();
        }
        else
        {
            dbExtAvX = pTNext->GetAbsAval();
            dbExtAvY = pTNext->GetOrdAval();
            dbExtAvZ = pTNext->GetHautAval();
        }      

        std::vector<double> largeursVoies(m_nVoie, m_dbLargeurVoie);
        pTIAv = new TuyauMicro(m_pReseau, pT->GetLabel()+pTNext->GetLabel(), '0', '0', NULL, NULL, m_cType, m_strRevetement, largeursVoies, 
            dbExtAmX,  dbExtAmY, dbExtAvX,  dbExtAvY, dbExtAmZ, dbExtAvZ, 
            m_nVoie, m_pReseau->GetTimeStep(),  0, m_dbVitMax, DBL_MAX, 0, "");

        pTIAv->SetBriqueParente(this);

        LstTI.push_back(pTIAv);
		        
		pTIAv->AddTypeVeh(m_pReseau->GetLstTypesVehicule()[0]);
        
        // Ajout à la liste des tuyaux et des tuyaux micro du réseau
        m_pReseau->GetLstTuyauxMicro()->push_back(pTIAv);
        m_pReseau->GetLstTuyaux()->push_back(pTIAv);
        m_pReseau->m_mapTuyaux[pTIAv->GetLabel()] = pTIAv;

        // Ajout à la liste des tuyaux internes du giratoire
        m_LstTInt.push_back(pTIAv);

        pTIAv->SetID( (int)m_pReseau->GetLstTuyauxMicro()->size());

        // Calcul de la longueur et segmentation
        pTIAv->CalculeLongueur(0);      
        pTIAv->Segmentation();
    }

    for(int i=0; i<(int)m_LstTAmAv.size();i++)
    {
        pT = m_LstTAmAv[i];

        if(i==0)
            pTIP = LstTI[ LstTI.size()-1 ];
        else
            pTIP = LstTI[ i-1 ];

        // Si tronçon d'entrée
        if(pT->GetBriqueAval()==this)
        {
            // Création d'un convergent   
            strTmp = "C_" + pT->GetLabel();
			pCvgt = new Convergent(strTmp, m_dbTagreg, m_dbGamma, m_dbMu, m_pReseau);            
            
            pCvgt->AddEltAmont(pTIP);
            pCvgt->AddEltAmont(pT); 

            pCvgt->AddEltAval(LstTI[i]);

            pCvgt->SetGiratoire(this);

            m_pReseau->AddConvergent(strTmp, pCvgt );

            for(int j=0; j<(int)m_LstTf.size(); j++)
                pCvgt->AddTf( m_LstTf[j].pTV, m_LstTf[j].dbtc );

            // MAJ des tronçons internes et d'entrée
            LstTI[i]->setConnectionAmont(pCvgt);
            LstTI[i]->SetTypeAmont('C');
            pTIP->setConnectionAval(pCvgt);
            pTIP->SetTypeAval('C');
            pT->setConnectionAval(pCvgt);
            pT->SetTypeAval('C');            

        }
        else    // Tronçon de sortie du giratoire
        {
            // Création d'un divergent    
            strTmp = "D_" + pT->GetLabel();
			pDvgt = new Divergent(strTmp, m_pReseau);            
            pDvgt->Init( pTIP, pT, LstTI[i]);  

            m_pReseau->AddDivergent(strTmp, pDvgt);

            // MAJ des tronçons internes et de sortie
            LstTI[i]->setConnectionAmont(pDvgt);
            LstTI[i]->SetTypeAmont('D');
            pTIP->setConnectionAval(pDvgt);
            pTIP->SetTypeAval('D');
            pT->setConnectionAmont(pDvgt);
            pT->SetTypeAmont('D');
        }
    }
}

//================================================================
    void    Giratoire::AddTf
//----------------------------------------------------------------
// Fonction  : Ajout un temps d'insertion pour un type de véhicule
// Remarque  : 
// Version du: 23/11/2007
// Historique: 23/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    TypeVehicule *pTV, 
    double dbTf,
    double dbTt
)
{
    TempsCritique TpsIns;
    TempsCritique TpsTra;

    TpsIns.pTV = pTV;
    TpsIns.dbtc = dbTf;

    m_LstTf.push_back(TpsIns);

    TpsTra.pTV = pTV;
    TpsTra.dbtc = dbTt;
    m_LstTt.push_back(TpsTra);
}

//================================================================
    bool    Giratoire::GetTuyauxInternesRestants
//----------------------------------------------------------------
// Fonction  : Retourne la liste des tuyaux internes du giratoire
//             entre le tuyau interne passé en paramèters et 
//             le tuyau aval pTAv
// Remarque : en effet, pas besoin de connaitre le tronçon
//            d'entrée dans le giratoire.
// Version du: 14/11/2012
// Historique: 14/11/2012 (O.Tonck - IPSIS)
//             Création
//================================================================
( 
    TuyauMicro *pTInt, 
    Tuyau *pTAv, 
    std::vector<Tuyau*> &dqTuyaux
)
{
    Tuyau   *pTmp;    

    pTmp = pTInt;

    while( pTmp != pTAv)
    {        
        dqTuyaux.push_back(pTmp);        

        if( pTmp->get_Type_aval() == 'C' )
        {
            pTmp = pTmp->getConnectionAval()->m_LstTuyAv.front();
        }
        else if( pTmp->get_Type_aval() == 'D' )
        {
            if( pTmp->getConnectionAval()->m_LstTuyAv.front() == pTAv)
                return true;
            else
                pTmp = pTmp->getConnectionAval()->m_LstTuyAv[1];
        }
    }
    return true;
}

//================================================================
    bool    Giratoire::GetTuyauxInternes
//----------------------------------------------------------------
// Fonction  : Retourne la liste des tuyaux internes du giratoire
//             définissant l'itinéraire de pTAm à pTAv
// Version du: 30/09/2008
// Historique: 30/09/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
( 
    Tuyau *pTAm, 
    Tuyau *pTAv, 
    std::vector<Tuyau*> &dqTuyaux
)
{
    Tuyau   *pTmp;    

    pTmp = pTAm->getConnectionAval()->m_LstTuyAv.front();

    while( pTmp != pTAv)
    {        
        dqTuyaux.push_back(pTmp);        

        if( pTmp->get_Type_aval() == 'C' )
        {
            pTmp = pTmp->getConnectionAval()->m_LstTuyAv.front();
        }
        else if( pTmp->get_Type_aval() == 'D' )
        {
            if( pTmp->getConnectionAval()->m_LstTuyAv.front() == pTAv)
                return true;
            else
                pTmp = pTmp->getConnectionAval()->m_LstTuyAv[1];
        }
    }
    return true;
}

//================================================================
    bool    Giratoire::GetTuyauxInternes
//----------------------------------------------------------------
// Fonction  : Retourne la liste des tuyaux internes du giratoire
//             définissant l'itinéraire d'une voie amont à une voie aval
// Version du: 30/09/2008
// Historique: 30/09/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
( 
    Tuyau	*pTAm, 
	int		nVAm,
    Tuyau	*pTAv,
	int		nVAv,
    std::vector<Tuyau*> &dqTuyaux
)
{
    return GetTuyauxInternes(pTAm, pTAv, dqTuyaux);
}

//================================================================
    set<Tuyau*>    Giratoire::GetAllTuyauxInternes
//----------------------------------------------------------------
// Fonction  : Retourne la liste de tous les tuyaux internes
//             potentiellement utilisables entre le tuyau aval
//             et amont donnés en paramètres
// Version du: 22/05/2013
// Historique: 22/05/2013 (O. Tonck - Ipsis)
//             Création
//================================================================
( 
    Tuyau	*pTAm, 
    Tuyau	*pTAv
)
{
    set<Tuyau*> result;
    vector<Tuyau*> tuyauxInt;
    if(GetTuyauxInternes(pTAm, pTAv, tuyauxInt))
    {
        result.insert(tuyauxInt.begin(), tuyauxInt.end());
    }
    return result;
}

//================================================================
    void Giratoire::FinInit
//----------------------------------------------------------------
// Fonction  : Fin d'initialisation du giratoire 
// Version du: 20/10/2008
// Historique: 20/10/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(    
)
{
    TuyauMicro  *pT;
    TuyauMicro  *pTNext;
    VoieMicro   *pVNext;

	// Vérification si tous les tronçons internes ont été segmentés
	for(int i=0; i<(int)m_LstTInt.size();i++)
	{
		pT = m_LstTInt[i];
		if( pT->GetLstLanes().size() == 0)
			pT->Segmentation();
	}

    // Construction des mouvements des convergents aux entrées du giratoire
    for(int i=0; i<(int)m_LstTInt.size();i++)
    {
        pT = m_LstTInt[i];
        if( pT->get_Type_amont() == 'C')
        {
            Convergent *pCvgt = (Convergent*)pT->getConnectionAmont();

            pCvgt->Init(pCvgt->m_LstTuyAv.front());  

            if(pCvgt)
            {
                for(size_t j=0; j < (size_t)m_nVoie; j++)
                {
                    pCvgt->AddMouvement(  (VoieMicro*)pCvgt->m_LstTuyAm.front()->GetLstLanes()[j], (VoieMicro*)pT->GetLstLanes()[j], 1 );  // Mouvement prioritaire (anneau)
                    // Mouvements non prioritaires (branche d'entrée)
                    // OTK - 04/09/2012 - correction bug n°13
                    pCvgt->AddMouvement(  (VoieMicro*)pCvgt->m_LstTuyAm[1]->GetLstLanes()[min(j, pCvgt->m_LstTuyAm[1]->GetLstLanes().size() -1)], (VoieMicro*)pT->GetLstLanes()[j], 2 );  
                }
            }

            pCvgt->FinInit(pCvgt->m_LstTuyAv.front(), 10);
        }
    }

    // Initialisation des voies précédentes des voies internes du giratoire
    for(int i=0; i<(int)m_LstTInt.size();i++)
    {
        pT = m_LstTInt[i];
        for(int j=0; j<pT->getNb_voies(); j++)
        {
            if( pT->get_Type_aval() == 'D')
            {
                pTNext = (TuyauMicro*)pT->getConnectionAval()->m_LstTuyAv[1];
                pVNext = (VoieMicro*)pTNext->GetLstLanes()[j];
                pVNext->SetPrevVoie((VoieMicro*)pT->GetLstLanes()[j]);
            }
            else
            {
                pTNext = (TuyauMicro*)pT->getConnectionAval()->m_LstTuyAv.front();
                pVNext = (VoieMicro*)pTNext->GetLstLanes()[j];
                pVNext->SetPrevVoie((VoieMicro*)pT->GetLstLanes()[j]);
            }
        }
    }

    // Ajout des traversées pour les véhicules s'insérant sur le tronçon interne
    for(int i=0; i<(int)m_LstTAmAv.size();i++)
    {
        pT = m_LstTAmAv[i];

        if( pT->GetBriqueAval() == this) // Tronçon d'entrée du giratoire
        {
            // Pour chaque voie du tronçon d'entrée et chaque voie interne du giratoire sauf la voie extérieure
            int nbVoie = pT->getNbVoiesDis();
            for(int iVoie = 0; iVoie < nbVoie; iVoie++)
            {
                VoieMicro * pVoieEntree = (VoieMicro*)pT->GetLstLanes()[iVoie];
                int nbVoieInt = GetNbVoie();
                // Pas de traversée si giratoire monovoie
                for(int iVoieDes = 1; iVoieDes < nbVoieInt; iVoieDes++)
                {
                    VoieMicro * pVoieDes = (VoieMicro*)pT->getConnectionAval()->m_LstTuyAm.front()->GetLstLanes()[iVoieDes];

                    // Création du groupe de traversée
                    GrpPtsConflitTraversee *pGrpTra = new GrpPtsConflitTraversee();
                    pGrpTra->dbDstPtAttente = pVoieEntree->GetLength();
                    pGrpTra->dbPosPtAttente = pVoieEntree->GetLength();
                    pGrpTra->pVPtAttente = pVoieEntree;
			        pGrpTra->nNbMaxVehAttente = 0;
                    pGrpTra->dbInstLastTraversee = UNDEF_INSTANT;
                    pGrpTra->LstTf = m_LstTfTra;  

                    for(int iVoieInt = 0; iVoieInt < iVoieDes; iVoieInt++)
                    {
                        boost::shared_ptr<PtConflitTraversee> pPtTra = boost::make_shared<PtConflitTraversee>();

                        pPtTra->pT1 = pVoieEntree;
                        pPtTra->pT2 = (VoieMicro*)pT->getConnectionAval()->m_LstTuyAm.front()->GetLstLanes()[iVoieInt]; // Voie interne en amont du convergent d'insertion dans le giratoire

                        pPtTra->pTProp = pPtTra->pT2;

                        pPtTra->dbPos1 = pPtTra->pT1->GetLength();
                        pPtTra->dbPos2 = pPtTra->pT2->GetLength();
                        pPtTra->dbDst1 = pPtTra->pT1->GetLength();
                        pPtTra->dbDst2 = pPtTra->pT2->GetLength();
                
                        pGrpTra->lstPtsConflitTraversee.push_back(pPtTra);
                    }

                    // Association du groupe aux voies d'entrée et de sortie et ajout a la liste
                    GroupeTraverseeEntreeGir grpEntree;
                    grpEntree.pVoieEntree = pVoieEntree;
                    grpEntree.pVoieInterne = pVoieDes;
                    grpEntree.pGrpTra = pGrpTra;
                    m_LstGrpTraEntree.push_back(grpEntree);
                }
            }
        }
        else // Tronçon de sortie du giratoire
        {
            // Pour chaque voie du tronçon interne à l'exception de la voie extérieure
            int nbVoieInt = GetNbVoie();
            // Pas de traversée si giratoire monovoie
            for(int iVoieSource= 1; iVoieSource < nbVoieInt; iVoieSource++)
            {
                VoieMicro * pVoieSource = (VoieMicro*)pT->getConnectionAmont()->m_LstTuyAm.front()->GetLstLanes()[iVoieSource];

                // Création du groupe de traversée
                GrpPtsConflitTraversee *pGrpTra = new GrpPtsConflitTraversee();
                pGrpTra->dbDstPtAttente = pVoieSource->GetLength();
                pGrpTra->dbPosPtAttente = pVoieSource->GetLength();
                pGrpTra->pVPtAttente = pVoieSource;
			    pGrpTra->nNbMaxVehAttente = 0;
                pGrpTra->dbInstLastTraversee = UNDEF_INSTANT;
                pGrpTra->LstTf = m_LstTfTra;  

                for(int iVoieInt = 0; iVoieInt < iVoieSource; iVoieInt++)
                {
                    boost::shared_ptr<PtConflitTraversee> pPtTra = boost::make_shared<PtConflitTraversee>();

                    pPtTra->pT1 = pVoieSource;
                    pPtTra->pT2 = (VoieMicro*)pT->getConnectionAmont()->m_LstTuyAm.front()->GetLstLanes()[iVoieInt]; // Voie interne en amont du divergent d'insertion dans le giratoire

                    pPtTra->pTProp = pPtTra->pT2;

                    pPtTra->dbPos1 = pPtTra->pT1->GetLength();
                    pPtTra->dbPos2 = pPtTra->pT2->GetLength();
                    pPtTra->dbDst1 = pPtTra->pT1->GetLength();
                    pPtTra->dbDst2 = pPtTra->pT2->GetLength();
                
                    pGrpTra->lstPtsConflitTraversee.push_back(pPtTra);
                }

                // Association du groupe à la voie interne et au tuyau de sortie et ajout a la liste
                GroupeTraverseeSortieGir grpSortie;
                grpSortie.pTuyauSortie = (TuyauMicro*)pT;
                grpSortie.pVoieInterne = pVoieSource;
                grpSortie.pGrpTra = pGrpTra;
                m_LstGrpTraSortie.push_back(grpSortie);
            }
        }
    }

    // Calcul une fois pour toutes des couts à vides pour chaque mouvement du giratoire
    for(size_t i=0; i<m_LstTAmAv.size();i++)
    {
        pT = m_LstTAmAv[i];

        if( pT->GetBriqueAval() == this) // Tronçon d'entrée du giratoire
        {
            for(size_t j=0; j<m_LstTAmAv.size();j++)
            {
                pTNext = m_LstTAmAv[j];
                if( pTNext->GetBriqueAval() != this) // Tronçon de sortie du giratoire
                {
                    std::vector<Tuyau*> lstTuyauxInternes;
                    GetTuyauxInternes(pT, pTNext, lstTuyauxInternes);
                    TypeVehicule * pTypeVeh;
                    for (size_t iTypeVeh = 0; iTypeVeh < m_pReseau->m_LstTypesVehicule.size(); iTypeVeh++)
                    {
                        pTypeVeh = m_pReseau->m_LstTypesVehicule[iTypeVeh];
                        double dbCost = 0;
                        for (size_t k = 0; k < lstTuyauxInternes.size(); k++)
                        {
                            // rmq. : on n'utilise pas les méthodes GetCoutAVide qui demande un type de véhicule
                            // à cause des variantes qu'il peut y avoir sur les troncon non internes, qui ne nous concernent pas ici.
                            double dbVitReg = std::min<double>(lstTuyauxInternes[k]->GetVitReg(), pTypeVeh->GetVx());
                            dbCost += lstTuyauxInternes[k]->GetLength() / dbVitReg;
                        }
                        m_LstCoutsAVide[std::make_pair(pT, pTNext)][pTypeVeh] = dbCost;
                    }
                }
            }
        }
    }
}

double Giratoire::GetAdditionalPenaltyForUpstreamLink(Tuyau* pTuyauAmont)
{
    // rmq. : ajout d'une pénalité additive pour l'insertion et une autre pour la sortie du giratoire
    return m_pReseau->GetNonPriorityPenalty() + m_pReseau->GetRightTurnPenalty();
}

bool isBriqueAmontGir(Tuyau * pTuyau, void * pArg)
{
    BriqueDeConnexion * pBrique = (BriqueDeConnexion*)pArg;
    return pTuyau->GetBriqueAmont() == pBrique;
}

// Gestion des traversées des giratoires
void Giratoire::CalculTraversee(Vehicule *pVeh, double dbInstant, std::vector<int> & vehiculeIDs)
{
    if(m_bSimulationTraversees)
    {
        // On détecte si le véhicule entre dans le giratoire au cours du pas de temps
        // pour gérer une éventuelle traversée d'entrée dans le giratoire. Approximation faite :
        // une traversée ne sera pas détectée si le véhicule "saute" le tronçon d'entrée (PDT très grand ou troncon très petit)
        bool bEntree = false;
        for(int i=0; i<(int)m_LstTAmAv.size();i++)
        {
            TuyauMicro * pT = m_LstTAmAv[i];

            if( pT->GetBriqueAval() == this) // Tronçon d'entrée du giratoire
            {
                if(pVeh->GetLink(1) == pT) // le véhicule est sur le troncon d'entrée du gir au debut du PDT
                {
                    if(pVeh->GetLink(0) && pVeh->GetLink(0) != pT) // le véhicule n'est plus sur le tronçon d'entrée du gir à la fin du PDT
                    {
                        // il y a traversée d'entrée
                        bEntree = true;
                    }
                }
            }
        }

        // détection de la sortie (traversée de sortie)
        bool bSortie = false;
        for(int i=0; i<(int)m_LstTAmAv.size();i++)
        {
            TuyauMicro * pT = m_LstTAmAv[i];

            if( pT->GetBriqueAmont() == this) // Tronçon de sortie du giratoire
            {
                if(pVeh->GetLink(0) == pT) // le véhicule est sur le troncon de sortie du gir à la fin du PDT
                {
                    if(pVeh->GetLink(1) && pVeh->GetLink(1)->GetBriqueParente() == this) // le véhicule était au début du pas de temps sur un troncon interne au giratoire
                    {
                        // il y a traversée de sortie
                        bSortie = true;
                    }
                }
            }
        }

        // Gestion de la traversée d'entrée
        if(bEntree)
        {
            GrpPtsConflitTraversee * pGrpTra = NULL;

            // récupération de la voie d'entrée
            VoieMicro * pVoieEntree = pVeh->GetVoie(1);
            // récupération de la voie d'insertion sur le giratoire (un peu plus compliqué ...)
            VoieMicro * pVoieIns = NULL;
            Tuyau * pTuyauSortie = NULL;
            int iTuyIdx = pVeh->GetItineraireIndex(isBriqueAmontGir, this);
            if(iTuyIdx != -1)
            {
                pTuyauSortie = pVeh->GetItineraire()->at(iTuyIdx);
            }
            else if (m_pReseau->IsCptDirectionnel() && pVeh->GetNextTuyau() && this->IsTuyauAval(pVeh->GetNextTuyau()))
            {
                pTuyauSortie = pVeh->GetNextTuyau();
            }
            // pTuyauSortie peut être NULL si aucun itinéraire n'a été trouvé pour le véhicule...
            if(pTuyauSortie)
            {
                std::map<int, boost::shared_ptr<MouvementsSortie> > mapCoeffs;
                GetCoeffsMouvementsInsertion(pVeh, pVoieEntree, pTuyauSortie, mapCoeffs);
                std::map<int, boost::shared_ptr<MouvementsSortie> >::iterator coefsIt;
                for(coefsIt = mapCoeffs.begin(); coefsIt != mapCoeffs.end(); coefsIt++)
                {
                    if(coefsIt->second->m_dbTaux == 1.0)
                    {
                        pVoieIns = (VoieMicro*)((TuyauMicro*)pVoieEntree->GetParent())->getConnectionAval()->m_LstTuyAm.front()->GetLstLanes()[coefsIt->first];
                    }
                }

                // Récupération du groupe de conflits correspondant
                for(size_t i = 0; i < m_LstGrpTraEntree.size(); i++)
                {
                    if(m_LstGrpTraEntree[i].pVoieEntree == pVoieEntree
                        &&  m_LstGrpTraEntree[i].pVoieInterne == pVoieIns)
                    {
                        pGrpTra = m_LstGrpTraEntree[i].pGrpTra;
                    }
                }

                if(pGrpTra)
                {
                    // Boucle sur les points de conflit du groupe
                    for( std::deque<boost::shared_ptr<PtConflitTraversee> >::iterator itTra = pGrpTra->lstPtsConflitTraversee.begin(); itTra != pGrpTra->lstPtsConflitTraversee.end(); itTra++)
                    {
                        bool bOK = m_pReseau->CalculTraversee( pVeh, vehiculeIDs, (*itTra).get(), pGrpTra, GetTfTra(pVeh->GetType()), dbInstant );
                        if(!bOK || !pVeh->IsRegimeFluide() )
                        {
                            // on positionne le véhicule en attente d'insertion
                            pVeh->SetPos( (*itTra)->dbPos1 );     
                            pVeh->SetTuyau( (TuyauMicro*)(*itTra)->pT1->GetParent(), dbInstant );
                            pVeh->SetVoie( (*itTra)->pT1 );    

                            double dbVit = pVeh->GetDstParcourueEx() / m_pReseau->GetTimeStep();
                            pVeh->SetVit(dbVit);      

                            double dbAcc = (pVeh->GetVit(0) - pVeh->GetVit(1)) / m_pReseau->GetTimeStep();
                            pVeh->SetAcc( dbAcc );  

                            // dans le casdes giratoires il suffit d'un véhicule prioritaire détecté sur une des voies internes
                            // pour faire stopper le vehicule sur le point d'attente
                            break;
                        }
                    }
                }
            }
        } // fin de la gestion des la traversée d'entrée

        // Gestion de la traversée de sortie
        if(bSortie)
        {
            GrpPtsConflitTraversee * pGrpTra = NULL;
            // Récupération du groupe de conflits correspondant
            for(size_t i = 0; i < m_LstGrpTraSortie.size(); i++)
            {
                if(m_LstGrpTraSortie[i].pTuyauSortie == pVeh->GetLink(0)
                    &&  m_LstGrpTraSortie[i].pVoieInterne->GetNum() == pVeh->GetVoie(1)->GetNum() )
                {
                    pGrpTra = m_LstGrpTraSortie[i].pGrpTra;
                }
            }

            if(pGrpTra)
            {
                // Boucle sur les points de conflit du groupe
                for( std::deque<boost::shared_ptr<PtConflitTraversee> >::iterator itTra = pGrpTra->lstPtsConflitTraversee.begin(); itTra != pGrpTra->lstPtsConflitTraversee.end(); itTra++)
                {
                    bool bOK = m_pReseau->CalculTraversee( pVeh, vehiculeIDs, (*itTra).get(), pGrpTra, GetTfTra(pVeh->GetType()), dbInstant );
                    if(!bOK || !pVeh->IsRegimeFluide() )
                    {
                        // on positionne le véhicule en attente d'insertion
                        pVeh->SetPos( (*itTra)->dbPos1 );     
                        pVeh->SetTuyau( (TuyauMicro*)(*itTra)->pT1->GetParent(), dbInstant );
                        pVeh->SetVoie( (*itTra)->pT1 );    

                        double dbVit = pVeh->GetDstParcourueEx() / m_pReseau->GetTimeStep();
                        pVeh->SetVit(dbVit);      

                        double dbAcc = (pVeh->GetVit(0) - pVeh->GetVit(1)) / m_pReseau->GetTimeStep();
                        pVeh->SetAcc( dbAcc );  

                        // dans le casdes giratoires il suffit d'un véhicule prioritaire détecté sur une des voies internes
                        // pour faire stopper le vehicule sur le point d'attente
                        break;
                    }
                }
            }
        } // fin de la gestion des la traversée de sortie
    }
}

// Gestion de l'insertion sur les giratoires à plusieurs voies
bool	Giratoire::GetCoeffsMouvementsInsertion(Vehicule * pVeh, Voie *pVAm, Tuyau *pTAv, std::map<int, boost::shared_ptr<MouvementsSortie> > &mapCoeffs)
{
    // Si le tirage a déja été effectué pour ce véhicule et pour ce couple entree/sortie, on le réutilise.
    bool bFound = false;
    const std::vector<MouvementsInsertion> & mvtsInsertion = pVeh->GetMouvementsInsertion();
    for(size_t i = 0; i < mvtsInsertion.size() && !bFound; i++)
    {
        if(mvtsInsertion[i].pVoieEntree == pVAm && mvtsInsertion[i].pTuyauSortie == pTAv)
        {
            mapCoeffs = mvtsInsertion[i].mvtsInsertion;
            bFound = true;
        }
    }

    // Sinon, on le calcule :
    if(!bFound)
    {
        // si le couple origine / destination correspond à un coefficient manuel, on l'utilise.
        bool bManual = false;

        for(size_t i = 0; i < m_LstCoeffsInsertion.size() && !bManual; i++)
        {
            if(m_LstCoeffsInsertion[i].pTuyauSortie == pTAv // même tuyau de sortie
                && m_LstCoeffsInsertion[i].pTuyauEntree == pVAm->GetParent() // même tuyau d'entrée
                && m_LstCoeffsInsertion[i].nVoieEntree == pVAm->GetNum()) // même voie d'entrée
            {
                // Tirage de la voie d'insertion choisie par le véhicule
                double dbRand = m_pReseau->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;      
			    double dbSum = 0;
                bool bAffected = false;
			    for(int iVoie = 0; iVoie < m_nVoie; iVoie++)
                {
					dbSum += m_LstCoeffsInsertion[i].coefficients[iVoie];

                    boost::shared_ptr<MouvementsSortie> mvt(new MouvementsSortie());
                    mvt->m_pReseau = m_pReseau;

				    if( dbRand <= dbSum && !bAffected )
				    {
					    mvt->m_dbTaux = 1.0;					
                        bAffected = true;
				    }
                    else
                    {
                        mvt->m_dbTaux = 0.0;
                    }

                    mapCoeffs[iVoie] = mvt;
			    }   

                bManual = true;
            }
        }

        if(!bManual)
        {
            // si pas de coefficient manuel, on calcule des coefficients :

            // boucle sur l'ensemble des troncons extérieurs du giratoire pour compter
            // le nombre de sorties, et l'index de la sortie pTAv par rapport à l'entrée pVAm
            size_t nbTroncons = m_LstTAmAv.size();
            int nbSorties = 0;
            int idxSortie = 0;
            // on commence par se placer sur le troncon amont
            size_t i;
            for(i = 0; i < nbTroncons; i++)
            {
                if(m_LstTAmAv[i] == pVAm->GetParent())
                    break;
            }
            // ensuite on fait un tour de boucle en notant l'index de la sortie et le nombre de sorties
            if(i == nbTroncons - 1)
            {
                i = 0;
            }
            else
            {
                i++;
            }
            while(m_LstTAmAv[i] != pVAm->GetParent())
            {
                TuyauMicro * pTuy = m_LstTAmAv[i];
                // prise en compte des sorties uniquement
                if(pTuy->GetBriqueAmont() == this)
                {
                    if(pTuy == pTAv)
                    {
                        idxSortie = nbSorties;
                    }
                    nbSorties++;
                }

                if(i == nbTroncons-1)
                {
                    i = 0;
                }
                else
                {
                    i++;
                }
            }

            // Construction pour chaque voie du giratoire du coefficient adapté
            for(int iVoie = 0; iVoie < m_nVoie; iVoie++)
            {
                boost::shared_ptr<MouvementsSortie> mvt(new MouvementsSortie());
                mvt->m_pReseau = m_pReseau;
                mvt->m_dbTaux = 0;
                mapCoeffs[iVoie] = mvt;
            }

            double dbCoef = ((double)(idxSortie+1)/nbSorties)*m_nVoie;
            // Cas des extremités
            if(dbCoef <= 1.0)
            {
                mapCoeffs[0]->m_dbTaux = 1.0;
            }
            else if(dbCoef >= m_nVoie)
            {
                mapCoeffs[m_nVoie-1]->m_dbTaux = 1.0;
            }
            // cas général
            else
            {
                int iVoieDroite = (int) floor(dbCoef);
                int iVoieGauche = iVoieDroite+1;

                if(mapCoeffs.find(iVoieGauche-1) != mapCoeffs.end())
                {
                    // Cas où un tirage est nécessaire

                    // Tirage de la voie d'insertion choisie par le véhicule
                    double dbRand = m_pReseau->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
                    if(dbRand <= dbCoef-iVoieDroite)
                    {
                        mapCoeffs[iVoieGauche-1]->m_dbTaux = 1.0;
                    }
                    else
                    {
                        mapCoeffs[iVoieDroite-1]->m_dbTaux = 1.0;
                    }
                }
                else
                {
                    // cas ou pas de voie de gauche
                    mapCoeffs[iVoieDroite-1]->m_dbTaux = 1.0;
                }
            }
        }

        // Ajout des mouvements autorisés pour le véhicule en question
        MouvementsInsertion mvts;
        mvts.mvtsInsertion = mapCoeffs;
        mvts.pTuyauSortie = pTAv;
        mvts.pVoieEntree = pVAm;
        pVeh->GetMouvementsInsertion().push_back(mvts);
    }

    return true;
}

// Ajout de coefficients manuels (prioritaires sur les coefficients calculés par défaut)
void    Giratoire::AddCoefficientInsertion(Tuyau * pTuyauEntree, int nVoieEntree, Tuyau * pTuyauSortie, const std::vector<double> & coeffs)
{
    CoeffsInsertion coef;
    coef.coefficients = coeffs;
    coef.pTuyauEntree = pTuyauEntree;
    coef.pTuyauSortie = pTuyauSortie;
    coef.nVoieEntree = nVoieEntree;
    m_LstCoeffsInsertion.push_back(coef);
}

//================================================================
    Point*  Giratoire::CalculBarycentre
//----------------------------------------------------------------
// Fonction  : Calcule la position du barycentre, à partir des 
// coordonnées des points de l'anneau interne.
// Version du: 19/04/2012
// Historique: 19/04/2012 (O.Tonck - IPSIS)
//             Création
//================================================================
(   
)
{
    // calcul de l'isobarycentre de la brique
    int nbPts = 0;
    Point * isoBarycentre = new Point();
    isoBarycentre->dbX = 0;
    isoBarycentre->dbY = 0;
    isoBarycentre->dbZ = 0;

    // Parcours des tronçons internes
    for(std::deque<TuyauMicro*>::iterator iterTuy = m_LstTInt.begin(); iterTuy != m_LstTInt.end(); iterTuy++)
    {
        // on ne prend que les points amont et internes (pas les points aval car ils sont en double
        // puisque le point aval d'un tuyau est le point amont du tuyau suivant)
        // récupération des points aval des tronçons amont
        Point * pPoint;
        pPoint = (*iterTuy)->GetExtAmont();
        isoBarycentre->dbX += pPoint->dbX;
        isoBarycentre->dbY += pPoint->dbY;
        isoBarycentre->dbZ += pPoint->dbZ;
        nbPts++;
        for(size_t i = 0; i < (*iterTuy)->GetLstPtsInternes().size(); i++)
        {
            pPoint = (*iterTuy)->GetLstPtsInternes()[i];
            isoBarycentre->dbX += pPoint->dbX;
            isoBarycentre->dbY += pPoint->dbY;
            isoBarycentre->dbZ += pPoint->dbZ;
            nbPts++;
        }     
    }
    
    // on normalise
    isoBarycentre->dbX /= nbPts;
    isoBarycentre->dbY /= nbPts;
    isoBarycentre->dbZ /= nbPts;

    return isoBarycentre;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void GroupeTraverseeEntreeGir::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void GroupeTraverseeEntreeGir::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void GroupeTraverseeEntreeGir::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(pVoieEntree);
    ar & BOOST_SERIALIZATION_NVP(pVoieInterne);
    ar & BOOST_SERIALIZATION_NVP(pGrpTra);
}

template void GroupeTraverseeSortieGir::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void GroupeTraverseeSortieGir::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void GroupeTraverseeSortieGir::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(pTuyauSortie);
    ar & BOOST_SERIALIZATION_NVP(pVoieInterne);
    ar & BOOST_SERIALIZATION_NVP(pGrpTra);
}

template void CoeffsInsertion::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CoeffsInsertion::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CoeffsInsertion::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(pTuyauEntree);
    ar & BOOST_SERIALIZATION_NVP(pTuyauSortie);
    ar & BOOST_SERIALIZATION_NVP(nVoieEntree);
    ar & BOOST_SERIALIZATION_NVP(coefficients);
}

template void MouvementsInsertion::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void MouvementsInsertion::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void MouvementsInsertion::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(pTuyauSortie);
    ar & BOOST_SERIALIZATION_NVP(pVoieEntree);
    ar & BOOST_SERIALIZATION_NVP(mvtsInsertion);
}

template void Giratoire::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Giratoire::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Giratoire::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(BriqueDeConnexion);

	ar & BOOST_SERIALIZATION_NVP(m_strNom);
    ar & BOOST_SERIALIZATION_NVP(m_pReseau);
    ar & BOOST_SERIALIZATION_NVP(m_strRevetement);

    ar & BOOST_SERIALIZATION_NVP(m_dbTagreg);
    ar & BOOST_SERIALIZATION_NVP(m_dbGamma);
    ar & BOOST_SERIALIZATION_NVP(m_dbMu);

    ar & BOOST_SERIALIZATION_NVP(m_dbLargeurVoie);
    ar & BOOST_SERIALIZATION_NVP(m_nVoie);
    ar & BOOST_SERIALIZATION_NVP(m_cType);

    ar & BOOST_SERIALIZATION_NVP(m_dbBeta);
    ar & BOOST_SERIALIZATION_NVP(m_dbBetaInt);

    ar & BOOST_SERIALIZATION_NVP(m_LstTAmAv);
    ar & BOOST_SERIALIZATION_NVP(m_LstTInt);

    ar & BOOST_SERIALIZATION_NVP(m_LstTf);
    ar & BOOST_SERIALIZATION_NVP(m_LstTt);

    ar & BOOST_SERIALIZATION_NVP(m_LstCoeffsInsertion);

    ar & BOOST_SERIALIZATION_NVP(m_LstGrpTraEntree);
    ar & BOOST_SERIALIZATION_NVP(m_LstGrpTraSortie);
}