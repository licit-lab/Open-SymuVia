#include "stdafx.h"
#include "Affectation.h"


#include "SimulationSnapshot.h"

#include "reseau.h"
#include "tuyau.h"
#include "TuyauMeso.h"
#include "Connection.h"
#include "CarrefourAFeuxEx.h"
#include "TraceDocTrafic.h"
#include "ZoneDeTerminaison.h"
#include "RandManager.h"
#include "vehicule.h"
#include "Xerces/XMLUtil.h"
#include "SystemUtil.h"
#include "ConnectionPonctuel.h"
#include "voie.h"
#include "Xerces/DOMLSSerializerSymu.hpp"
#include "Logger.h"
#include "SymuCoreManager.h"
#include "TravelTimesOutputManager.h"
#include "usage/SymuViaFleet.h"
#include "usage/SymuViaFleetParameters.h"
#include "usage/Trip.h"

#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>

#include <algorithm>

using namespace std;

Affectation::Affectation()
{
    m_XmlWriterAffectation = NULL;
    m_bReroutageVehiculesExistants = false;

    m_dbStartPeriodInstant = 0;
}

Affectation::Affectation(int nPeriodeAffectation, int nAffectEqui, double dbVariationTempsDeParcours, int nNbNiveauAmont, double dbTetaLogit, char cTypeCalcul, int nMode, double dbSeuilConvergence, int nNbItMax, bool bDijkstra, int nNbPlusCourtCheminCalcule, double dbSeuilTempsParcours, bool bReroutageVehiculesExistants, double dbDstMinAvantReroutage, double dbWardropTolerance)
{
	m_nPeriodeAffectation = nPeriodeAffectation;
	m_dbVariationTempsDeParcours = dbVariationTempsDeParcours;
	m_nNbNiveauAmont = nNbNiveauAmont;

	m_dbTetaLogit = dbTetaLogit;

	m_cTypeTpsParcours = cTypeCalcul;

	m_nAffectEqui = nAffectEqui;
	if( nAffectEqui > 1)
		m_bConvergent = true;
	else
		m_bConvergent = false; 

	m_nIteration = 0;

	m_bLastInstSimu = false;

	m_dbSeuilTempsParcours = 0.1;

	m_cLieuAffectation = 'E';

	m_nMode = nMode;

	m_dbSeuilConvergence = dbSeuilConvergence;
	m_dbSeuilTempsParcours = dbSeuilTempsParcours;

	m_nNbItMax = nNbItMax;
	
	m_XmlWriterAffectation = NULL;

	m_bDijkstra = bDijkstra;

	m_nNbPlusCourtCheminCalcule = nNbPlusCourtCheminCalcule;

    m_bReroutageVehiculesExistants = bReroutageVehiculesExistants;
    m_dbDstMinAvantReroutage = dbDstMinAvantReroutage;
    m_dbWardropTolerance = dbWardropTolerance;

    m_dbStartPeriodInstant = 0;
}

Affectation::~Affectation()
{
    for (size_t iSnapshot = 0; iSnapshot < m_SimulationSnapshots.size(); iSnapshot++)
    {
        delete m_SimulationSnapshots[iSnapshot];
    }
}

 void Affectation::CalculTempsDeParcours(Reseau * pReseau, double dbInstant, char cTypeEvent, double dbPeriode)
 {
     std::deque<TuyauMicro*>::iterator itT;
     std::deque<CTuyauMeso*>::iterator itTMeso;
     std::deque<TypeVehicule*>::iterator itTypeVeh;
     for (itTypeVeh = pReseau->m_LstTypesVehicule.begin(); itTypeVeh != pReseau->m_LstTypesVehicule.end(); ++itTypeVeh)
     {
         for (itT = pReseau->m_LstTuyauxMicro.begin(); itT != pReseau->m_LstTuyauxMicro.end(); ++itT)
         {
             // Calcul du temps de parcours du tronçon
             (*itT)->CalculTempsDeParcours(dbInstant, *itTypeVeh, cTypeEvent, dbPeriode, this->m_cTypeTpsParcours);
         }
         for (itTMeso = pReseau->m_LstTuyauxMeso.begin(); itTMeso != pReseau->m_LstTuyauxMeso.end(); ++itTMeso)
         {
             // Calcul du temps de parcours du tronçon
             (*itTMeso)->CalculTempsDeParcours(dbInstant, *itTypeVeh, cTypeEvent, m_nPeriodeAffectation*pReseau->GetTimeStep(), this->m_cTypeTpsParcours);
         }
     }
     // Calcul des temps de parcours des mouvements des briques
     for (size_t i = 0; i < pReseau->Liste_carrefoursAFeux.size(); i++)
     {
         pReseau->Liste_carrefoursAFeux[i]->CalculTempsParcours();
     }
     for (size_t i = 0; i < pReseau->Liste_giratoires.size(); i++)
     {
         pReseau->Liste_giratoires[i]->CalculTempsParcours();
     }
 }

 void Affectation::FinCalculTempsDeParcours(Reseau * pReseau, double dbInstant, char cTypeEvent, double dbPeriode)
 {
     std::deque<TuyauMicro*>::iterator itT;
     std::deque<TypeVehicule*>::iterator itTypeVeh;
     for (itTypeVeh = pReseau->m_LstTypesVehicule.begin(); itTypeVeh != pReseau->m_LstTypesVehicule.end(); itTypeVeh++)
     {
         for (itT = pReseau->m_LstTuyauxMicro.begin(); itT != pReseau->m_LstTuyauxMicro.end(); itT++)
         {
             // Fin de calcul du temps de parcours du tronçon
             (*itT)->FinCalculTempsDeParcours(dbInstant, *itTypeVeh, cTypeEvent, dbPeriode, this->m_cTypeTpsParcours);
         }
     }
 }

// Type d'évènement :
// 'P' : périodique
// 'V' : modification des vitesses réglementaires
// 'R' : modification des répartitions O/D
// 'A' : traitement d'un couple OD en particulier uniquement. Si non renseigné, MAJ
//       des itinéraires si les paramètres relatifs ont changé par rapport au pas de temps précédent
// ...
bool Affectation::Run(Reseau *pReseau, double dbInstant, char cTypeEvent, bool bForceGraphReset, TypeVehicule * pTVA, SymuViaTripNode * pDestinationA, SymuViaTripNode * pOrigineA)
{
    clock_t startTime = clock();

	std::deque<TuyauMicro*>::iterator itT;
	std::deque<TypeVehicule*>::iterator itTypeVeh;
	std::set<SymuViaTripNode*, LessOriginPtr<SymuViaTripNode>> dqCnxACalculer;
	std::deque<SymuViaTripNode*>::iterator itO;
	int nRes;

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Test du bon instant
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	nRes = pReseau->GetInstSim() % m_nPeriodeAffectation;
    // rmq. : pour l'affectation statique, on ne s'occupe pas de la valeur de la période d'affectation, on sort les résultats quelquesoit l'instant de fin
    if( cTypeEvent == 'P' && pReseau->IsAffectationDynamique())
	{
		if( nRes >= 1 )
			return true;	// Au cours de la période d'affectation
	}

	if(m_bLastInstSimu)
	{
		m_bLastInstSimu = false;
		return true;
	}

	if( fabs( pReseau->GetInstSim() - pReseau->GetDureeSimu() ) < 0.0001 )
		m_bLastInstSimu = true;

	// Préparation de la sauvegarde de l'affectation
	if (IsSaving())
	{
		if ((!m_bConvergent && dbInstant == 0) || (m_bConvergent && m_nIteration == 0))
		{
			m_Periode.InitPeriode();
		}
	}

    bool bUpdateItineraires = pOrigineA == NULL && cTypeEvent == 'A';

    std::map<std::pair<std::string, std::string>, CCoupleOD> * pListCoupleOD = NULL;
    std::deque<SymuViaTripNode*>::iterator itDestination;

    if(cTypeEvent != 'A' && pReseau->IsAffectationDynamique())
    {
	    ////////////////////////////////////////////////////////////////////////////////////////////////////////
	    // Calcul de l'estimation du temps de parcours du tronçon pour chaque tronçon et chaque type de véhicule
	    ////////////////////////////////////////////////////////////////////////////////////////////////////////
        CalculTempsDeParcours(pReseau, dbInstant, cTypeEvent, m_nPeriodeAffectation*pReseau->GetTimeStep());
    }

    // Détermination des types de véhicules à traiter
    std::deque<TypeVehicule*> lstTypes;
    if(cTypeEvent == 'A' && pTVA != NULL)
    {
        lstTypes.push_back(pTVA);
    }
    else
    {
        lstTypes = pReseau->m_LstTypesVehicule;
    }

    // MAJ des couts pour les types de véhicules considérés
    pReseau->GetSymuScript()->UpdateCosts(lstTypes, bForceGraphReset);

    if(cTypeEvent != 'A')
    {
	    ////////////////////////////////////////////////////////////////////////////////////////////////////////
	    // Calcul des indicateurs de proximité de l'affectation
	    ////////////////////////////////////////////////////////////////////////////////////////////////////////
	    double dbIndProx = 1;
	    double dbIndProxCal = 0;
        // Si affectation convergente, on regarde si on a atteind l'équilibre désiré :
        // Calcul de l'indicateur de proximité		
        if (cTypeEvent == 'P' && dbInstant > 0 && pReseau->IsAffectationDynamique())
        {
            for (itTypeVeh = pReseau->m_LstTypesVehicule.begin(); itTypeVeh != pReseau->m_LstTypesVehicule.end(); ++itTypeVeh)
            {
                
                dbIndProxCal = GetProportionCritereOK(pReseau, *itTypeVeh);
                dbIndProx = min(dbIndProx, dbIndProxCal);

                pReseau->log() << "Proximity index : " << dbIndProx << std::endl;
            }
        }

	    // Positionnement de l'indice de proximité et du temps de calcul pour l'itération qui vient de se terminer
	    if (IsSaving())
	    {
		    if (((!m_bConvergent) || (m_nIteration > 0)) && (dbInstant > 0))
		    {
			    m_Periode.SetLastPreditIterationIndProx(dbIndProx);
		    }
	    }

        // rmq. : pour ne pas polluer le fichier de sortie, on ne trace que en mode de sortie des résultats d'affectation
        // (mais on le fait quand même la première fois pour avoir la iste des entrées sorties du réseau)
        if(IsSaving() || dbInstant == 0)
        {
	        ////////////////////////////////////////////////////////////////////////////////////////////////////////
	        // Trace des temps de parcours réalisés pour chaque itinéraire
	        ////////////////////////////////////////////////////////////////////////////////////////////////////////
            pReseau->log() << std::endl << "---- Realized travel time for every itinerary ---"<<std::endl;
	        for( itTypeVeh = pReseau->m_LstTypesVehicule.begin(); itTypeVeh != pReseau->m_LstTypesVehicule.end(); ++itTypeVeh)
	        {	
	            pReseau->log() << " Vehicle type : " << (*itTypeVeh)->GetLabel() << std::endl;	

		        // Itération sur les origines
		        for (itO = pReseau->Liste_origines.begin(); itO!= pReseau->Liste_origines.end(); ++itO)   // Boucle sur les origines du réseau
		        {
                    Connexion* pConnexion = (*itO)->GetOutputConnexion();
		            pReseau->log() << " Origin : " << (*itO)->GetOutputID() << std::endl;	
			        std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> > dtaAss;
			        if( pConnexion->m_mapAssignment.find( (*itTypeVeh) )!= pConnexion->m_mapAssignment.end())
			        {				
				        dtaAss = pConnexion->m_mapAssignment.find( (*itTypeVeh) )->second;

				        // Itération sur les sorties
				        std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> >::iterator itAss;
				        for(itAss = dtaAss.begin(); itAss!=dtaAss.end(); ++itAss)
				        {
				            pReseau->log() << " Destination : " << (*itAss).first.second->GetInputID() << std::endl;		
				            pReseau->log() << " Itineraries : " << std::endl;

					        // Itération sur les Données d'affectation
					        std::deque<AssignmentData>::iterator itad;
					        for(itad = (*itAss).second.begin(); itad != (*itAss).second.end(); ++itad)
					        {
						        // Itération sur les tuyaux
						        AssignmentData ad = (*itad);
						        double dbTpsParcours = 0;						
						        for( vector<Tuyau*>::iterator itT = ad.dqTuyaux.begin(); itT != ad.dqTuyaux.end(); ++itT)
						        {
						            pReseau->log() << (*itT)->GetLabel() << " ";
						        }
                                pReseau->log() << " --> " << pReseau->GetSymuScript()->ComputeCost(*itTypeVeh, ad.dqTuyaux, bForceGraphReset) << " s " << std::endl;											
					        }
				        }
			        }
		        }
	        }
        }

	    ////////////////////////////////////////////////////////////////////////////////////////////////////////
	    // Test de convergence
	    ////////////////////////////////////////////////////////////////////////////////////////////////////////
	    bool bIsEquilibre = false;
	    int nIterationFin = m_nIteration;
	    if( m_bConvergent && m_nIteration > 0)
	    {
		    // Test de convergence
		    if( dbIndProx > (1 - this->m_dbSeuilConvergence)  || m_nIteration >= this->m_nNbItMax )	// Equilibre atteint
		    {	
			    bIsEquilibre = true;
                SimulationCommit(pReseau);
                m_nIteration = 0;
			    
                FinCalculTempsDeParcours(pReseau, dbInstant, cTypeEvent, m_nPeriodeAffectation*pReseau->GetTimeStep());
		    }
		    else
		    {
		        pReseau->log() << std::endl << "Iteration " << m_nIteration << " of the assignment module" << std::endl;				

			    // Restitution de l'état initial de la période d'affectation
                SimulationRollback(pReseau, 0);
		    }		
	    }
	
	    // Préparation de la sauvegarde de l'affectation
	    if( IsSaving() )
	    {
		    if ( !m_bConvergent || bIsEquilibre ) // && (m_nIteration > 0)
		    {
			    std::string sTypeEvt;
			    switch(cTypeEvent)
			    {
			    case 'P':
				    sTypeEvt = "periodique";
				    break;
			    case 'V':
				    sTypeEvt = "evenementiel";
				    break;
			    }
                m_Periode.FinPeriode(pReseau->GetSymuScript(), sTypeEvt, m_bConvergent?nIterationFin:0, (1 - this->m_dbSeuilConvergence), dbInstant, this->m_dbSeuilTempsParcours, bForceGraphReset);
			    m_Periode.write(m_XmlWriterAffectation);			
			    m_Periode.CleanPeriode(true);
		    }
		    m_Periode.CleanPeriode(false);
	    }

	    ////////////////////////////////////////////////////////////////////////////////////////////////////////
	    // Initialisation d'une période de convergence
	    ////////////////////////////////////////////////////////////////////////////////////////////////////////
	    if( m_bConvergent)
	    {
		    if( pReseau->GetInstSim() < pReseau->GetDureeSimu() ) // fin de simulation ?
		    {
                if (m_nIteration == 0)
                {
                    assert(m_SimulationSnapshots.empty()); // Dans SymuVia, on ne peut pas avoir plus d'un snapshot (contrairement à SymuMaster)
                    TakeSimulationSnapshot(pReseau);
                }
		    }
		    else
			    return true;
	    }
        else
        {
            // On note l'instant dans cette variable quand même pour éviter un plantage dans GetProportionCritereOk dans le cas non convergent
            m_dbStartPeriodInstant = dbInstant;
        }
	    m_nIteration++;	

	    pListCoupleOD = NULL;

	    if( m_bConvergent && m_nAffectEqui>1 )		// Si affectation avec recherche de convergence, la première itération de la période
		    if( dbInstant > 0)						// d'affectation s'effectue avec les coefficients de la dernière itération (donc convergente)
			    if( m_nIteration == 1)				// de la période précédente
			    {
                
                    // ************************************************************
                    // Recalcul des coefficients à partir des temps de parcours
                    // ************************************************************

                    // Itération sur les types de véhicules
	                for( itTypeVeh = pReseau->m_LstTypesVehicule.begin(); itTypeVeh != pReseau->m_LstTypesVehicule.end(); itTypeVeh++)
	                {
                        // ************************************************************
                        // 1 - Constitution de la liste des origines à traiter
                        // ************************************************************
		                dqCnxACalculer.clear();
                        dqCnxACalculer.insert(pReseau->Liste_origines.begin(), pReseau->Liste_origines.end());

                        // ************************************************************
                        // 2 - Traitement de chaque origine ...
                        // ************************************************************
                        for (std::set<SymuViaTripNode*, LessOriginPtr<SymuViaTripNode>>::iterator itC= dqCnxACalculer.begin(); itC!= dqCnxACalculer.end(); itC++)
		                { 
                            // ****************************************************************************
                            // 2.1 - Récupération de l'origine
                            // ****************************************************************************
                            SymuViaTripNode * pOrigine = *(itC);

                            // *******************************************
                            // 2.2 -On boucle sur toutes les destinations
                            // *******************************************
                            std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> > & mapAssData = (*itC)->GetMapAssignment()[*itTypeVeh];
                            std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> >::iterator itAD; 
                            for(itAD = mapAssData.begin(); itAD != mapAssData.end(); itAD++)
                            {
                                if(itAD->first.first == nullptr)
                                {
                                    SymuViaTripNode * pDestination = itAD->first.second;

                                    // cas particulier du couple OD pour lequel des itinéraires sont imposés dans le fichier d'entrée
                                    VectDest * pVectDest = NULL;
                                    SimMatOrigDest *pMat = GetVariation(dbInstant, &pOrigine->GetLstCoeffDest(*itTypeVeh), pReseau->GetLag());
                                    for(size_t iDest = 0; pMat && iDest < pMat->MatOrigDest.size() && !pVectDest; iDest++)
                                    {
                                        if(pMat->MatOrigDest[iDest]->pDest == pDestination)
                                        {
                                            pVectDest = pMat->MatOrigDest[iDest];
                                        }
                                    }

                                    // Cas où on doit recalculer les coefficients en fonction des temps de parcours (sauf pour les itinéraires prédéfinis)
						            if( itAD != (*itC)->GetMapAssignment().find(*itTypeVeh)->second.end() )
						            {
                                        double realTetaLogit = 	m_dbTetaLogit;
                                        if(pVectDest && pVectDest->bHasTetaLogit)
                                        {
                                            realTetaLogit = pVectDest->dbTetaLogit;
                                        }
                                        if( realTetaLogit > 10 )		// Traitement d'un teta grand (sinon passage à la limite mal traité numériquement)
		                                {
                                            realTetaLogit = 10;		
                                        }

                                        double dbSum = 0;
                                        double dbCoutMin = DBL_MAX;
                                        int nItiCouMin = 0;
                                        int nNbItiCalcules = 0;
                                        double dbCoeffRestant = 1.0;
                                        for(size_t nIti=0; nIti<itAD->second.size(); nIti++)
							            {
                                            // on exclu du calcul les itinéraires prédéfinis
                                            if(!itAD->second[nIti].bPredefini)
                                            {
                                                nNbItiCalcules++;
                                                itAD->second[nIti].dbCout = pReseau->GetSymuScript()->ComputeCost(*itTypeVeh, itAD->second[nIti].dqTuyaux, bForceGraphReset);
                                                dbSum += exp( - realTetaLogit * itAD->second[nIti].dbCout );

						                        if( itAD->second[nIti].dbCout < dbCoutMin )
						                        {
                                                    dbCoutMin = itAD->second[nIti].dbCout;
						                        }
                                            }
                                            else
                                            {
                                                dbCoeffRestant -= itAD->second[nIti].dbCoeff;
                                            }
                                        }
                                        for(size_t nIti=0; nIti<itAD->second.size(); nIti++)
                                        {
                                            // on exclu du calcul les itinéraires prédéfinis
                                            if(!itAD->second[nIti].bPredefini)
                                            {
                                                if (itAD->second[nIti].dbCout == 0 || (fabs(itAD->second[nIti].dbCout - dbCoutMin) / dbCoutMin < m_dbWardropTolerance))
                                                {
                                                    nItiCouMin++;
                                                }
                                            }
                                        }

							            for(size_t nIti=0; nIti<itAD->second.size(); nIti++)
							            {
                                            // on exclu du calcul les itinéraires prédéfinis
                                            if(!itAD->second[nIti].bPredefini)
                                            {
								                if( (int)itAD->second.size() - nNbItiCalcules == 1)
									                itAD->second[nIti].dbCoeff = dbCoeffRestant;		// Un seul itinéraire calculé possible
								                else
								                {
									                if( m_nMode == 1)	// Logit
									                {
										                if( itAD->second[nIti].dbCout > 0)
											                itAD->second[nIti].dbCoeff = dbCoeffRestant * (exp( - realTetaLogit * itAD->second[nIti].dbCout ) / dbSum) ;
                                                        else if(itAD->second[nIti].dbCout == 0)
                                                            itAD->second[nIti].dbCoeff = dbCoeffRestant / nItiCouMin;
										                else
											                itAD->second[nIti].dbCoeff = 0;
									                }
									                else	// wardrop
									                {
                                                        if (itAD->second[nIti].dbCout == 0 || (fabs(itAD->second[nIti].dbCout - dbCoutMin) / dbCoutMin < m_dbWardropTolerance))
											                itAD->second[nIti].dbCoeff = dbCoeffRestant / nItiCouMin ;
										                else
											                itAD->second[nIti].dbCoeff = 0;
									                }
								                }
                                            }
                                        }
                                    }                        
                                }
                            }
                        }
                    }

                    // *****************************************************************
                    // Fin du recalcul des coefficients à partir des temps de parcours
                    // *****************************************************************
                    for( itTypeVeh = pReseau->m_LstTypesVehicule.begin(); itTypeVeh != pReseau->m_LstTypesVehicule.end(); itTypeVeh++)
				    {
					    if (IsSaving())
					    {
						    pListCoupleOD = m_Periode.addTypeVehicule((*itTypeVeh));
						    for (itO = pReseau->Liste_origines.begin(); itO!= pReseau->Liste_origines.end(); itO++)
						    {							
							    for (itDestination = pReseau->Liste_destinations.begin(); itDestination != pReseau->Liste_destinations.end(); itDestination++)
							    {
								    CCoupleOD * pCplOD = NULL;
								    pCplOD = m_Periode.addCoupleOD(pListCoupleOD, (*itO)->GetOutputID(), (*itDestination)->GetInputID());

                                    Connexion* pOriginConnection = (*itO)->GetOutputConnexion();
						
                                    std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> >::iterator itAD; 
                                    itAD = pOriginConnection->m_mapAssignment.find(*itTypeVeh)->second.find(pair<Tuyau*, SymuViaTripNode*>(nullptr, *itDestination));

                                    if (itAD != pOriginConnection->m_mapAssignment.find(*itTypeVeh)->second.end())
                                    {
									
										for(int nIti=0; nIti<(int)itAD->second.size(); nIti++)
										{
											// Trace
											std::vector<Tuyau*>::iterator itT;	
											std::vector<std::string> elements;
										    pReseau->log() << std::endl <<  "Itinerary " << nIti+1 << " - ";
											for(itT = itAD->second[nIti].dqTuyaux.begin(); itT != itAD->second[nIti].dqTuyaux.end(); itT++)
											{
											    pReseau->log() << (*itT)->GetLabel() << " ";
												elements.push_back((*itT)->GetLabel());
												elements.push_back((*itT)->GetCnxAssAv()->GetID());
											}
									            
										    pReseau->log() << " - assignment coeff. : " << itAD->second[nIti].dbCoeff << std::endl; 
											if (IsSaving())
											{
												if (elements.size() > 0)
													elements.pop_back();
												CIteration * pIter = pCplOD->AddPreditIteration((size_t)(m_bConvergent?m_nIteration:0));
												pIter->addItineraire(itAD->second[nIti].dqTuyaux, elements, itAD->second[nIti].dbCoeff, itAD->second[nIti].dbCout, itAD->second[nIti].bPredefini, GetItineraryLength(itAD->second[nIti].dqTuyaux));
											}	
										}
									}
							    }
						    }
					    }
				    }
                
				    return true;
			    }
    } // Fin si cTypeEvent != 'A'

    // Initialisation de la liste des connexions pour lesquels on calcule les plus courts chemins
	if( cTypeEvent == 'P' || cTypeEvent == 'R' || cTypeEvent == 'A')
	{
        if(cTypeEvent == 'A' && pOrigineA != NULL)
        {
            dqCnxACalculer.insert(pOrigineA);
        }
        else
        {
            dqCnxACalculer.insert(pReseau->Liste_origines.begin(), pReseau->Liste_origines.end());
        }
	}

    // rmq. : en l'état, la boucle suivant ne doit plus servir à rien ...
	// Itération sur les Tuyaux Micros du Réseau
	// Analyse de la variation du temps de parcours
	//for(itT = pReseau->m_LstTuyauxMicro.begin(); itT != pReseau->m_LstTuyauxMicro.end(); itT++)
	//{						
	//	if( /*cTypeEvent == 'P' ||*/ cTypeEvent == 'V')
	//	{
	//		// Test si variation temps de parcours est supérieur au critère
	//		if( (*itT)->m_mapTempsParcours.find(*itTypeVeh)->second > 0)
	//		{
	//			if( fabs( (*itT)->m_mapTempsParcoursPrecedent.find(*itTypeVeh)->second - (*itT)->m_mapTempsParcours.find(*itTypeVeh)->second ) / (*itT)->m_mapTempsParcours.find(*itTypeVeh)->second >= m_dbVariationTempsDeParcours)
	//			{
	//				int nNbNiveauAmont = m_nNbNiveauAmont;
	//				// Détermination des connexions internes où le calcul des coefficents d'affectation doit être relancé
	//				GetCnxAmont( (*itT), nNbNiveauAmont, dqCnxACalculer);						
	//			}
	//		}
	//	}
	//}

    std::deque<SymuViaTripNode*> lstDestinations;
    if(cTypeEvent == 'A' && pDestinationA != NULL)
    {
        lstDestinations.push_back(pDestinationA);
    }
    else
    {
        lstDestinations = pReseau->Liste_destinations;
    }

    double dbEndPeriodTime;
    if (pReseau->IsAffectationDynamique())
    {
        dbEndPeriodTime = dbInstant + m_nPeriodeAffectation*pReseau->GetTimeStep();
    }
    else
    {
        dbEndPeriodTime = pReseau->GetDureeSimu();
    }

	for( itTypeVeh = lstTypes.begin(); itTypeVeh != lstTypes.end(); itTypeVeh++)
	{
        if(!bUpdateItineraires)
        {
	        pReseau->log() << std::endl << "Studied vehicle type : " << (*itTypeVeh)->GetLabel() << std::endl;
        }

		if (IsSaving() && cTypeEvent != 'A')
		{
			pListCoupleOD = m_Periode.addTypeVehicule((*itTypeVeh));
		}

		// Calcul des plus courts chemins entre les connexions désirées et les sorties du réseau
		std::set<SymuViaTripNode*, LessOriginPtr<SymuViaTripNode>>::iterator itC;

        // On calcule au préalable les couples ODs pour lesquels le calcul d'affectation est utile.
        std::map<SymuViaTripNode*, std::map<SymuViaTripNode*, bool> > isOriginLinkedToDestination;
        bool bShowType;
        if(cTypeEvent != 'A' || bUpdateItineraires)
        {
            bShowType = false;
            for (itC= dqCnxACalculer.begin(); itC!= dqCnxACalculer.end(); ++itC)   // Boucle sur les entrées ou les connexions du réseau
		    {
                SymuViaTripNode * pOrigine = *(itC);
                for (itDestination=pReseau->Liste_destinations.begin();itDestination!=pReseau->Liste_destinations.end();++itDestination)	// Boucle sur les destinations du réseau
			    {
                    bool isLinkedOD = pOrigine->IsLinkedToDestination(*itDestination, *itTypeVeh, dbInstant, dbEndPeriodTime);
                    isOriginLinkedToDestination[pOrigine][*itDestination] = isLinkedOD;
                    if(isLinkedOD)
                    {
                        bShowType = true;
                    }
                }
            }
        }
        else
        {
            bShowType = true;
        }

        if(!bShowType)
        {
            if(!bUpdateItineraires)
            {
	            pReseau->log() << std::endl << "Vehicle type ignored." << std::endl;
            }
        }
        else
        {
		    // Itération sur les connexions
		    // Recalcul de l'affectation des entrées vers les sorties du réseau sur évènement périodique ou modif. des matrices OD		
		    for (itC= dqCnxACalculer.begin(); itC!= dqCnxACalculer.end(); ++itC)   // Boucle sur les entrées ou les connexions du réseau
		    {                
                if(!bUpdateItineraires)
                {
		            pReseau->log() << std::endl << "Computing shortest paths from " << (*itC)->GetOutputID() << " to every destination : ";
                }
                                  
                SymuViaTripNode * pOrigine = *(itC);

                if(cTypeEvent != 'A')
                {
			        if(m_nIteration==1 || !m_bConvergent)
			        {
					    if( (*itC)->GetMapAssignment().find(*itTypeVeh) != (*itC)->GetMapAssignment().end() )
						    (*itC)->GetMapAssignment().erase( (*itC)->GetMapAssignment().find(*itTypeVeh));
			        }
			        else
				        (*itC)->InitAssignmentPeriod(*itTypeVeh);
                }

                SimMatOrigDest *pMat = GetVariation(dbInstant, &pOrigine->GetLstCoeffDest(*itTypeVeh), pReseau->GetLag());
                SimMatOrigDest *pMatPrev = NULL;
                if (!bForceGraphReset)
                {
                    pMatPrev = GetVariation(dbInstant-pReseau->GetTimeStep(), &pOrigine->GetLstCoeffDest(*itTypeVeh), pReseau->GetLag());
                }

			    // Itération sur les sorties
			    CCoupleOD * pCplOD = NULL;
			    for (itDestination=lstDestinations.begin();itDestination!=lstDestinations.end();++itDestination)	// Boucle sur les destinations du réseau
			    {
                    if(!bUpdateItineraires)
                    {
			            pReseau->log() << std::endl << "Destination " << (*itDestination)->GetInputID() << std::endl;
                    }

				    if (IsSaving() && cTypeEvent != 'A')
				    {
					    pCplOD = m_Periode.addCoupleOD(pListCoupleOD, (*itC)->GetOutputID(),(*itDestination)->GetInputID());
				    }

				    // Itération sur des paires ((Tuyau, Destination), map(Tuyau, double))
				    // Remise à zéro des coefficients d'affectation des connexions à recalculer pour la destination concernée		
                    // rmq. : m_mapCoutAffectation jamais utilisé : pour l'instant on laisse de côté
				    //std::map< std::pair<Tuyau*, Destination*>, std::map<Tuyau*, double, LessPtr<Tuyau>>>::iterator itCpl;	
				    //if( cTypeEvent != 'A' && (*itC)->m_mapCoutAffectation.find( *itTypeVeh ) !=  (*itC)->m_mapCoutAffectation.end() )
				    //{
				    //	std::map< Connexion::typeCoupleEntreeDestination, Connexion::typeCoeffAffectation> * mapCplEntrDestCoeff;
				    //	mapCplEntrDestCoeff = &(*itC)->m_mapCoutAffectation.find( *itTypeVeh )->second;
				    //	for( itCpl = mapCplEntrDestCoeff->begin(); itCpl != mapCplEntrDestCoeff->end(); itCpl++)
				    //	{
				    //		if( (*itCpl).first.second == (*itDestination) )
				    //			(*itCpl).second.clear();
				    //	}
                    //  }


				    // Calcul des n plus courts chemins entre l'entrée et la sortie

                    // Evo. n°30 : Si on n'a pas de demande > 0 sur aucune variante temporelle,
                    // que le type n'est dans la répartition, et qu'aucun véhicule de ce type n'est dans la liste des véhicules
                    // à créer pour cette origine, on ne traite pas  cette origine.
                    bool bDoCompute = (cTypeEvent == 'A' && !bUpdateItineraires) || isOriginLinkedToDestination[pOrigine][*itDestination];					
					if(bDoCompute)
                    {
                        // si des itinéraires prédéfinis sont présents, on les utilise plutôt que de calculer les plus courts...
                        VectDest * pVectDest = NULL;
                        for(size_t iDest = 0; pMat && iDest < pMat->MatOrigDest.size() && !pVectDest; iDest++)
                        {
                            if(pMat->MatOrigDest[iDest]->pDest == (*itDestination))
                            {
                                pVectDest = pMat->MatOrigDest[iDest];
                            }
                        }

                        // Cas de la modification des itinéraires prédéfinis
						bool bDoUpdateItinetaires = true;
                        if(bUpdateItineraires)
                        {
                            if (!bForceGraphReset)
                            {
                                VectDest * pVectDestPrev = NULL;
                                for(size_t iDest = 0; pMatPrev && iDest < pMatPrev->MatOrigDest.size() && !pVectDestPrev; iDest++)
                                {
                                    if(pMatPrev->MatOrigDest[iDest]->pDest == *itDestination)
                                    {
                                        pVectDestPrev = pMatPrev->MatOrigDest[iDest];
                                    }
                                }
                                bDoUpdateItinetaires = !(*pVectDest == *pVectDestPrev);
                            }

                            if(bDoUpdateItinetaires)
                            {
                                if(m_nIteration==1 || !m_bConvergent)
			                    {
                                    std::map<TypeVehicule*, std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> > >::iterator iterMapAss = (*itC)->GetMapAssignment().find(*itTypeVeh);
					                if( iterMapAss != (*itC)->GetMapAssignment().end() )
                                    {
                                        std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> > & mapAssForVehType = iterMapAss->second;
                                        std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> >::iterator mapAssForVehTypeIter;
                                        for(mapAssForVehTypeIter = mapAssForVehType.begin(); mapAssForVehTypeIter != mapAssForVehType.end();)
                                        {
                                            if(mapAssForVehTypeIter->first.second == *itDestination)
                                            {
                                                mapAssForVehType.erase(mapAssForVehTypeIter++);
                                            }
                                            else
                                            {
                                                ++mapAssForVehTypeIter;
                                            }
                                        }
                                    }
			                    }
                                pReseau->log() << std::endl << "Computing shortest paths from " << (*itC)->GetOutputID() << " to each network's output : ";
                            }
                            if(bDoUpdateItinetaires)
                            {
                                pReseau->log() << std::endl << "Destination " << (*itDestination)->GetInputID() << std::endl;
                            }
                        }

                        if(bDoUpdateItinetaires)
                        {
                            int realNbPlusCourtsChemins = m_nNbPlusCourtCheminCalcule;
                            if(pVectDest && pVectDest->bHasNbPlusCourtsChemins)
                            {
                                realNbPlusCourtsChemins = pVectDest->nNbPlusCourtsChemins;
                            }

                            double realTetaLogit = 	m_dbTetaLogit;
                            if(pVectDest && pVectDest->bHasTetaLogit)
                            {
                                realTetaLogit = pVectDest->dbTetaLogit;
                            }
                            if( realTetaLogit > 10 )		// Traitement d'un teta grand (sinon passage à la limite mal traité numériquement)
		                    {
                                realTetaLogit = 10;		
                            }

                            std::vector<PathResult> paths;
                            std::map<std::vector<Tuyau*>, double> MapFilteredItis;
                            pReseau->GetSymuScript()->ComputePaths(*itC, *itDestination, *itTypeVeh, dbInstant, realNbPlusCourtsChemins, paths, MapFilteredItis, bForceGraphReset);

                            double dbSum = 0;				// Calcul de la somme des coûts des itinéraires
				            double dbCoutMin = DBL_MAX;		// Calcul du coût minimum des itinéraires
                            int nbPredefined = 0;
                            bool bChange = false;
                            for(size_t iPath = 0; iPath < paths.size(); iPath++)
                            {
                                const PathResult & myPath = paths[iPath];
                                // On ne prend pas en compte les itinéraires prédéfinis pour celà
                                if(!myPath.bPredefined)
                                {
                                    dbSum += exp( - realTetaLogit * myPath.dbCost );
                                    if(myPath.dbCost < dbCoutMin)
                                    {
                                        dbCoutMin = myPath.dbCost;
                                    }
                                }
                                else
                                {
                                    nbPredefined++;
                                }

                                if ((*itC)->AddEltAssignment(*itTypeVeh, NULL, *itDestination, myPath.links, myPath.dbCost, myPath.dbPenalizedCost, myPath.dbCommonalityFactor, myPath.bPredefined, myPath.strName, myPath.pJunction))
                                {
                                    bChange = true;
                                }
                            }

                            pReseau->log() << std::endl << paths.size() << " path(s) found ( " << nbPredefined << " predefined )" << std::endl; 

				            int nItiCouMin = 0;				// Nombre de plus court chemin de coût min

                            // Evolution SymuVia n°114 : affichage des chemins refusé par commonality factor
                            std::map<std::vector<Tuyau*>, double>::const_iterator iterFilteredItis;
                            for(iterFilteredItis = MapFilteredItis.begin(); iterFilteredItis != MapFilteredItis.end(); ++iterFilteredItis)
                            {
                                pReseau->log() << std::endl << "discarded path : ";
                                for(size_t iTuy = 0; iTuy < iterFilteredItis->first.size(); iTuy++)
						        {
							        pReseau->log() << iterFilteredItis->first[iTuy]->GetLabel() << " ";
						        }
                                pReseau->log() << " - commonality factor : " << iterFilteredItis->second << std::endl; 
                            }

                            std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> >::iterator itAD;
                            std::map<TypeVehicule*, std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> > >::iterator itMapAssignment = (*itC)->GetMapAssignment().find(*itTypeVeh);
                            size_t nCheminCalcules = paths.size();
                            if(itMapAssignment != (*itC)->GetMapAssignment().end())
                            {
    			                itAD = itMapAssignment->second.find(pair<Tuyau*,SymuViaTripNode*>(nullptr, *itDestination));
                                if( itAD != itMapAssignment->second.end() )
				                {
                                    for(int nIti=0; nIti<(int)itAD->second.size(); nIti++)
						            {
                                        if(!itAD->second[nIti].bPredefini)
                                        {
                                            // S'il s'agit d'un itinéraire de l'itération précédente et non redétecté ici, 
                                            // on met à jour son coût pour le prendre en compte malgré tout
                                            if( itAD->second[nIti].dbCout == -1)
							                {
                                                itAD->second[nIti].dbCout = pReseau->GetSymuScript()->ComputeCost(*itTypeVeh, itAD->second[nIti].dqTuyaux, bForceGraphReset);

                                                dbSum += exp( - realTetaLogit * itAD->second[nIti].dbCout );

                                                nCheminCalcules++;
							                }

                                            if (itAD->second[nIti].dbCout == 0 || (fabs(itAD->second[nIti].dbCout - dbCoutMin) / dbCoutMin < m_dbWardropTolerance))
                                            {
                                                nItiCouMin++;
                                            }
                                        }
                                    }
                                }
                            }
			
				            // Calcul des coefficients pour les itinéraires affectés aux entrées

                            if(!paths.empty())
				            {
						        if( itAD != itMapAssignment->second.end() )
						        {
                                    int iItiPredef = 0;
							        for(size_t nIti=0; nIti<itAD->second.size(); nIti++)
							        {	
                                        AssignmentData & assignmentData = itAD->second[nIti];
                                        double dbTmp = 0;
								        double dbTmp2 = 0, dbTmp3 = 0;
                                        if(assignmentData.bPredefini)
                                        {
                                            assignmentData.dbCoeff = pVectDest->lstItineraires[iItiPredef++].first;
                                        }
                                        else
                                        {
                                            double dbRelicatCoeff = pVectDest?pVectDest->dbRelicatCoeff:1.0;
								            if( nCheminCalcules == 1)
                                                assignmentData.dbCoeff = dbRelicatCoeff;		// Un seul itinéraire possible
								            else
								            {
									            if( m_nMode == 1)	// Logit
									            {
										            if( assignmentData.dbCout > 0)
											            assignmentData.dbCoeff = dbRelicatCoeff * (exp( - realTetaLogit * assignmentData.dbCout ) / dbSum) ;
                                                    else if (assignmentData.dbCout == 0)
                                                        assignmentData.dbCoeff = dbRelicatCoeff / nItiCouMin;
										            else
											            assignmentData.dbCoeff = 0;
									            }
									            else	// wardrop
									            {
                                                    if (assignmentData.dbCout == 0 || (fabs(assignmentData.dbCout - dbCoutMin) / dbCoutMin < m_dbWardropTolerance))
											            assignmentData.dbCoeff = dbRelicatCoeff / nItiCouMin ;
										            else
											            assignmentData.dbCoeff = 0;
									            }
								            }

								            if( m_nAffectEqui == 3 && m_nIteration > 1)  // MSA
								            {
									            dbTmp2 = assignmentData.dbCoeff;
									            dbTmp3 = assignmentData.dbCoeffPrev;

									            dbTmp = 0;
									            if(assignmentData.dbCoeff >= 0)
										            dbTmp = (1./(float)m_nIteration)*assignmentData.dbCoeff;
									
									            if(assignmentData.dbCoeffPrev >= 0)
										            dbTmp += (1-1./(float)m_nIteration)*assignmentData.dbCoeffPrev;	// MSA		

									            assignmentData.dbCoeff = dbTmp;
								            }
                                        }

                                        if(cTypeEvent != 'A' || bUpdateItineraires)
                                        {
                                            // Trace
								            std::vector<Tuyau*>::iterator itT;	
								            std::vector<std::string> elements;
								            pReseau->log() << std::endl <<  "Itinerary " << nIti+1;
                                            if(assignmentData.bPredefini)
                                            {
                                                pReseau->log() << " (predefined)";
                                            }
                                            pReseau->log() << " - ";
                                            if( assignmentData.strRouteId != "")
                                            {
                                                pReseau->log() <<" route Id : " << assignmentData.strRouteId << " - ";
                                            }
                                    
								            for(itT = assignmentData.dqTuyaux.begin(); itT != assignmentData.dqTuyaux.end(); ++itT)
								            {
								                pReseau->log() << (*itT)->GetLabel() << " ";
                                                if (IsSaving())
								                {
									                elements.push_back((*itT)->GetLabel());
									                elements.push_back((*itT)->GetCnxAssAv()->GetID());
                                                }
								            }
								            pReseau->log() << " - predicted cost : " << assignmentData.dbCout; 
                                            pReseau->log() << " - penalized cost : " << assignmentData.dbCoutPenalise; 
                                            if(!assignmentData.bPredefini)
                                            {
								                pReseau->log() << " - prev. assignment coeff. : " << dbTmp3;
								                pReseau->log() << " - new assignment coeff. : " << dbTmp2;
                                            }
								            pReseau->log() << " - assignment coeff. : " << assignmentData.dbCoeff; 
                                            // Evolution n°114 : affichage du commonality factor
                                            if(pReseau->GetCommonalityFilter() && !assignmentData.bPredefini)
                                            {
                                                // rmq : bug potentiel ici
                                                pReseau->log() << " - commonality factor : " << assignmentData.dbCommonalityFactor; 
                                            }
                                            pReseau->log() << std::endl;
								            if (IsSaving())
								            {
									            if (elements.size() > 0)
										            elements.pop_back();
									            CIteration * pIter = pCplOD->AddPreditIteration((size_t)(m_bConvergent?m_nIteration:0));
									            pIter->addItineraire(itAD->second[nIti].dqTuyaux, elements, itAD->second[nIti].dbCoeff, itAD->second[nIti].dbCout, itAD->second[nIti].bPredefini, GetItineraryLength(itAD->second[nIti].dqTuyaux));
								            }		
                                        }
							        }													
						        }

                                // **************************************************************
                                // Mise à jour dynamique des itinéraires des véhicules existants
                                // **************************************************************
                                // En cas de modif des itinéraires ou de leur coefficient, on modifie dynamiquement les itinéraires des véhicules concernés
                                if(m_bReroutageVehiculesExistants && bChange && (cTypeEvent != 'A' || bUpdateItineraires))
                                {
                                    // On constitue la liste des véhicules à tester : ceux qui sont sur le réseau + les véhicules en attente pour l'origine dont le traitement est en cours...
                                    vector<boost::shared_ptr<Vehicule>> lstVehicules = pReseau->m_LstVehicles;
                                    map<Tuyau*, map<int, deque<boost::shared_ptr<Vehicule>>>>::iterator mapVehEnAttenteIter = pOrigine->m_mapVehEnAttente.begin();
                                    for(mapVehEnAttenteIter = pOrigine->m_mapVehEnAttente.begin(); mapVehEnAttenteIter != pOrigine->m_mapVehEnAttente.end(); mapVehEnAttenteIter++)
                                    {
                                        map<int, deque<boost::shared_ptr<Vehicule>>>::iterator mapVehEnAttenteIter2;
                                        for(mapVehEnAttenteIter2 = mapVehEnAttenteIter->second.begin(); mapVehEnAttenteIter2 != mapVehEnAttenteIter->second.end(); mapVehEnAttenteIter2++)
                                        {
                                            lstVehicules.insert(lstVehicules.end(), mapVehEnAttenteIter2->second.begin(), mapVehEnAttenteIter2->second.end());
                                        }
                                    }

                                    // pour tous les véhicules associés à ce couple OD et du type de véhicule en cours de traitement ...
                                    for(size_t iVehIdx = 0; iVehIdx < lstVehicules.size(); iVehIdx++)
                                    {
                                        Vehicule * pVehicule = lstVehicules[iVehIdx].get();
                                        SymuViaFleet * pSymuViaFleet = dynamic_cast<SymuViaFleet*>(pVehicule->GetFleet());
                                        if(pVehicule->GetOrigine() == pOrigine
                                            && pVehicule->GetDestination() == *itDestination
                                            && pVehicule->GetType() == *itTypeVeh
                                            && (!pSymuViaFleet || !((SymuViaFleetParameters*)pVehicule->GetFleetParameters())->IsTerminaison()))
                                        {
                                            // identification des itinéraires empruntables par le véhicule :
                                            // Si le véhicule est en attente, tous le sont.
                                            // Si le véhicule est déjà sur le réseau, les itinéraires possibles sont ceux contenant le tronçon courant du véhicule.
                                            std::deque<AssignmentData> assData;
                                            bool bRecalculItineraireParticulier = false;
                                            if(pReseau->GetVehiculeFromID(pVehicule->GetID()))
                                            {
                                                // cas du véhicule déjà sur le réseau

                                                // remplissage de assData ...
                                                double dbSumCoeffs = 0.0;
                                                for(size_t iItiIdx = 0; iItiIdx < itAD->second.size(); iItiIdx++)
                                                {
                                                    if(itAD->second[iItiIdx].dbCoeff > 0.0)
                                                    {
                                                        const std::vector<Tuyau*> & iti = itAD->second[iItiIdx].dqTuyaux;
                                                        for(size_t iTuyIdx = 0; iTuyIdx < iti.size(); iTuyIdx++)
                                                        {
                                                            if((pVehicule->GetLink(0) && pVehicule->GetLink(0)->GetBriqueParente() && pVehicule->GetNextTuyau() == iti[iTuyIdx])
                                                                || pVehicule->GetLink(0) == iti[iTuyIdx])
                                                            {
                                                                // Respect de la distance minimum pour changement de destination
                                                                if((iTuyIdx+1 < iti.size()) && pVehicule->GetLink(0) == iti[iTuyIdx] && (pVehicule->GetPos(0) > pVehicule->GetLink(0)->GetLength()-m_dbDstMinAvantReroutage)
                                                                    && (!pVehicule->GetNextTuyau() || (pVehicule->GetNextTuyau() != iti[iTuyIdx+1])))
                                                                {
                                                                    continue;
                                                                }

                                                                // L'itinéraire est possible...
                                                                assData.push_back(itAD->second[iItiIdx]);
                                                                dbSumCoeffs += itAD->second[iItiIdx].dbCoeff;
                                                                break;
                                                            }
                                                        }
                                                    }
                                                }

                                                // on normalise les coefficients des itinéraires possibles
                                                for(size_t iItiIdx = 0; iItiIdx < assData.size(); iItiIdx++)
                                                {
                                                    assData[iItiIdx].dbCoeff /= dbSumCoeffs;
                                                }

                                                // Dans ce cas, le véhicule ne va pas pouvoir être rerouté sur un des itinéraires calculés pour le couple OD
                                                // (le véhicule est sur un tronçon qui ne fait pas partie des tronçons des itinéraires calculés)
                                                if(assData.size() == 0)
                                                {
                                                    bRecalculItineraireParticulier = true;
                                                }
                                            }
                                            else
                                            {
                                                // cas du véhicule en liste d'attente
                                                assData = itAD->second;
                                            }

                                            // Cas pour lequel on peut affecter au véhicule un itinéraire correspondant au couple OD
                                            // (le véhicule est dont sur un tronçon faisant partie d'un des nouveaux itinéraires pour ce couple OD)
                                            if(!bRecalculItineraireParticulier)
                                            {
                                                // Tirage du nouvel itinéraire							
								                double dbSum = 0;    
								                double dbRand = pReseau->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;        

								                // Détermination de l'itinéraire éligible							    							
								                for(size_t iItiIdx=0; iItiIdx < assData.size(); iItiIdx++)
								                {
									                dbSum += assData[iItiIdx].dbCoeff;

									                if( dbRand <= dbSum )
									                {
                                                        const std::vector<Tuyau*> & dqNewIti = assData[iItiIdx].dqTuyaux;

                                                        // suppression des tuyaux précédant le tuyau courant du véhicule
                                                        for(size_t iTuyIdx = 0; iTuyIdx < dqNewIti.size(); iTuyIdx++)
                                                        {
                                                            if(pVehicule->GetLink(0)->GetBriqueParente() && pVehicule->GetNextTuyau() == dqNewIti[iTuyIdx])
                                                            {
                                                                // cas du véhicule actuellement sur une brique de connexion
                                                                pVehicule->GetTrip()->ChangeCurrentPath(dqNewIti, (int)iTuyIdx-1);
                                                                pVehicule->SetNextTuyau((TuyauMicro*)dqNewIti[iTuyIdx]);
                                                                break;
                                                            }
                                                            else if(pVehicule->GetLink(0) == dqNewIti[iTuyIdx])
                                                            {
                                                                pVehicule->GetTrip()->ChangeCurrentPath(dqNewIti, (int)iTuyIdx);
                                                                pVehicule->SetNextTuyau(dqNewIti.size() > iTuyIdx+1?(TuyauMicro*)dqNewIti[iTuyIdx+1]:NULL);
                                                                break;
                                                            }
                                                        }
										        
                                                        break;
									                }
                                                }
                                            }
                                            else // Cas du véhicule dont l'itinéraire n'a pu être changé
                                            {
                                                // Dans ce cas, on regarde si l'itinéraire actuel est possible. S'il ne l'est pas,
                                                // on essaye de lui recalculer un itinéraire particulier.
                                                std::vector<Tuyau*> * dqFinIti = pVehicule->GetItineraire();
                                                for(size_t iTuyIdx = 0; iTuyIdx < dqFinIti->size(); iTuyIdx++)
                                                {
                                                    if((pVehicule->GetLink(0) == dqFinIti->at(iTuyIdx))
                                                    || (pVehicule->GetLink(0)->GetBriqueParente() && pVehicule->GetNextTuyau() == dqFinIti->at(iTuyIdx))) // cas du véhicule actuellement sur une brique de connexion
                                                    {
                                                        // On regarde si l'itinéraire à partir d'ici est réalisable
                                                        bool bFinItiOk = true;
                                                        for(size_t iTuyIdx2 = iTuyIdx+1; iTuyIdx2 < dqFinIti->size() && bFinItiOk; iTuyIdx2++)
                                                        {
                                                            Connexion * pCnxAmont = dqFinIti->at(iTuyIdx2)->GetCnxAssAm();
                                                            if( !pCnxAmont->IsMouvementAutorise(dqFinIti->at(iTuyIdx2-1), dqFinIti->at(iTuyIdx2), pVehicule->GetType(), &pVehicule->GetSousType()) )
					                                        {
						                                        bFinItiOk = false;  
					                                        }
                                                        }
                                                        if(!bFinItiOk)
                                                        {
                                                            // tentative de calcul d'une nouvelle fin d'itinéraire réalisable par le véhicule

                                                            std::vector<PathResult> newPaths;
                                                            std::map<std::vector<Tuyau*>, double> placeHolder;
                                                            pReseau->GetSymuScript()->ComputePaths(dqFinIti->at(iTuyIdx)->GetCnxAssAv(), *itDestination, pVehicule->GetType(), dbInstant, 1, dqFinIti->at(iTuyIdx), newPaths, placeHolder, bForceGraphReset);

                                                            if(newPaths.size() == 1)
                                                            {
                                                                // Reconstruction de l'itinéraire
                                                                std::vector<Tuyau*> newIti;
                                                                newIti.push_back(dqFinIti->at(iTuyIdx));
                                                                newIti.insert(newIti.end(), newPaths.front().links.begin(), newPaths.front().links.end());
                                                                pVehicule->GetTrip()->ChangeRemainingPath(newIti, pVehicule);
                                                            }
                                                            else
                                                            {
                                                                pReseau->log() << "Failure to recompute path for vehicle ID=" << pVehicule->GetID() << " during dynamic rerouting of vehicles on the network." << std::endl;
                                                            }
                                                        }
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                
                                }
                                // **************************************************************
                                // Fin de la mise à jour dynamique des itinéraires des véhicules
                                // **************************************************************
				            }//(nChemin>0)	
                        } // Fin du if sur la mise à jour de l'itinéraire
                    }
                    else
                    {
                        if(!bUpdateItineraires)
                        {
                            pReseau->log() << "ignored." << std::endl;
                        }
                    }
			    }// Itération sur les sorties
		    }// Fin itération sur les connexions
        }// Fin du test de l'ignorage du type de véhicule
	}// Fin itération sur les types de véhicules

    pReseau->log() << Logger::Info << "Assignment computation duration : " <<  ((float)(clock() - startTime))/CLOCKS_PER_SEC << " s" << std::endl << std::endl;

    return true;
}

TraceDocTrafic * Affectation::GetCurrentDocTraficTmp()
{
    TraceDocTrafic * pResult = NULL;
    if (!m_SimulationSnapshots.empty())
    {
        pResult = m_SimulationSnapshots.back()->m_XmlDocTraficTmp;
    }
    return pResult;
}

void Affectation::TakeSimulationSnapshot(Reseau *pReseau)
{
    SimulationSnapshot * pSnapshot = new SimulationSnapshot();
    pSnapshot->Backup(pReseau);
    pSnapshot->SwitchToTempFiles(pReseau, m_SimulationSnapshots.size());
    m_SimulationSnapshots.push_back(pSnapshot);
}

void Affectation::SimulationCommit(Reseau *pReseau)
{
    for (size_t iSnapshot = 0; iSnapshot < m_SimulationSnapshots.size(); iSnapshot++)
    {
        SimulationSnapshot * pSnapshot = m_SimulationSnapshots[iSnapshot];
        pSnapshot->ValidateTempFiles(pReseau);
        pSnapshot->DiscardTempFiles();
        delete pSnapshot;
    }
    m_SimulationSnapshots.clear();
}

void Affectation::SimulationRollback(Reseau *pReseau, size_t snapshotIdx)
{
    // rmq. : une si on rollback à un snapshot, on nettoie tous les snapshots antérieurs (on les valide)
    
    for (size_t iSnapshot = 0; iSnapshot < m_SimulationSnapshots.size(); iSnapshot++)
    {
        SimulationSnapshot * pSnapshot = m_SimulationSnapshots[iSnapshot];
        if(pSnapshot)
        {
            if (iSnapshot < snapshotIdx)
            {
                pSnapshot->ValidateTempFiles(pReseau);
                pSnapshot->DiscardTempFiles();
                delete pSnapshot;
                pSnapshot = NULL;
            }
            else if (iSnapshot > snapshotIdx)
            {
                pSnapshot->DiscardTempFiles();
                delete pSnapshot;
                pSnapshot = NULL;
            }
        }
    }

    SimulationSnapshot * pNewCurrentSnapshot = m_SimulationSnapshots[snapshotIdx];
    if(pNewCurrentSnapshot)
    {
        
        pNewCurrentSnapshot->DiscardTempFiles();
        
        pNewCurrentSnapshot->Restore(pReseau);
        
        pNewCurrentSnapshot->SwitchToTempFiles(pReseau, 0);
        
        m_SimulationSnapshots.assign(1, pNewCurrentSnapshot);
        
    }
}

// Fonction récursive de détection des connexions en amont d'un tronçon jusqu'à un certain niveau
// dans le but 
// (niveau 1 = connexion amont du tronçon)
void Affectation::GetCnxAmont( TuyauMicro* pT, int &nNiveau, std::set<Connexion*> &dqCnx)
{
	Connexion *pCnx;
	std::deque<Tuyau*>::iterator itT;

	pCnx = pT->GetCnxAssAm();

	if( pCnx->GetNbElAval() > 1)	// Si connexion avec un seul élément aval, inutile de relancer les calculs de coeff d'affectation
	{
        dqCnx.insert( pCnx );
	}
	nNiveau--;

	if( nNiveau > 0)
	{		
		if(pCnx->GetNbElAmont() != 0)	// Exclusion des entrées du réseau
		{
			for( itT = pCnx->m_LstTuyAssAm.begin(); itT != pCnx->m_LstTuyAssAm.end(); itT++ )
				GetCnxAmont( (TuyauMicro*)(*itT), nNiveau, dqCnx);
		}
	}
}

// Fonction qui retourne la proportion de tronçon (au prorata de la longueur) qui respecte le critère de proximité
double Affectation::GetProportionCritereOK(Reseau *pReseau, TypeVehicule *pTV)
{
	std::deque<TuyauMicro*>::iterator itT;	
	double dbLongueurOK = 0;
	double dbIndic = -1;
	double dbEcartTpsParcours;
	double dbSumLengthTrAssNet = 0;

    pReseau->log() << "Computing proximity indicator for " << pTV->GetLabel() << std::endl;

	for(itT = pReseau->m_LstTuyauxMicro.begin(); itT != pReseau->m_LstTuyauxMicro.end(); itT++)
	{		
		if( (*itT)->GetNbVehPeriodeAffectation(pTV) > 0)	// Le tronçon est pris en compte uniquement si il a vu passer des véhicules au cours de la période d'affectation considérée
		{
			if( (*itT)->m_mapTempsParcours.find( pTV )!=(*itT)->m_mapTempsParcours.end() )
			{
				// Calcul de l'écart du temps de parcours
				dbEcartTpsParcours = ( fabs( (*itT)->m_mapTempsParcoursRealise.find( pTV )->second - (*itT)->m_mapTempsParcoursPrecedent.find( pTV )->second )) / (*itT)->m_mapTempsParcours.find( pTV )->second;

				// Si écart < epsilon, prise en compte du tronçon au prorata de sa longeur
				if( dbEcartTpsParcours < m_dbSeuilTempsParcours )
					dbLongueurOK += (*itT)->GetLength();		

				dbSumLengthTrAssNet += (*itT)->GetLength();		

				pReseau->log() << "Link " << (*itT)->GetLabel() << " - length :  " << (*itT)->GetLength() ;
				pReseau->log() << " - predicted travel time : "	<<  (*itT)->m_mapTempsParcoursPrecedent.find( pTV )->second;
				pReseau->log() << " - realized travel time : "	<<  (*itT)->m_mapTempsParcoursRealise.find( pTV )->second;
				//if( this->m_cTypeTpsParcours =='P')
				pReseau->log() << " - predictive travel time : "	<<  (*itT)->m_mapTempsParcours.find( pTV )->second;
				pReseau->log() << " - gap : "	<<  dbEcartTpsParcours << std::endl << std::endl;
			}
		}
		else
		{
		    pReseau->log() << "Link " << (*itT)->GetLabel() << " No detected vehicle" << std::endl << std::endl;

			// Si un tronçon ne voit passer personne mais pour lequel un temps de parcours a été estimé, il faut le prendre en compte dans le calcul
			// de l'indicateur
			if( (*itT)->m_mapTempsParcours.find( pTV )!=(*itT)->m_mapTempsParcours.end() )
			{
                if (fabs((*itT)->m_mapTempsParcours.find(pTV)->second - (*itT)->ComputeCost(!m_SimulationSnapshots.empty() ? m_SimulationSnapshots.back()->GetInstSvg() : m_dbStartPeriodInstant, pTV, false)) > 1)
					dbSumLengthTrAssNet += (*itT)->GetLength();		
			}
		}
	}
	//dbIndic =  dbLongueurOK / pReseau->m_dbSumLengthTrAssNet;
    if(dbSumLengthTrAssNet != 0)
    {
	    dbIndic =  dbLongueurOK / dbSumLengthTrAssNet;
    }

	return dbIndic;

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Méthode de la classe "Affectation" qui gèrent la sauvegarde des périodes d'affectation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void	Affectation::InitSaveAffectation(bool bSave, Reseau *pReseau)
{
	if (m_XmlWriterAffectation != NULL)
	{
		delete m_XmlWriterAffectation;
		m_XmlWriterAffectation = NULL;
	}
	if (!bSave)
	{
		return;
	}

	std::string sPathAff = pReseau->GetPrefixOutputFiles() + "_afc" + pReseau->GetSuffixOutputFiles() + ".xml";
	m_XmlWriterAffectation = new DOMLSSerializerSymu();
	m_XmlWriterAffectation->initXmlWriter(sPathAff.c_str(), gXMLDecl_ver10, gXMLDecl_encodingASCII);
	m_XmlWriterAffectation->setIndent(true);

	m_XmlWriterAffectation->writeXmlDeclaration();
	m_XmlWriterAffectation->writeStartElement(XS("OUT_AFFECTATION"));

	//Recopie du réseau
	m_XmlWriterAffectation->writeStartElement(XS("IN"));
	DOMElement * xmlRoot = pReseau->m_XMLDocData->getDocumentElement();
	DOMElement * child = xmlRoot->getFirstElementChild();
	while (child != NULL)
	{
		m_XmlWriterAffectation->writeNode(child);
		child = child->getNextElementSibling();
	}
	m_XmlWriterAffectation->writeEndElement();//"IN"

	SaveAffectationSimulation(pReseau);//Sauvegarde de l'élément SIMULATION

	//Début de l'écriture des périodes
	m_XmlWriterAffectation->writeStartElement(XS("PERIODES"));
}

void	Affectation::SaveAffectationSimulation(Reseau *pReseau)
{
	if (m_XmlWriterAffectation == NULL)
		return;

	//Calcul de "nb_paireOD"
	size_t nb_paireOD = 0;
	nb_paireOD = pReseau->GetLstOrigines().size() * pReseau->GetLstDestinations().size();
	//Calcul de "nb_noeud"
	size_t nb_noeud = pReseau->Liste_repartiteurs.size()
					+ pReseau->Liste_convergents.size()
					+ pReseau->Liste_giratoires.size()
					+ pReseau->Liste_carrefoursAFeux.size();


	//Ecriture des paramètres de la simulation
	m_XmlWriterAffectation->writeStartElement(XS("SIMULATION"));
	m_XmlWriterAffectation->writeAttributeString(XS("version"),XS(("SymuVia " + SystemUtil::GetFileVersion()).c_str()));
	m_XmlWriterAffectation->writeAttributeString(XS("timeTrafic"), XS(pReseau->GetDateSimulation().ToString().c_str())); //SDateTime::Now().ToString().c_str()
	m_XmlWriterAffectation->writeAttributeString(XS("debut"), XS(pReseau->GetSimuStartTime().ToString().c_str()));
	m_XmlWriterAffectation->writeAttributeString(XS("fin"), XS(pReseau->GetSimuEndTime().ToString().c_str()));
	switch(m_nMode)
	{
	case 1:
		m_XmlWriterAffectation->writeAttributeString(XS("type"), XS("logit"));
		break;
	case 2:
		m_XmlWriterAffectation->writeAttributeString(XS("type"), XS("wardrop"));
		break;
	default:
		m_XmlWriterAffectation->writeAttributeString(XS("type"), XS(""));
	}
	m_XmlWriterAffectation->writeAttributeString(XS("seuil_convergence"), XS(SystemUtil::ToString(3, m_dbSeuilConvergence).c_str()));
	m_XmlWriterAffectation->writeAttributeString(XS("seuil_tempsparcours"), XS(SystemUtil::ToString(3, m_dbSeuilTempsParcours).c_str()));
	m_XmlWriterAffectation->writeAttributeString(XS("nb_it_max"), XS(SystemUtil::ToString(m_nNbItMax).c_str()));
	std::string sTypeTpsParcours;
	sTypeTpsParcours = m_cTypeTpsParcours;/////A revoir plus tard ???
	m_XmlWriterAffectation->writeAttributeString(XS("calcul_temps_parcours"), XS(sTypeTpsParcours.c_str()));
	m_XmlWriterAffectation->writeAttributeString(XS("nb_classe_veh"),XS(SystemUtil::ToString(pReseau->m_LstTypesVehicule.size()).c_str()));
	m_XmlWriterAffectation->writeAttributeString(XS("nb_noeud"), XS(SystemUtil::ToString(nb_noeud).c_str()));
	m_XmlWriterAffectation->writeAttributeString(XS("nb_lien"), XS(SystemUtil::ToString(pReseau->GetLstTuyaux()->size()).c_str()));//**
	m_XmlWriterAffectation->writeAttributeString(XS("nb_paireOD"), XS(SystemUtil::ToString(nb_paireOD).c_str()));
	m_XmlWriterAffectation->writeEndElement();//"SIMULATION"
}

void	Affectation::CloseSaveAffectation()
{
	if (m_XmlWriterAffectation != NULL)
	{
		m_XmlWriterAffectation->writeEndElement();//"PERIODES"
		m_XmlWriterAffectation->writeEndElement();//"OUT_AFFECTATION"
		m_XmlWriterAffectation->close();
		delete m_XmlWriterAffectation;
		m_XmlWriterAffectation = NULL;
	}
}
void Affectation::IncreaseNBVehEmis(TypeVehicule*pVeh, const std::string & sOrig, const std::string & sDest)
{
	if (!IsSaving())
		return;

	CCoupleOD * pCpl = m_Periode.getCoupleOD(pVeh, sOrig, sDest);
	if (pCpl != NULL)
	{
		pCpl->nb_veh_emis++;
	}
}

void Affectation::IncreaseNBVehRecus(TypeVehicule*pVeh, const std::string & sOrig, const std::string & sDest)
{
	if (!IsSaving())
		return;

	CCoupleOD * pCpl = m_Periode.getCoupleOD(pVeh, sOrig, sDest);
	if (pCpl != NULL)
	{
		pCpl->nb_veh_recus++;
	}
}

void Affectation::AddRealiseItineraire(TypeVehicule * pTVeh, const std::string & sOrig, const std::string & sDest, const std::vector<std::string> & listIti, const std::vector<Tuyau*> & tuyaux,
                                       bool bPredefini, int vehID)
{
	if (!IsSaving())
		return;

	CCoupleOD * pCpl = m_Periode.getCoupleOD(pTVeh, sOrig, sDest);
	pCpl->AddRealiseItineraire(tuyaux, listIti, bPredefini, vehID, GetItineraryLength(tuyaux));
}

double Affectation::GetItineraryLength(const std::vector<Tuyau*> & itinerary)
{
    std::map<std::vector<Tuyau*>, double>::const_iterator iter = m_mapItineraryLengths.find(itinerary);
    if(iter != m_mapItineraryLengths.end())
    {
        return iter->second;
    }
    else
    {
        // Calcul de la distance
        double dbLength = 0.0;
        Tuyau * pTuyau;
        BriqueDeConnexion * pBrique;
        for(int i = 0; i < (int)itinerary.size()-1; i++)
        {
            pTuyau =  itinerary[i];
            dbLength += pTuyau->GetLength();
            // prise en compte du mouvement aval le cas échéant
            pBrique = pTuyau->GetBriqueAval();
            if(pBrique)
            {
                std::vector<Tuyau*> move;
                pBrique->GetTuyauxInternes(pTuyau, itinerary[i+1], move);
                for(size_t j = 0; j < move.size(); j++)
                {
                    dbLength += move[j]->GetLength();
                }
            }
        }
        if(!itinerary.empty())
        {
            dbLength += itinerary.back()->GetLength();
        }
        m_mapItineraryLengths[itinerary] = dbLength;
        return dbLength;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void Affectation::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void Affectation::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void Affectation::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_Periode);
    ar & BOOST_SERIALIZATION_NVP(m_bLastInstSimu);
    ar & BOOST_SERIALIZATION_NVP(m_nPeriodeAffectation);
    ar & BOOST_SERIALIZATION_NVP(m_nMode);
    ar & BOOST_SERIALIZATION_NVP(m_dbSeuilConvergence);
    ar & BOOST_SERIALIZATION_NVP(m_nNbItMax);
    ar & BOOST_SERIALIZATION_NVP(m_bConvergent);
    ar & BOOST_SERIALIZATION_NVP(m_nAffectEqui);
    ar & BOOST_SERIALIZATION_NVP(m_cTypeTpsParcours);
    ar & BOOST_SERIALIZATION_NVP(m_cLieuAffectation);

    ar & BOOST_SERIALIZATION_NVP(m_dbVariationTempsDeParcours);
    ar & BOOST_SERIALIZATION_NVP(m_nNbNiveauAmont);

    ar & BOOST_SERIALIZATION_NVP(m_dbTetaLogit);
    ar & BOOST_SERIALIZATION_NVP(m_dbSeuilTempsParcours);
    ar & BOOST_SERIALIZATION_NVP(m_bDijkstra);
    ar & BOOST_SERIALIZATION_NVP(m_nNbPlusCourtCheminCalcule);
   
    ar & BOOST_SERIALIZATION_NVP(m_nIteration);

    ar & BOOST_SERIALIZATION_NVP(m_bReroutageVehiculesExistants);
    ar & BOOST_SERIALIZATION_NVP(m_dbDstMinAvantReroutage);
    ar & BOOST_SERIALIZATION_NVP(m_dbWardropTolerance);
    
    ar & BOOST_SERIALIZATION_NVP(m_dbStartPeriodInstant);

    ar & BOOST_SERIALIZATION_NVP(m_SimulationSnapshots);
}