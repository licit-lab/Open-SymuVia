///////////////////////////////////////////////////////////
//  ControleurDeFeux.cpp
//  Implementation of the Class ControleurDeFeux
//  Created on:      23-oct.-2008 13:38:10
//  Original author: becarie
///////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ControleurDeFeux.h"

#include "tuyau.h"
#include "DiagFonda.h"
#include "reseau.h"
#include "SystemUtil.h"
#include "vehicule.h"
#include "ConnectionPonctuel.h"
#include "voie.h"
#include "Logger.h"
#include "usage/Trip.h"
#include "usage/PublicTransportFleet.h"

#include <boost/serialization/deque.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>

#pragma warning(disable : 4003)
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#pragma warning(default : 4003)

#include <list>

using namespace std;

ControleurDeFeux::ControleurDeFeux(Reseau *pReseau, char *sID, double dbRougeDegagement, double dbVertMin)
{
    m_pReseau = pReseau;
    m_strID = sID;

    m_pLstPlanDeFeux = new ListOfTimeVariationEx<PlanDeFeux>;       
	m_pLstPlanDeFeuxInit = new ListOfTimeVariationEx<PlanDeFeux>;       

    m_dbDureeRougeDegagement = dbRougeDegagement;
	m_dbDureeRougeDegagementInit = dbRougeDegagement;
    m_dbDureeVertMin = dbVertMin;
	m_dbDureeVertMinInit = dbVertMin;

    m_cModeFct = 'N';

    m_PDFActif = NULL;

    m_pSeqPrePriorite=NULL;
    m_pSeqRetour1=NULL;
    m_pSeqRetour2=NULL;
}



ControleurDeFeux::~ControleurDeFeux()
{
    delete m_pLstPlanDeFeux;
	delete m_pLstPlanDeFeuxInit;

    for(size_t i = 0; i < m_LstLGP.size(); i++)
    {
        delete m_LstLGP[i];
    }
}


void ControleurDeFeux::AddCoupleEntreeSortie(Tuyau* pEntree, Tuyau* pSortie)
{
    // On vérifie que ce couple n'a pas déjà été ajouté
    for(size_t i=0; i<m_LstCoupleEntreeSortie.size(); i++)
    {
        if( m_LstCoupleEntreeSortie[i].pEntree == pEntree && m_LstCoupleEntreeSortie[i].pSortie == pSortie)
            return;
    }

    CoupleEntreeSortie cpl;

    cpl.pEntree = pEntree;
    cpl.pSortie = pSortie;

    m_LstCoupleEntreeSortie.push_back( cpl );

	UpdatePosition();

}


/**
 * Calcule la couleur des feux à l'instant précisé et retourne vrai si le feu est
 * vert et la proportion de temps de vert durant le pas de temps
 */
eCouleurFeux ControleurDeFeux::GetStatutCouleurFeux(double dbInstant, Tuyau *pTE, Tuyau *pTS, bool *pbPremierInstCycle, double *pdbPropVertOrange, bool  *pbPrioritaire)
{    
    eCouleurFeux eCouleur;          // Indique la couleur de feux à l'instant considéré
    eCouleurFeux ePrevCouleur;      // Indique la couleur de feux à l'instant considéré

    if (pbPremierInstCycle != NULL)
		*pbPremierInstCycle = false;

    if(pbPrioritaire != NULL)
        *pbPrioritaire = false;
        
    if(!pTS || !pTE)
        return FEU_ROUGE;   

    // Ajout du couple entrée sortie s'il n'est pas déjà recensé
    AddCoupleEntreeSortie(pTE, pTS);

    eCouleur = GetCouleurFeux( dbInstant,  pTE, pTS, pbPremierInstCycle, pdbPropVertOrange, pbPrioritaire);

    if(dbInstant - m_pReseau->GetTimeStep()>0)
        ePrevCouleur = GetCouleurFeux( dbInstant - m_pReseau->GetTimeStep() ,  pTE, pTS);
    else
        ePrevCouleur = FEU_VERT;

    if((eCouleur == FEU_ROUGE) && (ePrevCouleur == FEU_ROUGE))    // Rouge tout le temps
    {
		if (pdbPropVertOrange != NULL)
			*pdbPropVertOrange = 0;
        return FEU_ROUGE;
    }

    if(((eCouleur == FEU_VERT) || (eCouleur == FEU_ORANGE)) && ((ePrevCouleur == FEU_VERT) || (ePrevCouleur == FEU_ORANGE)))      // Vert ou orange tout le temps
    {
		if (pdbPropVertOrange != NULL)
			*pdbPropVertOrange = 1;
        return eCouleur;
    }

    if((eCouleur == FEU_VERT) && (ePrevCouleur == FEU_ROUGE))     // Rouge au début du pas de temps et vert à la fin
    {
        return FEU_VERT;
    }

    if((eCouleur == FEU_ROUGE) && ((ePrevCouleur == FEU_VERT) || (ePrevCouleur == FEU_ORANGE)))     // Vert (ou orange) au début du pas de temps et rouge à la fin
    {
		if (pdbPropVertOrange != NULL)
			*pdbPropVertOrange = 1;

        return FEU_ROUGE;
    }

    return FEU_ROUGE;
}

PlanDeFeux* ControleurDeFeux::GetTrafficLightCycle
(
    double      dbInstant,      // Instant courant de la simulation
	STimeSpan   &tsTpsEcPlan    // Retourne le temps écoulé depuis le début du plan
)
{
    STimeSpan   tsInst( (int)dbInstant);
    STimeSpan   tsTmp;
    STime   tHrCrt;           
    PlanDeFeux* pPDF = NULL;
   
    tHrCrt = m_pReseau->GetSimuStartTime() + tsInst;

    if(m_pLstPlanDeFeux)  
    {        
        // Recherche du plan de feux
        pPDF = m_pLstPlanDeFeux->GetFirstVariationEx();
        if(pPDF)
        {
            if( pPDF->GetStartTime() > tHrCrt)  // si l'heure de début du premier plan est supérieur à l'instant traité
            {
                while( pPDF->GetStartTime() > tHrCrt )
                {
                    STimeSpan tsTmp( (int)pPDF->GetCycleLength() );
                    tHrCrt = tHrCrt + tsTmp;
                }
            }
        }

        pPDF = m_pLstPlanDeFeux->GetVariationEx(tHrCrt);

        // Calcul de la durée écoulée depuis le début du plan de feux
        tsTpsEcPlan = tHrCrt - pPDF->GetStartTime();
    }
    // else return NULL

    return pPDF;
}

PlanDeFeux* ControleurDeFeux::GetPlanDeFeuxFromID(const std::string & sID)
{
    PlanDeFeux *pPDF = NULL;
    for(int i=0; i<(int)m_pLstPlanDeFeux->GetLstTV()->size(); i++)
    {
        pPDF = m_pLstPlanDeFeux->GetVariation(i);

        if(pPDF->GetID()==sID)
            return pPDF;
    }

    return NULL;
}

CDFSequence* ControleurDeFeux::GetSequence
(   
    double dbInstant,     
    double  &dbTpsPrevSeqs,     // Somme des durées des séquences précédentes du cyle
    double  &dbTpsCurSeq,       // Temps depuis le début de la séquence courante
    bool    &bPrioritaire       // séquence liée à un VGP ?
)
{
    PlanDeFeux* pPDF;
    STimeSpan   tsTpsEcPlan;   
    STimeSpan   tsTmp;
    int         nSec;  
    int         nTmp;
    CDFSequence    *pSeq;

    bPrioritaire = false;

    if(!m_PDFActif)   
    {
        pPDF = GetTrafficLightCycle(dbInstant, tsTpsEcPlan); 

        // Le plan de feux calculé est-il remplacé par un plan de feux de VGP prioritaire
        if(m_pPDFEx && dbInstant >= m_dbDebPDFEx)
        {
            if(dbInstant <= m_dbFinPDFEx)
            {
                pPDF = m_pPDFEx.get();
                tsTpsEcPlan = (int)(dbInstant - m_dbDebPDFEx);
                bPrioritaire = true;
            }
            else
            {
                m_pPDFEx.reset();    // le plan de feux de VGP prioritaire est terminé
            }
        }
    }
    else
    {
        pPDF = m_PDFActif;
		STimeSpan   tsInst( (int)dbInstant);
        tsTpsEcPlan = m_pReseau->GetSimuStartTime() + tsInst - pPDF->GetStartTime();        
    }
   
    if(!pPDF)
        return NULL;
    
    // Recherche de la séquence concernée
    nSec = (int)tsTpsEcPlan.GetTotalSeconds( );
	nTmp = nSec % (int)pPDF->GetCycleLength();

    pSeq = pPDF->GetSequence(nTmp, dbTpsPrevSeqs, dbTpsCurSeq);

    return pSeq;
}

eCouleurFeux ControleurDeFeux::GetCouleurFeux(double dbInstant, Tuyau *pTE, Tuyau *pTS, bool *pbPremierInstCycle, double *pdbPropVertOrange, bool  *pbPrioritaire)
{    
    STimeSpan    tsTpsEcPlan;
    STimeSpan    tsInst( (int)dbInstant);
    STimeSpan    tsTmp;
    int          nTmp = 0;
    CDFSequence     *pSeq;
    SignalActif  *pSignalActif;
    double       dbCum;
    eCouleurFeux eCouleur;
    double       dbTpsCurSeq;

	if (pbPremierInstCycle != NULL)
		*pbPremierInstCycle = false;

    if(pbPrioritaire != NULL)
        *pbPrioritaire = false;
   
    if(!pTS || !pTE)
        return FEU_ROUGE;

    bool bPrioritaire;
    pSeq = GetSequence(dbInstant, dbCum, dbTpsCurSeq, bPrioritaire); 
    if(pbPrioritaire != NULL)
        *pbPrioritaire = bPrioritaire;

    if(!pSeq)
        return FEU_VERT;
	
	if (pbPremierInstCycle != NULL)
	{
		if(dbTpsCurSeq<m_pReseau->GetTimeStep() && pSeq->GetNum() == 0)
			*pbPremierInstCycle = true;
		else
			*pbPremierInstCycle = false;
	}

    // Recherche du signal actif concerné
    pSignalActif = pSeq->GetSignalActif(pTE, pTS);
    if(!pSignalActif)
        return FEU_ROUGE;

    eCouleur =  pSignalActif->GetCouleurFeux(dbTpsCurSeq);    

	if (pdbPropVertOrange != NULL)
		*pdbPropVertOrange = pSignalActif->GetRapVertOrange(dbTpsCurSeq, m_pReseau->GetTimeStep(), (eCouleur != FEU_ROUGE));   // Calcul de la proportion de vert

    return eCouleur;    

}

// Mise à jour des caractéristiques dymamiques du contrôleur de feux à la fin du pas de temps considéré
void ControleurDeFeux::Update(double dbInstant)
{
    bool bPDFChange = false;

    // Un VGP a t'il été vu par un capteur amont ?
    for(int i=0; i< (int)m_LstLGP.size(); i++)
    {
        LGP *plgp = m_LstLGP[i];

        for(int j=0; j < (int)plgp->LstLTG.size(); j++)
        {
            Trip *pLB = plgp->LstLTG[j];
            std::vector<boost::shared_ptr<Vehicule>> lstBus = m_pReseau->GetPublicTransportFleet()->GetVehicles(pLB);

            for(int k=0; k< (int)lstBus.size(); k++)
            {
                boost::shared_ptr<Vehicule> pBus = lstBus[k];

                if( pBus->IsPasse( plgp->pTCptAm, plgp->dbPosCptAm ) ) 
                {                        
                    // Mise en place du plan de feux 'pre-priorité'

                    // Calcul de la durée pour que le VGP atteigne la zone controlée
                    double dbTZone, dbDst;                        
                    double dbDstArret;
                    TripNode  *objArr;
                    double dbPasTemps = m_pReseau->GetTimeStep();

                    // Existe t'il un arrêt entre la postion du VGP et l'entrée de la zone ?
                    // On fait l'hypothèse qu'il existe au plus un arrêt
                    dbDstArret = pBus->GetNextTripNodeDistance(0, &objArr);

                    // Distance qui reste à parcourir avant l'entrée de la zone du contrôleur
                    double dbPosEntreeZone = plgp->dbPosEntreeZone;
                    // Cas où le tronçon du bus est avant le tronçon de la zone : on se remet dans le repère du tronçon du bus.
                    // rmq. ne va pas fonctionner si plus d'un tronçon entre le capteur amont et la zone (pas grave ?)
                    if (plgp->pTEntreeZone != pBus->GetLink(0))
                    {
                        dbPosEntreeZone += pBus->GetLink(0)->GetLength();
                    }

                    dbDst = dbPosEntreeZone - pBus->GetPos(0);

                    if(dbDstArret>=dbDst)    // Pas d'arrêt à gérer
                    {
                        double dbAcc = pBus->GetType()->GetAccMax(pBus->GetVit(0));
                        double dbVitMax = std::min<double>( pBus->GetDiagFonda()->GetVitesseLibre(), plgp->pTEntreeZone->GetVitRegByTypeVeh(pBus->GetType(),
                            dbInstant, pBus->GetPos(0), pBus->GetVoie(0)->GetNum()));

                        // Nombre de pas de temps nécessaire pour atteindre la vitesse libre
						int no = (int)ceill(  ( dbVitMax - pBus->GetVit(0) ) / (dbAcc*dbPasTemps));
                        double dbXno = pBus->GetPos(0) + no*pBus->GetVit(0)*dbPasTemps + no*(no+1)/2*dbAcc*dbPasTemps*dbPasTemps;
                           

                        if (dbXno <= dbPosEntreeZone)
                        {
                            // Le bus atteind sa vitesse libre avant d'entrer dans la zone
                            dbTZone = dbPasTemps*(no + floor((dbPosEntreeZone - dbXno) / (dbVitMax * dbPasTemps)));
                        }
                        else
                        {
                            // Le bus est en train d'accélérer quand il rentre dans la zone
                            dbTZone = 0;                                
                            double dbVitVGP = std::min<double>( dbVitMax, pBus->GetVit(0) + pBus->GetAccMax(pBus->GetVit(0))*dbPasTemps);
                            double dbXVGP = pBus->GetPos(0) + dbVitVGP*dbPasTemps;
                            while (dbXVGP <= dbPosEntreeZone)
                            {
                                dbVitVGP = std::min<double>( dbVitMax, dbVitVGP + pBus->GetAccMax(dbVitVGP)*dbPasTemps);
                                dbXVGP += dbVitVGP*dbPasTemps;
                                dbTZone += dbPasTemps;
                            }                                
                        }
                    }
                    else // Un arrêt à gérer
                    {                                       
                        dbTZone = pBus->CalculTempsAvecArret(dbInstant, dbPasTemps, 0, plgp->pTEntreeZone, plgp->dbPosEntreeZone, objArr);
                    }

                    // Si on avait déjà un véhicule guidé prioritaire, une séquence a déjà été établie pour le faire passer.
                    // On ne va donc que rajouter un signal pour le nouveau véhicule guidé prioritaire, en prolongeant la séquence si nécessaire
                    bool bFirstVGP = m_LstVGPs.size() == 0;

                    // Ajout du VGP et de la LGP à la liste à traiter
                    m_LstVGPs.push_back(make_pair(pBus, plgp));

                    // Recupération des troncons d'entrée et de sortie du CAF
                    Tuyau *pTuyauAmont, *pTuyauAval;
                    GetTuyauAmAv(pBus.get(), &pTuyauAmont, &pTuyauAval);

                    if(m_pReseau->IsVGPDebug()) 
                        m_pReseau->log() << "Remaining time to reach the controller's zone : " << dbTZone << " s" << endl;

                    double dbInstArriveeVGP = dbInstant + dbTZone;

                    // Traitement du cas classique : aucun VGP n'esten cours de traitement
                    if(bFirstVGP)
                    {
                        m_cModeFct = 'O';

                        // Dans le cas où on était en mode Post priorité pour des VGPs précédent, on a déjà un plan de feux particulier en place : on le termine
                        if(m_pPDFEx)
                        {
                            m_pPDFEx.reset();
                        }

                        // Calcul de l'instant à partir duquel il faut appliquer le plan priorité                        
                        double  dbTpsPrevSeqs = 0 , dbTpsCurSeq = 0;
                        PlanDeFeux *pPDF;
                        STimeSpan ts;       
                        int nPrevSequence;
                        bool bPrioritaire;
                        
                        pPDF = GetTrafficLightCycle( dbInstant, ts);
                        CDFSequence* pSeqZone = GetSequence(dbInstant + dbTZone, dbTpsPrevSeqs, dbTpsCurSeq, bPrioritaire);  // Séquence d'arrivée

                        nPrevSequence = pSeqZone->GetNum() - 1;
                        if(nPrevSequence<0)
                            nPrevSequence = (int)pPDF->GetLstSequences().size()-1;

                        CDFSequence* pPrevSequence = pPDF->GetLstSequences()[nPrevSequence];
                          
                        double dbInstPlanPrePriorite = dbInstant + dbTZone - dbTpsCurSeq - pPrevSequence->GetTotalLength();

                        if(m_pReseau->IsVGPDebug())
                            m_pReseau->log() << "Applying 'pre-priority' plan instant : " << dbInstPlanPrePriorite  << endl;
                        
                        // Calcul de la séquence actuelle
                        CDFSequence* pActSequence = GetSequence(dbInstant, dbTpsPrevSeqs, dbTpsCurSeq, bPrioritaire);                        

                        // Création du plan de feux de la phase du VGP prioritaire                        
                        m_pPDFEx.reset(new PlanDeFeux());
                        if( dbInstant <= dbInstPlanPrePriorite)
                            m_dbDebPDFEx = dbInstPlanPrePriorite; // Début du plan d'application plus tard
                        else 
                            m_dbDebPDFEx = dbInstant;   // le plan est appliqué dès maintenant
                        m_dbFinPDFEx = m_dbDebPDFEx + DBL_MAX;

                        // Couple de séquences de retour (après la sortie du VGP
                        m_pSeqRetour1 = pPDF->GetNextSequence( pActSequence );
                        m_pSeqRetour2 = pPDF->GetNextSequence( m_pSeqRetour1 );

                        // Détermination du type de gestion du mode 'pre-priorité'

                        // Calcul de la durée minimale pour la séquence d'arrivée
                        double dbDureeMin = pSeqZone->GetMinDuree(m_dbDureeVertMin);
                        if((pSeqZone == pActSequence) && (dbTpsCurSeq +  dbTZone  >=  dbDureeMin + m_dbDureeRougeDegagement))
                        {
                            m_nTypeGestionPrePriorite = 1;  // 'respect temps min' = cas 3.1
                            m_pSeqPrePriorite = pSeqZone;
                            if(m_pReseau->IsVGPDebug())
                                m_pReseau->log() << "Management type : minimum time respect" << endl;

                            // Construction de la séquence à appliquer en pré-priorité                            
                            // Construction des séquences modifiées
                            CDFSequence *pSeq;

                            // Pré-priorité                            
                            pSeq = new CDFSequence();
                            pSeq->Copy( pSeqZone );
                            pSeq->SetTotalLength ( dbTZone - (m_dbDebPDFEx-dbInstant) - m_dbDureeRougeDegagement);
                            m_pPDFEx->AddSequence(pSeq);

                            if(!plgp->bPrioriteTotale && plgp->nSeqPartagee == pSeqZone->GetNum() ) // Prolongment de la séquence
                            {
                                pSeq->SetTotalLength ( DBL_MAX);
                                SignalActif *pSA = new SignalActif(pTuyauAmont, pTuyauAval, pSeq->GetTotalLength(), 0, 0);
                                pSeq->AddActiveSignal(pSA);
                            }
                            else
                            {
                                // Priorité
                                pSeq = new CDFSequence();
                                pSeq->SetNum(1);
                                pSeq->SetTotalLength ( DBL_MAX);  // Durée de la séquence de priorité totale infinie
                                m_pPDFEx->AddSequence(pSeq);
                                SignalActif *pSA = new SignalActif(pTuyauAmont, pTuyauAval, pSeq->GetTotalLength(), 0, 0);
                                pSeq->AddActiveSignal(pSA);
                            }
                        }
                        else
                        {
                            if(pSeqZone == pActSequence) // la séquence d'arrivée du VGP dans la zone de contrôle est égale à la séquence actuelle
                            {
                                m_nTypeGestionPrePriorite = 2;  // 'séq. actuelle écourtée'
                                m_pSeqPrePriorite = pSeqZone;
                                if(m_pReseau->IsVGPDebug())
                                    m_pReseau->log() << "Management type : current seq. shortened" << endl;

                                // Construction de la séquence à appliquer en pré-priorité   
                                // Construction des séquences modifiées
                                CDFSequence *pSeq;   

                                m_dbDebPDFEx -= dbTpsCurSeq;       

                                // Pré-priorité                            
                                pSeq = new CDFSequence();
                                pSeq->Copy( pSeqZone );
                                double dbDureeMin = pSeq->GetMinDuree(m_dbDureeVertMin);    // Durée de vert min
                                pSeq->SetTotalLength ( dbDureeMin ); 

                                // Rouge pour la LGP
                                SignalActif* pSA = pSeq->GetSignalActif(pTuyauAmont, pTuyauAval);
                                // Seulement si le signal est actif ...
                                if(pSA) {
                                    pSA->SetGreenDuration(0);
                                }

                                    m_pPDFEx->AddSequence(pSeq);

                                // Séquence de rouge de dégagement
                                pSeq = new CDFSequence();
                                pSeq->Copy( pSeqZone );
                                pSeq->SetNum(1);
                                pSeq->SetTotalLength (m_dbDureeRougeDegagement);
                                m_pPDFEx->AddSequence(pSeq);

                                for(int nSA=0; nSA<(int)pSeq->GetLstActiveSignals().size(); nSA++)
                                {
                                    SignalActif* pSA = pSeq->GetLstActiveSignals()[nSA];
                                    pSA->SetGreenDuration(0); 
                                }                                                                     
                               
                                // Priorité
                                pSeq = new CDFSequence();
                                pSeq->SetNum(2);
                                pSeq->SetTotalLength ( DBL_MAX);  // Durée de la séquence de priorité totale infinie
                                m_pPDFEx->AddSequence(pSeq);
                                pSA = new SignalActif(pTuyauAmont, pTuyauAval, pSeq->GetTotalLength(), 0, 0);
                                pSeq->AddActiveSignal(pSA);
                                

                            }
                            else
                            {
                                m_nTypeGestionPrePriorite = 3;  // 'prolongement séq. actuelle'
                                m_pSeqPrePriorite = pActSequence;
                                if(m_pReseau->IsVGPDebug())
                                    m_pReseau->log() << "Management type : current seq. prolongation" << endl;

                                // Construction de la séquence à appliquer en pré-priorité   
                                // Construction des séquences modifiées
                                CDFSequence *pSeq;   

                                m_dbDebPDFEx -= dbTpsCurSeq;       

                                // Pré-priorité                            
                                pSeq = new CDFSequence();
                                pSeq->Copy( pActSequence );    
                                pSeq->SetNum(0);
                                pSeq->SetTotalLength ( dbInstArriveeVGP - m_dbDebPDFEx ); 
                                m_pPDFEx->AddSequence(pSeq);                               

                                // Priorité
                                pSeq = new CDFSequence();
                                // si priorité partagée
                                if(!plgp->bPrioriteTotale && plgp->nSeqPartagee == pSeqZone->GetNum() ) // ajout des priorités partagées
                                {
                                    pSeq->Copy(pSeqZone);
                                }
                                pSeq->SetNum(1);
                                pSeq->SetTotalLength ( DBL_MAX);  // Durée de la séquence de priorité totale infinie
                                m_pPDFEx->AddSequence(pSeq);
                                SignalActif *pSA = new SignalActif(pTuyauAmont, pTuyauAval, pSeq->GetTotalLength(), 0, 0);
                                pSeq->AddActiveSignal(pSA);
                            }
                        }
                    }
                    else
                    {
                        // Ici, on est sensé avoir un plan de feu pour assurer la priorité des VGP, et on n'est pas encore en mode "séquences de retour à la normale"
                        assert(m_cModeFct != 'Q');
                        assert(m_pPDFEx);
                        // Dans ce cas, c'est facile, en supposant qu'il n'y a pas de croisement entre les lignes prioritaires,
                        // on ajoute simplement à la dernière séquence de durée infinie (qui se terminera à la détection de la sortie du dernier VGP en cours de traitement)
                        // le signal actif permettant de faire circuler le véhicule qui arrive.
                        m_pPDFEx->GetLstSequences().back()->AddActiveSignal(new SignalActif(pTuyauAmont, pTuyauAval, m_pPDFEx->GetLstSequences().back()->GetTotalLength(), 0, 0));
                    }

                    bPDFChange = true;
                }
            }
        }
    }        

    // Fonctionnement PRE-PRIORITE
    if( m_cModeFct == 'O' ) 
    {
        // Un VGP en cours de traitement est-il entré dans la zone du contrôleur de feux
        for(size_t i = 0; i < m_LstVGPs.size(); i++)
        {
            if( m_LstVGPs[i].first->IsPasse( m_LstVGPs[i].second->pTEntreeZone, m_LstVGPs[i].second->dbPosEntreeZone ) )
            {
                // Modification du mode de fonctionnement : si un VGP est entré dans la zone de contrôle, on passe en mode 'priorité'
                m_cModeFct = 'P';
                break;
            }
        }
    }

    // Fonctionnement PRIORITE
    if( m_cModeFct == 'P' ) 
    {
        std::deque<std::pair<boost::shared_ptr<Vehicule>, LGP*> >::iterator iter = m_LstVGPs.begin();
        while(iter != m_LstVGPs.end())
        {
            // Suppression des véhicules qui sont sortis de la zone
            if( iter->first->IsPasse( iter->second->pTCptAv, iter->second->dbPosCptAv ) || !iter->first || !iter->first->GetLink(0) ) // cas du bus NULL : le bus est sorti du réseau (dernier arrêt atteint par exemple)
            {
                iter = m_LstVGPs.erase(iter);
            }
            else
            {
                iter++;
            }
        }
        if(m_pReseau->IsVGPDebug() && m_LstVGPs.size() != 0)
        {
            m_pReseau->log() << "=> Traffic light contoller " << m_strID << " waits for the prioritary vehicles exit ";
            for(size_t i = 0; i < m_LstVGPs.size(); i++)
            {
                m_pReseau->log() << m_LstVGPs[i].first->GetID() << ",";
            }
            m_pReseau->log() << std::endl;
        }
        // Une fois tous les véhicules prioritaires sortis de la zone, on passe en mode post-priorité
        if( m_LstVGPs.size() == 0 )
        {
            // Modification du mode de fonctionnement : si un VGP est sorti de la zone de contrôle, on passe en mode 'post-priorité'
            m_cModeFct = 'Q';

            // Mise en place du plan de feux 'post-priorité'
            if(m_pPDFEx)
                m_pPDFEx.reset();
 
            // Construction du couple de séquence pour redevenir à la normal

            // Recherche de la séquence de retour 2 dans la suite des séquences normales            
            double dbDureeRetour; // Durée du retour à la normal
            double dbVertSeq1, dbVertSeq2;
            bool bPrioritaire;
            STimeSpan tsTmp;
            double dbTps1, dbTps2;
            CDFSequence *pSeq = GetSequence( dbInstant, dbTps1, dbTps2, bPrioritaire);
            dbDureeRetour = pSeq->GetTotalLength() - dbTps2;
            pSeq = GetTrafficLightCycle(dbInstant, tsTmp)->GetNextSequence(pSeq);
            dbDureeRetour += pSeq->GetTotalLength();            

            while(pSeq != m_pSeqRetour2)
            {
                pSeq = GetTrafficLightCycle(dbInstant, tsTmp)->GetNextSequence(pSeq);                
                dbDureeRetour += pSeq->GetTotalLength();
            }

            double dbDureeSeqRet = m_pSeqRetour1->GetTotalLength() + m_pSeqRetour2->GetTotalLength();
         
            // Dilation ou réduction du couple de séquence de retour
            m_pPDFEx.reset(new PlanDeFeux);
            CDFSequence *pSeq1 = new CDFSequence();
            pSeq1->Copy( m_pSeqRetour1 );
            pSeq1->SetTotalLength( m_pSeqRetour1->GetTotalLength() * dbDureeRetour / dbDureeSeqRet );

            for(int nSA = 0; nSA < (int)pSeq1->GetLstActiveSignals().size(); nSA++)
            {
                SignalActif *pSA = pSeq1->GetLstActiveSignals()[nSA];
                dbVertSeq1 = pSA->GetGreenDuration() + ( pSeq1->GetTotalLength() - m_pSeqRetour1->GetTotalLength());
                pSA->SetGreenDuration(dbVertSeq1);
            }
            m_pPDFEx->AddSequence( pSeq1 );

            CDFSequence *pSeq2 = new CDFSequence();
            pSeq2->Copy( m_pSeqRetour2 );
            pSeq2->SetTotalLength( m_pSeqRetour2->GetTotalLength() * dbDureeRetour / dbDureeSeqRet );

            for(int nSA = 0; nSA<(int)pSeq2->GetLstActiveSignals().size(); nSA++)
            {
                SignalActif *pSA = pSeq2->GetLstActiveSignals()[nSA];
                dbVertSeq2 = pSA->GetGreenDuration() + ( pSeq2->GetTotalLength() - m_pSeqRetour2->GetTotalLength());
                pSA->SetGreenDuration( dbVertSeq2 );
            }
            m_pPDFEx->AddSequence( pSeq2 );

            // Si réduction, la durée de vert min. est elle respectée ?
            if(dbDureeRetour <  dbDureeSeqRet)
            {
                if( pSeq1->GetMinDureeVert() < m_dbDureeVertMin || pSeq2->GetMinDureeVert() < m_dbDureeVertMin )
                {
                    // Ajout d'une 3ième séquence
                    CDFSequence *pSeqRetour3 = GetTrafficLightCycle(dbInstant, tsTmp)->GetNextSequence(m_pSeqRetour2);
                    CDFSequence *pSeq3 = new CDFSequence();
                    pSeq3->Copy( pSeqRetour3 );

                    dbDureeRetour = pSeq3->GetTotalLength();

                    if( pSeq1->GetMinDureeVert() < m_dbDureeVertMin )
                    {
                        pSeq1->SetTotalLength( pSeq1->GetTotalLength() + m_dbDureeVertMin - pSeq1->GetMinDureeVert() );
                        for(int nSA = 0; nSA < (int)pSeq1->GetLstActiveSignals().size(); nSA++)
                        {
                            SignalActif *pSA = pSeq1->GetLstActiveSignals()[nSA];                           
                            pSA->SetGreenDuration( std::max<double>(m_dbDureeVertMin, pSA->GetGreenDuration()) );
                        } 
                        dbDureeRetour += pSeq1->GetTotalLength();
                    }
                    else
                        dbDureeRetour += m_pSeqRetour1->GetTotalLength();

                    if( pSeq2->GetMinDureeVert() < m_dbDureeVertMin )
                    {
                        pSeq2->SetTotalLength( pSeq2->GetTotalLength() + m_dbDureeVertMin - pSeq2->GetMinDureeVert() );
                        for(int nSA = 0; nSA < (int)pSeq2->GetLstActiveSignals().size(); nSA++)
                        {
                            SignalActif *pSA = pSeq2->GetLstActiveSignals()[nSA];                           
                            pSA->SetGreenDuration( std::max<double>(m_dbDureeVertMin, pSA->GetGreenDuration() ) );
                        }
                        dbDureeRetour += pSeq2->GetTotalLength();
                    }
                    else
                        dbDureeRetour += m_pSeqRetour2->GetTotalLength();
                    
                    dbDureeSeqRet = m_pSeqRetour1->GetTotalLength() - dbTps2 + m_pSeqRetour2->GetTotalLength() + pSeqRetour3->GetTotalLength();

                    m_pPDFEx->RemoveAllSequences();
                   
                    if( pSeq1->GetMinDureeVert() >= m_dbDureeVertMin )
                    {
                        pSeq1->SetTotalLength( m_pSeqRetour1->GetTotalLength() * dbDureeRetour / dbDureeSeqRet );
                        for(int nSA = 0; nSA < (int)pSeq1->GetLstActiveSignals().size(); nSA++)
                        {
                            SignalActif *pSA = pSeq1->GetLstActiveSignals()[nSA];
                            dbVertSeq1 = pSA->GetGreenDuration() + ( pSeq1->GetTotalLength() - m_pSeqRetour1->GetTotalLength());
                            pSA->SetGreenDuration(dbVertSeq1);
                        }
                    }
                    m_pPDFEx->AddSequence( pSeq1 );

                    if( pSeq2->GetMinDureeVert() >= m_dbDureeVertMin )
                    {
                        pSeq2->SetTotalLength( m_pSeqRetour2->GetTotalLength() * dbDureeRetour / dbDureeSeqRet );
                        for(int nSA = 0; nSA<(int)pSeq2->GetLstActiveSignals().size(); nSA++)
                        {
                            SignalActif *pSA = pSeq2->GetLstActiveSignals()[nSA];
                            dbVertSeq2 = pSA->GetGreenDuration() + ( pSeq2->GetTotalLength() - m_pSeqRetour2->GetTotalLength());
                            pSA->SetGreenDuration( dbVertSeq2 );
                        }                        
                    }
                    m_pPDFEx->AddSequence( pSeq2 );

                    pSeq3->SetTotalLength( pSeqRetour3->GetTotalLength() * dbDureeRetour / dbDureeSeqRet );
                    for(int nSA = 0; nSA<(int)pSeq3->GetLstActiveSignals().size(); nSA++)
                    {
                        SignalActif *pSA = pSeq3->GetLstActiveSignals()[nSA];                        
                        pSA->SetGreenDuration( pSA->GetGreenDuration() + ( pSeq3->GetTotalLength() - pSeqRetour3->GetTotalLength()) );
                    }
                    m_pPDFEx->AddSequence( pSeq3 );

                }
            }
            
            m_dbDebPDFEx = dbInstant;
            m_dbFinPDFEx = dbInstant + m_pPDFEx->GetCycleLength();

            bPDFChange = true;
        }
    }

    // Fonctionnement POST-PRIORITE
    if( m_cModeFct == 'Q' ) 
    {        
        // Test pour savoir si on sort du mode post-priorite
        if(dbInstant >= m_dbFinPDFEx)
        {
            m_cModeFct = 'N';

            m_pPDFEx.reset();

            m_dbFinPDFEx = 0;

            m_LstVGPs.clear();

            bPDFChange = true;
        }
    }

    if (bPDFChange)
    {
        // Gestion pour le mode méso de l'évènement de sortie du mdoe prioritaire du CDF :
        // les arrivals sur les tuyaux amont du noeud dont l'instant correspond à la fin de la simu
        // correspondent à des arrivées au moment du cycle de feu prioritaire de durée indéterminée.
        // on recalcule donc les instants d'arrivées en prenant l'instant de vert suivant qu'on peut à présent calculer :

        // récupération des noeuds liés à ce CDF :
        for (size_t iNode = 0; iNode < m_pReseau->m_LstUserCnxs.size(); iNode++)
        {
            Connexion * pNode = m_pReseau->m_LstUserCnxs[iNode];
            if (pNode->GetCtrlDeFeux() == this)
            {
                for (size_t iUpstreamLink = 0; iUpstreamLink < pNode->m_LstTuyAssAm.size(); iUpstreamLink++)
                {
                    Tuyau * pUpstreamLink = pNode->m_LstTuyAssAm[iUpstreamLink];

                    std::pair<double, Vehicule*> minArrival(DBL_MAX, nullptr);

                    std::list<std::pair<double, Vehicule*> > arrivals = pNode->GetArrivalTimes(pUpstreamLink);
                    for (std::list<std::pair<double, Vehicule*> >::iterator iterArrival = arrivals.begin(); iterArrival != arrivals.end(); ++iterArrival)
                    {
                        if (iterArrival->first >= m_pReseau->GetDureeSimu())
                        {
                            Tuyau * pOutgoingLink = iterArrival->second->GetNextTuyau();
                            assert(pOutgoingLink != NULL); // Si ca arrive, prévoir un recalcul du tuyau suivant ici...

                            double dbNextGreen = GetInstantVertSuivant(dbInstant, m_pReseau->GetDureeSimu(), pUpstreamLink, pOutgoingLink);

                            // rmq. : petit écart au modèle méso ici le next supply time du tuyau aval...
                            iterArrival->first = dbNextGreen;

                            if (iterArrival->first < minArrival.first)
                            {
                                minArrival = *iterArrival;
                            }
                        }
                    }

                    pNode->ReplaceArrivalTimes(pUpstreamLink, arrivals);

                    // dans le cas où le plus petit arrivalTime est inférieur au next arrival time courant, on remplace le next arrival par le minArrival
                    if (minArrival.first < pNode->GetNextArrival(pUpstreamLink).first)
                    {
                        m_pReseau->ChangeMesoNodeEventTime(pNode, minArrival.first, pUpstreamLink);
                        pNode->SetNextArrival(pUpstreamLink, minArrival);
                    }
                }
            }
        }
    }

    UpdatePosition();

    if(m_pReseau->IsVGPDebug()) {
        if( m_cModeFct == 'Q' ) 
            m_pReseau->log() << " Traffic light contoller " << m_strID << " in POST-PRIORITY mode" << std::endl;
        else if ( m_cModeFct == 'N' ) 
            m_pReseau->log() << " Traffic light contoller " << m_strID << " in NORMAL mode" << std::endl;
        else if ( m_cModeFct == 'O' ) 
            m_pReseau->log() << " Traffic light contoller " << m_strID << " in PRE-PRIORITY mode" << std::endl;
        else if ( m_cModeFct == 'P' ) 
            m_pReseau->log() << " Traffic light contoller " << m_strID << " in PRIORITY mode" << std::endl;
    }
}

//================================================================
    int  ControleurDeFeux::SendSignalPlan
//----------------------------------------------------------------
// Fonction  : Force le plan de feux actif
// Version du: 20/04/2009
// Historique: 20/04/2009 (C.Bécarie - Tinea)
//================================================================
(   
	const std::string & sSP     // ID du plan de feux à activer
)
{
    PlanDeFeux *pPDF;

    pPDF = GetPlanDeFeuxFromID(sSP);
    if(!pPDF)
        return -3;  // Plan de feux non définie

    m_PDFActif = pPDF;

    STimeSpan   tsInst( (time_t)(m_pReseau->GetInstSim()*m_pReseau->GetTimeStep()) );
    m_PDFActif->SetHrDeb(m_pReseau->GetSimuStartTime()+tsInst );

    return 1;   // OK
}

bool isSameTuyauCDF(Tuyau * pTuyau, void * pArg)
{
    Tuyau * pTuyauRef = (Tuyau*)pArg;
    return pTuyauRef == pTuyau;
}

//Update the position of the static element
void ControleurDeFeux::UpdatePosition()
{
    //Point is the mean of all LigneDeFeux for this ControleurDeFeux
    Point ptLignePose;
	int nbLigneDeFeux = 0;
	m_ptPos.dbX = 0.0;
	m_ptPos.dbY = 0.0;
	m_ptPos.dbZ = 0.0;
    for(size_t j=0; j<GetLstCoupleEntreeSortie()->size(); j++)
    {
        CoupleEntreeSortie Couple = (*GetLstCoupleEntreeSortie())[j];
        Voie* pVoie;
        for(size_t k=0; k<Couple.pEntree->GetLstLanes().size(); k++)
        {
            pVoie = Couple.pEntree->GetLstLanes()[k];
			if (pVoie->GetLignesFeux().size() == 0)
			{
				nbLigneDeFeux++;
				CalcCoordFromPos(Couple.pEntree, Couple.pEntree->GetLength(), ptLignePose);

				m_ptPos.dbX += ptLignePose.dbX;
				m_ptPos.dbY += ptLignePose.dbY;
				m_ptPos.dbZ += ptLignePose.dbZ;
			}else{
				for (size_t l = 0; l < pVoie->GetLignesFeux().size(); l++)
				{
					LigneDeFeu ligne = pVoie->GetLignesFeux()[l];
					if (ligne.m_pControleur == this)
					{
						nbLigneDeFeux++;
						if (ligne.m_dbPosition < 0)
							CalcCoordFromPos(Couple.pEntree, Couple.pEntree->GetLength() + ligne.m_dbPosition, ptLignePose);
						else
							CalcCoordFromPos(Couple.pSortie, ligne.m_dbPosition, ptLignePose);

						m_ptPos.dbX += ptLignePose.dbX;
						m_ptPos.dbY += ptLignePose.dbY;
						m_ptPos.dbZ += ptLignePose.dbZ;
					}
				}
            }
        }
    }
    m_ptPos.dbX = m_ptPos.dbX / nbLigneDeFeux;
    m_ptPos.dbY = m_ptPos.dbY / nbLigneDeFeux;
    m_ptPos.dbZ = m_ptPos.dbZ / nbLigneDeFeux;
}

//================================================================
    void ControleurDeFeux::GetTuyauAmAv
//----------------------------------------------------------------
// Fonction  : Détermine les tuyaux amont et aval du CAF controlé
//             par lequel va arriver le véhicule guidé prioritaire
//             passé en paramètre
// Remarque  : Dans le cas où le CDF controle plusieurs
//             connexions, on fait l'hypothèse que celui qui
//             concerne le calcul actuel est le prochain qui est
//             traversé par le véhicule guidé prioritaire
// Version du: 09/09/2011
// Historique: 09/09/2011 (O.Tonck - IPSIS)
//================================================================
(   
	Vehicule * pBus,         // véhicule en approche des feux
    Tuyau ** pTuyAmont,
    Tuyau ** pTuyAval
)
{
    *pTuyAmont = NULL;
    *pTuyAval = NULL;

    // rmq. problème potentiel ici si tuyau(0) et tuyau(1) sont tous les deux internes. regarder les tuyaux parents des voies parcourues si c'est le cas ?
    int tuyIdx = pBus->GetItineraireIndex(isSameTuyauCDF, pBus->GetLink(1) != NULL && !pBus->GetLink(1)->GetBriqueParente() ? pBus->GetLink(1) : pBus->GetLink(0));
    if(tuyIdx != -1)
    {
        // on se positionne sur le tuyau actuel
        size_t currentTuyauIdx = (size_t)tuyIdx;

        // tant que la connexion aval du tuyau n'est pas controlée par this,
        // on avance dans l'itinéraire
        std::vector<Tuyau*> * pLstTuyaux =  pBus->GetItineraire();
        for(;currentTuyauIdx<pLstTuyaux->size() && !(*pTuyAmont);currentTuyauIdx++)
        {
            if(pLstTuyaux->at(currentTuyauIdx)->GetCnxAssAv()->GetCtrlDeFeux() == this)
            {
                *pTuyAmont = pLstTuyaux->at(currentTuyauIdx);
                if(currentTuyauIdx<pLstTuyaux->size()-1) {
                    *pTuyAval = pLstTuyaux->at(currentTuyauIdx+1);
                }
            }
        }
    }    
}

double ControleurDeFeux::GetInstantVertSuivant(double dRefTime, double dInstantMax, Tuyau * pTuyauAmont, Tuyau * pTuyauAval )
{
    double dTpsPrevSeq;
    double dTpsCurSeq;
    bool bPrioritaire;
    CDFSequence *pSequence = NULL;
   
    double dInstantVert= 0;

    double dInstantSequence = dRefTime;

    // rmq. : en cas de ligne guidée prioritaire et en mode meso, on ne détecte pas avec cette méthode le prochain instant de vert... du coup
    // on recalcule les prochains instants d'arrivée meso en fin de méthode ::Update lors des modifications dynamique des plans de feux à cause
    // des lignes guidées prioritaires.

    std::set<CDFSequence*> lstSequences;
    bool bAbort = false;

    do
    {
        pSequence = GetSequence(dInstantSequence, dTpsPrevSeq, dTpsCurSeq, bPrioritaire);

        SignalActif *pSignal= pSequence->GetSignalActif( pTuyauAmont, pTuyauAval);
        if( pSignal && pSignal->GetActivationDelay()+dInstantSequence> dRefTime )
        {
            dInstantVert = pSignal->GetActivationDelay()  +dInstantSequence ;
        }
      
        // passage à la séquence suivante
        dInstantSequence += pSequence->GetTotalLength() - dTpsCurSeq;

        // Pour ne pas boucler jusqu'à la fin de la simulation si un feu est toujours rouge par exemple
        if(!pSignal)
        {
            if(lstSequences.find(pSequence) != lstSequences.end())
            {
                bAbort = true;
                break;
            }
            lstSequences.insert(pSequence);
        }

	} while (dInstantVert - dTpsCurSeq<  dRefTime &&
        dInstantSequence<dInstantMax); // evite les boucles infinies
       
      if( bAbort || dInstantSequence>= dInstantMax)
      {
          return dInstantMax;
      }
      else
      {
         return dInstantVert - dTpsCurSeq;
      }
}

double ControleurDeFeux::GetInstantDebutVert(double dRefTime, Tuyau *pTuyauAmont, Tuyau * pTuyauAval)
{
    double dbInstantDebutVert;

    double dTpsPrevSeq;
    double dTpsCurSeq;
    bool bPrioritaire;
    CDFSequence *pSequence = GetSequence(dRefTime, dTpsPrevSeq, dTpsCurSeq, bPrioritaire);
    dbInstantDebutVert = (int)dRefTime - dTpsCurSeq; // rmq. : on tronque dRefTime car dTpsCurSeq est tronqué dans GetSequence ce qui produit un résultat incohérent dans certains cas

    SignalActif *pSignal = pSequence->GetSignalActif(pTuyauAmont, pTuyauAval);

    assert(pSignal); // Si on est là c'est que le feu est vert : on doit avoir le signal ...

    dbInstantDebutVert += pSignal->GetActivationDelay();

    return dbInstantDebutVert;
}

//! Estime le temps d'attente moyen théorique pour un tronçon amont donné, en ne tenant compte que des plans de feux définis dans le scénario initial.
double ControleurDeFeux::GetMeanWaitTime(Tuyau *pTuyauAmont)
{
    double dbMeanWaitTime;
    std::map<Tuyau*, double>::const_iterator iter = m_MeanWaitTimes.find(pTuyauAmont);
    if(iter != m_MeanWaitTimes.end())
    {
        dbMeanWaitTime = iter->second;
    }
    else
    {
        dbMeanWaitTime = 0;
        PlanDeFeux * pPDF = GetLstPlanDeFeuxInit()->GetFirstVariationEx();
        SignalActif * pSignal;
        if(pPDF)
        {
            // Pour chaque tronçon aval possible, calcul du temps d'attente moyen, puis on moyenne les temps d'attente pour sa ramener à une unique
            // valeur pour le tronçon amont
            std::map<Tuyau*, double> mapDownstreamLinks;
            for (size_t i = 0; i < pPDF->GetLstSequences().size(); i++)
            {
                CDFSequence * pSeq = pPDF->GetLstSequences()[i];
                for (size_t j = 0; j < pSeq->GetLstActiveSignals().size(); j++)
                {
                    pSignal = pSeq->GetLstActiveSignals()[j];
                    if (pSignal->GetInputLink() == pTuyauAmont)
                    {
                        mapDownstreamLinks[pSignal->GetOutputLink()] = 0;
                    }
                }
            }

            for (std::map<Tuyau*, double>::iterator iterDownstreamLink = mapDownstreamLinks.begin(); iterDownstreamLink != mapDownstreamLinks.end(); ++iterDownstreamLink)
            {
                // On détermine les plages de feux non verts pour le mouvement considéré (en fusionnant les durées de rouge le cas échéant).
                // Pas la peine de s'occuper des moments ou le feu est vert.
                // On se cale pour notre t0 sur une début de feu vert pour ne pas avoir à gérer le bouclage du cycle.
                std::vector<double> redPeriods;
                double dbCurrentRedLength = 0.0;
                double dbDelayBeforeFirstGreen = 0.0;
                bool bFirstGreen = false;
                // Parcours des séquences du plan de feux.
                for (size_t i = 0; i < pPDF->GetLstSequences().size(); i++)
                {
                    CDFSequence * pSeq = pPDF->GetLstSequences()[i];
                    pSignal = pSeq->GetSignalActif(pTuyauAmont, iterDownstreamLink->first);
                    if (!pSignal || pSignal->GetGreenDuration() <= 0.0)
                    {
                        // le feu est rouge sur toute la durée de la séquence
                        if (!bFirstGreen)
                        {
                            dbDelayBeforeFirstGreen += pSeq->GetTotalLength();
                        }
                        else
                        {
                            dbCurrentRedLength += pSeq->GetTotalLength();
                        }
                    }
                    else
                    {
                        // le feu passe au vert au moins une partie de la séquence
                        if (!bFirstGreen)
                        {
                            dbDelayBeforeFirstGreen += pSignal->GetActivationDelay();
                            bFirstGreen = true;

                            dbCurrentRedLength = pSeq->GetTotalLength() - pSignal->GetActivationDelay() - pSignal->GetGreenDuration();
                        }
                        else
                        {
                            redPeriods.push_back(dbCurrentRedLength + pSignal->GetActivationDelay());

                            // réinitialisation pour la prochaine séquence de rouge
                            dbCurrentRedLength = pSeq->GetTotalLength() - pSignal->GetActivationDelay() - pSignal->GetGreenDuration();
                        }
                    }
                }
                // On ajoute la dernière séquence
                if (dbCurrentRedLength + dbDelayBeforeFirstGreen > 0)
                {
                    redPeriods.push_back(dbCurrentRedLength + dbDelayBeforeFirstGreen);
                }

                for (size_t i = 0; i < redPeriods.size(); i++)
                {
                    iterDownstreamLink->second += redPeriods[i] * redPeriods[i];
                }

                // On divise par la durée totale sur 2
                double dbTotalLength = pPDF->GetCycleLength();
                if (dbTotalLength > 0)
                {
                    iterDownstreamLink->second /= 2.0*dbTotalLength;
                }
                else
                {
                    // On ne sait pas faire le calcul dans ce cas fantaisiste
                    iterDownstreamLink->second = 0;
                }
            } // Fin boucle sur les tronçon aval

            for (std::map<Tuyau*, double>::iterator iterDownstreamLink = mapDownstreamLinks.begin(); iterDownstreamLink != mapDownstreamLinks.end(); ++iterDownstreamLink)
            {
                dbMeanWaitTime += iterDownstreamLink->second;
            }
            if (mapDownstreamLinks.size() > 0)
            {
                dbMeanWaitTime /= mapDownstreamLinks.size();
            }
        }

        m_MeanWaitTimes[pTuyauAmont] = dbMeanWaitTime;
    }
    return dbMeanWaitTime;
}

// *********************
// *** PlanDeFeux
// ******************
PlanDeFeux::PlanDeFeux()
{
    m_dbDureeCycle = 0;
}

PlanDeFeux::PlanDeFeux(char *sID, STime tHrDeb)
{
    m_strID = sID;
    m_tHrDeb = tHrDeb;

    m_dbDureeCycle = 0;
}

PlanDeFeux::~PlanDeFeux()
{
    for(size_t i=0; i<m_LstSequence.size(); i++)
    {
        delete m_LstSequence[i];
    }
    m_LstSequence.erase(m_LstSequence.begin(), m_LstSequence.end());    
}

void PlanDeFeux::CopyTo(PlanDeFeux * pTarget)
{
    for(size_t i=0; i<m_LstSequence.size(); i++)
    {
        CDFSequence * pNewSeq = new CDFSequence();
        pNewSeq->Copy(m_LstSequence[i]);
        pTarget->AddSequence(pNewSeq);
    }
}

CDFSequence* PlanDeFeux::GetSequence
(
    double dbInstCycle,     // Instant considéré par rapport au début du cycle courant
    double &dbTpsPrevSeqs,  // Temps passé dans les séquenes précédentes du cycle courant
    double &dbTpsCurSeq     // Temps passé dans la séquence courante
)
{
    dbTpsPrevSeqs = 0;

    for(size_t i=0; i< m_LstSequence.size(); i++)
    {                
        if( dbTpsPrevSeqs + m_LstSequence[i]->GetTotalLength() > dbInstCycle)
        {
            dbTpsCurSeq = dbInstCycle - dbTpsPrevSeqs;
            return m_LstSequence[i];        
        }
        dbTpsPrevSeqs += m_LstSequence[i]->GetTotalLength();
    }
    return NULL;
}

// Retourne la somme des durées cumulées des nNum première séquences
double PlanDeFeux::GetDureeCumSeq
(
    int nNum
)
{
    if( nNum >= (int)m_LstSequence.size() )
        return m_dbDureeCycle;

    double dbCum = 0;
    for(int i=0; i<=nNum; i++)
        dbCum += m_LstSequence[i]->GetTotalLength();

    return dbCum;
}

// Replace all signal plans by a new signal stored in Json string
bool ControleurDeFeux::ReplaceSignalPlan(std::string strJsonSignalPlan)
{
	// decode JSON signal plan input
	rapidjson::Document document;
	document.Parse(strJsonSignalPlan.c_str());

	if (!document.IsObject())
		return false;

	double dbDuration;

	STime dtDebut;
	SDateTime dt;
	dtDebut = dt.ToTime();

	PlanDeFeux *pPlanDeFeux = new PlanDeFeux("NewFromJSON", dtDebut);
	bool bOK = true;

	rapidjson::Value sequences = document["sequences"].GetArray();
	for (rapidjson::SizeType iSq = 0; iSq < sequences.Size(); iSq++)
	{
		dbDuration = sequences[iSq]["duration"].GetDouble();
		rapidjson::Value signals = sequences[iSq]["signals"].GetArray();

		CDFSequence *pSequence = new CDFSequence(dbDuration, (int)iSq);

		for (rapidjson::SizeType iSg = 0; iSg < signals.Size(); iSg++)
		{
			std::string sInputID = signals[iSg]["input_id"].GetString();
			Tuyau* pInput = m_pReseau->GetLinkFromLabel(sInputID);
			if (!pInput)
			{
				bOK = false;
				continue;
			}

			std::string sOutputID = signals[iSg]["output_id"].GetString();
			Tuyau* pOutput = m_pReseau->GetLinkFromLabel(sOutputID);
			if (!pOutput)
			{
				bOK = false;
				continue;
			}

			double dbGreenDuration = signals[iSg]["green_duration"].GetDouble();
			double dbDelayActivation = signals[iSg]["delay_activation"].GetDouble();

			AddCoupleEntreeSortie(pInput, pOutput);
			SignalActif *pSignal = new SignalActif(pInput, pOutput, dbGreenDuration, 0.0, dbDelayActivation);

			pSequence->AddActiveSignal(pSignal);
		}
		pPlanDeFeux->AddSequence(pSequence);
	}

	if (!bOK)
	{
		delete pPlanDeFeux;
		return false;
	}

	// TO DO: problem with m_pLstPlanDeFeuxInit !
	//for (int i = 0; i < (int)m_pLstPlanDeFeux->GetLstTV()->size(); i++)
		//delete (m_pLstPlanDeFeux->GetVariation(i));

	m_pLstPlanDeFeux->GetLstTV()->clear();
	m_pLstPlanDeFeux->AddVariation(dtDebut, pPlanDeFeux);
	
	return true;
}

CDFSequence* PlanDeFeux::GetNextSequence(CDFSequence *pSeq)
{
    for(int i=0; i<(int)m_LstSequence.size(); i++)
    {
        if( pSeq == m_LstSequence[i] )
        {
            if( i+1 < (int)m_LstSequence.size() )
                return m_LstSequence[i+1];
            else
                return m_LstSequence[0];
        }
    }

    return NULL;
}

std::string PlanDeFeux::GetJsonDescription()
{
	rapidjson::Document JsonSignalPlan(rapidjson::kObjectType);
	rapidjson::Document::AllocatorType& allocator = JsonSignalPlan.GetAllocator();

	rapidjson::Value sequences(rapidjson::kArrayType);

	std::vector<CDFSequence*>::iterator itSeq;
	for( itSeq=m_LstSequence.begin();itSeq!= m_LstSequence.end();itSeq++)
	{
		rapidjson::Value sequence(rapidjson::kObjectType);
		sequence.AddMember("duration", (*itSeq)->GetTotalLength(), allocator);
		rapidjson::Value signals(rapidjson::kArrayType);

		std::vector<SignalActif*>::iterator itSA;
		for (itSA = (*itSeq)->m_LstSignActif.begin(); itSA != (*itSeq)->m_LstSignActif.end(); itSA++)
		{
			rapidjson::Value signal(rapidjson::kObjectType);

			rapidjson::Value strValue(rapidjson::kStringType);
			strValue.SetString((*itSA)->GetInputLink()->GetLabel().c_str(),allocator);
			signal.AddMember("input_id", strValue, allocator);

			strValue.SetString((*itSA)->GetOutputLink()->GetLabel().c_str(), allocator);
			signal.AddMember("output_id", strValue, allocator);

			signal.AddMember("green_duration", (*itSA)->GetGreenDuration(), allocator);
			signal.AddMember("delay_activation", (*itSA)->GetActivationDelay(), allocator);

			signals.PushBack(signal, allocator);
		}
		sequence.AddMember("signals", signals, allocator);
		sequences.PushBack(sequence, allocator);
	}

	JsonSignalPlan.AddMember("sequences", sequences, allocator);

	rapidjson::StringBuffer buffer;
	buffer.Clear();
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	JsonSignalPlan.Accept(writer);

	std::cout << buffer.GetString() << std::endl;

	return buffer.GetString();
}

CDFSequence::CDFSequence(double dbDureeTotale, int nNum)
{
    m_dbDureeTotale = dbDureeTotale;
    m_nNum = nNum;
}

CDFSequence::~CDFSequence()
{
    for(int i=0; i<(int)m_LstSignActif.size(); i++)
    {
        delete m_LstSignActif[i];
    }
    m_LstSignActif.erase(m_LstSignActif.begin(), m_LstSignActif.end());    
}

// Copie d'une séquence
void  CDFSequence::Copy(CDFSequence *pSeqSrc)
{
    SignalActif *pSigDst;

    m_dbDureeTotale = pSeqSrc->m_dbDureeTotale;
    m_nNum = pSeqSrc->m_nNum;

    for(size_t i=0; i< pSeqSrc->m_LstSignActif.size(); i++)
    {
        SignalActif *pSigSrc = pSeqSrc->m_LstSignActif[i];       

        pSigDst = new SignalActif( pSigSrc->GetInputLink(), pSigSrc->GetOutputLink(), pSigSrc->GetGreenDuration(), pSigSrc->GetOrangeDuration(), pSigSrc->GetActivationDelay());      

        AddActiveSignal(pSigDst);
    }
}

// Retourne la durée minimum d'une séquence afin que pour chaque signal l'allumage et un temps de vert supérieur à la durée de vert minimum soit respecté
double CDFSequence::GetMinDuree(double dbDureeVertMin)
{
    double dbDureeMin;

    dbDureeMin = m_LstSignActif[0]->GetActivationDelay() + dbDureeVertMin;
    for(int i=1; i<(int)m_LstSignActif.size();i++)
    {
        dbDureeMin = std::max<double>(dbDureeMin, m_LstSignActif[i]->GetActivationDelay() + dbDureeVertMin);
    }
    return dbDureeMin;
}

// Retourne la plus petite durée de vert sur l'ensemble des signaux actifs
double CDFSequence::GetMinDureeVert()
{
    double dbDureeMin;

    dbDureeMin = DBL_MAX;
    for(int i=0; i<(int)m_LstSignActif.size();i++)
    {
        dbDureeMin = std::min<double>(dbDureeMin, m_LstSignActif[i]->GetGreenDuration() );
    }
    return dbDureeMin;
}

double CDFSequence::GetInstMaxFeuxVerts()
{
    int nNbVert = 0;   
    int nNbVertTmp = 0;
    double dbInst = m_dbDureeTotale;
    
    for(int nI = 0; nI < (int)m_dbDureeTotale; nI++)
    {
        nNbVertTmp = 0;
        for(int i=0; i<(int)m_LstSignActif.size(); i++)
        {
            if( m_LstSignActif[i]->GetActivationDelay() > nI
                && nI <= m_LstSignActif[i]->GetActivationDelay() +
                m_LstSignActif[i]->GetGreenDuration() )
                nNbVertTmp++;
        }
        if(nNbVertTmp > nNbVert)
        {
            nNbVert = nNbVertTmp;
            dbInst = (double)nI;
        }
    }
    return dbInst;
}

double CDFSequence::GetInstantVertSuivant(double dInstant, Tuyau *pTuyauAmont, Tuyau * pTuyauAval )
{
    int nNbVert = (int)m_LstSignActif.size();
    int nNbVertTmp = 0;
    double dbInstVert =dInstant==0?0:m_dbDureeTotale;
    SignalActif * pSignalActif = GetSignalActif(pTuyauAmont, pTuyauAval );
    
    for(int nI = (int)floor(dInstant); nI < (int)m_dbDureeTotale; nI++)
    {
        nNbVertTmp = 0;
        for(int i=0; i<(int)m_LstSignActif.size(); i++)
        {
            if( m_LstSignActif[i]->GetActivationDelay() > nI && nI <= m_LstSignActif[i]->GetActivationDelay() + m_LstSignActif[i]->GetGreenDuration() )
            {
                nNbVertTmp = std::min<int>( nNbVertTmp, nI);
            }
        }
        if(nNbVertTmp < nNbVert &&   // inférieur à la valeur stocké
             dInstant< nI) // sup à l'intanst voulu
        {
            nNbVert = nNbVertTmp;
            dbInstVert = (double)nI;
        }
    }
    return dbInstVert;
}

SignalActif* CDFSequence::GetSignalActif(Tuyau *pTE, Tuyau *pTS)
{
    SignalActif * pSignal;
    for(size_t i=0; i<m_LstSignActif.size(); i++)
    {
        pSignal = m_LstSignActif[i];
        if( pSignal->GetInputLink() == pTE && pSignal->GetOutputLink () == pTS)
            return pSignal;
    }
    return NULL;
}

// Constructeur par défaut
SignalActif::SignalActif()
{
}

// associé à un couple d'entrée/sortie
SignalActif::SignalActif(Tuyau *pTE, Tuyau *pTS, double dbDureeVert, double dbDureeOrange, double dbRetardAllumage)
{
    m_pTuyEntree = pTE;
    m_pTuySortie = pTS;
    m_dbDureeVert = dbDureeVert;
    m_dbDureeRetardAllumage = dbRetardAllumage;
	m_dbDureeOrange = dbDureeOrange;
}

eCouleurFeux SignalActif::GetCouleurFeux(double dbInstant)
{
    if( dbInstant < m_dbDureeRetardAllumage)
		return FEU_ROUGE;

    if( dbInstant < m_dbDureeRetardAllumage + m_dbDureeVert)
	{
		return FEU_VERT;
	}
	else
	{
		if( 
			( m_dbDureeRetardAllumage + m_dbDureeVert <= dbInstant )
			&&
			( dbInstant < m_dbDureeRetardAllumage + m_dbDureeVert + m_dbDureeOrange ) 
		  )
		{
			return FEU_ORANGE;
		}
		else
		{
			return FEU_ROUGE;
		}
	}
}

// Calcul la proportion de pas de temps pour laquelle le feu a été vert ou orange
double  SignalActif::GetRapVertOrange(double dbInstant, double dbPasTemps, bool bVertOuOrange)
{    
    if(bVertOuOrange)
    {
        return std::min<double>(1,(dbInstant - m_dbDureeRetardAllumage) / dbPasTemps);
    }
    else
    {
        if(dbInstant < m_dbDureeRetardAllumage)
            return 1 - std::min<double>(1, dbInstant / dbPasTemps) ;
        else
            return 1 - std::min<double>(1, ( dbInstant - (m_dbDureeRetardAllumage + m_dbDureeVert + m_dbDureeOrange) / dbPasTemps));
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void CoupleEntreeSortie::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CoupleEntreeSortie::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CoupleEntreeSortie::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(pEntree);
    ar & BOOST_SERIALIZATION_NVP(pSortie);
}

template void LGP::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void LGP::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void LGP::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(pTCptAm);
    ar & BOOST_SERIALIZATION_NVP(dbPosCptAm);
    ar & BOOST_SERIALIZATION_NVP(pTCptAv);
    ar & BOOST_SERIALIZATION_NVP(dbPosCptAv);
    ar & BOOST_SERIALIZATION_NVP(pTEntreeZone);
    ar & BOOST_SERIALIZATION_NVP(dbPosEntreeZone);
    ar & BOOST_SERIALIZATION_NVP(bPrioriteTotale);
    ar & BOOST_SERIALIZATION_NVP(nSeqPartagee);
    ar & BOOST_SERIALIZATION_NVP(LstLTG);
}

template void SignalActif::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void SignalActif::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void SignalActif::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_dbDureeRetardAllumage);
    ar & BOOST_SERIALIZATION_NVP(m_dbDureeVert);
    ar & BOOST_SERIALIZATION_NVP(m_dbDureeOrange);
    ar & BOOST_SERIALIZATION_NVP(m_pTuyEntree);
    ar & BOOST_SERIALIZATION_NVP(m_pTuySortie);
}

template void CDFSequence::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CDFSequence::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CDFSequence::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_dbDureeTotale);
    ar & BOOST_SERIALIZATION_NVP(m_LstSignActif);
    ar & BOOST_SERIALIZATION_NVP(m_nNum);
}

template void PlanDeFeux::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void PlanDeFeux::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void PlanDeFeux::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_tHrDeb);
    ar & BOOST_SERIALIZATION_NVP(m_strID);
    ar & BOOST_SERIALIZATION_NVP(m_dbDureeCycle);
    ar & BOOST_SERIALIZATION_NVP(m_LstSequence);
}

template void ControleurDeFeux::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ControleurDeFeux::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ControleurDeFeux::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(StaticElement);

	ar & BOOST_SERIALIZATION_NVP(m_strID);
	ar & BOOST_SERIALIZATION_NVP(m_pLstPlanDeFeux);
	ar & BOOST_SERIALIZATION_NVP(m_pLstPlanDeFeuxInit);
	ar & BOOST_SERIALIZATION_NVP(m_pReseau);

    ar & BOOST_SERIALIZATION_NVP(m_PDFActif);

    ar & BOOST_SERIALIZATION_NVP(m_dbDureeVertMin);
    ar & BOOST_SERIALIZATION_NVP(m_dbDureeVertMinInit);
    ar & BOOST_SERIALIZATION_NVP(m_dbDureeRougeDegagement);
    ar & BOOST_SERIALIZATION_NVP(m_dbDureeRougeDegagementInit);

    ar & BOOST_SERIALIZATION_NVP(m_LstCoupleEntreeSortie);
    ar & BOOST_SERIALIZATION_NVP(m_cModeFct);
    ar & BOOST_SERIALIZATION_NVP(m_LstLGP);
    ar & BOOST_SERIALIZATION_NVP(m_LstVGPs);

    ar & BOOST_SERIALIZATION_NVP(m_pPDFEx);
    ar & BOOST_SERIALIZATION_NVP(m_dbDebPDFEx);
    ar & BOOST_SERIALIZATION_NVP(m_dbFinPDFEx);

    ar & BOOST_SERIALIZATION_NVP(m_pSeqPrePriorite);
    ar & BOOST_SERIALIZATION_NVP(m_pSeqRetour1);
    ar & BOOST_SERIALIZATION_NVP(m_pSeqRetour2);
    ar & BOOST_SERIALIZATION_NVP(m_nTypeGestionPrePriorite);
}
