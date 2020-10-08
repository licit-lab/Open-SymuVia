#ifdef USE_MATLAB_MFD

#include "MFDInstance.h"

#include "Simulation/Simulators/TraficSimulatorHandler.h"
#include "MFDConfig.h"
#include "MFDDrivingPattern.h"
#include "MFDHandler.h"
#include "MFDInterface.h"

#include "Simulation/Users/PPath.h"
#include "Simulation/Config/SimulationConfiguration.h"

#include <Graph/Model/MultiLayersGraph.h>
#include <Graph/Model/Node.h>
#include <Graph/Model/Link.h>
#include <Graph/Model/Driving/DrivingPattern.h>
#include <Graph/Model/NullPenalty.h>

#include <Demand/Population.h>
#include <Demand/Populations.h>
#include <Demand/Trip.h>
#include <math.h>

#include <Users/IUserHandler.h>

#include <Utils/Point.h>

#include <MatlabEngine.hpp>
#include <MatlabDataArray.hpp>

#include <boost/log/trivial.hpp>
#include <boost/math/special_functions.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

using namespace SymuMaster;
using namespace SymuCore;

using namespace matlab::engine;

MFDInstance::MFDInstance(TraficSimulatorHandler * pParent, int iInstance) :
    TraficSimulatorInstance(pParent, iInstance),
    m_bHasStartPeriodSnapshot(false),
    m_bAccBased(false)
{
}

MFDInstance::~MFDInstance()
{

}

const std::string & MFDInstance::GetSimulatorName() const
{
    static const std::string MFDName = "MFD";
    return MFDName;
}

bool MFDInstance::Initialize(const SimulationConfiguration & config, SymuCore::IUserHandler * pUserHandler, TraficSimulatorHandler * pMasterSimulator)
{
    m_pUserHandler = pUserHandler;
    MFDConfig * pConfig = (MFDConfig*)m_pParentSimulationHandler->GetSimulatorConfiguration();

    try {
        // Start matlab engine
        m_pMatlabEngine = startMATLAB();

        // Set the workspace folder to the MFD simulator matlab folder (it includes the network definitions)
        matlab::data::ArrayFactory factory;
        std::vector<matlab::data::Array> args({factory.createCharArray(pConfig->GetMatlabWorkspacePath().c_str())});
        m_pMatlabEngine->feval(convertUTF8StringToUTF16String("cd"), args);

		//Clear all previous information on MatLab Workspace;
		m_pMatlabEngine->eval(convertUTF8StringToUTF16String("clear all"));

        // Load simulation settings and reservoirs definition
		m_pMatlabEngine->setVariable(convertUTF8StringToUTF16String("OutPutDirectory"), std::move(factory.createCharArray(config.GetStrOutpuDirectoryPath())));
		m_pMatlabEngine->eval(convertUTF8StringToUTF16String("Init_SymuMaster"));

        matlab::data::StructArray simulationArray = m_pMatlabEngine->getVariable("Simulation");
        matlab::data::Struct simulation = simulationArray[0];

        matlab::data::TypedArray<double> solverArray = simulation["Solver"];
        m_bAccBased = solverArray[0] == 1;

        // En fait, ce pas de temps n'est utilisé que pour le accbased. En trip based, pas besoin de pas de temps, si ce n'est pour découper l'exécution
        // et récupérer les utilisateurs sortants entre ces périodes.

        // TimeStep variable
        boost::posix_time::time_duration timeStep;
        if (m_bAccBased)
        {
            matlab::data::TypedArray<double> timeStepArray = simulation["TimeStep"];
            double dbTimeStep = timeStepArray[0];
            timeStep = boost::posix_time::microseconds((int64_t)boost::math::round(dbTimeStep*1000000.0));
        }
        else
        {
            timeStep = pMasterSimulator->GetSimulationDescriptor().GetTimeStepDuration();
        }
        BOOST_LOG_TRIVIAL(debug) << "timeStep : " << timeStep;

        // Duration variable
        matlab::data::TypedArray<double> durationArray = simulation["Duration"];
        double dbSimulationDuration = durationArray[0];
        BOOST_LOG_TRIVIAL(debug) << "dbSimulationDuration : " << dbSimulationDuration;

        

        boost::gregorian::date startDate = boost::gregorian::date(1970, 1, 1);
        boost::posix_time::ptime startTime = boost::posix_time::ptime(startDate, boost::posix_time::time_duration(0, 0, 0));

        boost::posix_time::ptime endTime = startTime + boost::posix_time::microseconds((int64_t)boost::math::round(dbSimulationDuration*1000000.0));

        m_pParentSimulationHandler->GetSimulationDescriptor().SetStartTime(startTime);
        m_pParentSimulationHandler->GetSimulationDescriptor().SetEndTime(endTime);
        m_pParentSimulationHandler->GetSimulationDescriptor().SetTimeStepDuration(timeStep);
    }
    catch (matlab::execution::Exception & ex)
    {
		BOOST_LOG_TRIVIAL(error) << ex.what();
        return false;
    }

    return true;
}

bool MFDInstance::BuildSimulationGraph(SymuCore::MultiLayersGraph *pParentGraph, SymuCore::MultiLayersGraph *pSimulatorGraph, bool bIsPrimaryGraph)
{
    try {

        Graph * pDrivingLayer = m_pParentSimulationHandler->GetSimulationDescriptor().GetGraph()->CreateAndAddLayer(ST_Driving);

        std::map<int, Node*> mapNodes;

        std::vector<Node*> lstOrigins, lstDestinations;

        matlab::data::StructArray macronodes = m_pMatlabEngine->getVariable("MacroNode");
		BOOST_LOG_TRIVIAL(debug) << "Number of nodes : " << macronodes.getNumberOfElements();

        matlab::data::StructArray routes = m_pMatlabEngine->getVariable("Route");
        BOOST_LOG_TRIVIAL(debug) << "Number of routes : " << routes.getNumberOfElements();

        // Create a node for each macronode
		for (size_t iNodes = 0; iNodes < macronodes.getNumberOfElements(); iNodes++)
        {
			int macronodeID = (int)iNodes + 1;
			matlab::data::Struct macronode = macronodes.operator[](iNodes);

			matlab::data::CharArray type = macronode["Type"];
			std::string strType = type.toAscii();
			if (strType == "border"){
				continue;
			}

			matlab::data::Array coordinate = macronode["Coord"];

			Node * pNode = pDrivingLayer->CreateAndAddNode(std::to_string(macronodeID), NT_MacroNode, &Point(coordinate[0], coordinate[1]));

			mapNodes[macronodeID] = pNode;

            if (pDrivingLayer->getOrigin(pNode->getStrName()))
            {
                lstOrigins.push_back(pNode);
            }
            if (pDrivingLayer->getDestination(pNode->getStrName()))
            {
                lstDestinations.push_back(pNode);
            }
        }

        // Create a pattern for each route
        for (std::map<int, Node*>::const_iterator iter = mapNodes.begin(); iter != mapNodes.end(); ++iter)
        {
            std::map<int, std::vector<int> > routesByDestination;
            for (size_t iRoute = 0; iRoute < routes.getNumberOfElements(); iRoute++)
            {
                matlab::data::Struct route = routes.operator[](iRoute);
                int iOriginID = route["NodeOriginID"][0];
                if (iOriginID == iter->first)
                {
                    int iDestinationID = route["NodeDestinationID"][0];
                    routesByDestination[iDestinationID].push_back((int)iRoute + 1);
                }
            }

            for (std::map<int, std::vector<int> >::const_iterator iterDest = routesByDestination.begin(); iterDest != routesByDestination.end(); ++iterDest)
            {
                Link * pNewLink = pDrivingLayer->CreateAndAddLink(iter->second, mapNodes.at(iterDest->first));

                for (size_t iRoute = 0; iRoute < iterDest->second.size(); iRoute++)
                {
                    pNewLink->getListPatterns().push_back(new MFDDrivingPattern(pNewLink, iterDest->second.at(iRoute), (MFDHandler*)m_pParentSimulationHandler));
                }
            }
        }

        if (bIsPrimaryGraph)
        {
            for (size_t iOrigin = 0; iOrigin < lstOrigins.size(); iOrigin++)
            {
                m_pParentSimulationHandler->GetSimulationDescriptor().GetGraph()->CreateLinkToOrigin(lstOrigins[iOrigin]);
            }
            for (size_t iDestination = 0; iDestination < lstDestinations.size(); iDestination++)
            {
                m_pParentSimulationHandler->GetSimulationDescriptor().GetGraph()->CreateLinkToDestination(lstDestinations[iDestination]);
            }
        }
    }
    catch (matlab::execution::Exception & ex)
    {
		BOOST_LOG_TRIVIAL(error) << ex.what();
        return false;
    }

    return true;
}

bool MFDInstance::Step(const boost::posix_time::ptime & startTimeStep, const boost::posix_time::time_duration & timeStepDuration)
{
    try
    {
        m_pMatlabEngine->eval(convertUTF8StringToUTF16String("SolveStep"));

        // Récupération des usagers terminés
        matlab::data::Array arrTerminatedUsers = m_pMatlabEngine->getVariable("TerminatedUsers");
        if (!arrTerminatedUsers.isEmpty())
        {
            matlab::data::StructArray terminatedUsers = (matlab::data::StructArray)arrTerminatedUsers;
            for (size_t iUser = 0; iUser < terminatedUsers.getNumberOfElements(); iUser++)
			{
                matlab::data::Struct structUser = terminatedUsers[iUser];

                auto idField = structUser["ID"];
                matlab::data::ArrayType idType = idField.getType();
                int id = (int)idField[0];
                
                matlab::data::TypedArray<double> arrayFracTime = structUser["exitTimeFraction"];
                double dbFractionnalTime = arrayFracTime[0];

                m_pUserHandler->OnPPathCompleted(id, dbFractionnalTime);
            }
        }
    }
    catch (matlab::execution::Exception & ex)
    {
		BOOST_LOG_TRIVIAL(error) << ex.what();
        return false;
    }

    return true;
}

SymuCore::Origin * MFDInstance::GetParentOrigin(SymuCore::Origin * pChildOrigin)
{
    return NULL;
}

SymuCore::Destination * MFDInstance::GetParentDestination(SymuCore::Destination * pChildDestination)
{
    return NULL;
}

bool MFDInstance::FillTripsDescriptor(SymuCore::MultiLayersGraph * pGraph, SymuCore::Populations & populations,
    const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & endSimulationTime, bool bIgnoreSubAreas,
    std::vector<Trip> & lstTrips)
{
    try {

        matlab::data::StructArray odMacros = m_pMatlabEngine->getVariable("ODmacro");

		for (size_t iMacro = 0; iMacro < odMacros.getNumberOfElements(); iMacro++)
        {
			matlab::data::Struct odMacro = odMacros.operator[](iMacro);
			matlab::data::StructArray demandArray = odMacro["Demand"];
			int iOriginID = odMacro["NodeOriginID"][0];
			int iDestinationID = odMacro["NodeDestinationID"][0];

            for (int iPurpose = 0; iPurpose < demandArray.getNumberOfElements(); iPurpose++)
            {
                matlab::data::Struct demandStruct = demandArray[iPurpose];
                matlab::data::CharArray purpose = demandStruct["Purpose"];
                std::string strPurpose = purpose.toAscii();

                // determine creation instants :
                matlab::data::ArrayFactory factory;
                matlab::data::StructArray args = factory.createStructArray({ 1, 1 }, std::vector<std::string>({ "Tsimul", "dN", "tinD", "qinD" }));
                args[0]["Tsimul"] = factory.createScalar<double>(((double)(m_pParentSimulationHandler->GetSimulationDescriptor().GetEndTime() - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds()) / 1000000);
                args[0]["dN"] = factory.createScalar<double>(1);
				args[0]["tinD"] = demandStruct["Time"];
				args[0]["qinD"] = demandStruct["Data"];;

                matlab::data::TypedArray<double> creationTimes = m_pMatlabEngine->feval("demdiscr_bis", args);

                for (size_t iTripInstant = 0; iTripInstant < creationTimes.getNumberOfElements(); iTripInstant++)
                {
                    double dbCreationTime = creationTimes[iTripInstant];

                    Population * pPopulationForTrip;
                    SubPopulation * pSubPopulationForTrip;
                    VehicleType * pVehType;

                    populations.getPopulationAndVehicleType(&pPopulationForTrip, &pSubPopulationForTrip, &pVehType, std::string(), strPurpose);

                    if (pPopulationForTrip == NULL)
                    {
						BOOST_LOG_TRIVIAL(error) << "No population found";
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
                            pSubPopulationForTrip = pPopulationForTrip->GetListSubPopulations().front();
                        }
                    }
                        

                    Trip newTrip(
                        -1,
                        startSimulationTime + boost::posix_time::microseconds((int64_t)boost::math::round(dbCreationTime * 1000000)),   // departure time
						pGraph->GetOrCreateOrigin(std::to_string(iOriginID)),                                                           // origin
						pGraph->GetOrCreateDestination(std::to_string(iDestinationID)),                                                 // destination
                        pSubPopulationForTrip,                                                                                          // population
                        pVehType);                                                                                                      // vehicle type

                    lstTrips.push_back(newTrip);
                }
            }
        }
    }
    catch (matlab::execution::Exception & ex)
    {
		BOOST_LOG_TRIVIAL(error) << ex.what();
        return false;
    }

    return true;
}

bool MFDInstance::ComputeNewPeriodCosts(const std::map<SubPopulation*, CostFunction> & mapCostFunctions)
{
    // we need this before filling the costs at the beginning of the assignment period, since the MFDDriving pattern
    // needs the current costs in m_mapCostByRoute
    try
    {
        updateMapCosts(mapCostFunctions);
    }
    catch (matlab::execution::Exception & ex)
    {
		BOOST_LOG_TRIVIAL(error) << ex.what();
        return false;
    }

    return true;
}

void MFDInstance::updateMapCosts(const std::map<SubPopulation*, CostFunction> & mapCostFunctions)
{
    // to avoid an error reading the "Marginals" fields as it doesn't exists until the marginals are implemented in the MFD simulator.
    bool bHasMarginals = false;
    for (std::map<SubPopulation*, CostFunction>::const_iterator iter = mapCostFunctions.begin(); iter != mapCostFunctions.end(); ++iter)
    {
        if (iter->second == CF_Marginals)
        {
            bHasMarginals = true;
            break;
        }
    }
    
    // fetch the new costs of routes
    std::map<int, std::map<SymuCore::CostFunction, SymuCore::Cost> > & mapCostByRoute = ((MFDHandler*)m_pParentSimulationHandler)->GetMapCost(m_iInstance);
    matlab::data::StructArray routes = m_pMatlabEngine->getVariable("Route");
    for (size_t iRoute = 0; iRoute < routes.getNumberOfElements(); iRoute++)
    {
        matlab::data::Struct route = routes.operator[](iRoute);
        double dbTravelTime = route["TotalTime"][0];
        double dbMarginals = bHasMarginals ? (route["Marginals"][0]) : dbTravelTime;

        {
            Cost routeCost;
            routeCost.setUsedCostValue(dbTravelTime);
            routeCost.setOtherCostValue(CF_Marginals, dbMarginals);
            routeCost.setTravelTime(dbTravelTime);

            mapCostByRoute[(int)iRoute + 1][CF_TravelTime] = routeCost;
        }

        {
            Cost routeCost;
            routeCost.setUsedCostValue(dbMarginals);
            routeCost.setOtherCostValue(CF_TravelTime, dbTravelTime);
            routeCost.setTravelTime(dbTravelTime);

            mapCostByRoute[(int)iRoute + 1][CF_Marginals] = routeCost;
        }
    }
}

bool MFDInstance::ComputeCosts(int iTravelTimesUpdatePeriodIndex,
    boost::posix_time::ptime startTravelTimesUpdatePeriodTime,
    boost::posix_time::ptime endTravelTimesUpdatePeriodTime,
    const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    double dbElapsedTimeSinceStart = (startTravelTimesUpdatePeriodTime - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001;
    try
    {
        // ask the MFD simulator to update the route's costs since dbElapsedTimeSinceStart :
        m_pMatlabEngine->eval(convertUTF8StringToUTF16String("ComputeCostsStartTime = " + std::to_string(dbElapsedTimeSinceStart) + ";\nComputeCosts;"));

        updateMapCosts(mapCostFunctions);
    }
    catch (matlab::execution::Exception & ex)
    {
		BOOST_LOG_TRIVIAL(error) << ex.what();
        return false;
    }

    return true;
}

bool MFDInstance::ActivatePPath(const PPath & currentPPath, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::ptime & departureTime)
{
    try
    {
        double dbCreationTime = (double)(departureTime - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() / 1000000.0;

        std::string originId = currentPPath.GetTrip()->GetPath(m_iInstance).GetlistPattern()[currentPPath.GetFirstPatternIndex()]->getLink()->getUpstreamNode()->getStrName();
        MFDDrivingPattern * pMFDPattern = NULL;
        for (size_t iPattern = currentPPath.GetFirstPatternIndex(); iPattern <= currentPPath.GetLastPatternIndex() && !pMFDPattern; iPattern++)
        {
            SymuCore::Pattern * pPattern = currentPPath.GetTrip()->GetPath(m_iInstance).GetlistPattern()[iPattern];

            pMFDPattern = dynamic_cast<MFDDrivingPattern*>(pPattern);
        }

        if (!pMFDPattern)
        {
			BOOST_LOG_TRIVIAL(error) << "No MFD driving pattern for a ppath to be activated on the MFD Simulator ...";
            return false;
        }

        matlab::data::ArrayFactory factory;
        matlab::data::StructArray args = factory.createStructArray({ 1, 1 }, std::vector<std::string>({ "CreationTime", "ID", "OriginID", "RouteID" }));
        args[0]["CreationTime"] = factory.createScalar<double>(dbCreationTime);
        args[0]["ID"] = factory.createScalar<int>(currentPPath.GetTrip()->GetID());
        args[0]["OriginID"] = factory.createScalar<int>(boost::lexical_cast<int>(originId));
        args[0]["RouteID"] = factory.createScalar<int>(pMFDPattern->getRouteID());

        m_pMatlabEngine->feval("CreateVehicle", args);
    }
    catch (matlab::execution::Exception & ex)
    {
		BOOST_LOG_TRIVIAL(error) << ex.what();
        return false;
    }
    return true;
}

bool MFDInstance::Terminate()
{
    return true;
}

bool MFDInstance::ProcessPathsAssignment(const std::vector<Trip*> & assignedTrips, const boost::posix_time::time_duration & predictionPeriodDuration, const boost::posix_time::ptime & startTime, const boost::posix_time::ptime & endTime)
{
	int istartTime = (int)floor(startTime.time_of_day().total_microseconds() * 0.000001);
	int iendTime = (int)floor(endTime.time_of_day().total_microseconds() * 0.000001);
    if (m_bAccBased)
	{
        int nbPeriodPerPrediction = ((MFDConfig*)m_pParentSimulationHandler->GetSimulatorConfiguration())->GetDemandPeriodsNbPerPredictionPeriod();

        boost::posix_time::time_duration demandPeriodDuration = predictionPeriodDuration / nbPeriodPerPrediction;

        double dbNumberOfTimeStepsPerDemandPeriod = ((double)demandPeriodDuration.ticks()) / m_pParentSimulationHandler->GetSimulationDescriptor().GetTimeStepDuration().ticks();
        int iNumberOfTimeStepsPerDemandPeriod = (int)dbNumberOfTimeStepsPerDemandPeriod;

        if (iNumberOfTimeStepsPerDemandPeriod != dbNumberOfTimeStepsPerDemandPeriod)
        {
            BOOST_LOG_TRIVIAL(error) << "The resulting demand period duration " << demandPeriodDuration << " is not a multiple of the time step (" << m_pParentSimulationHandler->GetSimulationDescriptor().GetTimeStepDuration() << ")";
			return false; 
        }
		std::vector<Trip*> WholeTripsList; //List of all assignedTrips wich we add previous trips that could enter in SymuRes during the assigned period
		for (std::vector<Trip*>::iterator itPrevTrip = m_previousAssignedTrips.begin(); itPrevTrip != m_previousAssignedTrips.end(); itPrevTrip++){
			if ((*itPrevTrip)->GetDepartureTime() < startTime)
			{
				WholeTripsList.push_back((*itPrevTrip));
			}
		}
		m_currentAssignedTrips = assignedTrips;
		WholeTripsList.insert(WholeTripsList.end(), assignedTrips.begin(), assignedTrips.end());

        // regroupement des trips par route et par periode
        std::map<int, std::map<size_t, std::vector<Trip*> > > mapTripsPerRoutePerPeriod;
        std::vector<double> periodStartTimes, periodEndTimes;
        boost::posix_time::ptime currentTime = startTime;
		std::map<int, std::vector<Trip*> > mapSimuResEntryTime; // Use to sort trip by entry time and route in SimuRes
		std::map<int, boost::posix_time::ptime> mapTripToEntryTime;
        size_t iTrip = 0;
        size_t iPeriod = 0;
        while (currentTime < endTime)
        {
            boost::posix_time::ptime endDemandPeriodTime = std::min<boost::posix_time::ptime>(currentTime + demandPeriodDuration, endTime);
            periodStartTimes.push_back((currentTime - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001);
            periodEndTimes.push_back((endDemandPeriodTime - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001);

			while (iTrip < WholeTripsList.size() && WholeTripsList.at(iTrip)->GetDepartureTime() <= endDemandPeriodTime)
            {
                // route ?
				boost::posix_time::ptime entryTime = WholeTripsList.at(iTrip)->GetDepartureTime();
				for (int i = 1; i < WholeTripsList.at(iTrip)->GetPath(m_iInstance).GetlistPattern().size(); i++)
				{
					if (iTrip == 300){
						int test = 5;
					}
					MFDDrivingPattern * pMFDPattern = dynamic_cast<MFDDrivingPattern*>(WholeTripsList.at(iTrip)->GetPath(m_iInstance).GetlistPattern()[i]);
					if (pMFDPattern)
					{
						int iRoute = pMFDPattern->getRouteID();
						mapSimuResEntryTime[iRoute].push_back(WholeTripsList.at(iTrip));
						mapTripToEntryTime[WholeTripsList.at(iTrip)->GetID()] = entryTime;
						//mapTripsPerRoutePerPeriod[iRoute][iPeriod].push_back(WholeTripsList.at(iTrip));
						break;
					}
					else{
						entryTime += boost::posix_time::seconds((long)WholeTripsList.at(iTrip)->GetPath(m_iInstance).GetlistPattern()[i]->getPatternCost(m_iInstance, entryTime.time_of_day().total_microseconds() * 0.000001, WholeTripsList.at(iTrip)->GetSubPopulation())->getTravelTime());
					}
					if (entryTime > endDemandPeriodTime){
						break;
					}
				}
                iTrip++;
            }

			for (std::map<int, std::vector<Trip*> >::iterator itRoute = mapSimuResEntryTime.begin(); itRoute != mapSimuResEntryTime.end(); itRoute++)
			{
				mapTripsPerRoutePerPeriod[itRoute->first][iPeriod] = itRoute->second;
			}
			mapSimuResEntryTime.clear();
            currentTime = endDemandPeriodTime;
            iPeriod++;
        }
        size_t nbPeriods = iPeriod;

        // Initialisation de la structure des résultats à passer à matlab

        struct demandData {
            double dbStartTime;
            double dbEndTime;
            double dbDemand;
        };

        std::map<int, std::vector<demandData> > demandDatas;

        // Pour chaque route :
        for (std::map<int, std::map<size_t, std::vector<Trip*> > >::iterator iterRoute = mapTripsPerRoutePerPeriod.begin(); iterRoute != mapTripsPerRoutePerPeriod.end(); ++iterRoute)
        {
            std::vector<demandData> & demandDatasForRoute = demandDatas[iterRoute->first];

            // Pour chaque periode :
            for (iPeriod = 0; iPeriod < nbPeriods; iPeriod++)
            {
                double dbPeriodStartTime = periodStartTimes.at(iPeriod);
                double dbPeriodEndTime = periodEndTimes.at(iPeriod);

                const std::vector<Trip*> & trips = iterRoute->second[iPeriod];

                // Calcul du deltaN et deltaT à considérer pour le calcul de demande :
                double dbDeltaN, dbDemandStartTime, dbDemandEndTime;
                if (trips.size() == 0)
                {
                    dbDeltaN = 0;
                    dbDemandStartTime = dbPeriodStartTime;
                    dbDemandEndTime = dbPeriodEndTime;
                }
                else
                {
                    dbDeltaN = (double)trips.size();

                    // Calcul du dbStartTime :
                    /////////////////////////////////

                    // si on a une création à la période précédente
                    if (iPeriod > 0 && iterRoute->second[iPeriod - 1].size() > 0)
                    {
                        dbDemandStartTime = dbPeriodStartTime;

                        // on retranche la partie du premier véhicule créé lié à la demande de la période précédente
						double dbPrevDepTime = (mapTripToEntryTime[iterRoute->second[iPeriod - 1].back()->GetID()] - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001;
						double dbFirstDepTime = (mapTripToEntryTime[trips.front()->GetID()] - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001;
                        dbDeltaN -= (dbPeriodStartTime - dbPrevDepTime) / (dbFirstDepTime - dbPrevDepTime);
                    }
                    // sinon, on prend le max entre le début de la période et l'instant interpolé de création du dernier véhicule :
                    else
                    {
                        if (trips.size() >= 2)
                        {
							boost::posix_time::ptime prevStartTime = mapTripToEntryTime[trips[0]->GetID()] - (mapTripToEntryTime[trips[1]->GetID()] - mapTripToEntryTime[trips[0]->GetID()]);
                            dbDemandStartTime = std::max<double>(dbPeriodStartTime, (prevStartTime - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001);
                        }
                        // on n'a qu'un point : on ne peut pas interpoler donc on prend le début de la période
                        else
                        {
                            dbDemandStartTime = dbPeriodStartTime;
                        }
                    }

                    // Calcul du dbEndTime:
                    /////////////////////////////////

                    // Si on a un départ dans la prochaine période, on lisse la demande jusqu'à la fin de la période courante
                    if ((iPeriod < nbPeriods - 1) && iterRoute->second[iPeriod + 1].size() > 0)
                    {
                        dbDemandEndTime = dbPeriodEndTime;

                        // on ajoute la partie du premier véhicule de la période suivante lié à la demande de la période courante
						double dbNextDepTime = (mapTripToEntryTime[iterRoute->second[iPeriod + 1].front()->GetID()] - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001;
						double dbLastDepTime = (mapTripToEntryTime[trips.back()->GetID()] - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001;
                        dbDeltaN += (dbPeriodEndTime - dbLastDepTime) / (dbNextDepTime - dbLastDepTime);
                    }
                    else
                    {
                        // Sinon, on s'arrête au dernier instant de création de la période courante (on n'interpole pas comme pour l'instant de début car on ne sait
                        // pas si on aura un prochain véhicule).
						dbDemandEndTime = (mapTripToEntryTime[trips.back()->GetID()] - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001;

                        // Si on n'a que des instants de création sur dbDemandStartTime, on passe quand même dbDemandEndTime à dbDemandStartTime + 1 pas de temps,
                        // histoire de ne pas avoir une plage de durée null qui est ignorée plus bas :
                        if (dbDeltaN > 0) {
                            dbDemandEndTime = std::max<double>(dbDemandEndTime, dbDemandStartTime + m_pParentSimulationHandler->GetSimulationDescriptor().GetTimeStepDuration().total_microseconds() * 0.000001);
                        }
                    }
                }

                // Peut arriver avec un véhicule créé pile à l'instant de début d'une plage temporelle, lié à la demande de la plage précédente.
                if (dbDemandEndTime > dbDemandStartTime)
                {
					double dbDemandValue = dbDeltaN / (dbDemandEndTime - dbDemandStartTime); 

                    demandData newData;
                    newData.dbDemand = dbDemandValue;
                    newData.dbStartTime = dbDemandStartTime;
                    newData.dbEndTime = dbDemandEndTime;
                    demandDatasForRoute.push_back(newData);
                }
            }

            // Seconde passe sur les résultats pour ajouter les periodes de demande nulle au début et à la fin le cas échéant, et ajuster les instants de début de fin de plages temporelles

            if (demandDatasForRoute.front().dbStartTime != (startTime - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001)
            {
                // Ajout d'une plage temporelle de demande nulle avant le premier instant de début de la demande :
                demandData newData;
                newData.dbDemand = 0;
                newData.dbStartTime = (startTime - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001;
                newData.dbEndTime = demandDatasForRoute.front().dbStartTime;
                demandDatasForRoute.insert(demandDatasForRoute.begin(), newData);
            }

            if (demandDatasForRoute.back().dbEndTime != (endTime - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001)
            {
                // Ajout d'une plage temporelle de demande nulle après le dernier instant de fin de la demande :
                demandData newData;
                newData.dbDemand = 0;
                newData.dbStartTime = demandDatasForRoute.back().dbEndTime;
                newData.dbEndTime = (endTime - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001;
                demandDatasForRoute.push_back(newData);
            }

            // Ajustement des bornes temporelles pour qu'elles soient jointives
            for (size_t iData = 0; iData < demandDatasForRoute.size()-1; iData++)
            {
                if (demandDatasForRoute[iData].dbEndTime != demandDatasForRoute[iData + 1].dbStartTime)
                {
                    if (demandDatasForRoute[iData].dbDemand == 0)
                    {
                        assert(demandDatasForRoute[iData + 1].dbDemand != 0);

                        demandDatasForRoute[iData].dbEndTime = demandDatasForRoute[iData + 1].dbStartTime;
                    }
                    else
                    {
                        assert(demandDatasForRoute[iData+1].dbDemand == 0);

                        demandDatasForRoute[iData+1].dbStartTime = demandDatasForRoute[iData].dbEndTime;
                    }
                }
            }
        }

        // passe d'ajout des routes existantes mais pas présentes dans demandDatas
        const std::map<int, std::map<SymuCore::CostFunction, SymuCore::Cost> > & mapCostByRoute = ((MFDHandler*)m_pParentSimulationHandler)->GetMapCost(m_iInstance);
        for (std::map<int, std::map<SymuCore::CostFunction, SymuCore::Cost> >::const_iterator iterRoute = mapCostByRoute.begin(); iterRoute != mapCostByRoute.end(); ++iterRoute)
        {
            if (demandDatas.find(iterRoute->first) == demandDatas.end())
            {
                demandData newData;
                newData.dbDemand = 0;
                newData.dbStartTime = (startTime - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001;
                newData.dbEndTime = (endTime - m_pParentSimulationHandler->GetSimulationDescriptor().GetStartTime()).total_microseconds() * 0.000001;
                demandDatas[iterRoute->first].push_back(newData);
            }
        }

        size_t nbDatas = 0;
        for (std::map<int, std::vector<demandData> >::const_iterator iterData = demandDatas.begin(); iterData != demandDatas.end(); ++iterData)
        {
            nbDatas += iterData->second.size();
        }
 
        // appel à matlab pour positionner la demande à proprement parler
        try
        {
			// Old method
			/*matlab::data::ArrayFactory factory;
            matlab::data::StructArray args = factory.createStructArray({ 1, nbDatas }, std::vector<std::string>({ "StartTime", "EndTime", "RouteID", "Demand" }));

			
            size_t iValue = 0;
            for (std::map<int, std::vector<demandData> >::const_iterator iterData = demandDatas.begin(); iterData != demandDatas.end(); ++iterData)
            {
                for (size_t iData = 0; iData < iterData->second.size(); iData++)
                {
                    const demandData & data = iterData->second.at(iData);

                    args[iValue]["StartTime"] = factory.createScalar<double>(data.dbStartTime);
                    args[iValue]["EndTime"] = factory.createScalar<double>(data.dbEndTime);
                    args[iValue]["RouteID"] = factory.createScalar<int>(iterData->first);
                    args[iValue]["Demand"] = factory.createScalar<double>(data.dbDemand);
                    iValue++;
                }
            }*/


			double TimeStep = m_pParentSimulationHandler->GetSimulationDescriptor().GetTimeStepDuration().total_microseconds() * 0.000001;
			matlab::data::ArrayFactory factory;
			matlab::data::StructArray DemandPerRoute = factory.createStructArray({ 1, demandDatas.size() }, std::vector<std::string>({ "Demand" }));
			matlab::data::TypedArray<int> startTimeArray = factory.createArray<int>({ 1, 1 }, { istartTime + 1 });
			size_t iValue = 0;
			for (std::map<int, std::vector<demandData> >::const_iterator iterData = demandDatas.begin(); iterData != demandDatas.end(); ++iterData) // For each Route
			{
				std::vector<demandData> DemandForRoute = iterData->second;

				std::vector<double> DemandValueList;
				// process demand for route for every timestep :
				int Temp_StartTimeID = (int)floor(DemandForRoute[0].dbStartTime / TimeStep) + 1;
				int Temp_EndTimeID = (int)floor(DemandForRoute[DemandForRoute.size() - 1].dbEndTime / TimeStep);
				matlab::data::ArrayFactory factory;
				//matlab::data::StructArray Demand = factory.createStructArray({ Temp_StartTimeID, Temp_EndTimeID }, std::vector<double>({ "StartTime", "EndTime", "RouteID", "Demand" }));
				for (int i_time = Temp_StartTimeID; i_time <= Temp_EndTimeID; i_time++){
					// compute combined demand for timestep
					double StartTime = (i_time - 1)*TimeStep;
					double EndTime = StartTime + TimeStep;
					double DemandValue = 0;
					for (int i_demand_period = 0; i_demand_period < DemandForRoute.size(); i_demand_period++){
						double CommonTimePeriod = std::min<double>(EndTime, DemandForRoute[i_demand_period].dbEndTime) - std::max<double>(StartTime, DemandForRoute[i_demand_period].dbStartTime);
						if (CommonTimePeriod > 0){
							DemandValue = DemandValue + CommonTimePeriod * DemandForRoute[i_demand_period].dbDemand;
						}
					}
					DemandValue = DemandValue / TimeStep;
					DemandValueList.push_back(DemandValue);
					//m_pMatlabEngine->eval(convertUTF8StringToUTF16String("Route(" + std::to_string(iterData->first) + ").Demand(" + std::to_string(i_time) + ") = " + std::to_string(DemandValue) + ";"));
				}
				//matlab::data::Array Demand = factory.createArray({ (uint64_t)Temp_StartTimeID, (uint64_t)Temp_EndTimeID }, DemandValueList.begin(), DemandValueList.end());
				DemandPerRoute[iValue]["Demand"] = factory.createArray({ 1, (uint64_t)(Temp_EndTimeID - Temp_StartTimeID + 1) }, DemandValueList.begin(), DemandValueList.end());
				iValue++;
			}

			std::vector<matlab::data::Array> args;
			args.push_back(DemandPerRoute);
			args.push_back(startTimeArray);
			m_pMatlabEngine->feval("SetDemand", args);
            m_pMatlabEngine->eval(convertUTF8StringToUTF16String("InitTripLengths"));
        }
        catch (matlab::execution::Exception & ex)
        {
			BOOST_LOG_TRIVIAL(error) << ex.what();
            return false;
        }
    }

    return true;
}


bool MFDInstance::AssignmentPeriodStart(int iPeriod, bool bIsNotTheFinalIterationForPeriod)
{
	if (bIsNotTheFinalIterationForPeriod){
		m_previousAssignedTrips = m_currentAssignedTrips;
	}

    if (!m_bHasStartPeriodSnapshot)
	{
        try
        {
            m_pMatlabEngine->eval(convertUTF8StringToUTF16String("TakeSnapshot"));
            m_bHasStartPeriodSnapshot = true;
        }
        catch (matlab::execution::Exception & ex)
        {
			BOOST_LOG_TRIVIAL(error) << ex.what();
            return false;
        }
        return true;
    }
    else
    {
        return true;
    }
}

bool MFDInstance::AssignmentPeriodEnded()
{
    try
    {
        m_pMatlabEngine->eval(convertUTF8StringToUTF16String("TakeSnapshot"));
    }
    catch (matlab::execution::Exception & ex)
    {
		BOOST_LOG_TRIVIAL(error) << ex.what();
        return false;
    }
    return true;
}

bool MFDInstance::AssignmentPeriodRollback()
{
    try
    {
        m_pMatlabEngine->eval(convertUTF8StringToUTF16String("RestoreSnapshot(1)"));
    }
    catch (matlab::execution::Exception & ex)
    {
		BOOST_LOG_TRIVIAL(error) << ex.what();
        return false;
    }
    return true;
}

bool MFDInstance::AssignmentPeriodCommit(bool bAssignmentPeriodEnded)
{
    if (!bAssignmentPeriodEnded)
	{
		m_previousAssignedTrips = m_currentAssignedTrips;

        try
        {
            m_pMatlabEngine->eval(convertUTF8StringToUTF16String("Commit"));
            m_bHasStartPeriodSnapshot = false;
        }
        catch (matlab::execution::Exception & ex)
        {
			BOOST_LOG_TRIVIAL(error) << ex.what();
            return false;
        }
    }
    else
    {
        try
        {
            m_pMatlabEngine->eval(convertUTF8StringToUTF16String("RestoreSnapshot(2)"));
        }
        catch (matlab::execution::Exception & ex)
        {
			BOOST_LOG_TRIVIAL(error) << ex.what();
            return false;
        }
    }
    return true;
}

bool MFDInstance::IsExtraIterationToProduceOutputsRelevant()
{
    return false;
}

AbstractSimulatorInterface* MFDInstance::MakeInterface(const std::string & strName, bool bIsUpstreamInterface)
{
    return new MFDInterface(m_pParentSimulationHandler, strName, bIsUpstreamInterface);
}

bool MFDInstance::GetOffers(const std::vector<const AbstractSimulatorInterface*> & interfacesToGetOffer, std::map<const AbstractSimulatorInterface*, double> & offerValues)
{
    try
    {
        matlab::data::ArrayFactory factory;
        std::vector<matlab::data::Array> args;

        std::vector<int> routesIDs(interfacesToGetOffer.size());
        for (size_t iRoute = 0; iRoute < interfacesToGetOffer.size(); iRoute++)
        {
            routesIDs[iRoute] = atoi(interfacesToGetOffer[iRoute]->getStrName().c_str());
        }
        matlab::data::Array tmpArray = factory.createArray<int>({ 1, interfacesToGetOffer.size() }, &routesIDs[0], &routesIDs[0] + routesIDs.size());
        args.push_back(tmpArray);
        matlab::data::Array offerValuesResult = m_pMatlabEngine->feval("GetOffers", args);

        for (size_t iRoute = 0; iRoute < offerValuesResult.getNumberOfElements(); iRoute++)
        {
            double dbOffer = offerValuesResult.operator[](iRoute);
			if (std::numeric_limits<double>::infinity() == dbOffer){
				dbOffer = 0;
			}
            offerValues[interfacesToGetOffer[iRoute]] = dbOffer;
        }
    }
    catch (matlab::execution::Exception & ex)
    {
		BOOST_LOG_TRIVIAL(error) << ex.what();
        return false;
    }
    return true;
}

bool MFDInstance::SetCapacities(const std::vector<const AbstractSimulatorInterface*> & upstreamInterfaces, const AbstractSimulatorInterface* pDownstreamInterface, double dbCapacityValue)
{
    try
    {
        matlab::data::ArrayFactory factory;
        std::vector<matlab::data::Array> args;

        std::vector<int> routesIDs(upstreamInterfaces.size());
        for (size_t iRoute = 0; iRoute < upstreamInterfaces.size(); iRoute++)
        {
            routesIDs[iRoute] = atoi(upstreamInterfaces[iRoute]->getStrName().c_str());
        }
        matlab::data::Array tmpArray = factory.createArray<int>({ 1, upstreamInterfaces.size() }, &routesIDs[0], &routesIDs[0] + routesIDs.size());
        args.push_back(tmpArray);
        args.push_back(factory.createScalar<double>(dbCapacityValue));
        m_pMatlabEngine->feval("SetCapacity", args);
    }
    catch (matlab::execution::Exception & ex)
    {
		BOOST_LOG_TRIVIAL(error) << ex.what();
        return false;
    }
    return true;
}

#endif // USE_MATLAB_MFD