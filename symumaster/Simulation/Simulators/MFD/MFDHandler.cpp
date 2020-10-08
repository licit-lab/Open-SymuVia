#include "MFDHandler.h"


using namespace SymuMaster;
using namespace SymuCore;


MFDHandler::MFDHandler(const SimulatorConfiguration * pConfig, TraficSimulators * pParentTraficSimulators) :
TraficSimulatorHandler(pConfig, pParentTraficSimulators)
{
}

MFDHandler::~MFDHandler()
{

}

void MFDHandler::AddSimulatorInstance(TraficSimulatorInstance * pSimInstance)
{
    TraficSimulatorHandler::AddSimulatorInstance(pSimInstance);

    m_lstMapCostByRoute.resize(m_lstMapCostByRoute.size() + 1);
}

const std::map<int, std::map<SymuCore::CostFunction, SymuCore::Cost> > & MFDHandler::GetMapCost(int iInstance) const
{
    return m_lstMapCostByRoute.at(iInstance);
}

std::map<int, std::map<SymuCore::CostFunction, SymuCore::Cost> > & MFDHandler::GetMapCost(int iInstance)
{
    return m_lstMapCostByRoute.at(iInstance);
}
