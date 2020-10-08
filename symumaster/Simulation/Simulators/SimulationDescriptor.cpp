#include "SimulationDescriptor.h"

#include <Graph/Model/MultiLayersGraph.h>

using namespace SymuMaster;
using namespace SymuCore;

SimulationDescriptor::SimulationDescriptor()
{
    m_pGraph = new MultiLayersGraph();
}

SimulationDescriptor::~SimulationDescriptor()
{
    // SimulationDescriptor does'nt have ownership on the m_pGraph, so do not delete m_pGraph here.
}

void SimulationDescriptor::SetStartTime(boost::posix_time::ptime startTime)
{
    m_StartTime = startTime;
}

boost::posix_time::ptime SimulationDescriptor::GetStartTime()
{
    return m_StartTime;
}

void SimulationDescriptor::SetEndTime(boost::posix_time::ptime endTime)
{
    m_EndTime = endTime;
}

boost::posix_time::ptime SimulationDescriptor::GetEndTime()
{
    return m_EndTime;
}

void SimulationDescriptor::SetTimeStepDuration(boost::posix_time::time_duration timeStep)
{
    m_TimeStepDuration = timeStep;
}

boost::posix_time::time_duration SimulationDescriptor::GetTimeStepDuration()
{
    return m_TimeStepDuration;
}

SymuCore::MultiLayersGraph * SimulationDescriptor::GetGraph()
{
    return m_pGraph;
}

SymuCore::Populations& SimulationDescriptor::GetPopulations()
{
    return m_Populations;
}

const Populations &SimulationDescriptor::GetPopulations() const
{
    return m_Populations;
}

void SimulationDescriptor::SetPopulations(const SymuCore::Populations &Populations)
{
    m_Populations = Populations;
}

std::map<SymuCore::ServiceType, std::vector<SymuCore::SubPopulation *> >& SimulationDescriptor::GetSubPopulationsByServiceType()
{
    return m_mapSubPopulationsByServiceType;
}

const std::map<SymuCore::ServiceType, std::vector<SymuCore::SubPopulation *> >& SimulationDescriptor::GetSubPopulationsByServiceType() const
{
    return m_mapSubPopulationsByServiceType;
}

void SimulationDescriptor::SetSubPopulationsByServiceType(const std::map<SymuCore::ServiceType, std::vector<SymuCore::SubPopulation *> > &mapSubpopulationsByServiceType)
{
    m_mapSubPopulationsByServiceType = mapSubpopulationsByServiceType;
}

std::map<TraficSimulatorHandler*, std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> > > & SimulationDescriptor::GetSimulatorsInterfaces()
{
    return m_mapSimulatorsInterfaces;
}

const std::map<TraficSimulatorHandler*, std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> > > & SimulationDescriptor::GetSimulatorsInterfaces() const
{
    return m_mapSimulatorsInterfaces;
}
