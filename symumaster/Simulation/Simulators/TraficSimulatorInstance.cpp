#include "TraficSimulatorInstance.h"

#include "Simulation/Simulators/SimulationDescriptor.h"

#include <Graph/Model/Link.h>
#include <Graph/Model/Node.h>
#include <Graph/Model/Pattern.h>
#include <Graph/Model/AbstractPenalty.h>

#include <boost/make_shared.hpp>


using namespace SymuMaster;
using namespace SymuCore;

TraficSimulatorInstance::TraficSimulatorInstance(TraficSimulatorHandler * pParentSimulationHandler, int iInstance) :
m_pParentSimulationHandler(pParentSimulationHandler),
m_iInstance(iInstance)
{

}

TraficSimulatorInstance::~TraficSimulatorInstance()
{
    for (std::map<std::string, AbstractSimulatorInterface*>::const_iterator iter = m_mapSimulatorUpstreamInterfaces.begin(); iter != m_mapSimulatorUpstreamInterfaces.end(); ++iter)
    {
        delete iter->second;
    }
    for (std::map<std::string, AbstractSimulatorInterface*>::const_iterator iter = m_mapSimulatorDownstreamInterfaces.begin(); iter != m_mapSimulatorDownstreamInterfaces.end(); ++iter)
    {
        delete iter->second;
    }
}

TraficSimulatorHandler * TraficSimulatorInstance::GetParentSimulator()
{
    return m_pParentSimulationHandler;
}

AbstractSimulatorInterface* TraficSimulatorInstance::GetOrCreateInterface(const std::string & strName, bool bIsUpstreamInterface)
{
    std::map<std::string, AbstractSimulatorInterface*> & mapSimulatorInterfaces = bIsUpstreamInterface ? m_mapSimulatorUpstreamInterfaces : m_mapSimulatorDownstreamInterfaces;
    AbstractSimulatorInterface * pInterface = NULL;
    std::map<std::string, AbstractSimulatorInterface*>::const_iterator iter = mapSimulatorInterfaces.find(strName);
    if (iter != mapSimulatorInterfaces.end())
    {
        pInterface = iter->second;
    }
    else
    {
        pInterface = MakeInterface(strName, bIsUpstreamInterface);
		if (!pInterface->GetNode())
		{
			delete pInterface;
			return NULL;
		}
        mapSimulatorInterfaces[strName] = pInterface;
		pInterface->GetNode()->setIsSimulatorsInterface();
    }
    return pInterface;
}
