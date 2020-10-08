#include "TraficSimulatorHandler.h"

#include "Simulation/Simulators/SimulationDescriptor.h"
#include "Simulation/Users/UserEngines.h"
#include "Simulation/Users/UserEngine.h"

#include "TraficSimulators.h"

#include "TraficSimulatorInstance.h"

#include <Graph/Model/Link.h>
#include <Graph/Model/Node.h>
#include <Graph/Model/Pattern.h>
#include <Graph/Model/AbstractPenalty.h>

#include <boost/make_shared.hpp>
#include <boost/thread.hpp>

#include <set>

using namespace SymuMaster;
using namespace SymuCore;

TraficSimulatorHandler::TraficSimulatorHandler(const SimulatorConfiguration * pConfig, TraficSimulators * pParentTraficSimulators) :
    m_pConfig(pConfig),
    m_pParentTraficSimulators(pParentTraficSimulators)
{

}

TraficSimulatorHandler::~TraficSimulatorHandler()
{
    for (size_t i = 0; i < m_lstInstances.size(); i++)
    {
        delete m_lstInstances[i];
    }
}

const std::vector<TraficSimulatorInstance*> & TraficSimulatorHandler::GetSimulatorInstances() const
{
    return m_lstInstances;
}

SimulationDescriptor & TraficSimulatorHandler::GetSimulationDescriptor()
{
    return m_SimulationDescriptor;
}

const SimulationDescriptor & TraficSimulatorHandler::GetSimulationDescriptor() const
{
    return m_SimulationDescriptor;
}

const SimulatorConfiguration * TraficSimulatorHandler::GetSimulatorConfiguration() const
{
    return m_pConfig;
}

void TraficSimulatorHandler::AddSimulatorInstance(TraficSimulatorInstance * pSimInstance)
{
    m_lstInstances.push_back(pSimInstance);
}

const std::string & TraficSimulatorHandler::GetSimulatorName() const
{
    return m_lstInstances.front()->GetSimulatorName();
}

// Thread class used to do simulatio instance initialization in parallel
class SimulatorInitializerThread {
public:
    SimulatorInitializerThread(TraficSimulatorInstance * pInstance,
        const SimulationConfiguration & config,
        UserEngine * pUserEngine,
        TraficSimulatorHandler * pMasterSimulator) :
        m_pInstance(pInstance),
        m_Config(config),
        m_pUserEngine(pUserEngine),
        m_pMasterSimulator(pMasterSimulator),
        m_bOk(true),
        m_Thread(&SimulatorInitializerThread::run, this)
    {
    }

    void run() {
        m_bOk = m_pInstance->Initialize(m_Config, m_pUserEngine, m_pMasterSimulator);
    }

    bool isOK() {
        return m_bOk;
    }

    void join() {
        m_Thread.join();
    }

private:
    TraficSimulatorInstance * m_pInstance;
    const SimulationConfiguration & m_Config;
    UserEngine * m_pUserEngine;
    TraficSimulatorHandler * m_pMasterSimulator;
    bool m_bOk;
    boost::thread m_Thread;
};

bool TraficSimulatorHandler::Initialize(const SimulationConfiguration & config, UserEngines & userEngines, TraficSimulatorHandler * pMasterSimulator)
{
    bool bOk = true;

    std::vector<SimulatorInitializerThread*> threads;
    for (size_t iThread = 0; iThread < m_lstInstances.size(); iThread++)
    {
        threads.push_back(new SimulatorInitializerThread(m_lstInstances[iThread], config, userEngines.GetUserEngines()[iThread], pMasterSimulator));
    }

    for (size_t iThread = 0; iThread < threads.size(); iThread++)
    {
        SimulatorInitializerThread * pThread = threads.at(iThread);

        pThread->join();

        bOk = bOk && pThread->isOK();

        delete pThread;
    }

    return bOk;
}

bool TraficSimulatorHandler::BuildSimulationGraph(SymuCore::MultiLayersGraph *pParentGraph, SymuCore::MultiLayersGraph *pSimulatorGraph)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstInstances.size() && bOk; i++)
    {
        bOk = m_lstInstances[i]->BuildSimulationGraph(pParentGraph, i == 0 ? pSimulatorGraph : m_pParentTraficSimulators->GetSecondaryGraph()[i - 1], i == 0);
    }
    return bOk;
}

bool TraficSimulatorHandler::Step(const boost::posix_time::ptime & startTimeStep, const boost::posix_time::time_duration & timeStepDuration, int iInstance)
{
    return m_lstInstances[iInstance]->Step(startTimeStep, timeStepDuration);
}

SymuCore::Origin * TraficSimulatorHandler::GetParentOrigin(SymuCore::Origin * pChildOrigin)
{
    return m_lstInstances.front()->GetParentOrigin(pChildOrigin);
}

SymuCore::Destination * TraficSimulatorHandler::GetParentDestination(SymuCore::Destination * pChildDestination)
{
    return m_lstInstances.front()->GetParentDestination(pChildDestination);
}

bool TraficSimulatorHandler::FillTripsDescriptor(SymuCore::MultiLayersGraph * pGraph, SymuCore::Populations & populations,
    const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & endSimulationTim, bool bIgnoreSubArease,
    std::vector<SymuCore::Trip> & lstTrips)
{
    return m_lstInstances.front()->FillTripsDescriptor(pGraph, populations, startSimulationTime, endSimulationTim, bIgnoreSubArease, lstTrips);
}

bool TraficSimulatorHandler::ComputeCosts(int iTravelTimesUpdatePeriodIndex, boost::posix_time::ptime startTravelTimesUpdatePeriodTime, boost::posix_time::ptime endTravelTimesUpdatePeriodTime, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int iInstance)
{
    return m_lstInstances[iInstance]->ComputeCosts(iTravelTimesUpdatePeriodIndex, startTravelTimesUpdatePeriodTime, endTravelTimesUpdatePeriodTime, mapCostFunctions);
}

bool TraficSimulatorHandler::Terminate()
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstInstances.size() && bOk; i++)
    {
        bOk = m_lstInstances[i]->Terminate();
    }
    return bOk;
}

bool TraficSimulatorHandler::AssignmentPeriodStart(int iPeriod, bool bIsNotTheFinalIterationForPeriod, int nbMaxInstancesNb)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstInstances.size() && i < (size_t)nbMaxInstancesNb && bOk; i++)
    {
        bOk = m_lstInstances[i]->AssignmentPeriodStart(iPeriod, bIsNotTheFinalIterationForPeriod);
    }
    return bOk;
}

bool TraficSimulatorHandler::AssignmentPeriodEnded(int iInstance)
{
    return m_lstInstances[iInstance]->AssignmentPeriodEnded();
}

bool TraficSimulatorHandler::AssignmentPeriodRollback(int nbMaxInstancesNb)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstInstances.size() && i < (size_t)nbMaxInstancesNb && bOk; i++)
    {
        bOk = m_lstInstances[i]->AssignmentPeriodRollback();
    }
    return bOk;
}

bool TraficSimulatorHandler::AssignmentPeriodCommit(int nbMaxInstancesNb, bool bAssignmentPeriodEnded)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstInstances.size() && i < (size_t)nbMaxInstancesNb && bOk; i++)
    {
        bOk = m_lstInstances[i]->AssignmentPeriodCommit(bAssignmentPeriodEnded);
    }
    return bOk;
}

bool TraficSimulatorHandler::IsExtraIterationToProduceOutputsRelevant()
{
    return m_lstInstances.front()->IsExtraIterationToProduceOutputsRelevant();
}

bool TraficSimulatorHandler::ComputeNewPeriodCosts(const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    bool bOk = true;
    for (size_t i = 0; i < m_lstInstances.size() && bOk; i++)
    {
        // rmk. : for each simulation instance, the initial costs should be the same, so we could optimize this if the costs computation takes too much time
        bOk = m_lstInstances[i]->ComputeNewPeriodCosts(mapCostFunctions);
    }
    return bOk;
}

bool TraficSimulatorHandler::ProcessPathsAssignment(const std::vector<SymuCore::Trip*> & assignedTrips, const boost::posix_time::time_duration & predictionPeriodDuration, const boost::posix_time::ptime & startTime, const boost::posix_time::ptime & endTime, int iSimulationInstance)
{
    return m_lstInstances[iSimulationInstance]->ProcessPathsAssignment(assignedTrips, predictionPeriodDuration, startTime, endTime);
}

bool TraficSimulatorHandler::CheckInterfaces(const std::map<TraficSimulatorHandler*, std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> > > & interfaces)
{
	for (std::map<std::string, AbstractSimulatorInterface*>::const_iterator iter = m_lstInstances.front()->GetUpstreamInterfaces().begin(); iter != m_lstInstances.front()->GetUpstreamInterfaces().end(); ++iter)
    {
        // searching for associated downstream interfaces :
        std::vector<AbstractSimulatorInterface*> downstreamInterfaces;
        for (std::map<TraficSimulatorHandler*, std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> > >::const_iterator iterDown = interfaces.begin(); iterDown != interfaces.end(); ++iterDown)
        {
            for (std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> >::const_iterator iterUp = iterDown->second.begin(); iterUp != iterDown->second.end(); ++iterUp)
            {
                for (size_t iInterface = 0; iInterface < iterUp->second.size(); iInterface++)
                {
                    if (iterUp->second[iInterface].GetUpstreamInterface() == iter->second)
                    {
                        downstreamInterfaces.push_back(iterUp->second[iInterface].GetDownstreamInterface());
                    }
                }
            }
        }

        if (!iter->second->Check(downstreamInterfaces))
        {
            return false;
        }
    }

    for (std::map<std::string, AbstractSimulatorInterface*>::const_iterator iter = m_lstInstances.front()->GetDownstreamInterfaces().begin(); iter != m_lstInstances.front()->GetDownstreamInterfaces().end(); ++iter)
    {
        // searching for associated upstream interfaces :
        std::vector<AbstractSimulatorInterface*> upstreamInterfaces;
        for (std::map<TraficSimulatorHandler*, std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> > >::const_iterator iterDown = interfaces.begin(); iterDown != interfaces.end(); ++iterDown)
        {
            for (std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> >::const_iterator iterUp = iterDown->second.begin(); iterUp != iterDown->second.end(); ++iterUp)
            {
                for (size_t iInterface = 0; iInterface < iterUp->second.size(); iInterface++)
                {
                    if (iterUp->second[iInterface].GetDownstreamInterface() == iter->second)
                    {
                        upstreamInterfaces.push_back(iterUp->second[iInterface].GetUpstreamInterface());
                    }
                }
            }
        }

        if (!iter->second->Check(upstreamInterfaces))
        {
            return false;
        }
    }

    return true;
}

AbstractSimulatorInterface* TraficSimulatorHandler::GetOrCreateInterface(const std::string & strName, bool bIsUpstreamInterface)
{
    return m_lstInstances.front()->GetOrCreateInterface(strName, bIsUpstreamInterface);
}

bool TraficSimulatorHandler::GetOffers(int iInstance, const std::vector<const AbstractSimulatorInterface*> & interfacesToGetOffer, std::map<const AbstractSimulatorInterface*, double> & offerValues)
{
    return m_lstInstances[iInstance]->GetOffers(interfacesToGetOffer, offerValues);
}

bool TraficSimulatorHandler::SetCapacities(int iInstance, const std::vector<const AbstractSimulatorInterface*> & upstreamInterfaces, const AbstractSimulatorInterface* pDownstreamInterface, double dbCapacityValue)
{
    return m_lstInstances[iInstance]->SetCapacities(upstreamInterfaces, pDownstreamInterface, dbCapacityValue);
}




