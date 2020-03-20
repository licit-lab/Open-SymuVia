#include "stdafx.h"
#include "AbstractCarFollowing.h"

#include "CarFollowingContext.h"
#include "NewellContext.h"
#include "../vehicule.h"
#include "../reseau.h"
#include "../DiagFonda.h"
#include "../voie.h"
#include "../tuyau.h"
#include "../RandManager.h"
#include "../MergingObject.h"
#include "../convergent.h"
#include "../divergent.h"
#include "../sensors/SensorsManager.h"
#include "../Logger.h"

#include <boost/serialization/shared_ptr.hpp>


AbstractCarFollowing::AbstractCarFollowing()
{
    m_pNetwork = NULL;
    m_pPrevContext = NULL;
    m_pContext = NULL;

    m_dbComputedTravelledDistance = 0;
    m_dbComputedTravelSpeed = 0;
    m_dbComputedTravelAcceleration = 0;
}

AbstractCarFollowing::~AbstractCarFollowing()
{
    if(m_pPrevContext)
    {
        delete m_pPrevContext;
    }
    if(m_pContext)
    {
        delete m_pContext;
    }
}

void AbstractCarFollowing::Initialize(Reseau * pNetwork, Vehicule * pVehicle)
{
    m_pNetwork = pNetwork;
    m_pVehicle = pVehicle;
}

double AbstractCarFollowing::GetVehicleLength()
{
    return 1/m_pVehicle->GetDiagFonda()->GetKMax() - m_pVehicle->GetType()->GetEspacementArret();
}

void AbstractCarFollowing::CalculAgressivite(double dbInstant)
{
    // Pas de procédure d'aggressivité particulière par défaut
    return;
}

bool AbstractCarFollowing::TestLaneChange(double dbInstant, boost::shared_ptr<Vehicule> pVehFollower, boost::shared_ptr<Vehicule> pVehLeader, boost::shared_ptr<Vehicule> pVehLeaderOrig,
    VoieMicro * pVoieCible, bool bForce, bool bRabattement, bool bInsertion)
{
    double      dbSf;
    double      dbSi;
    double      dbPhi;
    double      dbDemandeOrig;
    double      dbDemandeDest;
    double      dbOffreDest;
    double      dbPi;
    double      dbRand;
    double      dbVitVoieCible;
    double      dbVitVoieOrig;    

    dbSi = 0;
    if(m_pVehicle->GetVoie(1))
    {
        if(pVehLeaderOrig)
            dbSi = pVehLeaderOrig->GetPos(1) - m_pVehicle->GetPos(1);   // Espacement avec le véhicule leader de la meme voie
        else
        {
            if(bForce)
            {					
                dbSi = std::max<double>( 1 / m_pVehicle->GetDiagFonda()->GetKMax(), m_pVehicle->GetVoie(1)->GetLength() - m_pVehicle->GetPos(1) );					
            }
            else
                dbSi = 1 / m_pVehicle->GetDiagFonda()->GetKCritique();
        }
    }

    // Calcul de l'inter-distance entre le véhicule follower et le vehicule leader de la voie cible
    if(pVehFollower && pVehLeader)
    {
        dbSf = m_pNetwork->GetDistanceEntreVehicules(pVehLeader.get(), pVehFollower.get());
        //dbSf = dbSf / pVehFollower->GetDeltaN(); // Demande modif ludo du 22/06/10
    }
    else
    {        
        dbSf = 1/m_pVehicle->GetDiagFonda()->GetKCritique();
    }

    // Dans le cas d'un changement de voie pour rabattement, le rabattement est possible
    // sil les interdistances physiques après changement de voie sont respectées
    if( bRabattement )
    {
        double dbSf1 = 1/m_pVehicle->GetDiagFonda()->GetKCritique();
        double dbSf2 = 1/m_pVehicle->GetDiagFonda()->GetKCritique();

        if( pVehLeader )
            dbSf1 = m_pNetwork->GetDistanceEntreVehicules(pVehLeader.get(), m_pVehicle);
        if( pVehFollower )
            dbSf2 = m_pNetwork->GetDistanceEntreVehicules(m_pVehicle, pVehFollower.get());

        if( pVehLeader && dbSf1 < 1/pVehLeader->GetDiagFonda()->GetKCritique())
            return false;

        if ( dbSf2 < 1/m_pVehicle->GetDiagFonda()->GetKCritique())
            return false;
    }

    double dbVit, dbVitFollower;   
    dbVit = std::min<double>( m_pVehicle->GetVitRegTuyAct(), m_pVehicle->GetDiagFonda()->GetVitesseLibre() );

    // Demande de la voie origine
    if( !m_pVehicle->IsAtTripNode() )
    {        
        dbDemandeOrig = std::min<double>( dbVit / dbSi, m_pVehicle->GetDiagFonda()->GetDebitMax() );
    }
    else
        dbDemandeOrig = m_pVehicle->GetDiagFonda()->GetDebitMax();      // Cas des véhicules qui veulent s'insérer depuis un TripNode (arrêt de bus par ex. )

    // Changement de voie obligatoire
    if( m_pVehicle->GetVoie(1) )
    {
        if( !(!pVehLeaderOrig && m_pVehicle->GetVoie(1)->IsChgtVoieObligatoire(m_pVehicle->GetType())) )
        {            
            dbDemandeOrig = std::min<double>( dbVit / dbSi, m_pVehicle->GetDiagFonda()->GetDebitMax() );
        }
    }

    // Demande de la voie cible
    if(pVehFollower)
    {
        dbVitFollower = std::min<double>( pVehFollower->GetDiagFonda()->GetVitesseLibre(), pVehFollower->GetVitRegTuyAct() );
        dbDemandeDest = std::min<double>( pVehFollower->GetDiagFonda()->GetVitesseLibre() / dbSf, pVehFollower->GetDiagFonda()->GetDebitMax() );
    }
    else        
        dbDemandeDest = std::min<double>( dbVit / dbSf, m_pVehicle->GetDiagFonda()->GetDebitMax() );

    // Offre de la voie cible
    if(pVehLeader)
        dbOffreDest = std::min<double>( (pVehLeader->GetDiagFonda()->GetKMax() - (1/dbSf))*(-1)*pVehLeader->GetDiagFonda()->GetW(), pVehLeader->GetDiagFonda()->GetDebitMax() );
    else
        dbOffreDest = m_pVehicle->GetDiagFonda()->GetDebitMax();

    // Demande Ludo du 25/06/10
    //if(pVehFollower)
    //	dbOffreDest = pVehFollower->GetVit(1) / dbSf;

    // Calcul de la fonction Pi
    if( m_pVehicle->IsAtTripNode() || bForce )
    {
        dbPi = 1 / m_pNetwork->GetTimeStep();			// autre changement de voie forcée (bus qui quitte son arrêt, changement de voie pour prise de direction)	
        m_pVehicle->SetPi(true);
        m_pVehicle->SetPi(dbPi);
    }	
    else                                         
    {
        // Calcul de la vitesse associée à l'inter-distance entre le véhicule follower et leader de la voie cible
        if(dbSf>0)
        {
            double dbVal1 = 1/dbSf;
            double dbVal2 = VAL_NONDEF;

            if(pVehLeader)
                pVehLeader->GetDiagFonda()->CalculVitEqDiagrOrig(dbVitVoieCible, dbVal1, dbVal2, true); // Utilisation du diagramme fondamental du véhicule leader de la voie cible
            else
                m_pVehicle->GetDiagFonda()->CalculVitEqDiagrOrig(dbVitVoieCible, dbVal1, dbVal2, true); // Utilisation du diagramme fondamental du véhicule candidat au changement de voie

            if(pVehLeader)
            {                
                dbVitVoieCible = pVehLeader->GetVit(1);
            }                
        }
        else
            dbVitVoieCible = 0;

        dbVitVoieCible = std::min<double>( dbVitVoieCible, pVoieCible->GetVitReg() );

        if(!pVehFollower && pVehLeader)  // si un leader et pas de follower, il faut aussi prendre en compte la vitesse du leader
        {
            dbVitVoieCible = std::max<double>(dbVitVoieCible, pVehLeader->GetVit(1) );            
            dbVitVoieCible = std::min<double>(dbVitVoieCible, pVehLeader->GetVitRegTuyAct() );
        }   

        dbVitVoieCible = std::min<double>(dbVitVoieCible, m_pVehicle->GetVitMax() );

        // Calcul de la vitesse sur la voie originale
        dbVitVoieOrig = DBL_MAX;
        if(dbSi > 0)
        {
            double dbVal1 = 1/dbSi;
            double dbVal2 = VAL_NONDEF;

            if(pVehLeaderOrig)
                pVehLeaderOrig->GetDiagFonda()->CalculVitEqDiagrOrig(dbVitVoieOrig, dbVal1, dbVal2, true); // Utilisation du diagramme fondamental du véhicule candidat au changement de voie
            else
                m_pVehicle->GetDiagFonda()->CalculVitEqDiagrOrig(dbVitVoieOrig, dbVal1, dbVal2, true); // Utilisation du diagramme fondamental du véhicule candidat au changement de voie

            NewellContext * pNewellContext = dynamic_cast<NewellContext*>(m_pVehicle->GetCarFollowing()->GetCurrentContext());
            if(pNewellContext && pNewellContext->GetDeltaN()<1 && pVehLeaderOrig)
            {                
                dbVitVoieOrig = pVehLeaderOrig->GetVit(1);
            }                
        }
        else
        {            
            if(pVehLeaderOrig)
                dbVitVoieOrig = pVehLeaderOrig->GetVit(1) ;
            else
                dbVitVoieOrig = m_pVehicle->GetVit(1);
        }

        dbVitVoieOrig = std::min<double>( dbVitVoieOrig, m_pVehicle->GetVoie(1)->GetVitReg() );
        dbVitVoieOrig = std::min<double>( dbVitVoieOrig, m_pVehicle->GetVitMax() );

        if( bRabattement )						
        {		
            dbPi = m_pVehicle->GetType()->GetPiRabattement();
            m_pVehicle->SetPi(true);
            m_pVehicle->SetPi(dbPi);
            if( fabs(dbVitVoieOrig - dbVitVoieCible) > 0.00001 )		// Pas de rabattement si vitesse origine est différente de la voie cible
                return false; 
        }
        else
        {			  
            //dbPi = max(0., dbVitVoieCible - dbVitVoieOrig) / ( min( GetVitMax(), m_dbVitRegTuyAct )* GetType()->GetTauChgtVoie());        
            dbPi = std::max<double>(0., dbVitVoieCible - dbVitVoieOrig) / ( m_pVehicle->GetLink(1)->GetMaxVitReg(dbInstant, m_pVehicle->GetPos(1), m_pVehicle->GetVoie(1)->GetNum()) * m_pVehicle->GetType()->GetTauChgtVoie());     
            m_pVehicle->SetPi(true);
            m_pVehicle->SetPi(dbPi);

            if(dbPi < 0.000001)    // Suppression des erreurs d'arrondis
                return false;
        }
    }

    if (!bInsertion && bForce && m_pVehicle->GetLink(1)->GetLength() - m_pVehicle->GetPos(1) < m_pVehicle->GetLink(1)->GetDstChgtVoieForce())
    {
        dbPhi = m_pVehicle->GetLink(1)->GetPhiChgtVoieForce();
    }
    else
    {
        if (!bRabattement && (!pVehFollower || !pVehLeader))
            dbDemandeDest = 0;

        if (dbDemandeDest > 0)
            dbPhi = std::min<double>(1., dbOffreDest / dbDemandeDest) * dbPi * dbDemandeOrig / dbVit*pVoieCible->GetPasTemps() * dbSi;
        else
            dbPhi = dbPi * dbDemandeOrig / dbVit *m_pVehicle->GetVoie(1)->GetPasTemps() * dbSi;

        // Test d'une nouvelle forme pour calcul le S de la voie initiale dans l'équation de Phi
        double dbPhiTmp = 0;

        if (bForce)
        {
            if (m_pVehicle->GetVoie(1)->IsChgtVoieObligatoire(m_pVehicle->GetType()))
            {
                // rmq : pour les tests de validation des convergents on utilise le mode asynchrone et les merging objects : le code ci-dessous ne sert donc pas.
                // à nettoyer éventuellement après.
                // Calcul de Pi (Ludo 12/07/2010)
                //double Pi = m_pReseau->GetAlphaMandatory() / (1 + m_pReseau->GetAlphaMandatory()) * dbVit / ( m_pVoie[1]->GetLength() - m_pVoie[1]->GetPosLastVeh());
                // 19/07/2010 :
                double Pi = m_pNetwork->GetFAlphaMandatory() * dbVit / (std::max<double>(1 / m_pVehicle->GetDiagFonda()->GetKMax(), m_pVehicle->GetVoie(1)->GetLength() - m_pVehicle->GetVoie(1)->GetPosLastVeh()));

                // Calcul de Phi 						
                dbPhi = std::min<double>(1, dbOffreDest / dbDemandeDest) * dbDemandeOrig * Pi / dbVit;
                dbPhiTmp = dbPhi;

                // Calcul de la probabilité			

                // Distribuée
                if (m_pNetwork->GetChgtVoieMandatoryProba() == 'D')
                {
                    //dbSi = ( m_pVoie[1]->GetLength() - m_pVoie[1]->GetPosLastVeh()) / m_pVoie[1]->GetNbVeh();
                    dbPhi = dbPhi * m_pVehicle->GetVoie(1)->GetPasTemps() * dbSi;
                }

                // Uniforme
                if (m_pNetwork->GetChgtVoieMandatoryProba() == 'U')
                {
                    dbSi = (m_pVehicle->GetVoie(1)->GetLength() - m_pVehicle->GetVoie(1)->GetPosLastVeh()) / m_pVehicle->GetVoie(1)->GetNbVeh();
                    dbPhi = dbPhi * m_pVehicle->GetVoie(1)->GetPasTemps() * dbSi;
                }
            }
        }

        if (m_pNetwork->GetChgtVoieMandatoryMode() == 'A')
        {
            // EVOLUTION n°70 - activation de l'algorithme des merging objects également dans le cas des convergents d'insertion "échangeurs"
            if (m_pVehicle->GetVoie(1)->IsChgtVoieObligatoire(m_pVehicle->GetType()) || m_pVehicle->GetVoie(1)->IsChgtVoieObligatoireEch())
            {
                dbPhi = 0;
                // Recherche du merging object correspondant
                std::deque<MergingObject*>::iterator itMO;
                for (itMO = m_pNetwork->m_LstMergingObjects.begin(); itMO != m_pNetwork->m_LstMergingObjects.end(); itMO++)
                {
                    if ((*itMO)->GetTuyPrincipal() == (Tuyau*)m_pVehicle->GetVoie(1)->GetParent())
                    {
                        dbPhi = (*itMO)->GetProbaliteInsertion(m_pVehicle);
                        break;
                    }
                }
            }
        }
    }

    m_pVehicle->SetPhi(dbPhi);
    m_pVehicle->SetPhi(true);

    // -----------------------------------------------
    // Tirage
    // -----------------------------------------------

    dbRand = m_pNetwork->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER; 
    m_pVehicle->SetRand(dbRand);
    m_pVehicle->SetRand(true);

    return dbRand < dbPhi;
}

void AbstractCarFollowing::ApplyLaneChange(VoieMicro * pVoieOld, VoieMicro * pVoieCible, boost::shared_ptr<Vehicule> pVehLeader, boost::shared_ptr<Vehicule> pVehFollower,
    boost::shared_ptr<Vehicule> pVehLeaderOrig)
{
    return;
}

bool AbstractCarFollowing::TestConvergentInsertion(PointDeConvergence * pPointDeConvergence, Tuyau *pTAmP, VoieMicro* pVAmP, VoieMicro* pVAmNP, int nVoieGir, double dbInstant,
    int j, double dbDemande1, double dbDemande2, double dbOffre,
    boost::shared_ptr<Vehicule> pVehLeader, boost::shared_ptr<Vehicule> pVehFollower, double & dbTr, double & dbTa)
{
    boost::shared_ptr<Vehicule> pVehFollInt;

    bool bResult;

    if(!pPointDeConvergence->IsFreeFlow())    // Régime congestionné en aval
    {        
        double dbq2g = pPointDeConvergence->GetCvg()->m_dbGamma * dbOffre / (1+pPointDeConvergence->GetCvg()->m_dbGamma);

        if( dbDemande2 < dbq2g)
        {
            bResult = true;           // l'insertion est possible
            pPointDeConvergence->SetRandNumbers(0);             // remise à zéro du compteur de tirage
        }
        else
        {                    
            double dbProba = dbq2g * pPointDeConvergence->GetCvg()->GetReseau()->GetTimeStep();   // Probabilité 'insertion
                    
            bResult = false;  

            int number = m_pNetwork->GetRandManager()->myRand();
            double dbRand = (double)number / (double)MAXIMUM_RANDOM_NUMBER;                       

            for (int k=0; k<pPointDeConvergence->GetRandNumbers(); k++)                    
            {                      
                number = m_pNetwork->GetRandManager()->myRand();
                dbRand = (double)number / (double)MAXIMUM_RANDOM_NUMBER;                           
                pPointDeConvergence->SetRandNumbers(pPointDeConvergence->GetRandNumbers()-1);                       // Décrémentation du compteur des tirages

                if( dbRand < dbProba)
                {
                    bResult = true;       // Succès                       
                    break;
                }                        
            }                                        
        }
    }
    else                // Régime fluide en aval
    {
        pPointDeConvergence->SetRandNumbers(0);  // On est en régime fluide, // remise à zéro du compteur de tirage

        bool bCondAval;
        // Condition aval
        if( dbInstant - pPointDeConvergence->m_dbInstLastPassage >= std::max<double>( pPointDeConvergence->GetCvg()->GetTf(m_pVehicle->GetType()), 1 / m_pVehicle->GetDiagFonda()->GetDebitMax() ))
            bCondAval = true;
        else
        {
            bCondAval = false;     
            bResult = false;
        }

        // Condition amont
        if(bCondAval)
        {
            // 1 - Etude du follower : si il sort gène t'il le véhicule en attente
            if(pVehFollower)
            {

                // Recherche du lien de sortie précédent
                Tuyau           *pSortie;
                Voie            *pNextVoie;

                if( pTAmP->get_Type_amont() == 'D' )
                {
                    Divergent *pDvgt = (Divergent*)pTAmP->getConnectionAmont();
                    if(pDvgt && pDvgt->m_LstTuyAv.size()>1 )
                    {
                        pSortie = pDvgt->m_LstTuyAv.front();       
						if(pSortie==pTAmP  )
                            pSortie = pDvgt->m_LstTuyAv[1];

                        // Le follower est-il amené à sortir par ce tronçon ?

                        // Recherche de la voie précédente
                        pNextVoie = NULL;
                        for(int i=0; i<(int)pVehFollower->m_LstVoiesSvt.size(); i++)
                        {
                            if( pVehFollower->m_LstVoiesSvt[i] == pDvgt->m_LstTuyAm.front()->GetLstLanes()[0] )
                            {
                                if(i+1 < (int)pVehFollower->m_LstVoiesSvt.size())
                                    pNextVoie = pVehFollower->m_LstVoiesSvt[i+1];
                                else
                                {
                                    // Calcul de la voie suivante et MAJ de la liste des voies du véhicule                                            
                                    pNextVoie = pVehFollower->CalculNextVoie(pVehFollower->m_LstVoiesSvt[i],dbInstant);
                                    pVehFollower->m_LstVoiesSvt.push_back( (VoieMicro*)pNextVoie);
                                }
                                break;
                            }
                        }
                        if(!pNextVoie)
                        {                                              
                            pNextVoie = pVehFollower->CalculNextVoie(pDvgt->m_LstTuyAm.front()->GetLstLanes()[0],dbInstant);
                            pVehFollower->m_LstVoiesSvt.push_back( (VoieMicro*)pNextVoie);
                        }

                        if(pNextVoie)
                        {
                            if(pPointDeConvergence->GetCvg()->GetGiratoire() && pNextVoie->GetParent() == pSortie)   // le follower sort
                            {
                                unsigned int    number;                                        
                                                                                
                                if( pVehFollower->GetResTirFoll() == -1 )
                                {
                                    number = m_pNetwork->GetRandManager()->myRand();
                                    double dbRand = (double)number / (double)MAXIMUM_RANDOM_NUMBER;                                            

                                    //pVehFollower->SetResTirFoll( (double)number / (double)UINT_MAX );
									pVehFollower->SetResTirFoll( dbRand );
                                }

                                if(pVehFollower->GetResTirFoll() > pPointDeConvergence->GetCvg()->GetGiratoire()->GetBeta())
                                // Le véhicule sortant ne gène pas le flux d'insertion
                                {
                                    pVehFollower->SetResTirFoll(-1);
                                    pVehFollower.reset();
                                    bResult = true;
                                }
                            }
                        }
                    }
                }
                else
                {
                    if(!m_pNetwork->IsCptDestination())
                    {
                        if(!pVehFollower->IsItineraire(pPointDeConvergence->m_pTAv))  // le véhicule ne gène pas
                        {                        
                            pVehFollower.reset();
                            bResult = true;
                        }
                    }
                }
            }
            // 2 - Si convergent multivoie, et si le véhicule qui est en attente doit s'insérer sur la voie externe
            // on regarde si un véhicule en amont sur la voie interne peut géner l'insertion 
            pVehFollInt.reset();
            if( pTAmP->getNb_voies()>1 && pPointDeConvergence->m_pTAv->getNb_voies() > 1)   // Test si convergent multivoie
            {
                if( nVoieGir == 0 ) // Test si l'insertion s'effectue sur la voie externe
                {
                    // Recherche du véhicule follower sur la voie interne
                    pVehFollInt = pPointDeConvergence->GetCvg()->GetReseau()->GetNearAmontVehicule( (VoieMicro*)pTAmP->GetLstLanes()[nVoieGir+1], pVAmP->GetLength());
                    if(pVehFollInt)
                    {
                        if( pVehFollInt->GetResTirFollInt() == -1 )
                        {                                 
                            double dbRand = (double)m_pNetwork->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
                            pVehFollInt->SetResTirFollInt( dbRand );
                        }

                        if(pPointDeConvergence->GetCvg()->GetGiratoire() && pVehFollInt->GetResTirFollInt() < pPointDeConvergence->GetCvg()->GetGiratoire()->GetBetaInt())
                        {   // Le véhicule en attente est géné par le véhicule arrivant sur la voie interne                           
                        }
                        else // Le véhicule sur la voie interne ne gène pas le flux d'insertion
                        {
                            pVehFollInt->SetResTirFollInt(-1);
                            pVehFollInt.reset();                                    
                        }
                    }
                }
            }

            double dbtfbarre, dbtmbarre, dbVitAm;
            double dbq1mu, dbdlag, dbEta;

            dbtfbarre = 0;  // tronçon non prioritaire
            if( pPointDeConvergence->GetCvg()->m_pGestionCapteur->GetNbVehTotal(pPointDeConvergence->GetCvg()->GetCapteur((TuyauMicro*)pVAmNP->GetParent()), j) == 0)     // Aucun véhicule vu par le capteur pendant T
                dbtfbarre = pPointDeConvergence->GetCvg()->GetTf( m_pVehicle->GetType() );
            else
                for(int i=0; i< (int)pPointDeConvergence->GetCvg()->GetReseau()->GetLstTypesVehicule().size();i++)
                {
                    dbtfbarre += pPointDeConvergence->GetCvg()->m_pGestionCapteur->GetRatioTypeVehicule( pPointDeConvergence->GetCvg()->GetCapteur((TuyauMicro*)pVAmNP->GetParent()), j, m_pNetwork->GetLstTypesVehicule()[i]) * pPointDeConvergence->GetCvg()->GetTf(m_pNetwork->GetLstTypesVehicule()[i]);                    
                }

            dbtmbarre = 0;  // tronçon prioritaire
            for(int i=0; i< (int)m_pNetwork->GetLstTypesVehicule().size();i++)
            {
                dbtmbarre += pPointDeConvergence->GetCvg()->m_pGestionCapteur->GetRatioTypeVehicule( pPointDeConvergence->GetCvg()->GetCapteur(pTAmP), 0, m_pNetwork->GetLstTypesVehicule()[i]) * (1/m_pNetwork->GetLstTypesVehicule()[i]->GetDebitMax());
            }

            dbq1mu = 1 / ((dbtfbarre * pPointDeConvergence->GetCvg()->m_dbMu) + dbtmbarre);

            // Recherche du premier véhicule sur le lien prioritaire
            boost::shared_ptr<Vehicule> pVeh = pPointDeConvergence->GetCvg()->GetReseau()->GetVehiculeAmont((TuyauMicro*)pTAmP, ((TuyauMicro*)pTAmP)->GetMaxRapVitSurQx(dbInstant),0, dbInstant );
            if(pVeh && pVeh->GetLink(1))
                dbVitAm = std::min<double>(pVeh->GetVitMax(), pVeh->GetLink(1)->GetVitRegByTypeVeh(pVeh->GetType(),dbInstant,pVeh->GetPos(1),pVeh->GetVoie(1)->GetNum()));
            else
                dbVitAm = std::min<double>(pPointDeConvergence->GetCvg()->GetReseau()->GetMaxVitMax(), pTAmP->GetMaxVitRegByTypeVeh(m_pVehicle->GetType(),dbInstant,pTAmP->GetLength()));    

            if( dbDemande1 <= dbq1mu)   //absoluted
            {                
                dbdlag = (- m_pVehicle->GetDiagFonda()->GetW() + dbVitAm) / ( - m_pVehicle->GetDiagFonda()->GetKMax() * m_pVehicle->GetDiagFonda()->GetW());
            }
            else    // limited
            {
                dbEta = (dbDemande1 - dbq1mu) / (1/dbtmbarre - dbq1mu);
                dbdlag = (-m_pVehicle->GetDiagFonda()->GetW() + dbVitAm) / ( - m_pVehicle->GetDiagFonda()->GetKMax() * m_pVehicle->GetDiagFonda()->GetW()) - dbEta* std::min<double>(m_pVehicle->GetVitMax(), ((Tuyau*)pVAmNP->GetParent())->GetVitRegByTypeVeh(m_pVehicle->GetType(),dbInstant,m_pVehicle->GetPos(1), pVAmNP->GetNum()));
                dbdlag = (- m_pVehicle->GetDiagFonda()->GetW() + dbVitAm) / ( - m_pVehicle->GetDiagFonda()->GetKMax() * m_pVehicle->GetDiagFonda()->GetW())* (1 - dbEta);
            }

            // Un véhicule dans dlag ?

            // Moment de l'insertion du véhicule en attente si elle existe     
			if( pVAmNP == m_pVehicle->GetVoie(1))
				dbTr = m_pNetwork->GetTimeStep() * (m_pVehicle->GetVoie(1)->GetLength() - m_pVehicle->GetPos(1)) / ( m_pVehicle->GetDstParcourueEx() );
			else
				dbTr = m_pNetwork->GetTimeStep() * (m_pVehicle->GetVoie(1)->GetLength() - m_pVehicle->GetPos(1) + pVAmNP->GetLength() ) / ( m_pVehicle->GetDstParcourueEx() );
            //*Reseau::m_pFicSimulation << "dbtr : " << dbtr << std::endl;

            if( dbInstant - m_pNetwork->GetTimeStep() + dbTr > pPointDeConvergence->m_dbInstLastPassage + std::max<double>( pPointDeConvergence->GetCvg()->GetTf(m_pVehicle->GetType()), 1 / m_pVehicle->GetDiagFonda()->GetDebitMax() ) )
            {
                // le véhicule en attente arrive au point de convergence après que le délai de follow-up se soit écoulé
                // Interpolation de la position du follower au moment où le véhicule en attente passe le point de convergence                
                if(pVehFollower || pVehFollInt) 
                {                    
                    double dbPosFol = 9999;
                    double dbPosFolEx = 9999;

                    double dbPositionLimit = pVAmP->GetLength() - dbdlag;

                    if(pVehFollower) 
                    {
                        dbPosFol = pVehFollower->CalculPosition(dbTr, pVAmP, dbPositionLimit);
                    }
                    if(pVehFollInt)
                    {
                        if(pVehFollInt->GetVit(1)>0)         // Si le véhicule est arrêté car il veu	t sortir (en fluide), il ne gène pas l'insertion                       
                            dbPosFolEx = pVehFollInt->CalculPosition(dbTr, pVAmP, dbPositionLimit);
                    }

                    dbPosFol = std::min<double>(dbPosFol, dbPosFolEx);
                            
                    if (dbPosFol <= dbPositionLimit)
                    {
                        bResult = true;
                        dbTa = m_pNetwork->GetTimeStep() - dbTr;
                    }
                    else
                    {
                        bResult = false;                    
                    }
                }
                else
                {
                    bResult = true;
                    dbTa = m_pNetwork->GetTimeStep() - dbTr;
                }
                        
            }
            else
            {
                // le véhicule en attente arrive au point de convergence avant que le délai de follow-up se soit écoulé (et celui_ci
                // s'écoule avant la fin du pas de temps courant
                // dbts correspond à l'instant dans le pas de temps pour lequel la condition aval est vérifiée
                dbTr = m_pNetwork->GetTimeStep() - ( dbInstant - (pPointDeConvergence->m_dbInstLastPassage + std::max<double>( pPointDeConvergence->GetCvg()->GetTf(m_pVehicle->GetType()), 1 / m_pVehicle->GetDiagFonda()->GetDebitMax() ) ) );

                if(!pVehFollower && !pVehFollInt)
                {
                    dbTa = m_pNetwork->GetTimeStep() - dbTr;
                    bResult = true;
                }
                else
                {
                                                
                    double dbPosFol = 9999;
                    double dbPosFolEx = 9999;

                    double dbPositionLimit = pVAmP->GetLength() - dbdlag;

                    if(pVehFollower)
                    {
                        dbPosFol = pVehFollower->CalculPosition(dbTr, pVAmP, dbPositionLimit);
                    }
                    if(pVehFollInt)
                    {
                        dbPosFolEx = pVehFollInt->CalculPosition(dbTr, pVAmP, dbPositionLimit);
                    }

                    dbPosFol = std::min<double>(dbPosFol, dbPosFolEx);


                    if (dbPosFol <= dbPositionLimit)
                    {
                        dbTa = m_pNetwork->GetTimeStep() - dbTr;
                        bResult = true;
                    }
                    else
                    {
                        bResult = false;
                    }        
                }
            }        
            // Autre vérification
            if(m_pNetwork->IsCalculTqConvergent() && bResult && pVehLeader)
            {
                double dbVit0, dbtau, dbtq;

                if( (pVAmNP->GetLength()  - m_pVehicle->GetPos(1)) < 0.01)
                    dbVit0 = 0;
                else
                    dbVit0 = (pVAmNP->GetLength()  - m_pVehicle->GetPos(1) ) / (m_pNetwork->GetTimeStep() - dbTa);

                if( m_pNetwork->IsAccBornee() )
                    dbtau = (std::min<double>(m_pVehicle->GetVitMax(), pPointDeConvergence->m_pTAv->GetMaxVitRegByTypeVeh(m_pVehicle->GetType(),dbInstant,0)) - dbVit0) / m_pVehicle->GetAccMax(dbVit0);
                else
                    dbtau = 0;

                dbtq = 1 / std::min<double>(pVehLeader->GetVitMax(), pPointDeConvergence->m_pTAv->GetMaxVitRegByTypeVeh(pVehLeader->GetType(),dbInstant, 0)) *
                    ( 0.5 * m_pVehicle->GetAccMax(dbVit0) * dbtau * dbtau + m_pVehicle->GetVit(1) * dbtau + 1/pVehLeader->GetDiagFonda()->GetKCritique() - pVehLeader->GetPos(1)) - dbtau;

                if(dbtq > m_pNetwork->GetTimeStep())
                {
                    bResult = false;
                }
                else
                {
                    if( dbTa > m_pNetwork->GetTimeStep()- dbtq )
                    {
                        dbTa = std::min<double>(dbTa, m_pNetwork->GetTimeStep()-dbtq);    
                    }
                }
            }
        }
        else
            bResult = false;
    }

    return bResult;
}

void AbstractCarFollowing::BuildContext(double dbInstant, double contextRange)
{
    // destruction du contexte le plus ancien le cas échéant
    if(m_pPrevContext)
    {
        delete m_pPrevContext;
    }

    // décalage du dernier contexte calculé
    m_pPrevContext = m_pContext;

    // Construction et calcul du nouveau nouveau contexte
    m_pContext = CreateContext(dbInstant);
    m_pContext->Build(contextRange, m_pPrevContext);
}

CarFollowingContext * AbstractCarFollowing::CreateContext(double dbInstant, bool bIsPostProcessing)
{
    return new CarFollowingContext(m_pNetwork, m_pVehicle, dbInstant, bIsPostProcessing);
}

void AbstractCarFollowing::Compute(double dbTimeStep, double dbInstant, CarFollowingContext * pContext)
{
    // Application de la formule de la loi de poursuite
    InternalCompute(dbTimeStep, dbInstant, pContext!=NULL?pContext:GetCurrentContext());

    // calcul de la distance parcourue à partir de la vitesse ou accélération en fonction du type de loi de poursuite
    if(!IsPositionComputed())
    {
        if(!IsSpeedComputed())
        {
            assert(IsAccelerationComputed()); // La loi de poursuite ne calcule rien ?

            m_dbComputedTravelledDistance = m_pVehicle->GetVit(1) * dbTimeStep + 0.5 * m_dbComputedTravelAcceleration * dbTimeStep * dbTimeStep;
        }
        else
        {
            m_dbComputedTravelledDistance = GetComputedTravelSpeed() * dbTimeStep;
        }
    }

    // Pour détecter les cas où la loi de poursuite pose problème (divisions par zero, etc...)
    if(m_dbComputedTravelledDistance != m_dbComputedTravelledDistance)
    {
        m_pNetwork->log() << std::endl << "Undefined value returned by the car following rule for vehicle " << m_pVehicle->GetID();
    }
}

CarFollowingContext* AbstractCarFollowing::GetCurrentContext()
{
    return m_pContext;
}

void AbstractCarFollowing::SetCurrentContext(CarFollowingContext *pContext)
{
    assert(!m_pContext);
    m_pContext = pContext;
}

CarFollowingContext* AbstractCarFollowing::GetPreviousContext()
{
    return m_pPrevContext;
}

double AbstractCarFollowing::GetComputedTravelledDistance() const
{
    return m_dbComputedTravelledDistance;
}

double AbstractCarFollowing::GetComputedTravelSpeed() const
{
    return m_dbComputedTravelSpeed;
}

double AbstractCarFollowing::GetComputedTravelAcceleration() const
{
    return m_dbComputedTravelAcceleration;
}

void AbstractCarFollowing::SetComputedTravelledDistance(double dbTravelledDistance)
{
    if(!IsPositionComputed())
    {
        m_pNetwork->log() << std::endl << "Assigning computed travelled distance, but this car following rule doesn't use position as state variable for vehicle " << m_pVehicle->GetID();
    }
    m_dbComputedTravelledDistance = dbTravelledDistance;
}

void AbstractCarFollowing::SetComputedTravelSpeed(double dbTravelSpeed)
{
    if(!IsSpeedComputed())
    {
        m_pNetwork->log() << std::endl << "Assigning computed speed, but this car following rule doesn't use speed as state variable for vehicle " << m_pVehicle->GetID();
    }
    m_dbComputedTravelSpeed = dbTravelSpeed;
}

void AbstractCarFollowing::SetComputedTravelAcceleration(double dbTravelAcceleration)
{
    if(!IsAccelerationComputed())
    {
        m_pNetwork->log() << std::endl << "Assigning computed acceleration, but this car following rule doesn't use acceleration as state variable for vehicle " << m_pVehicle->GetID();
    }
    m_dbComputedTravelAcceleration = dbTravelAcceleration;
}

Reseau * AbstractCarFollowing::GetNetwork()
{
    return m_pNetwork;
}

Vehicule * AbstractCarFollowing::GetVehicle()
{
    return m_pVehicle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void AbstractCarFollowing::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void AbstractCarFollowing::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void AbstractCarFollowing::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pNetwork);
    ar & BOOST_SERIALIZATION_NVP(m_pVehicle);

    ar & BOOST_SERIALIZATION_NVP(m_pContext);
    ar & BOOST_SERIALIZATION_NVP(m_pPrevContext);

    // rmq : on ne serialise pas les variables de travail qui n'ont de sens qu'au cours du pas de temps (m_dbComputedTravelledDistance, etc...)
}
