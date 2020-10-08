#include "MFDInterface.h"

#include "Simulation/Simulators/TraficSimulatorHandler.h"
#include "MFDDrivingPattern.h"

#include <Graph/Model/MultiLayersGraph.h>
#include <Graph/Model/Node.h>
#include <Graph/Model/Link.h>
#include <Graph/Model/NullPenalty.h>

#include <boost/log/trivial.hpp>


using namespace SymuMaster;
using namespace SymuCore;

MFDInterface::MFDInterface() :
AbstractSimulatorInterface(),
m_iRouteID(-1)
{

}

MFDInterface::MFDInterface(TraficSimulatorHandler * pSimulator, const std::string & strName, bool bUpstream) :
AbstractSimulatorInterface(pSimulator, strName, bUpstream)
{
    m_iRouteID = atoi(strName.c_str());

    // Le noeud correspond au réservoir amont de la route de nom strName
    const std::vector<Node*> & nodes = pSimulator->GetSimulationDescriptor().GetGraph()->GetListNodes();
    for (size_t iNode = 0; iNode < nodes.size() && !m_pNode; iNode++)
    {
        if (nodes[iNode]->getNodeType() == NT_MacroNode)
        {
            // Il faut identifier un éventuel pattern de type route MFD avec le nom strName
            const std::vector<Link*> & links = bUpstream ? nodes[iNode]->getDownstreamLinks() : nodes[iNode]->getUpstreamLinks();
            for (size_t iLink = 0; iLink < links.size() && !m_pNode; iLink++)
            {
                Link * pLink = links[iLink];
                for (size_t iPattern = 0; iPattern < pLink->getListPatterns().size() && !m_pNode; iPattern++)
                {
                    MFDDrivingPattern * pMFDPattern = dynamic_cast<MFDDrivingPattern*>(pLink->getListPatterns()[iPattern]);
                    if (pMFDPattern && pMFDPattern->getRouteID() == m_iRouteID)
                    {
                        m_pNode = bUpstream ? pLink->getDownstreamNode() : pLink->getUpstreamNode();
                        break;
                    }
                }
            }
        }
    }

    assert(m_pNode);
}

bool MFDInterface::CanRestrictCapacity() const
{
	return true;
}

bool MFDInterface::CanComputeOffer() const
{
	return true;
}

bool MFDInterface::Check(std::vector<AbstractSimulatorInterface*> & associatedInterfaces) const
{
    if (m_bUpstream)
	{
		//This test is not used anymore. On route can be connected to many SymuVia interface
        // Only one downstream interface for a given route
        /*if (associatedInterfaces.size() != 1)
        {
            BOOST_LOG_TRIVIAL(error) << "The MFD Interface for upstream route " << m_iRouteID << " must have exactly one downstream interface (one MFD route or one symuvia exit), but has " << associatedInterfaces.size();
            return false;
        }*/
    }
    else
    { 
		//This test is not used anymore. On route can be connected to many SymuVia interface
        // Only one upstream interface for a given route
        /*if (associatedInterfaces.size() != 1)
        {
            BOOST_LOG_TRIVIAL(error) << "The MFD Interface for downstream route " << m_iRouteID << " must have exactly one upstream interface (one MFD route or one symuvia entry), but has " << associatedInterfaces.size();
            return false;
        }*/
    }

    return true;
}

void MFDInterface::BuildUpstreamPenalties(SymuCore::Pattern * pPatternToVirtualNode)
{
    // Pour SimuRes, on a ici un pattern qui suit la route amont. Il faut définir ici une pénalité entre le pattern
    // correspondant à la route amont (et uniquement celle-ci) et le pattern d'interface aval à la route passé en argument :
    Node * pExitReservoirNode = pPatternToVirtualNode->getLink()->getUpstreamNode();
    for (size_t iUpLink = 0; iUpLink < pExitReservoirNode->getUpstreamLinks().size(); iUpLink++)
    {
        Link * pUpLink = pExitReservoirNode->getUpstreamLinks()[iUpLink];

        for (size_t iUpPattern = 0; iUpPattern < pUpLink->getListPatterns().size(); iUpPattern++)
        {
            Pattern * pUpPattern = pUpLink->getListPatterns()[iUpPattern];

            MFDDrivingPattern * pRoutePattern = dynamic_cast<MFDDrivingPattern*>(pUpPattern);
			if (pRoutePattern)
            {
                AbstractPenalty * pPenalty = new NullPenalty(pExitReservoirNode, PatternsSwitch(pUpPattern, pPatternToVirtualNode));
                pExitReservoirNode->getMapPenalties()[pUpPattern][pPatternToVirtualNode] = pPenalty;
            }
            else if (!pRoutePattern){ // Cas si on est sur un pattern pour changement de Graph (et donc pas un MFDDrivingPattern)
                AbstractPenalty * pPenalty = new NullPenalty(pExitReservoirNode, PatternsSwitch(pUpPattern, pPatternToVirtualNode));
                pExitReservoirNode->getMapPenalties()[pUpPattern][pPatternToVirtualNode] = pPenalty;
            }
        }
    }
}

void MFDInterface::BuildDownstreamPenalties(SymuCore::Pattern * pPatternFromVirtualNode)
{
    // Pour SimuRes, on a ici un pattern qui doit mener à la route aval. Il faut définir ici une pénalité entre le pattern
    // amont passé en argument et le pattern de la route aval (et uniquement celle-ci) :
    Node * pEntryReservoirNode = pPatternFromVirtualNode->getLink()->getDownstreamNode();
    for (size_t iDownLink = 0; iDownLink < pEntryReservoirNode->getDownstreamLinks().size(); iDownLink++)
    {
        Link * pDownLink = pEntryReservoirNode->getDownstreamLinks()[iDownLink];

        for (size_t iDownPattern = 0; iDownPattern < pDownLink->getListPatterns().size(); iDownPattern++)
        {
            Pattern * pDownPattern = pDownLink->getListPatterns()[iDownPattern];

            MFDDrivingPattern * pRoutePattern = dynamic_cast<MFDDrivingPattern*>(pDownPattern);
			if (pRoutePattern)
            {
                AbstractPenalty * pPenalty = new NullPenalty(pEntryReservoirNode, PatternsSwitch(pPatternFromVirtualNode, pDownPattern));
                pEntryReservoirNode->getMapPenalties()[pPatternFromVirtualNode][pDownPattern] = pPenalty;
            }
            else if (!pRoutePattern){ // Cas si on est sur un pattern pour changement de Graph (et donc pas un MFDDrivingPattern)
                AbstractPenalty * pPenalty = new NullPenalty(pEntryReservoirNode, PatternsSwitch(pPatternFromVirtualNode, pDownPattern));
                pEntryReservoirNode->getMapPenalties()[pPatternFromVirtualNode][pDownPattern] = pPenalty;
            }
        }
    }
}


