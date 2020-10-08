#include "AssignmentModel.h"
#include "Demand/Path.h"

#include <map>
#include <vector>

using namespace SymuMaster;

AssignmentModel::AssignmentModel()
{

}

AssignmentModel::AssignmentModel(SimulationConfiguration *pSimulationConfiguration, const boost::posix_time::ptime &dtStartSimulationTime, const boost::posix_time::ptime &dtEndSimulationTime)
{
    m_SimulationConfiguration = pSimulationConfiguration;
    m_dtStartSimulationTime = dtStartSimulationTime;
    m_dtEndSimulationTime = dtEndSimulationTime;
}

AssignmentModel::~AssignmentModel()
{
}

void AssignmentModel::SetStartSimulationTime(const boost::posix_time::ptime &startSimulationTime)
{
    m_dtStartSimulationTime = startSimulationTime;
}

std::map<SymuCore::SubPopulation *, std::map<SymuCore::Origin *, std::map<SymuCore::Destination *, std::map<SymuCore::Path, std::vector<double> >, AssignmentModel::SortDestination>, AssignmentModel::SortOrigin>, AssignmentModel::SortSubPopulation> &AssignmentModel::GetListPredefinedPaths()
{
    return m_mapPredefinedPaths;
}

const SimulationConfiguration *AssignmentModel::GetSimulationConfiguration() const
{
    return m_SimulationConfiguration;
}

void AssignmentModel::SetSimulationConfiguration(const SimulationConfiguration *pSimulationConfiguration)
{
    m_SimulationConfiguration = pSimulationConfiguration;
}

void AssignmentModel::SetEndSimulationTime(const boost::posix_time::ptime &dtEndSimulationTime)
{
    m_dtEndSimulationTime = dtEndSimulationTime;
}

