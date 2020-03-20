#include "stdafx.h"
#include "convergent.h"

#include "voie.h"
#include "reseau.h"
#include "divergent.h"
#include "giratoire.h"
#include "RandManager.h"
#include "DiagFonda.h"
#include "tuyau.h"
#include "sensors/SensorsManager.h"
#include "SystemUtil.h"
#include "vehicule.h"
#include "CarrefourAFeuxEx.h"
#include "carFollowing/AbstractCarFollowing.h"
#include "carFollowing/CarFollowingContext.h"
#include "sensors/PonctualSensor.h"

#include <boost/serialization/deque.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/shared_ptr.hpp>

using namespace std;

//================================================================
    PointDeConvergence::PointDeConvergence
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 23/06/2009
// Historique: 23/06/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{       
    m_pVAv = 0;    

    m_bFreeFlow = true;
    m_bInsertion = false;    

    m_dbInstLastPassage = UNDEF_INSTANT;

    m_nRandNumbers = 0;
}

//================================================================
    PointDeConvergence::PointDeConvergence
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 23/06/2009
// Historique: 23/06/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    VoieMicro   *pVAv,
    Convergent  *pCvg
)
{       
    m_pVAv = pVAv;   
    m_pCvg = pCvg;

    m_pTAv = (TuyauMicro*)m_pVAv->GetParent();

    m_bFreeFlow = true;
    m_bInsertion = false;    

    m_dbInstLastPassage = UNDEF_INSTANT;

    m_nRandNumbers = 0;
}

//================================================================
    VoieMicro*  PointDeConvergence::GetVoieAmontPrioritaire
//----------------------------------------------------------------
// Fonction  : Retourne la voie avec le niveau de plus forte priorité
// Version du: 23/06/2009
// Historique: 23/06/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{    
    int nTmp = INT_MAX;
    VoieMicro* pV = NULL;
    std::deque<DesVoieAmont>::iterator itVAm;

    for(itVAm = m_LstVAm.begin(); itVAm != m_LstVAm.end(); itVAm++)
    {
        if( (*itVAm).nNiveau < nTmp )        
        {
            pV = (*itVAm).pVAm;
            nTmp = (*itVAm).nNiveau;
        }
    }
    return pV;
}

	string PointDeConvergence::GetUnifiedID()
{
	string sID;
	VoieIndex VI;

	if (m_pVAv == NULL)
	{
		sID = NullPtr;
		sID += "/";
		sID += NullPtr;
		sID += "/";
	}
	else
	{
		VI = m_pVAv->GetUnifiedID();
		sID = VI.m_sTuyauID + "/" + SystemUtil::ToString(VI.m_nNumVoie) + "/";
	}
	if (m_pTAv == NULL)
	{
		sID += NullPtr;
	}
	else
	{
		sID += m_pTAv->GetUnifiedID();
	}
	{
		deque<DesVoieAmont>::iterator it, itEnd;
		itEnd = m_LstVAm.end();
		for (it=m_LstVAm.begin(); it!=itEnd; it++)
		{
			if (it->pVAm != NULL)
			{
				VI = it->pVAm->GetUnifiedID();
				sID += "/";
				sID += VI.m_sTuyauID;
				sID += SystemUtil::ToString(VI.m_nNumVoie);
			}
		}
	}
	return sID;
}

//================================================================
    Convergent::Convergent
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 20/11/2007
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{       
    m_pGestionCapteur = NULL;
}

//================================================================
    Convergent::Convergent
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 20/11/2007
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
 std::string    strID,
    double      dbTagreg,           // Période d'agrégation des capteurs
    double      dbGamma,     
    double      dbMu,
    Reseau      *pReseau
): ConnectionPonctuel(strID, pReseau)
{   
    m_pGestionCapteur = new SensorsManager(pReseau, dbTagreg, 0);

    m_dbGamma = dbGamma;    
    m_dbMu = dbMu;

    m_pReseau = pReseau;   

    m_pGiratoire = NULL;
    
}    
	
//================================================================
    Convergent::~Convergent
//----------------------------------------------------------------
// Fonction  : Destructeur
// Version du: 20/11/2007
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
)
{
    std::deque<PointDeConvergence*>::iterator itPtCvg;

    for(itPtCvg = m_LstPtCvg.begin(); itPtCvg!=m_LstPtCvg.end(); itPtCvg++)    
        delete *itPtCvg;                

    if(m_pGestionCapteur)
    {
        delete m_pGestionCapteur;
    }
}

//================================================================
    void Convergent::Init
//----------------------------------------------------------------
// Fonction  : Initialisation du convergent
// Version du: 20/11/2007
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(   
    Tuyau*      pTAv               // Tuyau aval   
)
{        
    // Construction des points de convergence (il y autant que le nombre de voie du tronçon aval)
    PointDeConvergence *pPtCvg;
    for(int i=0; i<pTAv->getNb_voies(); i++)
    {
        pPtCvg = new PointDeConvergence( (VoieMicro*)pTAv->GetLstLanes()[i], this);
        m_LstPtCvg.push_back(pPtCvg);
    }
}

//================================================================
    void Convergent::FinInit
//----------------------------------------------------------------
// Fonction  : Initialisation du convergent
// Version du: 20/11/2007
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(   
    Tuyau*      pTAv,               // Tuyau aval   
    double      dbPosCptTAv         // Position du capteur sur le tuyau aval
)
{        
    PonctualSensor * pCpt = m_pGestionCapteur->AddCapteurPonctuel("CptTav", pTAv, dbPosCptTAv);
    if(pCpt)
    {
        pTAv->getListeCapteursConvergents().push_back(pCpt);
    }

	for(int i=0; i< GetNbElAmont() ; i++)
    {
        pCpt = m_pGestionCapteur->AddCapteurPonctuel( "CptAm", m_LstTuyAm[i], m_LstTuyAm[i]->GetLength() - 5);
        if(pCpt)
        {
            m_LstTuyAm[i]->getListeCapteursConvergents().push_back(pCpt);
        }
    }
}

void Convergent::InitSimuTrafic()
{
	// Appel de la fonction parente
	ConnectionPonctuel::InitSimuTrafic();

	deque<PointDeConvergence*>::iterator itPtCvg;
	
	for( itPtCvg = m_LstPtCvg.begin(); itPtCvg != m_LstPtCvg.end(); itPtCvg++)
		(*itPtCvg)->m_dbInstLastPassage = 0;    
}

//================================================================
    void  Convergent::UpdateCapteurs
//----------------------------------------------------------------
// Fonction  : Met à jour les données du capteur suite au calcul 
//             des trajectoires des véhicules
// Remarque  : 
// Version du: 06/10/2008
// Historique: 26/11/2007 (C.Bécarie - Tinea)
//             Création
//             06/10/2008 (C.Bécarie - INEO TINEA)
//              Ajout possibilité de calculer le nombre de véhicule
//              vu par le capteur à l'aide des deltaN
//================================================================
(
)
{
    m_pGestionCapteur->UpdateTabNbVeh();
}

//================================================================
    void    Convergent::AddTf
//----------------------------------------------------------------
// Fonction  : Ajout un temps d'insertion pour un type de véhicule
// Remarque  : 
// Version du: 23/11/2007
// Historique: 23/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    TypeVehicule *pTV, 
    double dbTf
)
{
    TempsCritique TpsIns;

    TpsIns.pTV = pTV;
    TpsIns.dbtc = dbTf;

    m_LstTf.push_back(TpsIns);   
}

//================================================================
    double  Convergent::GetTf
//----------------------------------------------------------------
// Fonction  : Retourne le temps d'insertion du type de véhicule
//             passé en paramètre
// Remarque  : Retourne une valeur par défaut si le type de véhicule
//             n'a pas été trouvé dans la liste
// Version du: 23/11/2007
// Historique: 23/11/2007 (C.Bécarie - Tinea)
//             Création
//================================================================
(   
    TypeVehicule *pTV
)
{
    for(size_t i=0; i<m_LstTf.size();i++)
    {
        if( m_LstTf[i].pTV == pTV )
            return std::max<double>(m_LstTf[i].dbtc, 1 / pTV->GetDebitMax());
    }
    return 1 / pTV->GetDebitMax();   // Valeur par défaut
}

//================================================================
    void PointDeConvergence::CalculInsertion
//----------------------------------------------------------------
// Fonction  : Calcule si l'insertion du véhicule en attente sur le lien
//             non prioritaire sur le lien aval est possible
// Remarque  : 
// Version du: 23/06/2009
// Historique: 20/11/2007 (C.Bécarie - Tinea)
//             Création
//             06/10/2008 (C.Bécarie - INEO TINEA)
//              Ajout possibilité de calculer l'offre en aval du convergent 
//              à l'aide des deltaN ou de la quantité entière (paramètre
//              de simulation débrayable)
//             23/06/2009 (C.Bécarie - Tinea)
//              Devient membre de PointDeConvergence
//================================================================
(
    double  dbInstant        // Instant de calcul    
)
{
    boost::shared_ptr<Vehicule> pVehAttente;
    boost::shared_ptr<Vehicule> pVeh;
    double      dbOffre = 0, dbDemande2 = 0, dbDemande1 = 0;
    double      dbPos, dbVit, dbAcc;
    double      dbtr, dbta;
    double      dbPosLead0;    
    VoieMicro*       pVAmP;    
    VoieMicro*       pVAm;    
    vector<boost::shared_ptr<Vehicule>>  LstVehEnAttente;        // Liste des véhicules en attente d'un tronçon
    vector<VoieMicro*>                   LstVoieVehEnAttente;    // Liste des tuyaux sur lequels se trouvent les véhicules en attente
    vector<boost::shared_ptr<Vehicule>>  LstVehEnAttenteTmp;     
    unsigned int        number;    
    double              dbRand;
    VoieMicro*       pVAmNP;

    // Mise à jour du compteur des tirages aléatoires
    m_nRandNumbers++;

    // Détermination des tronçons prioritaires
    pVAmP = GetVoieAmontPrioritaire();    
    int nNivPriorite = GetPriorite( pVAmP );

    // Recherche du premier véhicule en attente
    int nNiv = 999;
    for(size_t i=0; i< m_LstVAm.size(); i++)
    {
        if( m_LstVAm[i].nNiveau > nNivPriorite)    // Voie non prioritaire
        {
            pVAm = m_LstVAm[i].pVAm;
             
            // Recherche si il existe des véhicules sur les voies du tronçon amont traité susceptible de s'insérer (cad qui passe le convergent avec la loi de poursuite classique)
            pVeh = m_pCvg->GetVehiculeEnAttente(pVAm);   // Recherche sur la voie j
			if(pVeh && pVeh->GetVit(0)>0 // On exclut les véhicules qui ont une vitesse nulle à la fin du pas de temps (en fait ceux qui se sont arrêté à cause d'une treversée)
                && !pVeh->IsForcedDriven()) // On exclut également les véhicule pilotés en mode forcé
            {
                LstVehEnAttenteTmp.push_back(pVeh);                    
            }
            else
            {
                LstVehEnAttenteTmp.push_back(boost::shared_ptr<Vehicule>()); // Pas de véhicule en attente sur la voie j                    
            }

            int nSize = 0;
            for(size_t j=0; j< LstVehEnAttenteTmp.size(); j++)
            {
                if( LstVehEnAttenteTmp[j] )
                    nSize++;
            }

            if(nSize>0)
            {
                if( m_LstVAm[i].nNiveau < nNiv ) // Existe t'il des véhicules susceptibles de s'insérer sur un tronçon plus prioritaire que lui ?
                {
                    for(size_t j=0; j<LstVehEnAttente.size(); j++) // Immobilisation des véhicules sur la voie moins prioritaire
                    {                        
                        pVehAttente = LstVehEnAttente[j];
                        if(pVehAttente)
                        {
                            pVehAttente->SetPos(LstVoieVehEnAttente[j]->GetLength()- 0.0001);
                            pVehAttente->SetTuyau( (TuyauMicro*)LstVoieVehEnAttente[j]->GetParent(), dbInstant );
                            pVehAttente->SetVoie( (VoieMicro*)LstVoieVehEnAttente[j]);
                            pVehAttente->SetVit( pVehAttente->GetDstParcourueEx() / m_pCvg->GetReseau()->GetTimeStep()); 
                            pVehAttente->SetAcc(pVehAttente->GetVit(1) / m_pCvg->GetReseau()->GetTimeStep() );
                            pVehAttente->UpdateLstUsedLanes();
                        }                        
                    }
                    LstVehEnAttente.clear();
                    LstVoieVehEnAttente.clear();
                    for(size_t j=0; j<LstVehEnAttenteTmp.size(); j++)
                    {                        
                        LstVehEnAttente.push_back( LstVehEnAttenteTmp[j] ); 
                        LstVoieVehEnAttente.push_back(pVAm);
                        if(LstVehEnAttenteTmp[j])
                        {
                            pVAmNP = pVAm;                            
                            nNiv = m_LstVAm[i].nNiveau;   
                        }
                    }
                }
                else
                {
                    // Les véhicules du tronçon moins prioritaire doivent s'arrêter
                    for(size_t j=0; j<LstVehEnAttenteTmp.size(); j++)
                    {
                        pVeh = LstVehEnAttenteTmp[j];
                        if(pVeh)
                        {
                            pVeh->SetPos(pVAm->GetLength()- 0.0001);
                            pVeh->SetTuyau((TuyauMicro*)pVAm->GetParent(), dbInstant);
                            pVeh->SetVoie( (VoieMicro*)pVAm);                            
                            pVeh->SetVit( pVeh->GetDstParcourueEx() / m_pCvg->GetReseau()->GetTimeStep()); 
                            pVeh->SetAcc(pVeh->GetVit(1) / m_pCvg->GetReseau()->GetTimeStep() );
                            pVeh->UpdateLstUsedLanes();
                        }
                    }
                }                
            }
            LstVehEnAttenteTmp.clear();
        }
    }
    
    // Existe au moins un véhicule en attente sur l'ensemble des voies du tronçon considéré
    for(size_t j=0; j<LstVehEnAttente.size(); j++)
    {
        if(LstVehEnAttente[j])
            pVehAttente = LstVehEnAttente[j];
    }
    if(!pVehAttente)    // Si aucun véhicule dans la zone d'influence est en attente, on sort
    {                
        return;
    }      
   
    // Véhicule follower (sur lien prioritaire et les suivants) au début du pas de temps    
    boost::shared_ptr<Vehicule> pVehFollower = m_pCvg->GetReseau()->GetNearAmontVehicule( pVAmP, pVAmP->GetLength()+0.01, m_pCvg->GetReseau()->GetTimeStep()*m_pCvg->GetReseau()->GetMaxVitMax());	

    Tuyau *pTAmP= (Tuyau*)pVAmP->GetParent();
    int nVoieGir = pVAmP->GetNum();

    // Si le follower n'existait pas au début du pas de temps, on ne le prends pas en compte...
    if(pVehFollower)
    {
        if(!pVehFollower->GetLink(1))
            pVehFollower.reset();
		else
		{
            if (pVehFollower->IsArretAuFeu()) {       // Follower non pris en compte si il est au feu
                pVehFollower.reset();
            }
            else if (!pVehFollower->GetCarFollowing()->GetCurrentContext()) {      // Follower non pris en compte si il n'y a pas de context
                pVehFollower.reset();
            }
		}
    }
	
    // Pas de follower, on regarde sur les liens de priorité plus haute si un follower
    if(!pVehFollower)
    {
        for(int i=0; i< (int)m_LstVAm.size(); i++)
        {
            if( m_LstVAm[i].nNiveau < nNiv && m_LstVAm[i].nNiveau > 1 )  
            {
                pVehFollower = m_pCvg->GetReseau()->GetNearAmontVehicule( m_LstVAm[i].pVAm, m_LstVAm[i].pVAm->GetLength());
                if(pVehFollower && !pVehFollower->IsArretAuFeu())
                {
                    nNiv = m_LstVAm[i].nNiveau;
                    pVAmP = m_LstVAm[i].pVAm;
                    pTAmP =(Tuyau*)pVAmP->GetParent();
                    nVoieGir = pVAmP->GetNum();
                }
                else
                    pVehFollower.reset();
            }
        }
    }

   // Si le follower n'existait pas au début du pas de temps, on ne le prends pas en compte...
   if(pVehFollower)
    {
        if(!pVehFollower->GetLink(1))
            pVehFollower.reset();
		else
		{
            if (pVehFollower->IsArretAuFeu()) {      // Follower non pris en compte si il est au feu
                pVehFollower.reset();
            }
            else if (!pVehFollower->GetCarFollowing()->GetCurrentContext()) {     // Follower non pris en compte si il n'y a pas de context
                pVehFollower.reset();
            }
		}
    }

   // Le follower prioritaire sort-il sur le tronçon prioritaire du convergent ? (sauf si giratoire )
   // Si non, on ne le prend pas en compte...
	if( !m_pCvg->m_pGiratoire )
		if( pVehFollower )
			if( !pVehFollower->IsItineraire( pTAmP ) )
                pVehFollower.reset();

    // Véhicule leader (sur lien aval)
    boost::shared_ptr<Vehicule> pVehLeader = m_pCvg->GetReseau()->GetNearAvalVehicule( m_pVAv, 0);    
    if(!pVehLeader && pVehAttente->GetLink(1) )
        pVehLeader = m_pCvg->GetReseau()->GetPrevVehicule( pVehAttente.get(), true);

    // I - Calcule l'état congestionné ou fluide du convergent (correspond à l'état du tronçon aval)

    // Recherche du véhicule permettant de déterminer l'état (véhicule en amont du capteur du lien aval)
	if( m_pCvg->GetCapteur(m_pTAv) )
		pVeh = m_pCvg->GetReseau()->GetNearAmontVehicule(m_pVAv, m_pCvg->GetCapteur(m_pTAv)->GetPosition());
		if( pVeh && pVeh->GetVoie(1) != m_pVAv)
            pVeh.reset();  

    if(!pVeh)
    {
        m_bFreeFlow = true;     // Régime fluide
    }
    else
    {
        m_bFreeFlow = pVeh->IsRegimeFluide();
    }    

    // Si aucun véhicule sur le tuyau amont prioritaire -et sur les précédents tant que l'on se trouve 
    // dans la zone d'influence- , on force l'état fluide
    if(!m_bFreeFlow)
        if( !m_pCvg->GetReseau()->GetVehiculeAmont((TuyauMicro*)pTAmP, ((TuyauMicro*)pTAmP)->GetMaxRapVitSurQx(dbInstant),0, dbInstant ))
            m_bFreeFlow = true;   

    // II - Détermination des demandes et de l'offre
     
    if(!m_bFreeFlow)    // Régime congestionné
    {
        // Calcul de l'offre
        dbOffre = std::min<double>( m_pCvg->GetReseau()->GetMaxDebitMax(), m_pCvg->m_pGestionCapteur->GetDebitMoyen( m_pCvg->GetCapteur(m_pTAv),nVoieGir));

        // Calcul de la demande du lien non prioritaire
        bool bTNPFreeFlow;
        pVeh = m_pCvg->GetReseau()->GetVehiculeAmont((TuyauMicro*)pVAmNP->GetParent(), ((TuyauMicro*)pVAmNP->GetParent())->GetMaxRapVitSurQx(dbInstant),0, dbInstant );
        if(pVeh)
        {
            if( pVeh->GetVit(1) < std::min<double>( pVeh->GetVitMax(), ((Tuyau*)pVAmNP->GetParent())->GetVitRegByTypeVeh(pVeh->GetType(),dbInstant,pVeh->GetPos(1),pVAmNP->GetNum())) )
                bTNPFreeFlow = false;
            else
                bTNPFreeFlow = true;
        }
        else
            bTNPFreeFlow = true;

        if(bTNPFreeFlow)
            dbDemande2 = std::min<double>(m_pCvg->GetReseau()->GetMaxDebitMax(), m_pCvg->m_pGestionCapteur->GetDebitMoyen( m_pCvg->GetCapteur((TuyauMicro*)pVAmNP->GetParent()), std::min<int>(pVAmNP->GetNum(),nVoieGir) ));
        else
            dbDemande2 = m_pCvg->GetReseau()->GetMaxDebitMax();

    }
    else                // Régime fluide
    {
        dbDemande1 = std::min<double>( m_pCvg->GetReseau()->GetMaxDebitMax(), m_pCvg->m_pGestionCapteur->GetDebitMoyen( m_pCvg->GetCapteur(pTAmP),nVoieGir));
    }

    // III - Détermination si il peut y avoir insertion (sur tous les véhicules susceptibles de d'insérer)   
    std::vector<bool> bInsertion(LstVehEnAttente.size());
    for( int j = 0; j<(int)LstVehEnAttente.size(); j++)
    {
        pVehAttente = LstVehEnAttente[j];

        if(pVehAttente)
        {            
            bInsertion[j] = pVehAttente->GetCarFollowing()->TestConvergentInsertion(this, pTAmP, pVAmP, pVAmNP, nVoieGir, dbInstant, j, dbOffre, dbDemande1, dbDemande2, pVehLeader, pVehFollower, dbtr, dbta);
        }
    }

    // Si il y a plusieurs insertions possibles, détermination du véhicule à insérer
    int nPos = 0;
    int nVoieArret;
    int nVoieAttente;
    boost::shared_ptr<Vehicule> pVehArret;

    for(int j=0; j<(int)LstVehEnAttente.size();j++)
    {
        if( LstVehEnAttente[j] && bInsertion[j] )
        {
            nPos++;
            pVehAttente = LstVehEnAttente[j];
            nVoieAttente = j;
        }
    }
    if(nPos>0)
    {
        if(nPos>1)
        {
            number = m_pCvg->GetReseau()->GetRandManager()->myRand();
            dbRand = (double)number / (double)MAXIMUM_RANDOM_NUMBER;

            if(dbRand < 0.5)
            {
                pVehAttente = LstVehEnAttente[0];
                pVehArret = LstVehEnAttente[1];
                nVoieArret = 1;
                nVoieAttente = 0;
            }
            else
            {
                pVehAttente = LstVehEnAttente[1];   
                pVehArret = LstVehEnAttente[0];
                nVoieArret = 0;
                nVoieAttente = 1;
            }

            pVehArret->SetPos( pVAmNP->GetLength() - 0.0001);
            pVehArret->SetTuyau( (TuyauMicro*)pVAmNP->GetParent(), dbInstant);
            pVehArret->SetVoie( pVAmNP);            
            //pVehArret->SetVit(0);
            pVehArret->SetVit(pVehArret->GetDstParcourueEx()/m_pCvg->GetReseau()->GetTimeStep());
            dbAcc = ( pVehArret->GetVit(0) - pVehArret->GetVit(1) )/ m_pCvg->GetReseau()->GetTimeStep();
            pVehArret->SetAcc(dbAcc);
            pVehArret->UpdateLstUsedLanes();
        }
        m_bInsertion = true;
    }
    else
        m_bInsertion = false;

    if(m_bInsertion)
    {
        // Mise à jour des positions des véhicules        
        if(pVehAttente)
        {
            if(m_bFreeFlow)
            {
                AbstractCarFollowing * pCarFollowing = pVehAttente->GetCarFollowing();
                // Création du contexte qui va nous permettre de calculer la position voulue
                CarFollowingContext * pInsertionContext = pCarFollowing->CreateContext(dbInstant, true);
                std::vector<boost::shared_ptr<Vehicule> > leaders;
                std::vector<double> leaderDistances;
                if(pVehLeader && pVehLeader->GetLink(1))
                {
                    // définition du leader et de sa distance avec le véhicule en attente à l'instant de l'insertion
                    dbPosLead0 = pVehLeader->GetPos(1) + (m_pCvg->GetReseau()->GetTimeStep() - dbta)*pVehLeader->GetVit(0);
                    // rmq. bug possible ici si le leader est sur une voie encore après (possibles si plusieurs voies courtes en aval d'un convergent).
                    // Reprendre ici en faisant une fonction qui calcule la distance quel que soit le nombre de voies intermédiaires ?
                    if( m_pVAv != pVehLeader->GetVoie(1) ) // le leader se trouve sur le tronçon suivant du tronçon aval du convergent
                    {
                        dbPosLead0 += m_pVAv->GetLength();
                    }
                    
                    leaders.push_back(pVehLeader);
                    leaderDistances.push_back(dbPosLead0);
                }
                // définition de la liste des voies sur lesquelles le véhicule en attente doit évoluer après son insertion
                // rmq. On ne se base pas sur la voie de sortie mais le tronçon (le point de convergence peut avoir une voie
                // aval différente de la voie aval du convergent empruntée par le véhicule en attente)
                std::vector<VoieMicro*> lstLanes;
                std::vector<VoieMicro*>::const_iterator iterLane;
                bool bDoAdd = false;
                for(iterLane = pCarFollowing->GetCurrentContext()->GetLstLanes().begin(); iterLane != pCarFollowing->GetCurrentContext()->GetLstLanes().end(); ++iterLane)
                {
                    if((*iterLane)->GetParent() == m_pVAv->GetParent())
                    {
                        bDoAdd = true;
                    }
                    if(bDoAdd)
                    {
                        lstLanes.push_back(*iterLane);
                    }
                }

                pInsertionContext->SetContext(leaders, leaderDistances, lstLanes, 0);
                pCarFollowing->Compute(dbta, dbInstant, pInsertionContext);
                double dbPosAtt = pCarFollowing->GetComputedTravelledDistance();
                delete pInsertionContext;
                if(dbPosAtt>0)
                {
                    // Calcul de la position du follower
                    double dbDstFol0, dbFollowerPosTr;
                    std::vector<VoieMicro*> lstLanesFol;
                    if(pVehFollower)
                    {
                        AbstractCarFollowing * pFollowerCarFollowing = pVehFollower->GetCarFollowing();
                        // Création du contexte qui va nous permettre de calculer la position voulue
                        CarFollowingContext * pFollowerContext = pFollowerCarFollowing->CreateContext(dbInstant, true);

                        // Calcul de la distance parcourue par le follower au moment de l'insertion
                        dbFollowerPosTr = pVehFollower->GetPos(1) + (m_pCvg->GetReseau()->GetTimeStep() - dbta)*pVehFollower->GetVit(0);
                        // On en déduit la voie du follower au moment de l'insertion, sa position sur cette voie, et la liste des voies suivantes atteignables
                        lstLanesFol = pFollowerCarFollowing->GetCurrentContext()->GetLstLanes();
                        while(dbFollowerPosTr > lstLanesFol.front()->GetParent()->GetLength())
                        {
                            dbFollowerPosTr -= lstLanesFol.front()->GetParent()->GetLength();
                            lstLanesFol.erase(lstLanesFol.begin());
                        }

                        std::vector<boost::shared_ptr<Vehicule> > leadersFol;
                        leadersFol.push_back(pVehAttente);
                        double distanceToConvergence = 0;
                        // calcul de la distance du follower au point de convergence.
                        for(size_t iLane = 0; iLane < lstLanesFol.size(); iLane++)
                        {
                            VoieMicro * pLane = lstLanesFol[iLane];
                            if(iLane == 0)
                            {
                                distanceToConvergence = pLane->GetLength() - dbFollowerPosTr;
                            }
                            else
                            {
                                distanceToConvergence += pLane->GetLength();
                            }
                            if(pLane->GetParent() == pTAmP)
                            {
                                break;
                            }

                        }
                        std::vector<double> leaderDistancesFol(1, distanceToConvergence);
                        pFollowerContext->SetContext(leadersFol, leaderDistancesFol, lstLanesFol, dbFollowerPosTr, pFollowerCarFollowing->GetCurrentContext());
                        pFollowerCarFollowing->Compute(dbta, dbInstant, pFollowerContext);
                        dbDstFol0 = pFollowerCarFollowing->GetComputedTravelledDistance();
                        delete pFollowerContext;

                        // on minore la distance totale recalculée en prenant en compte le convergent par la distance parcourue par le follower avant traitement du convergent
                        dbDstFol0 = std::min<double>(pVehFollower->GetDstParcourueEx(), (m_pCvg->GetReseau()->GetTimeStep() - dbta)*pVehFollower->GetVit(0) + dbDstFol0);
                    }                    
                    
                    if(!pVehFollower || dbDstFol0 >= 0)
                    {
                        bool bDontMove = false;
                        // positionnement du véhicule en attente sur sa position d'insertion calculée en fonction du leader
                        double dbTravelledDistance = 0;
                        for(size_t iLane = 0; iLane < lstLanes.size(); iLane++)
                        {
                            VoieMicro * pLane = lstLanes[iLane];
                            double laneLength = pLane->GetLongueurEx(pVehAttente->GetType());
                            double endPositionOnLane = dbPosAtt - dbTravelledDistance;

                            if (pLane == pVehAttente->GetVoie(0) && endPositionOnLane > pVehAttente->GetPos(0))
                            {
                                // On ne permet pas à un véhicule d'aller plus loin à cause du convergent que ce qu'il devait faire initialement
                                // (pas des problèmes de liste de voies parcourue incomplète ensuite). Il faudrait comprendre pourquoi ceci peut arriver.
                                bDontMove = true;
                                break;
                            }
                            // Si on ne va pas sur les voies suivantes, on sort de la boucle
                            else if(endPositionOnLane <= laneLength)
                            {
                                pVehAttente->SetTuyau((Tuyau*)pLane->GetParent(), dbInstant);
                                pVehAttente->SetVoie(pLane);
                                pVehAttente->SetPos(endPositionOnLane);
                                pVehAttente->UpdateLstUsedLanes();
                                break;
                            }
                            dbTravelledDistance += std::min<double>(endPositionOnLane,laneLength);
                        }

                        if (!bDontMove)
                        {
                            dbVit = (dbPosAtt + (m_pCvg->GetReseau()->GetTimeStep() - dbta)*pVehAttente->GetVit(0)) / m_pCvg->GetReseau()->GetTimeStep();
                            pVehAttente->SetVit(dbVit);
                            dbAcc = (pVehAttente->GetVit(0) - pVehAttente->GetVit(1)) / m_pCvg->GetReseau()->GetTimeStep();
                            pVehAttente->SetAcc(dbAcc);
                        }

                        if(pVehFollower)
                        {
                            dbTravelledDistance = 0;
                            lstLanes = pVehFollower->GetCarFollowing()->GetCurrentContext()->GetLstLanes();
                            for(size_t iLane = 0; iLane < lstLanes.size(); iLane++)
                            {
                                VoieMicro * pLane = lstLanes[iLane];
                                double startPositionOnLane = iLane == 0 ? pVehFollower->GetPos(1) : 0;
                                double laneLength = pLane->GetLongueurEx(pVehFollower->GetType());
                                double endPositionOnLane = dbDstFol0 - dbTravelledDistance + startPositionOnLane;

                                // Si on ne va pas sur les voies suivantes, on sort de la boucle
                                if(endPositionOnLane <= laneLength)
                                {
                                    pVehFollower->SetTuyau((Tuyau*)pLane->GetParent(), dbInstant);
                                    pVehFollower->SetVoie(pLane);
                                    pVehFollower->SetPos(endPositionOnLane);
                                    pVehFollower->UpdateLstUsedLanes();
                                    break;
                                }
                                dbTravelledDistance += std::min<double>(endPositionOnLane,laneLength) - startPositionOnLane;
                            }

                            dbVit = (dbDstFol0 + (m_pCvg->GetReseau()->GetTimeStep() - dbta)*pVehFollower->GetVit(0)) / m_pCvg->GetReseau()->GetTimeStep();
                            pVehFollower->SetVit(dbVit);
                            dbAcc = ( pVehFollower->GetVit(0) - pVehFollower->GetVit(1) )/ m_pCvg->GetReseau()->GetTimeStep();
                            pVehFollower->SetAcc(dbAcc);
                        }

                        m_dbInstLastPassage = dbInstant - m_pCvg->GetReseau()->GetTimeStep() + dbtr;                      
                        
                        return;
                    }
                }

                pVehAttente->SetPos(pVAmNP->GetLength() - 0.0001);
                pVehAttente->SetTuyau((TuyauMicro*)pVAmNP->GetParent(), dbInstant);
                pVehAttente->SetVoie(pVAmNP);
                dbVit = pVehAttente->GetDstParcourueEx()  / m_pCvg->GetReseau()->GetTimeStep();
                pVehAttente->SetVit(dbVit);
                dbAcc = ( pVehAttente->GetVit(0) - pVehAttente->GetVit(1) )/ m_pCvg->GetReseau()->GetTimeStep();
                pVehAttente->SetAcc(dbAcc);   
                pVehAttente->UpdateLstUsedLanes();

                m_pCvg->DelInsVeh(pVehAttente);

                return;
            }
            else    // Régime congestionné
            {
                // calcul de la distance entre le véhicule en attente et le point de convergence
                // rmq. : bug possible ici si le véhicule est sur un voie encore plus amont ?
                double dbDistanceToConv;
                if( pVehAttente->GetLink(1) == pVAmNP->GetParent())
                    dbDistanceToConv =  pVAmNP->GetLength() - pVehAttente->GetPos(1);
                else
                    dbDistanceToConv =  pVAmNP->GetLength() + pVehAttente->GetVoie(1)->GetLength() - pVehAttente->GetPos(1);

                AbstractCarFollowing * pCarFollowing = pVehAttente->GetCarFollowing();
                // Création du contexte qui va nous permettre de calculer la position voulue
                CarFollowingContext * pInsertionContext = pCarFollowing->CreateContext(dbInstant, true);
                std::vector<boost::shared_ptr<Vehicule> > leaders(1, pVehLeader);
                
                // définition du leader et de sa distance avec le véhicule en attente à l'instant de l'insertion
                dbPosLead0 = pVehLeader->GetPos(1);
                // rmq. bug possible ici si le leader est sur une voie encore après (possibles si plusieurs voies courtes en aval d'un convergent).
                // Reprendre ici en faisant une fonction qui calcule la distance quel que soit le nombre de voies intermédiaires ?
                if( m_pVAv != pVehLeader->GetVoie(1) ) // le leader se trouve sur le tronçon suivant du tronçon aval du convergent
                {
                    dbPosLead0 += m_pVAv->GetLength();
                }
                std::vector<double> leaderDistances(1, dbPosLead0);
                std::vector<VoieMicro*> lstLanes = pCarFollowing->GetCurrentContext()->GetLstLanes();
                pInsertionContext->SetContext(leaders, leaderDistances, lstLanes, pVehAttente->GetPos(1),NULL,true);
                // astuce pour utiliser un deltaN correspondant à une distance différente de la distance réelle dans le cas de Newell. En principe sans effet
                // pour les autres lois de poursuite
                dbPosLead0 += dbDistanceToConv;
                leaderDistances[0] = dbPosLead0;
                pInsertionContext->SetContext(leaders, leaderDistances, lstLanes, pVehAttente->GetPos(1));
                pCarFollowing->Compute(m_pCvg->GetReseau()->GetTimeStep(), dbInstant, pInsertionContext);
                double dbDistance = pCarFollowing->GetComputedTravelledDistance();
                delete pInsertionContext;

                // on minore la distance totale recalculée en prenant en compte le convergent par la distance parcourue par le véhicule en attente avant traitement du convergent
                dbDistance = std::min<double>(pVehAttente->GetDstParcourueEx(), dbDistance);

                if( dbDistance >= dbDistanceToConv)
                {                         
                    // positionnement du véhicule en attente sur sa position d'insertion calculée en fonction du leader
                    double dbTravelledDistance = 0;
                    for(size_t iLane = 0; iLane < lstLanes.size(); iLane++)
                    {
                        VoieMicro * pLane = lstLanes[iLane];
                        double startPositionOnLane = iLane == 0 ? pVehAttente->GetPos(1) : 0;
                        double laneLength = pLane->GetLongueurEx(pVehAttente->GetType());
                        double endPositionOnLane = dbDistance - dbTravelledDistance + startPositionOnLane;

                        // Si on ne va pas sur les voies suivantes, on sort de la boucle
                        if(endPositionOnLane <= laneLength)
                        {
                            pVehAttente->SetTuyau((Tuyau*)pLane->GetParent(), dbInstant);
                            pVehAttente->SetVoie(pLane);
                            pVehAttente->SetPos(endPositionOnLane);
                            pVehAttente->UpdateLstUsedLanes();
                            break;
                        }
                        dbTravelledDistance += std::min<double>(endPositionOnLane,laneLength) - startPositionOnLane;
                    }

                    dbVit = dbDistance / m_pCvg->GetReseau()->GetTimeStep();
                    pVehAttente->SetVit(dbVit);
                    dbAcc = ( pVehAttente->GetVit(0) - pVehAttente->GetVit(1) )/ m_pCvg->GetReseau()->GetTimeStep();
                    pVehAttente->SetAcc(dbAcc);      
                    
                    m_dbInstLastPassage = dbInstant - m_pCvg->GetReseau()->GetTimeStep();
                                       
                    if(pVehFollower)
                    {
                        AbstractCarFollowing * pFollowerCarFollowing = pVehFollower->GetCarFollowing();
                        // Création du contexte qui va nous permettre de calculer la position voulue
                        CarFollowingContext * pFollowerContext = pFollowerCarFollowing->CreateContext(dbInstant, true);

                        // Distance du follower au point de convergence
                        // rmq. : bug possible ici si le follower est sur un voie encore plus amont ?
                        if( pVehFollower->GetLink(1) == pTAmP)
                            dbPos =  pVAmP->GetLength() - pVehFollower->GetPos(1);
                        else
                            dbPos =  pVAmP->GetLength() + pVehFollower->GetVoie(1)->GetLength() - pVehFollower->GetPos(1);

                        // distance entre le follower et le véhicule en attente
                        double dbFollowerDst = dbPos;
                        std::vector<boost::shared_ptr<Vehicule> > leadersFol;
                        leadersFol.push_back(pVehAttente);
                        std::vector<double> leaderDistancesFol(1, dbFollowerDst);
                        std::vector<VoieMicro*> lstLanesFol = pFollowerCarFollowing->GetCurrentContext()->GetLstLanes();
                        // rmq. pour être conforme à l'ancien code, on considère pour le calcul que le vehicule en attente est sur le point de convergence au début du pas de temps...
                        pFollowerContext->SetContext(leadersFol, leaderDistancesFol, lstLanesFol, pVehFollower->GetPos(1),NULL,true);
                        pFollowerCarFollowing->Compute(m_pCvg->GetReseau()->GetTimeStep(), dbInstant, pFollowerContext);
                        double dbDstFol0 = pFollowerCarFollowing->GetComputedTravelledDistance();
                        // on ne recule pas pour autant
                        dbDstFol0 = std::max<double>(0, dbDstFol0);
                        delete pFollowerContext;

                        // cas ou la distance parcourue par le follower doit être minorée
                        if(pVehFollower->GetDstParcourueEx() > dbDstFol0)
                        {
                            dbTravelledDistance = 0;
                            for(size_t iLane = 0; iLane < lstLanesFol.size(); iLane++)
                            {
                                VoieMicro * pLane = lstLanesFol[iLane];
                                double startPositionOnLane = iLane == 0 ? pVehFollower->GetPos(1) : 0;
                                double laneLength = pLane->GetLongueurEx(pVehFollower->GetType());
                                double endPositionOnLane = dbDstFol0 - dbTravelledDistance + startPositionOnLane;

                                // Si on ne va pas sur les voies suivantes, on sort de la boucle
                                if(endPositionOnLane <= laneLength)
                                {
                                    pVehFollower->SetTuyau((Tuyau*)pLane->GetParent(), dbInstant);
                                    pVehFollower->SetVoie(pLane);
                                    pVehFollower->SetPos(endPositionOnLane);
                                    pVehFollower->UpdateLstUsedLanes();
                                    break;
                                }
                                dbTravelledDistance += std::min<double>(endPositionOnLane,laneLength) - startPositionOnLane;
                            }

                            dbVit = dbDstFol0 / m_pCvg->GetReseau()->GetTimeStep();
                            pVehFollower->SetVit(dbVit);
                            dbAcc = ( pVehFollower->GetVit(0) - pVehFollower->GetVit(1) )/ m_pCvg->GetReseau()->GetTimeStep();
                            pVehFollower->SetAcc(dbAcc);
                        }
                    }
                }
                else // Le véhicule en attente ne s'insère pas, la position du follower n'est donc pas modifié
                {
                    return; // PAS NORMAL !!!
                }
            }
        }
    }
    else
    {        
        // Véhicule entrant (sur lien non prioritaire) : il ne s'insère pas
        for(int j=0; j<(int)LstVehEnAttente.size(); j++)
        {
            pVehAttente = LstVehEnAttente[j];
            if(pVehAttente)
            {
                pVehAttente->SetPos( pVAmNP->GetLength() - 0.0001);
                pVehAttente->SetTuyau( (TuyauMicro*)pVAmNP->GetParent(), dbInstant );
                pVehAttente->SetVoie( (VoieMicro*)pVAmNP);
                
                pVehAttente->SetVit(pVehAttente->GetDstParcourueEx()/m_pCvg->GetReseau()->GetTimeStep());

                dbAcc = ( pVehAttente->GetVit(0) - pVehAttente->GetVit(1) )/ m_pCvg->GetReseau()->GetTimeStep();
                pVehAttente->SetAcc(dbAcc);
                pVehAttente->UpdateLstUsedLanes();
            }
        }
    }
    
}

//================================================================
    PonctualSensor* Convergent::GetCapteur
//----------------------------------------------------------------
// Fonction  : Retourne le capteur correspondant au tronçon 
// Version du: 15/05/2008
// Historique: 15/05/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    Tuyau *pT
)
{
    if( pT == m_LstTuyAv.front() )
        return (PonctualSensor*)m_pGestionCapteur->GetCapteur(0);

    std::vector<AbstractSensor*>::iterator itCpt;
    for(itCpt = m_pGestionCapteur->m_LstCapteurs.begin(); itCpt != m_pGestionCapteur->m_LstCapteurs.end(); itCpt++)
    {
        if( ((PonctualSensor*)(*itCpt))->GetTuyau() == pT )
            return (PonctualSensor*)(*itCpt);
    }

    return NULL;
}

//================================================================
    void PointDeConvergence::InitPriorite
//----------------------------------------------------------------
// Fonction  : Initialisation des priorités des tronçons amont
// Version du: 19/05/2008
// Historique: 19/05/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    VoieMicro   *pVAm,      // Tronçon amont
    int         nNiveau     // Niveau de priorité
)
{
    for(size_t i=0; i<this->m_LstVAm.size(); i++)
    {
        if(pVAm == m_LstVAm[i].pVAm)
        {
            m_LstVAm[i].nNivRef = nNiveau;
            m_LstVAm[i].nNiveau = nNiveau;
            return;
        }
    }

}

//================================================================
    int PointDeConvergence::GetPrioriteRef
//----------------------------------------------------------------
// Fonction  : Retourne le niveau de priorité de référence
// Version du: 19/05/2008
// Historique: 19/05/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    VoieMicro   *pVAm      // Voie amont    
)
{
    for(size_t i=0; i<this->m_LstVAm.size(); i++)
    {
        if(pVAm == m_LstVAm[i].pVAm)
        {
            return m_LstVAm[i].nNivRef;
        }
    }
	return -1;
}

//================================================================
    int PointDeConvergence::GetPriorite
//----------------------------------------------------------------
// Fonction  : Retourne le niveau de priorité à l'instant considéré
// Version du: 19/05/2008
// Historique: 19/05/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    VoieMicro   *pVAm      // Voie amont    
)
{
    for(size_t i=0; i<this->m_LstVAm.size(); i++)
    {
        if(pVAm == m_LstVAm[i].pVAm)
        {
            return m_LstVAm[i].nNiveau;
        }
    }
	return -1;
}

//================================================================
    boost::shared_ptr<Vehicule> Convergent::GetVehiculeEnAttente
//----------------------------------------------------------------
// Fonction  : Retourne le véhicule suceptible de s'insérer
//             sur le tronçon et la voie passés en paramètre
// Version du: 21/07/2008
// Historique: 21/07/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    VoieMicro*    pVAm       // Voie amont        
)
{
    boost::shared_ptr<Vehicule> pV;

    for(size_t i=0; i<m_LstInsVeh.size();i++)
    {
        pV = m_LstInsVeh[i];

        // Le véhicule est-il passé par la voie amont ?
        if( pV->GetVoie(1) == pVAm && pV->GetVoie(0)!= pVAm )           
            return pV;

        for(size_t j=0; j< pV->m_LstUsedLanes.size(); j++)
        {
            if( pV->m_LstUsedLanes[j] == pVAm && pV->GetVoie(0)!= pVAm )                   
                return pV;
        }
    }

    return boost::shared_ptr<Vehicule>();
}


//================================================================
    double Convergent::CalculVitesseLeader
//----------------------------------------------------------------
// Fonction  : Calcule la vitesse du leader d'une branche
//             d'entrée d'un convergent à partir de l'instant
//             de la dernière insertion
// Remarque  : Retourne -1 si la vitesse n'est pas imposée
// Version du: 17/10/2008
// Historique: 17/10/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    Vehicule    *pVehLeader,        // Véhicule leader de la branche
    double      dbInstant,          // Instant de la simulation
    double      dbPasTemps          // Pas de temps (peut être différent du pas de temps de simu si premier instant de création)
)
{
    double  dbt0;
    double  dbv0;
    double  dbPos1;

    if(!pVehLeader)
        return -1;

    dbv0 = -1;
                
    if( !pVehLeader->GetNextVoie() )
        pVehLeader->SetNextVoie( (VoieMicro*)pVehLeader->CalculNextVoie(pVehLeader->GetVoie(1), dbInstant) );    

    if( GetInstLastPassage(pVehLeader->GetNextVoie()) >= 0)
        dbt0 = GetInstLastPassage(pVehLeader->GetNextVoie()) + GetTf(pVehLeader->GetType()) - (dbInstant - dbPasTemps);
    else
        dbt0 = 0;

    if(pVehLeader->GetPos(1) <= UNDEF_POSITION)  // Véhicule créé durant le pas de temps
        dbPos1 = 0;
    else
        dbPos1 = pVehLeader->GetPos(1);

    if(dbt0>0)
    {
        dbv0 = (pVehLeader->GetVoie(1)->GetLength() - dbPos1) / dbt0;
    }
    else
        dbv0 = pVehLeader->GetVitMax();

    return dbv0;
}

//================================================================
    double Convergent::GetInstLastPassage
//----------------------------------------------------------------
// Fonction  : Retourne le dernier instant de passage pour le point 
//             de passage concerné
// Version du: 23/06/2009
// Historique: 23/06/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    VoieMicro *pVAval
)
{    
    std::deque<PointDeConvergence*>::iterator itPtCvg;

    for( itPtCvg = m_LstPtCvg.begin(); itPtCvg!= m_LstPtCvg.end(); itPtCvg++)
        if( (*itPtCvg)->m_pVAv == pVAval)
            return (*itPtCvg)->m_dbInstLastPassage;
        
    return UNDEF_INSTANT;
}

//================================================================
    void Convergent::AddMouvement
//----------------------------------------------------------------
// Fonction  : Ajoute un mouvement au convergent
// Version du: 23/06/2009
// Historique: 23/06/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    VoieMicro*  pVAm,               // Voie amont
    VoieMicro*  pVAv,               // Pointeur sur la voie aval
    int         nPriorite /*=1*/    // Niveau de priorité
)
{
    DesVoieAmont tmp;
    std::deque<PointDeConvergence*>::iterator itPtCvg;

    tmp.pVAm = pVAm;
    tmp.nNiveau = nPriorite;    

    // Recherche du point de convergence concerné
    for( itPtCvg = m_LstPtCvg.begin(); itPtCvg!= m_LstPtCvg.end(); itPtCvg++)
    {
        if( (*itPtCvg)->m_pVAv == pVAv)
        {            
            (*itPtCvg)->m_LstVAm.push_back( tmp );
        }
    }
}

//================================================================
    bool Convergent::IsVoieAmontNonPrioritaire
//----------------------------------------------------------------
// Fonction  : Retourne vrai si voie non prioritaire
// Version du: 23/06/2009
// Historique: 23/06/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    VoieMicro* pVAm
)
{
	PointDeConvergence *pPtCvg;    
    std::deque<DesVoieAmont>::iterator itDes;

    int nNiv = 0;
    int nNivMin = INT_MAX;

	pPtCvg = this->GetPointDeConvergence( pVAm );

	if(pPtCvg)
	{
		for(itDes = pPtCvg->m_LstVAm.begin(); itDes != pPtCvg->m_LstVAm.end(); itDes++)
        {
            if( (*itDes).pVAm == pVAm)
			{				
                nNiv = (*itDes).nNiveau;				
			}       

			if( nNivMin > (*itDes).nNiveau)
				nNivMin = (*itDes).nNiveau;         
        }
	}

    return ( nNiv > nNivMin && nNiv > 1) ;
}

//================================================================
    bool Convergent::IsVoieAmontPrioritaire
//----------------------------------------------------------------
// Fonction  : Retourne vrai si voie prioritaire
// Version du: 23/06/2009
// Historique: 23/06/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    VoieMicro* pVAm
)
{
    PointDeConvergence *pPtCvg;    
    std::deque<DesVoieAmont>::iterator itDes;

    int nNiv;
    int nNivMin = INT_MAX;

	pPtCvg = GetPointDeConvergence( pVAm );

	if(!pPtCvg)
		return false;

    for(itDes = pPtCvg->m_LstVAm.begin(); itDes != pPtCvg->m_LstVAm.end(); itDes++)
    {
        if( (*itDes).pVAm == pVAm)
            nNiv = (*itDes).nNiveau;

        if( nNivMin > (*itDes).nNiveau)
            nNivMin = (*itDes).nNiveau;
    }

    return ( nNiv <= nNivMin) ;
}

VoieMicro*  Convergent::GetNextVoie(VoieMicro* pV)
{
    std::deque<PointDeConvergence*>::iterator itPtCvg;
    std::deque<DesVoieAmont>::iterator itDes;

    for( itPtCvg = m_LstPtCvg.begin(); itPtCvg!= m_LstPtCvg.end(); itPtCvg++)
    {
        for(itDes = (*itPtCvg)->m_LstVAm.begin(); itDes != (*itPtCvg)->m_LstVAm.end(); itDes++)
        {
            if( (*itDes).pVAm == pV)
                return (*itPtCvg)->m_pVAv;
        }
    }
    return NULL;
}

void Convergent::CalculInsertion(double dbInstant)
{
    std::deque<PointDeConvergence*>::iterator itPtCvg;

    for( itPtCvg = m_LstPtCvg.begin(); itPtCvg!= m_LstPtCvg.end(); itPtCvg++)
    {
        (*itPtCvg)->CalculInsertion(dbInstant);
    }
}

//================================================================
    PointDeConvergence*  Convergent::GetPointDeConvergence
//----------------------------------------------------------------
// Fonction  : Retourne le point de convergence associé au numéro 
//             de voie aval passé en paramètre
// Version du: 23/06/2009
// Historique: 23/06/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    int nVoieAval
)
{
    if(nVoieAval+1 > (int)m_LstPtCvg.size())
        return NULL;

    std::deque<PointDeConvergence*>::iterator itPtCvg;
    for(itPtCvg = m_LstPtCvg.begin(); itPtCvg != m_LstPtCvg.end(); itPtCvg++)
        if( (*itPtCvg)->m_pVAv->GetNum() == nVoieAval)
            return (*itPtCvg);

    return NULL;
}

//================================================================
    PointDeConvergence*  Convergent::GetPointDeConvergence
//----------------------------------------------------------------
// Fonction  : Retourne le point de convergence associé à la voie 
//             amont passée en paramètre
// Version du: 23/06/2009
// Historique: 23/06/2009 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    VoieMicro* pVAm
)
{
    std::deque<PointDeConvergence*>::iterator itPtCvg;
	std::deque<DesVoieAmont>::iterator itVAm;

    for(itPtCvg = m_LstPtCvg.begin(); itPtCvg != m_LstPtCvg.end(); itPtCvg++)
		for(itVAm = (*itPtCvg)->m_LstVAm.begin(); itVAm != (*itPtCvg)->m_LstVAm.end(); itVAm++)
			if( (*itVAm).pVAm == pVAm )
				return (*itPtCvg);

    return NULL;
}

//================================================================
bool Convergent::ComputeTraffic
//----------------------------------------------------------------
// Fonction  : Ajustement de la position calculée pour le véhicule
//             par la loi de poursuite en fonction des règles
//             d'écoulement associée au convergent
// Version du: 19/12/2014
// Historique: 19/12/2014 (O.Tonck - Ipsis)
//             Création
//================================================================
(
    boost::shared_ptr<Vehicule> pV,     // Véhicule en amont du convergent
    VoieMicro * pUpstreamLane,          // Voie sur laquelle va arriver le véhicule dans le convergent (éventuellement différent de la voie courante du véhicule s'il est loin)
    double      dbInstant,              // Instant de la simulation
    double      dbPasTemps,             // Pas de temps (peut être différent du pas de temps de simu si premier instant de création)
    double     &dbMinimizedGoalDistance // Distance parcourue par le véhicule minimisée pour tenir compte du convergent
)
{
    bool bHasInfluence = false;

    // Ajustement de la vitesse d'approche du leader d'une branche amont non prioritaire
    // pour respecter le temps de prochaine insertion
    // rmq. OTK : historiquement, on ne traite pas convergents plus loin que celui en aval de la voie courante du véhicule. A améliorer éventuellement ?
    if(pUpstreamLane == pV->GetVoie(1) && IsVoieAmontNonPrioritaire(pUpstreamLane))
    {            
        // Est-il le leader de la branche ?
        boost::shared_ptr<Vehicule> pVehLeader = m_pReseau->GetPrevVehicule(pV.get(), false, true);
        if(!pVehLeader)
        {
            double dbv0 = CalculVitesseLeader(pV.get(), dbInstant, dbPasTemps);
            dbMinimizedGoalDistance = dbv0*dbPasTemps;
            bHasInfluence = true;
        }
    }
    return bHasInfluence;
}

//! Calcule le cout d'un mouvement autorisé
double Convergent::GetAdditionalPenaltyForUpstreamLink(Tuyau* pTuyauAmont)
{
    int minPriority = -1, movePriority = -1;
    if (!m_LstPtCvg.empty())
    {
        PointDeConvergence * pPtCvg = m_LstPtCvg.front();

        for(size_t j = 0; j < pPtCvg->m_LstVAm.size(); j++)
        {
            if(pPtCvg->m_LstVAm[j].pVAm->GetParent() == pTuyauAmont)
            {
                movePriority = pPtCvg->m_LstVAm[j].nNiveau;
            }
            if(j == 0)
            {
                minPriority = pPtCvg->m_LstVAm[j].nNiveau;
            }
            else
            {
                minPriority = std::min<int>(minPriority, pPtCvg->m_LstVAm[j].nNiveau);
            }
        }
    }

    return minPriority == movePriority ? 0 : m_pReseau->GetNonPriorityPenalty();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void DesVoieAmont::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void DesVoieAmont::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void DesVoieAmont::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(pVAm);
    ar & BOOST_SERIALIZATION_NVP(nNivRef);
    ar & BOOST_SERIALIZATION_NVP(nNiveau);
}

template void PtConflitTraversee::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void PtConflitTraversee::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void PtConflitTraversee::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(pT1);
    ar & BOOST_SERIALIZATION_NVP(pT2);
    ar & BOOST_SERIALIZATION_NVP(dbPos1);
    ar & BOOST_SERIALIZATION_NVP(dbPos2);
    ar & BOOST_SERIALIZATION_NVP(dbDst1);
    ar & BOOST_SERIALIZATION_NVP(dbDst2);
    ar & BOOST_SERIALIZATION_NVP(pTProp);
    ar & BOOST_SERIALIZATION_NVP(mvt1);
    ar & BOOST_SERIALIZATION_NVP(mvt2);
}

template void GrpPtsConflitTraversee::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void GrpPtsConflitTraversee::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void GrpPtsConflitTraversee::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(lstPtsConflitTraversee);
    ar & BOOST_SERIALIZATION_NVP(pVPtAttente);
    ar & BOOST_SERIALIZATION_NVP(dbPosPtAttente);
    ar & BOOST_SERIALIZATION_NVP(dbDstPtAttente);
    ar & BOOST_SERIALIZATION_NVP(dbInstLastTraversee);
    ar & BOOST_SERIALIZATION_NVP(LstTf);
    ar & BOOST_SERIALIZATION_NVP(nNbMaxVehAttente);
    ar & BOOST_SERIALIZATION_NVP(listVehEnAttente);
}

template void PointDeConvergence::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void PointDeConvergence::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void PointDeConvergence::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_LstVAm);
	ar & BOOST_SERIALIZATION_NVP(m_pVAv);
    ar & BOOST_SERIALIZATION_NVP(m_pTAv);
	ar & BOOST_SERIALIZATION_NVP(m_dbInstLastPassage);
    ar & BOOST_SERIALIZATION_NVP(m_pCvg);
	ar & BOOST_SERIALIZATION_NVP(m_bFreeFlow);
	ar & BOOST_SERIALIZATION_NVP(m_bInsertion);
	ar & BOOST_SERIALIZATION_NVP(m_nRandNumbers);
}

template void Convergent::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Convergent::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Convergent::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(ConnectionPonctuel);
    ar & BOOST_SERIALIZATION_NVP(m_pGestionCapteur);
    ar & BOOST_SERIALIZATION_NVP(m_dbGamma);
    ar & BOOST_SERIALIZATION_NVP(m_dbMu);
    ar & BOOST_SERIALIZATION_NVP(m_LstTf);
    ar & BOOST_SERIALIZATION_NVP(m_pGiratoire);
    ar & BOOST_SERIALIZATION_NVP(m_LstPtCvg);
}
