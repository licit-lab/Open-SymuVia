#include "TraficSimulatorHandler.h"

#include "Simulation/Simulators/SimulationDescriptor.h"

#include <Graph/Model/Link.h>
#include <Graph/Model/Node.h>
#include <Graph/Model/Pattern.h>
#include <Graph/Model/AbstractPenalty.h>

#include <boost/make_shared.hpp>


using namespace SymuMaster;
using namespace SymuCore;

TraficSimulatorHandler::TraficSimulatorHandler(const SimulatorConfiguration * pConfig) :
    m_pConfig(pConfig)
{

}

TraficSimulatorHandler::~TraficSimulatorHandler()
{
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



