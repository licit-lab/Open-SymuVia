#include "SymuViaInstance.h" 

#include "SymuViaConfig.h"
#include "Simulation/Simulators/TraficSimulatorHandler.h"
#include "Simulation/Simulators/SimulationDescriptor.h"
#include "Simulation/Config/SimulationConfiguration.h"
#include "Simulation/Users/PPath.h"

#include "SymuViaInterface.h"

#ifdef USE_MATLAB_MFD
#include "Simulation/Simulators/MFD/MFDDrivingPattern.h"
#include "Simulation/Simulators/MFD/MFDInterface.h"
#endif // USE_MATLAB_MFD

#include <Demand/VehicleType.h>
#include <Graph/Model/Node.h>
#include <Graph/Model/Link.h>
#include <Graph/Model/Pattern.h>
#include <Graph/Model/PublicTransport/PublicTransportPattern.h>
#include <Graph/Model/PublicTransport/PublicTransportLine.h>
#include <Graph/Model/MultiLayersGraph.h>
#include <Demand/Origin.h>
#include <Demand/Destination.h>
#include <Demand/Population.h>
#include <Demand/SubPopulation.h>
#include <Demand/VehicleType.h>
#include <Demand/MacroType.h>
#include <Demand/Trip.h>
#include <Demand/Trip.h>

#include <SymExp.h>
#include <TronconDrivingPattern.h>

#include <boost/log/trivial.hpp>


#include <set>
#include <string.h>

using namespace SymuMaster;

SymuViaInstance::SymuViaInstance(TraficSimulatorHandler * pParentHandler, int iInstance) :
    TraficSimulatorInstance(pParentHandler, iInstance)
{
    m_iNetworkID = 0;
    m_bHasStartPeriodSnapshot = false;
    m_bHasTrajectoriesOutput = false;
}

SymuViaInstance::~SymuViaInstance()
{

}

const std::string & SymuViaInstance::GetSimulatorName() const
{
    static const std::string symuViaName = "SymuVia";
    return symuViaName;
}

bool SymuViaInstance::IsExtraIterationToProduceOutputsRelevant()
{
    return m_bHasTrajectoriesOutput;
}

bool SymuViaInstance::Initialize(const SimulationConfiguration & config, SymuCore::IUserHandler * pUserHandler, TraficSimulatorHandler * pMasterSimulator)
{
    SymuViaConfig* symuConfig = (SymuViaConfig*)m_pParentSimulationHandler->GetSimulatorConfiguration();

    boost::posix_time::ptime startTime, endTime;
    boost::posix_time::time_duration timeStepDuration;

    BOOST_LOG_TRIVIAL(trace) << "Loading SymuVia network [" << symuConfig->GetXMLFilePath() << "]...";

    m_iNetworkID = SymLoadNetwork(pUserHandler, symuConfig->GetXMLFilePath(), symuConfig->GetMarginalsDeltaN(),
		symuConfig->UseMarginalsMeanMethod(),
		symuConfig->UseMarginalsMedianMethod(),
		symuConfig->GetHistoryVehiclesNb(), symuConfig->GetHistoryPeriodsNb(),
		symuConfig->UseSpatialMethodForTravelTimesMeasurement(),
		symuConfig->UseClassicSpatialMethodForTravelTimesMeasurement(),
		symuConfig->EstimateTrafficLightsWaitTimeOnEmptyLinksTravelTimes(),
		symuConfig->GetMinimumSpeedForTravelTimes(),
		symuConfig->GetMaximumMarginalsValue(),
		symuConfig->UseTravelTimesAsMarginalsInAreas(),
		symuConfig->GetConcentrationRatioForFreeFlowCriterion(),
		symuConfig->GetNbClassRobustTTMethod(),
		symuConfig->GetMaxNbPointsPerClassRobustTTMethod(),
		symuConfig->GetMinNbPointsStatisticalFilterRobustTTMethod(),
		symuConfig->UseLastBusIfAnyForTravelTimes(),
		symuConfig->UseEmptyTravelTimesForBusLines(),
		symuConfig->GetMeanBusExitTime(),
		symuConfig->GetNbStopsToConnectOutsideOfSubAreas(),
		config.GetPopulations()->getMaxInitialWalkRadius(),
		config.GetPopulations()->getMaxIntermediateWalkRadius(),
		config.bWriteCosts(),
		symuConfig->GetMinLengthForMarginals(),
		symuConfig->GetRobustTravelTimesFilePath(),
		symuConfig->GetRobustTravelSpeedsFilePath(),
		symuConfig->GetRobustPointsBackup(),
		symuConfig->GetPollutantEmissionsComputation(),
		startTime, endTime, timeStepDuration, m_bHasTrajectoriesOutput);

    bool bOk = m_iNetworkID > 0;

    if (m_iInstance != 0) // dont write output files for other simulations.
    {
        SymDisableTrajectoriesOutput(m_iNetworkID, true);
    }
    else
    {
        m_pParentSimulationHandler->GetSimulationDescriptor().SetStartTime(startTime);
        m_pParentSimulationHandler->GetSimulationDescriptor().SetEndTime(endTime);
        m_pParentSimulationHandler->GetSimulationDescriptor().SetTimeStepDuration(timeStepDuration);
    }

    if (bOk)
    {
        BOOST_LOG_TRIVIAL(trace) << "SymuVia network loaded.";
    }
    else
    {
        BOOST_LOG_TRIVIAL(error) << "SymuVia's SymLoadNetwork failed.";
    }

    return bOk;
}

bool SymuViaInstance::BuildSimulationGraph(SymuCore::MultiLayersGraph *pParentGraph, SymuCore::MultiLayersGraph *pSimulatorGraph, bool bIsPrimaryGraph)
{
    // graph's construction
    BOOST_LOG_TRIVIAL(trace) << "Building SymuVia's graph layer...";
    bool bOk = SymBuildGraph(m_iNetworkID, pSimulatorGraph, bIsPrimaryGraph);
    if (!bOk)
    {
        BOOST_LOG_TRIVIAL(error) << "SymBuildGraph failed !";
    }
    else
    {
        BOOST_LOG_TRIVIAL(trace) << "SymuVia's graph layer built.";
    }

    return bOk;
}

bool SymuViaInstance::Step(const boost::posix_time::ptime & startTimeStep, const boost::posix_time::time_duration & timeStepDuration)
{
    std::string sXmlFluxInstant;
    bool bEnd;
    return SymRunNextStep(m_iNetworkID, sXmlFluxInstant, true, bEnd);
}

SymuCore::Origin * SymuViaInstance::GetParentOrigin(SymuCore::Origin * pChildOrigin)
{
    return SymGetParentOrigin(m_iNetworkID, pChildOrigin, m_pParentSimulationHandler->GetSimulationDescriptor().GetGraph());
}

SymuCore::Destination * SymuViaInstance::GetParentDestination(SymuCore::Destination * pChildDestination)
{
    return SymGetParentDestination(m_iNetworkID, pChildDestination, m_pParentSimulationHandler->GetSimulationDescriptor().GetGraph());
}

bool SymuViaInstance::FillTripsDescriptor(SymuCore::MultiLayersGraph * pGraph, SymuCore::Populations & populations,
    const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & endSimulationTime, bool bIgnoreSubAreas,
    std::vector<SymuCore::Trip> & lstTrips)
{
    // rmk. : getting only demand for private cars in SymuVia. There is some demand values for public transport too,
    // but too simplistic and not usable in our case.
    return SymFillPopulations(m_iNetworkID, pGraph, populations, startSimulationTime, endSimulationTime, bIgnoreSubAreas, lstTrips);
}

bool SymuViaInstance::ComputeCosts(int iTravelTimesUpdatePeriodIndex, boost::posix_time::ptime startTravelTimesUpdatePeriodTime, boost::posix_time::ptime endTravelTimesUpdatePeriodTime, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    const std::vector<SymuCore::SubPopulation*> & listSubPops = m_pParentSimulationHandler->GetSimulationDescriptor().GetSubPopulationsByServiceType()[SymuCore::ST_Driving];

    std::set<SymuCore::MacroType*> setmacroTypes;
    for (size_t iSubPop = 0; iSubPop < listSubPops.size(); iSubPop++)
    {
        setmacroTypes.insert(listSubPops.at(iSubPop)->GetPopulation()->GetMacroType());
    }

    return SymComputeCosts(m_iNetworkID, startTravelTimesUpdatePeriodTime, endTravelTimesUpdatePeriodTime, std::vector<SymuCore::MacroType*>(setmacroTypes.begin(), setmacroTypes.end()), mapCostFunctions);
}

bool SymuViaInstance::ActivateDrivingPPath(const PPath & currentPPath, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::ptime & departureTime)
{
    //const boost::posix_time::ptime & startTimeStepTime = currentPPath.Get
    const SymuCore::Trip * pTrip = currentPPath.GetTrip();

    // For now, SymuVia needs a vehicle type to create a vehicle, and builds by construction
    // a VehicleType for each Trip. This may change when SymuMaster will build the Trips itself...
    assert(pTrip->GetPopulation() != NULL);

    double dbTimefraction = (double)(departureTime - startTimeStepTime).total_microseconds() / 1000000.0;

    std::string junctionName;
    std::vector<std::string> links;
    const SymuCore::Path & vehiclePath = pTrip->GetPath(m_iInstance);
    std::string originId = vehiclePath.GetlistPattern()[currentPPath.GetFirstPatternIndex()]->getLink()->getUpstreamNode()->getStrName();
    std::string destinationId = vehiclePath.GetlistPattern()[currentPPath.GetLastPatternIndex()]->getLink()->getDownstreamNode()->getStrName();
    for (size_t iPattern = currentPPath.GetFirstPatternIndex(); iPattern <= currentPPath.GetLastPatternIndex(); iPattern++)
    {
        SymuCore::Pattern * pPattern = vehiclePath.GetlistPattern()[iPattern];

        TronconDrivingPattern * pTronconDrivingPattern = dynamic_cast<TronconDrivingPattern*>(pPattern);

        // SymuVia takes for now only "troncon" names as input for path of activated vehicle
        if (pTronconDrivingPattern) // zones links filtering...
        {
            links.push_back(pPattern->toString());
        }
        else
        {
            SymuCore::NodeType nodeType = pPattern->getLink()->getUpstreamNode()->getNodeType();
            // we are interested by the only "real" road node in the path :
            if (nodeType == SymuCore::NT_RoadJunction)
            {
                junctionName = pPattern->getLink()->getUpstreamNode()->getStrName();
            }
        }
    }
    char ** pLinks = new char *[links.size() + 1];
    for (size_t iPattern = 0; iPattern < links.size(); iPattern++)
    {
        pLinks[iPattern] = new char[links[iPattern].length() + 1];
#ifdef WIN32
        strcpy_s(pLinks[iPattern], links[iPattern].length() + 1, links[iPattern].c_str());
#else
        strcpy(pLinks[iPattern], links[iPattern].c_str());
#endif
    }
    pLinks[links.size()] = NULL;

    // récupération de l'identifiant de la route du prochain PPATH, le cas échéant. Utile pour appliquer les restrictions de capacité
    // en sortie en fonction de la route aval du véhicule.
    std::string nextRouteID;
#ifdef USE_MATLAB_MFD
    for (size_t iNextPattern = currentPPath.GetLastPatternIndex() + 1; iNextPattern < vehiclePath.GetlistPattern().size(); iNextPattern++)
    {
		MFDDrivingPattern * pRoutePattern = dynamic_cast<MFDDrivingPattern*>(vehiclePath.GetlistPattern()[iNextPattern]);
        if (pRoutePattern)
        {
            nextRouteID = pRoutePattern->toString();
            break;
        }
    }
#endif // USE_MATLAB_MFD

    int resCode = SymCreateVehicle(
        m_iNetworkID,
        const_cast<char*>(originId.c_str()),
        const_cast<char*>(destinationId.c_str()),
        const_cast<char*>(pTrip->GetVehicleType()->getStrName().c_str()),
        std::max<double>(0, dbTimefraction), // we can have negative dbTimefraction here with hybridation (multi-simulators), when a user changes simulator during a timestep.
        const_cast<const char**>(pLinks),
        //links.empty() ? const_cast<char*>(junctionName.c_str()) : NULL,
		links.size()==0 ? const_cast<char*>(junctionName.c_str()) : NULL,
        pTrip->GetID(),
        nextRouteID.empty() ? NULL : const_cast<char*>(nextRouteID.c_str()));

    char ** pLinksPtr = pLinks;
    while (*pLinksPtr != NULL)
    {
        delete[] * pLinksPtr++;
    }
    delete[] pLinks;

    return resCode >= 0;
}

bool SymuViaInstance::ActivatePublicTransportPPath(const PPath & currentPPath, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::ptime & departureTime)
{
    double dbTimefraction = (double)(departureTime - startTimeStepTime).total_microseconds() / 1000000.0;

    const SymuCore::Path & publicTransportPath = currentPPath.GetTrip()->GetPath(m_iInstance);
    SymuCore::PublicTransportPattern * pFirstPattern = (SymuCore::PublicTransportPattern*)publicTransportPath.GetlistPattern().at(currentPPath.GetFirstPatternIndex());
    SymuCore::Pattern * pLastPattern = publicTransportPath.GetlistPattern().at(currentPPath.GetLastPatternIndex());
    
    int resCode = SymCreatePublicTransportUser(
        m_iNetworkID,
        pFirstPattern->getLink()->getUpstreamNode()->getStrName(),
        pLastPattern->getLink()->getDownstreamNode()->getStrName(),
        pFirstPattern->getLine()->getStrName(),
        std::max<double>(0, dbTimefraction), // we can have negative dbTimefraction here with hybridation (multi-simulators), when a user changes simulator during a timestep.
        currentPPath.GetTrip()->GetID());

    return resCode >= 0;
}

bool SymuViaInstance::ActivatePPath(const PPath & currentPPath, const boost::posix_time::ptime & startTimeStepTime, const boost::posix_time::ptime & departureTime)
{
    bool bResult;
    switch (currentPPath.GetServiceType())
    {
    case SymuCore::ST_Driving:
        bResult = ActivateDrivingPPath(currentPPath, startTimeStepTime, departureTime);
        break;
    case SymuCore::ST_PublicTransport:
        bResult = ActivatePublicTransportPPath(currentPPath, startTimeStepTime, departureTime);
        break;
    default:
        BOOST_LOG_TRIVIAL(error) << "Service type '" << currentPPath.GetServiceType() << "' not handled by SymuVia (Trip ID " << currentPPath.GetTrip()->GetID() << ") !";
        bResult = false;
        break;
    }

    return bResult;
}

bool SymuViaInstance::Terminate()
{
    return SymQuit() == 0;
}

bool SymuViaInstance::AssignmentPeriodStart(int iPeriod, bool bIsNotTheFinalIterationForPeriod)
{    
    if (m_iInstance == 0) // Since we write output only for the first SymuVia instance, don't bother with the other ones
    {
        SymDisableTrajectoriesOutput(m_iNetworkID, bIsNotTheFinalIterationForPeriod);
    }
    if (!m_bHasStartPeriodSnapshot)
    {
        m_bHasStartPeriodSnapshot = true;
        return SymTakeSimulationSnapshot(m_iNetworkID);
    }
    else
    {
        return true;
    }
}

bool SymuViaInstance::AssignmentPeriodEnded()
{
    return SymTakeSimulationSnapshot(m_iNetworkID);
}

bool SymuViaInstance::AssignmentPeriodRollback()
{
    return SymSimulationRollback(m_iNetworkID, 0);
}

bool SymuViaInstance::AssignmentPeriodCommit(bool bAssignmentPeriodEnded)
{
    if (!bAssignmentPeriodEnded)
    {
        m_bHasStartPeriodSnapshot = false;
        return SymSimulationCommit(m_iNetworkID);
    }
    else
    {
        return SymSimulationRollback(m_iNetworkID, 1);
    }
}

AbstractSimulatorInterface* SymuViaInstance::MakeInterface(const std::string & strName, bool bIsUpstreamInterface)
{
    return new SymuViaInterface(m_pParentSimulationHandler, strName, bIsUpstreamInterface);
}

bool SymuViaInstance::GetOffers(const std::vector<const AbstractSimulatorInterface*> & interfacesToGetOffer, std::map<const AbstractSimulatorInterface*, double> & offerValues)
{
    std::vector<std::string> inputNames;
    for (size_t iInput = 0; iInput < interfacesToGetOffer.size(); iInput++)
    {
        inputNames.push_back(interfacesToGetOffer.at(iInput)->getStrName());
    }

    std::vector<double> offerValuesVect;
    bool bOk = SymGetOffers(m_iNetworkID, inputNames, offerValuesVect);

    for (size_t iInput = 0; bOk && iInput < interfacesToGetOffer.size(); iInput++)
    {
        offerValues[interfacesToGetOffer.at(iInput)] = offerValuesVect[iInput];
    }
    
    return bOk;
}

bool SymuViaInstance::SetCapacities(const std::vector<const AbstractSimulatorInterface*> & upstreamInterfaces, const AbstractSimulatorInterface* pDownstreamInterface, double dbCapacityValue)
{
    // This is not allowed to have mutliple symuvia's exits for one downstream MFD route.
    //assert(upstreamInterfaces.size() == 1);

    std::vector<std::string> outputNames;
    std::vector<double> capacityValuesVect;

    std::string downstreamRouteName;
#ifdef USE_MATLAB_MFD
    const MFDInterface * pMFDInterface = dynamic_cast<const MFDInterface*>(pDownstreamInterface);
    if (pMFDInterface)
    {
        downstreamRouteName = pMFDInterface->getStrName();
    }
#endif // USE_MATLAB_MFD

    return SymSetCapacities(m_iNetworkID, upstreamInterfaces.front()->getStrName(), downstreamRouteName, dbCapacityValue);
}


bool SymuViaInstance::ForbidLinks(const std::vector<std::string> & linkNames)
{
	return SymForbidLinks(m_iNetworkID, linkNames);
}
