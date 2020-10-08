#include "UserEngines.h"

#include "UserEngine.h"
#include "Simulation/Simulators/Walk/WalkSimulator.h"
#include "Simulation/Simulators/TraficSimulatorHandler.h"
#include "Simulation/Config/SimulationConfiguration.h"
#include "UserEngineSnapshot.h"
#include "UserPathWriting.h"

#include <Graph/Model/Pattern.h>
#include <Graph/Model/PublicTransport/PublicTransportPattern.h>
#include <Graph/Model/Link.h>
#include <Graph/Model/Graph.h>
#include <Graph/Model/MultiLayersGraph.h>

#include <Demand/Populations.h>
#include <Demand/Population.h>
#include <Demand/Trip.h>
#include <Demand/Origin.h>
#include <Demand/Destination.h>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/trivial.hpp>

#include <vector>
#include <fstream>

using namespace SymuCore;
using namespace SymuMaster;

UserEngines::UserEngines()
{
}

UserEngines::~UserEngines()
{
    for (size_t i = 0; i < m_lstEngines.size(); i++)
    {
        delete m_lstEngines[i];
    }
}

const std::vector<UserEngine*> & UserEngines::GetUserEngines() const
{
    return m_lstEngines;
}

std::vector<UserEngine*> & UserEngines::GetUserEngines()
{
    return m_lstEngines;
}

void UserEngines::Initialize(int nbInstances, const SimulationConfiguration & config, const std::vector<TraficSimulatorHandler*> & lstExternalTraficSimulators)
{
    for (int iInstance = 0; iInstance < nbInstances; iInstance++)
    {
        UserEngine * pUserEngine = new UserEngine(iInstance);

        std::vector<TraficSimulatorInstance*> simulatorInstances;
        for (size_t i = 0; i < lstExternalTraficSimulators.size(); i++)
        {
            simulatorInstances.push_back(lstExternalTraficSimulators[i]->GetSimulatorInstances()[iInstance]);
        }
        
        pUserEngine->Initialize(config, simulatorInstances);
        m_lstEngines.push_back(pUserEngine);
    }
}

void UserEngines::AssignmentPeriodStart(int nbMaxInstancesNb)
{
    for (size_t i = 0; i < m_lstEngines.size() && i < (size_t)nbMaxInstancesNb; i++)
    {
        m_lstEngines[i]->AssignmentPeriodStart();
    }
}

void UserEngines::AssignmentPeriodRollback(int nbMaxInstancesNb)
{
    for (size_t i = 0; i < m_lstEngines.size() && i < (size_t)nbMaxInstancesNb; i++)
    {
        m_lstEngines[i]->AssignmentPeriodRollback();
    }
}

void UserEngines::AssignmentPeriodCommit()
{
    for (size_t i = 0; i < m_lstEngines.size(); i++)
    {
        m_lstEngines[i]->AssignmentPeriodCommit();
    }
}
