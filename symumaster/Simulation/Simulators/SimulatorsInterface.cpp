#include "SimulatorsInterface.h"

using namespace SymuMaster;
using namespace SymuCore;

AbstractSimulatorInterface::AbstractSimulatorInterface() :
    m_pSimulator(nullptr),
    m_bUpstream(false),
    m_pNode(nullptr)
{

}

AbstractSimulatorInterface::AbstractSimulatorInterface(TraficSimulatorHandler * pSimulator, const std::string & strName, bool bUpstream) :
    m_pSimulator(pSimulator),
    m_strName(strName),
    m_bUpstream(bUpstream),
    m_pNode(nullptr)
{

}

const std::string & AbstractSimulatorInterface::getStrName() const
{
    return m_strName;
}

TraficSimulatorHandler * AbstractSimulatorInterface::GetSimulator() const
{
    return m_pSimulator;
}

SymuCore::Node * AbstractSimulatorInterface::GetNode() const
{
    return m_pNode;
}

SimulatorsInterface::SimulatorsInterface() :
    m_pUpstreamInterface(nullptr),
    m_pDownstreamInterface(nullptr)
{

}

SimulatorsInterface::SimulatorsInterface(AbstractSimulatorInterface * pUpstreamInterface, AbstractSimulatorInterface * pDownstreamInterface) :
    m_pUpstreamInterface(pUpstreamInterface),
    m_pDownstreamInterface(pDownstreamInterface)
{

}

void SimulatorsInterface::SetUpstreamInterface(AbstractSimulatorInterface * pUpstreamInterface)
{
    m_pUpstreamInterface = pUpstreamInterface;
}

void SimulatorsInterface::SetDownstreamInterface(AbstractSimulatorInterface * pDownstreamInterface)
{
    m_pDownstreamInterface = pDownstreamInterface;
}

AbstractSimulatorInterface * SimulatorsInterface::GetUpstreamInterface() const
{
    return m_pUpstreamInterface;
}

AbstractSimulatorInterface * SimulatorsInterface::GetDownstreamInterface() const
{
    return m_pDownstreamInterface;
}

bool SimulatorsInterface::HasToAdaptCapacityAndOffer() const
{
	return m_pUpstreamInterface->CanRestrictCapacity() && m_pDownstreamInterface->CanComputeOffer();
}
