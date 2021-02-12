#include "stdafx.h"
#include "BriqueDeConnexion.h"

#include "reseau.h"
#include "convergent.h"
#include "XMLDocSirane.h"
#include "Segment.h"
#include "vehicule.h"
#include "SystemUtil.h"
#include "tuyau.h"

#ifdef USE_SYMUCORE
#include "sensors/EdieSensor.h"
#include <Demand/MacroType.h>
#include <Demand/VehicleType.h>
#include <Utils/TravelTimeUtils.h>
#include <Utils/RobustTravelIndicatorsHelper.h>
#endif // USE_SYMUCORE

#include <boost/serialization/deque.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/shared_ptr.hpp>

//================================================================
    BriqueDeConnexion::BriqueDeConnexion
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 29/04/2008
// Historique: 29/04/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
):Connexion()
{   
    m_pCellSirane = NULL;
    m_bSimulationTraversees = true;
} 

//================================================================
    BriqueDeConnexion::BriqueDeConnexion
//----------------------------------------------------------------
// Fonction  : Constructeur
// Version du: 29/04/2008
// Historique: 29/04/2008 (C.Bécarie - Tinea)
//             Création
//================================================================
(
    char    *strID,     
    Reseau  *pReseau,
    char    cType,
    bool    bTraversees
	):Connexion(strID, pReseau)
{   
    m_cType = cType; 

    m_pCtrlDeFeux = NULL;

    m_pCellSirane = NULL;

    m_bSimulationTraversees = bTraversees;
} 

BriqueDeConnexion::~BriqueDeConnexion()
{
    if(m_pCellSirane != NULL)
    {
        delete m_pCellSirane;
    }

#ifdef USE_SYMUCORE
	std::map<std::pair<Tuyau*, Tuyau*>, SymuCore::RobustTravelIndicatorsHelper* >::const_iterator iterRobustTTHelper;
	for (iterRobustTTHelper = m_mapRobustTravelTimesHelper.begin(); iterRobustTTHelper != m_mapRobustTravelTimesHelper.end(); iterRobustTTHelper++)
		delete iterRobustTTHelper->second;

	for (iterRobustTTHelper = m_mapRobustTravelSpeedsHelper.begin(); iterRobustTTHelper != m_mapRobustTravelSpeedsHelper.end(); iterRobustTTHelper++)
		delete iterRobustTTHelper->second;
#endif
}


//================================================================
    void    BriqueDeConnexion::AddTfTra
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

    m_LstTfTra.push_back(TpsIns);
}

//================================================================
    double  BriqueDeConnexion::GetTfTra
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
    for(int i=0; i<(int)m_LstTfTra.size();i++)
    {
        if( m_LstTfTra[i].pTV == pTV )
            return m_LstTfTra[i].dbtc;
    }
    return 1 / pTV->GetDebitMax();   // Valeur par défaut
}

//================================================================
    void    BriqueDeConnexion::AddTfCvg
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

    m_LstTfCvg.push_back(TpsIns);
}

//================================================================
    double  BriqueDeConnexion::GetTfCvg
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
    for(int i=0; i<(int)m_LstTfCvg.size();i++)
    {
        if( m_LstTfCvg[i].pTV == pTV )
            return m_LstTfCvg[i].dbtc;
    }
    return 1 / pTV->GetDebitMax();   // Valeur par défaut
}

//================================================================
    void  BriqueDeConnexion::InitSimulationSirane
//----------------------------------------------------------------
// Fonction  : Initialise la brique de donnexion en tant que
// cellule pour le calcul des données nécessaires à Sirane
// Version du: 10/04/2012
// Historique: 10/04/2012 (O.Tonck - IPSIS)
//             Création
//================================================================
(   
)
{
    // instanciation de la cellule associée à la brique de connexion
    if(m_pCellSirane != NULL)
    {
        delete m_pCellSirane;
    }
    m_pCellSirane = new Segment();
    m_pCellSirane->SetReseau(m_pReseau);
    m_pCellSirane->SetLabel(GetID());

    // calcul de l'isobarycentre de la brique
    std::vector<Point*> lstPts;
    lstPts.push_back(CalculBarycentre());
    m_pReseau->m_XmlDocSirane->AddCellule(m_pCellSirane->GetLabel(), CELL_NOEUD, lstPts, UNDEFINED_PHASE);
    delete lstPts[0];
}


//================================================================
    Point*  BriqueDeConnexion::CalculBarycentre
//----------------------------------------------------------------
// Fonction  : Calcule la position du barycentre, à partir des 
// coordonnées des extrémités des tronçons liés à la brique.
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

    // récupération des points aval des tronçons amont
    Point * pPoint;
    for(size_t i = 0; i < m_LstTuyAm.size(); i++)
    {
        pPoint = m_LstTuyAm[i]->GetExtAval();
        isoBarycentre->dbX += pPoint->dbX;
        isoBarycentre->dbY += pPoint->dbY;
        isoBarycentre->dbZ += pPoint->dbZ;
        nbPts++;
    }
    // récupération des points amont des tronçons aval
    for(size_t i = 0; i < m_LstTuyAv.size(); i++)
    {
        pPoint = m_LstTuyAv[i]->GetExtAmont();
        isoBarycentre->dbX += pPoint->dbX;
        isoBarycentre->dbY += pPoint->dbY;
        isoBarycentre->dbZ += pPoint->dbZ;
        nbPts++;
    }
    // on normalise
    isoBarycentre->dbX /= nbPts;
    isoBarycentre->dbY /= nbPts;
    isoBarycentre->dbZ /= nbPts;

    return isoBarycentre;
}

void BriqueDeConnexion::CalculTempsParcours()
{
    assert(!m_pReseau->IsSymuMasterMode()); // ne devrait pas être appelée si utilisation avec SymuMaster

    m_LstCoutsMesures.clear();

    std::map<std::pair<Tuyau*,Tuyau*>, std::map<TypeVehicule*, std::pair<int, double> > > tempMap;
    std::map<std::pair<Tuyau*,Tuyau*>, std::map<int, std::pair< TypeVehicule*, std::pair<double,double> > > >::const_iterator iter;
    for(iter = m_mapVeh.begin(); iter != m_mapVeh.end(); ++iter)
    {
        std::map<TypeVehicule*, std::pair<int, double> > & tempMapMove = tempMap[iter->first];
        std::map<int, std::pair< TypeVehicule*, std::pair<double,double> > >::const_iterator iterVeh;
        for(iterVeh = iter->second.begin(); iterVeh != iter->second.end(); ++iterVeh)
        {
            if(tempMapMove.find(iterVeh->second.first) == tempMapMove.end())
            {
                tempMapMove[iterVeh->second.first] = std::make_pair(0, 0.0);
            }
            tempMapMove[iterVeh->second.first].first++;
            tempMapMove[iterVeh->second.first].second += iterVeh->second.second.second;
        }
    }

    // Calcul des moyennes ...
    std::map<std::pair<Tuyau*,Tuyau*>, std::map<TypeVehicule*, std::pair<int, double> > >::iterator iterMap;
    for(iterMap = tempMap.begin(); iterMap != tempMap.end(); ++iterMap)
    {
        std::map<TypeVehicule*, std::pair<int, double> >::iterator iterMove;
        for(iterMove = iterMap->second.begin(); iterMove != iterMap->second.end(); ++iterMove)
        {
            m_LstCoutsMesures[iterMap->first][iterMove->first] = iterMove->second.second / iterMove->second.first;
        }
    }

    m_mapVeh.clear();
}

//! Calcule le cout d'un mouvement autorisé
double BriqueDeConnexion::ComputeCost(TypeVehicule* pTypeVeh, Tuyau* pTuyauAmont, Tuyau * pTuyauAval)
{
    bool bFound = false;
    double dbResult = 0;
    std::pair<Tuyau*,Tuyau*> myPair = std::make_pair(pTuyauAmont, pTuyauAval);

    // si cout mesuré, on l'utilise, sinon cout à vide.
    std::map<std::pair<Tuyau*,Tuyau*>, std::map<TypeVehicule*, double> >::const_iterator iterMeas = m_LstCoutsMesures.find(myPair);
    if(iterMeas != m_LstCoutsMesures.end())
    {
        std::map<TypeVehicule*, double>::const_iterator iterTV = iterMeas->second.find(pTypeVeh);
        if(iterTV != iterMeas->second.end())
        {
            dbResult = iterTV->second;
            bFound = true;
        }
    }
    
    if(!bFound)
    {
        dbResult = ComputeEmptyCost(pTypeVeh, pTuyauAmont, pTuyauAval);
    }

    return dbResult;
}

double BriqueDeConnexion::ComputeEmptyCost(TypeVehicule* pTypeVeh, Tuyau* pTuyauAmont, Tuyau * pTuyauAval)
{
    double dbResult = 0;
    std::map<std::pair<Tuyau*, Tuyau*>, std::map<TypeVehicule*, double> >::const_iterator iter = m_LstCoutsAVide.find(std::make_pair(pTuyauAmont, pTuyauAval));
    if (iter != m_LstCoutsAVide.end())
    {
        dbResult = iter->second.at(pTypeVeh);
    }
    return dbResult;
}

#ifdef USE_SYMUCORE
void BriqueDeConnexion::CalculTempsParcours(double dbInstFinPeriode, SymuCore::MacroType * pMacroType, double dbPeriodDuration, bool bPollutantEmissionComputation)
{
	// On considère le premier type de véhicule venu appartenant au macro-type étant donné qu'un macro-type est sensé regrouper
	// des types de véhicules de diagramme fondamentaux semblables.
	TypeVehicule * pTypeVeh = m_pReseau->GetVehiculeTypeFromMacro(pMacroType);

	// Pour chaque mouvement
	std::map<std::pair<Tuyau*, Tuyau*>, std::map<TypeVehicule*, double> >::const_iterator iterMove;
	for (iterMove = m_LstCoutsAVide.begin(); iterMove != m_LstCoutsAVide.end(); ++iterMove)
	{
		std::map<int, std::pair< TypeVehicule*, std::pair<double, double> > > & mapVehForMove = m_mapVeh[iterMove->first];

		double dbTpsVitReg = pTypeVeh ? iterMove->second.at(pTypeVeh) : std::numeric_limits<double>::infinity();

		bool bMouvementInterdit = !pTypeVeh || !IsMouvementAutorise(iterMove->first.first, iterMove->first.second, pTypeVeh, NULL);

		std::vector<Tuyau*> tuyauxInternes;
		bool bOk = GetTuyauxInternes(iterMove->first.first, iterMove->first.second, tuyauxInternes);

		// Calcul de la concentration et du nombre moyen de véhicules sur la cellule :
		double dbLength = 0;
		double dbTravelledTimeForMacroType = 0;
		double dbTravelledDistanceForMacroType = 0;
		double dbTotalTravelledTime = 0;
		double dbTotalTravelledDistance = 0;
		TuyauMicro * pInternalLink;
		for (size_t iIntLink = 0; iIntLink < tuyauxInternes.size(); iIntLink++)
		{
			pInternalLink = (TuyauMicro*)tuyauxInternes[iIntLink];

			dbLength += pInternalLink->GetLength();

			EdieSensor * pEdieSensor = pInternalLink->m_pEdieSensor;

			assert(pEdieSensor);

			const std::map<TypeVehicule*, double> & mapTravelledTime = pEdieSensor->GetData().dbTotalTravelledTime.back();
			for (std::map<TypeVehicule*, double>::const_iterator iterTT = mapTravelledTime.begin(); iterTT != mapTravelledTime.end(); ++iterTT)
			{
				if (pMacroType->hasVehicleType(iterTT->first->GetLabel()))
				{
					dbTravelledTimeForMacroType += iterTT->second;
				}
				dbTotalTravelledTime += iterTT->second;
			}

			const std::map<TypeVehicule*, double> & mapTD = pEdieSensor->GetData().dbTotalTravelledDistance.back();
			for (std::map<TypeVehicule*, double>::const_iterator iterTD = mapTD.begin(); iterTD != mapTD.end(); ++iterTD)
			{
				if (pMacroType->hasVehicleType(iterTD->first->GetLabel()))
				{
					dbTravelledDistanceForMacroType += iterTD->second;
				}
				dbTotalTravelledDistance += iterTD->second;
			}
		}
		double dbMeanTotalVehicleNb = dbTotalTravelledTime / dbPeriodDuration;
		double dbMeanTotalConcentration = dbMeanTotalVehicleNb / dbLength;
		double dbMeanVehicleNbForMacroType = dbTravelledTimeForMacroType / dbPeriodDuration;
		double dbMeanConcentrationForMacroType = dbMeanVehicleNbForMacroType / dbLength;

		double & TTForMacroType = m_mapTTByMacroType[iterMove->first][pMacroType];
		double & dbMarginalForMacroType = m_mapMarginalByMacroType[iterMove->first][pMacroType];

		if (m_pReseau->UseSpatialTTMethod())
		{
			m_dbMeanNbVehiclesForTravelTimeUpdatePeriod[iterMove->first][pMacroType] = dbMeanVehicleNbForMacroType;
			m_dbMeanNbVehiclesForTravelTimeUpdatePeriod[iterMove->first][NULL] = dbMeanTotalVehicleNb;

			// Mode spatial classique
			if (m_pReseau->UseClassicSpatialTTMethod())
			{
				// Calcul du temps de parcours
				TTForMacroType = SymuCore::TravelTimeUtils::ComputeSpatialTravelTime(dbTravelledTimeForMacroType, dbTravelledDistanceForMacroType, dbLength, dbMeanTotalConcentration, dbTpsVitReg, bMouvementInterdit, m_pReseau->GetConcentrationRatioForFreeFlowCriterion(), m_pReseau->GetMainKx());

				// Calcul du marginal spatialisé : dans ce mode, on stocke le nombre moyen de véhicules et le temps de parcours, et on fait le calcul à la fin de la période de prédiction
				m_dbTTForAllMacroTypes[iterMove->first] = SymuCore::TravelTimeUtils::ComputeSpatialTravelTime(dbTotalTravelledTime, dbTotalTravelledDistance, dbLength, dbMeanTotalConcentration, dbTpsVitReg, bMouvementInterdit, m_pReseau->GetConcentrationRatioForFreeFlowCriterion(), m_pReseau->GetMainKx());
			}
			else
			{
				std::map<std::pair<Tuyau*, Tuyau*>, SymuCore::RobustTravelIndicatorsHelper* >::const_iterator iterRobustTTHelper = m_mapRobustTravelTimesHelper.find(iterMove->first);
				std::map<std::pair<Tuyau*, Tuyau*>, SymuCore::RobustTravelIndicatorsHelper* >::const_iterator iterRobustTSHelper = m_mapRobustTravelSpeedsHelper.find(iterMove->first);

				SymuCore::RobustTravelIndicatorsHelper* pRobustTravelTimesHelper = NULL;
				SymuCore::RobustTravelIndicatorsHelper* pRobustTravelSpeedsHelper = NULL;

				double TEForAllMacroTypes = 0;

				if (iterRobustTSHelper != m_mapRobustTravelSpeedsHelper.end())
				{
					pRobustTravelTimesHelper = iterRobustTTHelper->second;
					pRobustTravelSpeedsHelper = iterRobustTSHelper->second;
				}

				//if (!bPollutantEmissionComputation && !pRobustTravelTimesHelper && !bMouvementInterdit)
				if(!pRobustTravelTimesHelper && !bMouvementInterdit)
				{
					double Tmin, Tmax, maxSpatialVehNumber, Vmax;
					maxSpatialVehNumber = m_pReseau->GetMainKx() * dbLength;
					Tmin = dbLength / m_pReseau->GetMaxVitMax();
					Tmax = dbLength / m_pReseau->GetMinSpeedForTravelTime();
					Vmax = dbLength / Tmin;

					pRobustTravelTimesHelper = new SymuCore::RobustTravelIndicatorsHelper(SymuCore::RobustTravelIndicatorsHelper::travelindicator::time, m_pReseau->GetNbClassRobustTTMethod(), maxSpatialVehNumber, Tmin, Tmax, dbLength, m_pReseau->GetMaxNbPointsPerClassRobustTTMethod(), m_pReseau->GetMinNbPointsStatisticalFilterRobustTTMethod());
					m_mapRobustTravelTimesHelper.insert(std::pair<std::pair<Tuyau*, Tuyau*>, SymuCore::RobustTravelIndicatorsHelper*>(iterMove->first, pRobustTravelTimesHelper));

					pRobustTravelSpeedsHelper = new SymuCore::RobustTravelIndicatorsHelper(SymuCore::RobustTravelIndicatorsHelper::travelindicator::speed, m_pReseau->GetNbClassRobustTTMethod(), maxSpatialVehNumber, Vmax, m_pReseau->GetMinSpeedForTravelTime(), dbLength, m_pReseau->GetMaxNbPointsPerClassRobustTTMethod(), m_pReseau->GetMinNbPointsStatisticalFilterRobustTTMethod());
					m_mapRobustTravelSpeedsHelper.insert(std::pair<std::pair<Tuyau*, Tuyau*>, SymuCore::RobustTravelIndicatorsHelper*>(iterMove->first, pRobustTravelSpeedsHelper));
				}

				//std::cout << "AddTravelIndicatorsData Link: ;" << this->GetID();
				if (!bMouvementInterdit)
				{
					if (/*!bPollutantEmissionComputation &&*/ !pRobustTravelTimesHelper->IsPreComputed())
					{
						pRobustTravelTimesHelper->AddTravelIndicatorsData(dbMeanVehicleNbForMacroType, dbTotalTravelledTime, dbTotalTravelledDistance);
						pRobustTravelSpeedsHelper->AddTravelIndicatorsData(dbMeanVehicleNbForMacroType, dbTotalTravelledTime, dbTotalTravelledDistance);
					
						if (m_pReseau->GetRobustPointsBackupStream() && dbMeanVehicleNbForMacroType > 0.)
						{
							*(m_pReseau->GetRobustPointsBackupStream()) << dbInstFinPeriode << ";";
							*(m_pReseau->GetRobustPointsBackupStream()) << iterMove->first.first->GetLabel() << "->" << iterMove->first.second->GetLabel() << ";";
							*(m_pReseau->GetRobustPointsBackupStream()) << dbMeanVehicleNbForMacroType << ";";
							*(m_pReseau->GetRobustPointsBackupStream()) << dbTotalTravelledTime << ";";
							*(m_pReseau->GetRobustPointsBackupStream()) << dbTotalTravelledDistance << ";" << std::endl;
						}
					}

					//TTForMacroType = pRobustTravelTimesHelper->GetRobustTravelIndicator(dbMeanVehicleNbForMacroType, dbTotalTravelledDistance);
				
					//if(!bPollutantEmissionComputation)
					TTForMacroType = pRobustTravelTimesHelper->GetRobustTravelIndicator(dbMeanVehicleNbForMacroType, dbTotalTravelledDistance, false);
					//TTForMacroType = pRobustTravelSpeedsHelper->GetRobustTravelIndicator(dbMeanVehicleNbForMacroType, dbTotalTravelledDistance, false);

					TEForAllMacroTypes = pRobustTravelTimesHelper->GetRobustTravelIndicator(dbMeanVehicleNbForMacroType, dbTotalTravelledDistance, true);
					//TEForAllMacroTypes = pRobustTravelSpeedsHelper->GetRobustTravelIndicator(dbMeanVehicleNbForMacroType, dbTotalTravelledDistance, true);
				
					
				}
				else
					TTForMacroType = std::numeric_limits<double>::infinity();
					

				//std::cout << std::endl;
				//std::cout << ";" << TTForMacroType;

				// Pour l'instant !
				m_dbTTForAllMacroTypes[iterMove->first] = TTForMacroType;

				if (!bPollutantEmissionComputation)
					m_dbTTForAllMacroTypes[iterMove->first] = TTForMacroType;
				else
				{
					m_dbEmissionsForAllMacroTypes[iterMove->first] = TEForAllMacroTypes;
					m_dbTTForAllMacroTypes[iterMove->first] = TTForMacroType;
				}

				// Marginals 
				if (!bMouvementInterdit)
				{
					double dbITT;

					if (!bPollutantEmissionComputation)
					{
						dbITT = pRobustTravelTimesHelper->GetDerivative(dbMeanVehicleNbForMacroType, false);
						dbMarginalForMacroType = m_dbTTForAllMacroTypes[iterMove->first] + dbITT * dbMeanVehicleNbForMacroType;
						
					}
					else
					{
						//dbITT = pRobustTravelSpeedsHelper->GetDerivative(dbMeanVehicleNbForMacroType, true);
						//dbMarginalForMacroType = m_dbEmissionsForAllMacroTypes[iterMove->first] + dbITT * dbMeanVehicleNbForMacroType;
						dbITT = pRobustTravelTimesHelper->GetLinkMarginalEmission(dbMeanVehicleNbForMacroType);
						dbMarginalForMacroType = dbITT;
						//dbMarginalForMacroType = m_dbEmissionsForAllMacroTypes[iterMove->first] + dbITT * dbMeanVehicleNbForMacroType;
					}
						
				}
				else
					dbMarginalForMacroType = std::numeric_limits<double>::infinity();

				//std::cout << ";" << dbITT << ";" << dbMarginalForMacroType << ";" << std::endl;

			}

		}
		else
		{
			std::map<int, std::pair< TypeVehicule*, std::pair<double, double> > >::const_iterator itVeh;
			std::map<double, std::map<int, std::pair<bool, std::pair<double, double> > > > mapUsefuleVehicles;
			double dbInstSortie;
			for (itVeh = mapVehForMove.begin(); itVeh != mapVehForMove.end(); ++itVeh)
			{
				dbInstSortie = (*itVeh).second.second.first;
				if (dbInstSortie > 0)	// Véhicule du bon macrotype et sorti
				{
					mapUsefuleVehicles[dbInstSortie][itVeh->first] = std::make_pair(pMacroType->hasVehicleType(itVeh->second.first->GetLabel()), std::make_pair((*itVeh).second.second.second, 1.0)); // rmq. : on se fiche du ratio 1.0 ici car la longueur du mouvement est identique pour tous les véhicules
				}
			}

			double dbITT;
			std::vector<std::pair<std::pair<double, double>, double> > exitInstantAndTravelTimesWithLengthsForMacroType, exitInstantAndTravelTimesWithLengthsForAllMacroTypes;
			std::vector<int> vehicleIDsToClean;
			SymuCore::TravelTimeUtils::SelectVehicleForTravelTimes(mapUsefuleVehicles, dbInstFinPeriode, dbPeriodDuration, m_pReseau->GetNbPeriodsForTravelTimes(), m_pReseau->GetNbVehiclesForTravelTimes(), m_pReseau->GetMarginalsDeltaN(), m_pReseau->UseMarginalsMeanMethod(), m_pReseau->UseMarginalsMedianMethod(), dbITT, exitInstantAndTravelTimesWithLengthsForMacroType, exitInstantAndTravelTimesWithLengthsForAllMacroTypes, vehicleIDsToClean);

			// Nettoyage des véhicules devenus trop anciens :
			for (size_t iVeh = 0; iVeh < vehicleIDsToClean.size(); iVeh++)
			{
				mapVehForMove.erase(vehicleIDsToClean[iVeh]);
			}

			// Calcul du temps de parcours
			TTForMacroType = SymuCore::TravelTimeUtils::ComputeTravelTime(exitInstantAndTravelTimesWithLengthsForMacroType, dbMeanTotalConcentration, dbTpsVitReg, bMouvementInterdit, m_pReseau->GetNbVehiclesForTravelTimes(), m_pReseau->GetConcentrationRatioForFreeFlowCriterion(), m_pReseau->GetMainKx());

			// Calcul du marginal :
			// Pour les marginals, on prend tous les dernièrs véhicules sortis du tronçon quelquesoit leur macrotype : 
			dbMarginalForMacroType = SymuCore::TravelTimeUtils::ComputeTravelTime(exitInstantAndTravelTimesWithLengthsForAllMacroTypes, dbMeanTotalConcentration, dbTpsVitReg, bMouvementInterdit, m_pReseau->GetNbVehiclesForTravelTimes(), m_pReseau->GetConcentrationRatioForFreeFlowCriterion(), m_pReseau->GetMainKx());
			// Le dbITT est aussi pour tous les macrotypes : seul le nombre de véhicules sur le tronçon est pris pour le macro-type considéré.
			dbMarginalForMacroType += dbITT * dbMeanVehicleNbForMacroType;
		}
	} // Fin de la boucle sur les mouvements de la brique
}

double BriqueDeConnexion::GetMeanNbVehiclesForTravelTimeUpdatePeriod(SymuCore::MacroType * pMacroType, Tuyau* pTuyauAmont, Tuyau * pTuyauAval)
{
    std::map<std::pair<Tuyau*, Tuyau*>, std::map<SymuCore::MacroType*, double> >::const_iterator iter = m_dbMeanNbVehiclesForTravelTimeUpdatePeriod.find(std::make_pair(pTuyauAmont, pTuyauAval));
    if (iter != m_dbMeanNbVehiclesForTravelTimeUpdatePeriod.end())
    {
        std::map<SymuCore::MacroType*, double>::const_iterator iter2 = iter->second.find(pMacroType);
        if (iter2 != iter->second.end())
        {
            return iter2->second;
        }
    }
    // pas encore calculé : 0.
    return 0;
}

double BriqueDeConnexion::ComputeCost(SymuCore::MacroType * pMacroType, Tuyau * pUpstreamLink, Tuyau * pDownstreamLink)
{
    double result;

    bool bFound = false;
    std::map<std::pair<Tuyau*, Tuyau*>, std::map<SymuCore::MacroType*, double> >::const_iterator iterMacroType = m_mapTTByMacroType.find(std::make_pair(pUpstreamLink, pDownstreamLink));

    if (iterMacroType != m_mapTTByMacroType.end())
    {
        std::map<SymuCore::MacroType*, double>::const_iterator iter = iterMacroType->second.find(pMacroType);

        if (iter != iterMacroType->second.end())
        {
            bFound = true;

            result = iter->second;
        }
    }

    TypeVehicule * pTypeVeh = m_pReseau->GetVehiculeTypeFromMacro(pMacroType);
    bool bMouvementInterdit = !pTypeVeh || !IsMouvementAutorise(pUpstreamLink, pDownstreamLink, pTypeVeh, NULL);

    if (!bFound)
    {
        // On prend le cout à vide, auquel on applique la vitesse minimum le cas échéant
        result = bMouvementInterdit ? std::numeric_limits<double>::infinity() : m_LstCoutsAVide.at(std::make_pair(pUpstreamLink, pDownstreamLink)).at(pTypeVeh);
    }

    // Application de la vitesse minimum le cas échéant :
    if (m_pReseau->GetMinSpeedForTravelTime() > 0 && !bMouvementInterdit)
    {
        double dbLength = 0;
        std::vector<Tuyau*> tuyauxInternes;
        GetTuyauxInternes(pUpstreamLink, pDownstreamLink, tuyauxInternes);
        for (size_t iLink = 0; iLink < tuyauxInternes.size(); iLink++)
        {
            dbLength += tuyauxInternes[iLink]->GetLength();
        }
        result = std::min<double>(result, dbLength / m_pReseau->GetMinSpeedForTravelTime());
    }

    return result;
}

double BriqueDeConnexion::GetMarginal(SymuCore::MacroType* pMacroType, Tuyau* pUpstreamLink, Tuyau * pDownstreamLink)
{
    double result;

    bool bFound = false;
    std::map<std::pair<Tuyau*, Tuyau*>, std::map<SymuCore::MacroType*, double> >::const_iterator iterMacroType = m_mapMarginalByMacroType.find(std::make_pair(pUpstreamLink, pDownstreamLink));

    if (iterMacroType != m_mapMarginalByMacroType.end())
    {
        std::map<SymuCore::MacroType*, double>::const_iterator iter = iterMacroType->second.find(pMacroType);

        if (iter != iterMacroType->second.end())
        {
            bFound = true;

            result = iter->second;
        }
    }

    if (!bFound)
    {
        // Cas à vide : le marginals est le temps de parcours
        result = ComputeCost(pMacroType, pUpstreamLink, pDownstreamLink);
    }

    // Application du marginals minimum
    TypeVehicule * pTypeVeh = m_pReseau->GetVehiculeTypeFromMacro(pMacroType);
    bool bMouvementInterdit = !pTypeVeh || !IsMouvementAutorise(pUpstreamLink, pDownstreamLink, pTypeVeh, NULL);
    if (!bMouvementInterdit)
    {
        result = std::min<double>(result, m_pReseau->GetMaxMarginalsValue());
    }
    result = std::max<double>(0, result); // Pas de marginals négatifs

    return result;
}

#endif // USE_SYMUCORE

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void BriqueDeConnexion::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void BriqueDeConnexion::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void BriqueDeConnexion::serialize(Archive & ar, const unsigned int version)
{
    // classe de base
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Connexion);

    ar & BOOST_SERIALIZATION_NVP(m_cType);
    ar & BOOST_SERIALIZATION_NVP(m_dbVitMax);
    ar & BOOST_SERIALIZATION_NVP(m_bSimulationTraversees);
    //ar & BOOST_SERIALIZATION_NVP(m_XmlNodeRepartitionDestination); // rmq : utile uniquement au moment de l'init du carrefour à feux : on se permet de ne pas le serializer
    ar & BOOST_SERIALIZATION_NVP(m_LstTfTra);
    ar & BOOST_SERIALIZATION_NVP(m_LstTfCvg);
    ar & BOOST_SERIALIZATION_NVP(m_LstPtsConflitTraversee);
    ar & BOOST_SERIALIZATION_NVP(m_pCellSirane);
    ar & BOOST_SERIALIZATION_NVP(m_mapVeh);
    ar & BOOST_SERIALIZATION_NVP(m_LstCoutsMesures);
}
