#include "stdafx.h"
#include "SymuCoreDrivingTripsBuilder.h"

#include "SymuCoreDrivingGraphBuilder.h"
#include "SymuCoreGraphBuilder.h"
#include "reseau.h"
#include "RandManager.h"
#include "ZoneDeTerminaison.h"
#include "Plaque.h"
#include "usage/SymuViaTripNode.h"


#include <Graph/Model/MultiLayersGraph.h>

#include <Demand/Trip.h>
#include <Demand/Population.h>
#include <Demand/MacroType.h>
#include <Demand/Populations.h>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <boost/log/trivial.hpp>

#include <boost/math/special_functions/round.hpp>

#include <limits>


using namespace SymuCore;

SymuCoreDrivingTripsBuilder::SymuCoreDrivingTripsBuilder()
: m_pNetwork(NULL)
{
}

SymuCoreDrivingTripsBuilder::SymuCoreDrivingTripsBuilder(Reseau * pNetwork)
: m_pNetwork(pNetwork)
{
}


SymuCoreDrivingTripsBuilder::~SymuCoreDrivingTripsBuilder()
{
}

void SymuCoreDrivingTripsBuilder::AddTrip(std::vector<SymuCore::Trip> & symuViaTrips, MultiLayersGraph * pGraph, const boost::posix_time::ptime & simulationStartTime, double dbCreationTime, SymuViaTripNode * pOrigin, SymuViaTripNode * pDestination, TypeVehicule * pTypeVeh, VehicleType * pVehType, SubPopulation * pSubPopulationForTrip,
    bool bIgnoreSubAreas)
{
    double dbPlaceHolder;
    bool bPlaceHolder;
    Tuyau * pPlaceHolder, * pPlaceHolder2;

    // Tirage du motif pour l'utilisateur :
    CMotifCoeff * pMotif = pOrigin->GetMotif(dbCreationTime, pTypeVeh, pDestination);

    // Cas particulier des zones : on tire une plaque si possible
    ZoneDeTerminaison * pZoneOrigin = dynamic_cast<ZoneDeTerminaison*>(pOrigin);
    SymuCore::Origin * pSymuCoreOrigin = NULL;
    // on évite l'étape très couteuse du tirage de la plaque si de toute façon on ne s'en sert pas après
    if (pZoneOrigin && !bIgnoreSubAreas)
    {
        CPlaque * pPlaqueOrigin;
        Parking * pParkingOrigin;
        pZoneOrigin->TiragePlaqueOrigine(dbCreationTime, NULL, pTypeVeh, pMotif, pDestination, NULL, NULL, false, dbPlaceHolder, pPlaqueOrigin, bPlaceHolder, pParkingOrigin, bPlaceHolder);
        if (pPlaqueOrigin)
        {
            pSymuCoreOrigin = pGraph->GetOrCreateOrigin(pPlaqueOrigin->GetID());
        }
    }
    if (pSymuCoreOrigin == NULL)
    {
        pSymuCoreOrigin = pGraph->GetOrCreateOrigin(pOrigin->GetID());
    }

    ZoneDeTerminaison * pZoneDestination = dynamic_cast<ZoneDeTerminaison*>(pDestination);
    SymuCore::Destination * pSymuCoreDestination = NULL;
    // on évite l'étape très couteuse du tirage de la plaque si de toute façon on ne s'en sert pas après
    if (pZoneDestination && !bIgnoreSubAreas)
    {
        CPlaque * pPlaqueDestination;
        pZoneDestination->TiragePlaqueDestination(dbCreationTime, pMotif, NULL, pOrigin, pTypeVeh, pPlaqueDestination, pPlaceHolder, pPlaceHolder2);
        if (pPlaqueDestination)
        {
            pSymuCoreDestination = pGraph->GetOrCreateDestination(pPlaqueDestination->GetID());
        }
    }
    if (pSymuCoreDestination == NULL)
    {
        pSymuCoreDestination = pGraph->GetOrCreateDestination(pDestination->GetID());
    }

    SymuCore::Trip newTrip(
        -1,
        simulationStartTime + boost::posix_time::microseconds((int64_t)boost::math::round(dbCreationTime * 1000000)),    // departure time
        pSymuCoreOrigin,                                                                                    // origin
        pSymuCoreDestination,                                                                               // destination
        pSubPopulationForTrip,                                                                              // population
        pVehType);                                                                                          // vehicle type

    symuViaTrips.push_back(newTrip);
}

bool SymuCoreDrivingTripsBuilder::FillPopulations(MultiLayersGraph * pGraph, Populations & populations,
    const boost::posix_time::ptime & startSymuMasterSimulationTime, const boost::posix_time::ptime & endSymuMasterSimulationTime,
    bool bIgnoreSubAreas, std::vector<SymuCore::Trip> & lstTrips)
{
    // on fait en sorte que cette opération soit transparente d'un point de vue de la génération des nombre aléatoires
    // afin qu'une simulation SymuMaster qui réutilise un fichier CSV déjà écrit produise les mêmes résultats :
    unsigned int nRandCountSvg = m_pNetwork->GetRandManager()->getCount();

    boost::posix_time::ptime simulationStartTime = m_pNetwork->GetSimulationStartTime();

    double dbStartSymuMasterInstant = (double)((startSymuMasterSimulationTime - simulationStartTime).total_microseconds()) / 1000000.0;
    double dbEndSymuMasterInstant = (double)((endSymuMasterSimulationTime - simulationStartTime).total_microseconds()) / 1000000.0;
    double dbEndSimulationInstant = std::min<double>(m_pNetwork->GetDureeSimu(), dbEndSymuMasterInstant);

    // Stratégie : on traite indépendamment chaque origine, puis on merge les Trips en les ordonnant par instant de départ.
    for (size_t iOrigin = 0; iOrigin < m_pNetwork->Liste_origines.size(); iOrigin++)
    {
        SymuViaTripNode * pOrigin = m_pNetwork->Liste_origines[iOrigin];

        if (pOrigin->IsTypeDemande())
        {
            // On commence par déterminer les instants de création :
            std::vector<double> tripInstants;

            double dbCurrentTime = 0.0;
            double dbCriterion = 0;
            double dbNextCreationTime;
            double dbDemandValue = DBL_MAX;
            double dbEndDemandVariationTime = DBL_MAX;
            double dbEndTime = DBL_MAX;
            bool bEnd = false;
            while (!bEnd)
            {
                // rmk : +1 microsec to take care of overlapping instants between two demand variations
                dbDemandValue = pOrigin->GetDemandeValue(dbCurrentTime + 0.000001, dbEndDemandVariationTime);

                // Calcul de la durée avec cette valeur de demande pour atteindre dbCriterion == 1 :
                if (dbDemandValue > 0)
                {
                    if (!pOrigin->IsTypeDistribution())
                    {
                        dbNextCreationTime = dbCurrentTime + (1.0 - dbCriterion) / dbDemandValue;
                    }
                    else
                    {
                        dbNextCreationTime = dbCurrentTime + (1.0 - dbCriterion) * m_pNetwork->GetRandManager()->myExponentialRand(dbDemandValue);
                    }
                }
                else
                {
                    dbNextCreationTime = DBL_MAX;
                }

                dbEndTime = std::min<double>(dbEndDemandVariationTime, dbEndSimulationInstant);

                if (dbNextCreationTime <= dbEndTime)
                {
                    // création d'un véhicule (sauf si avant le début de la simulation SYmuMaster
                    if (dbNextCreationTime >= dbStartSymuMasterInstant)
                    {
                        tripInstants.push_back(dbNextCreationTime);
                    }
                    dbCriterion = 0;
                    dbCurrentTime = dbNextCreationTime;
                }
                else
                {
                    dbCriterion += (dbEndTime - dbCurrentTime) * dbDemandValue;
                    dbCurrentTime = dbEndTime;
                }

                if (dbCurrentTime >= dbEndSimulationInstant)
                {
                    bEnd = true;
                }
            }

            // Pour chaque instant de création, définition des autres éléments du Trip (type de véhicule, origine, destination,...) :
            for (size_t iTripInstant = 0; iTripInstant < tripInstants.size(); iTripInstant++)
            {
                double dbCreationTime = tripInstants[iTripInstant];

                TypeVehicule * pTypeVeh = pOrigin->CalculTypeNewVehicule(dbCreationTime, pOrigin->CalculNumVoie(dbCreationTime));

                Population * pPopulationForTrip;
                SubPopulation * pSubPopulationForTrip;
                VehicleType * pVehType;
                populations.getPopulationAndVehicleType(&pPopulationForTrip, &pSubPopulationForTrip, &pVehType, pTypeVeh->GetLabel());

                if (pPopulationForTrip == NULL)
                {
                    BOOST_LOG_TRIVIAL(error) << "No population for vehicle type " << pTypeVeh->GetLabel();
                    return false;
                }

                if (!pSubPopulationForTrip)
                {
                    if (!pSubPopulationForTrip && pPopulationForTrip->GetListSubPopulations().size() != 1)
                    {
                        BOOST_LOG_TRIVIAL(error) << "Too much possibilities to select SubPopulation for Population " << pPopulationForTrip->GetStrName();
                        return false;
                    }
                    else
                    {
                        pSubPopulationForTrip = pPopulationForTrip->GetListSubPopulations()[0];
                    }
                }
                

                // voie 0 en dur mais pas utilisée par la fonction GetDestination donc OK
                SymuViaTripNode * pDestination = pOrigin->GetDestination(dbCreationTime, 0, pTypeVeh);

                AddTrip(lstTrips, pGraph, simulationStartTime, dbCreationTime, pOrigin, pDestination, pTypeVeh, pVehType, pSubPopulationForTrip, bIgnoreSubAreas);
            }
        }
        else
        {
            // Cas d'une liste de véhicules (facile !)
            const std::deque<CreationVehicule> & lstCreations = pOrigin->GetLstCreationsVehicule();
            for (size_t iCreation = 0; iCreation < lstCreations.size(); iCreation++)
            {
                const CreationVehicule & creation = lstCreations[iCreation];

                if (creation.dbInstantCreation < dbStartSymuMasterInstant)
                {
                    continue;
                }

                // creation is time ordered. we stop the loop as soon as the endPeriodTime is reached
                if (creation.dbInstantCreation >= dbEndSimulationInstant)
                {
                    break;
                }

                Population * pPopulationForTrip;
                SubPopulation * pSubPopulationForTrip;
                VehicleType * pVehType;
                populations.getPopulationAndVehicleType(&pPopulationForTrip, &pSubPopulationForTrip, &pVehType, creation.pTypeVehicule->GetLabel());
                if (pPopulationForTrip == NULL)
                {
                    BOOST_LOG_TRIVIAL(error) << "No population for vehicle type " << creation.pTypeVehicule->GetLabel();
                    return false;
                }

                if (!pSubPopulationForTrip)
                {
                    if (pPopulationForTrip->GetListSubPopulations().size() != 1)
                    {
                        BOOST_LOG_TRIVIAL(error) << "Too much possibilities to select SubPopulation for Population " << pPopulationForTrip->GetStrName();
                        return false;
                    }
                    else
                    {
                        pSubPopulationForTrip = pPopulationForTrip->GetListSubPopulations()[0];
                    }
                }

                SymuCore::Destination * pSymuCoreDestination = pGraph->GetOrCreateDestination(creation.pDest->GetID());

                AddTrip(lstTrips, pGraph, simulationStartTime, creation.dbInstantCreation, pOrigin, creation.pDest, creation.pTypeVehicule, pVehType, pSubPopulationForTrip, bIgnoreSubAreas);
            }
        }
    }

    m_pNetwork->RestoreSeed(nRandCountSvg);

    return true;
}
