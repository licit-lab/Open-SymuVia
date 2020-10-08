#include "NetworkInterfacesFileHandler.h"

#include "Simulation/Simulators/TraficSimulators.h"
#include "CSVStringUtils.h"
#include "Simulation/Simulators/SimulatorsInterface.h"

#include <Graph/Model/MultiLayersGraph.h>
#include <Graph/Model/NullPattern.h>
#include <Graph/Model/NullPenalty.h>
#include <Graph/Model/Link.h>
#include <Graph/Model/Node.h>

#include <boost/log/trivial.hpp>
#include <boost/lexical_cast.hpp>

#include <vector>
#include <set>
#include <fstream>
#include <iostream>


const char s_CSV_SEPARATOR = ';';
const int s_ID_COLUMN = 0;

using namespace SymuMaster;
using namespace SymuCore;
using namespace std;

NetworkInterfacesFileHandler::NetworkInterfacesFileHandler()
{

}

NetworkInterfacesFileHandler::~NetworkInterfacesFileHandler()
{

}

bool NetworkInterfacesFileHandler::Load(const std::string & strFilePath, SimulationDescriptor & simulationDescriptor)
{
	string line;
	ifstream myfile(strFilePath.c_str());
	std::set<std::string> upstreamInterfaceNames, downstreamInterfaceNames;
	if (myfile.is_open())
	{
		int iLine = 0;
		while (getline(myfile, line))
		{
			iLine++;
			vector<string> values = CSVStringUtils::split(line, s_CSV_SEPARATOR);

			if (values.size() != 4)
			{
				BOOST_LOG_TRIVIAL(error) << "The number of columns of the networks interfaces definition file shoud be 4 (line " << iLine << ")";
				return false;
			}

			pair<pair<int, string>, pair<int, string> > networkInterface;
			for (int iLinkedInterface = 0; iLinkedInterface < 2; iLinkedInterface++)
			{
				bool bIsUpstreamInterface = iLinkedInterface == 0;
				try
				{
					int iSimulator = boost::lexical_cast<int>(values.at(iLinkedInterface * 2));
					const std::string & interfaceName = values.at(iLinkedInterface * 2 + 1);

					if (bIsUpstreamInterface)
					{
						networkInterface.first.first = iSimulator;
						networkInterface.first.second = interfaceName;
						upstreamInterfaceNames.insert(interfaceName);
					}
					else
					{
						networkInterface.second.first = iSimulator;
						networkInterface.second.second = interfaceName;
						downstreamInterfaceNames.insert(interfaceName);

						m_interfacesFileContents.push_back(networkInterface);
					}
				}
				catch (boost::bad_lexical_cast&)
				{
					BOOST_LOG_TRIVIAL(error) << "Simulator identifier '" << values.at(iLinkedInterface * 2) << "' must be a valid integer !";
					return false;
				}
			}
		}

		myfile.close();
	}

	// Add the interfaces to the graph so we can build the graph with the knownledge of those interfaces
	simulationDescriptor.GetGraph()->SetListUpstreamInterfaces(upstreamInterfaceNames);
	simulationDescriptor.GetGraph()->SetListDownstreamInterfaces(downstreamInterfaceNames);

	return true;
}

bool NetworkInterfacesFileHandler::Apply(SimulationDescriptor & simulationDescriptor, const std::vector<TraficSimulatorHandler*> & listSimulators)
{
    bool bOk = true;

	for (size_t iLine = 0; iLine < m_interfacesFileContents.size(); iLine++)
	{
		const pair<pair<int, string>, pair<int, string> > & networkInterface = m_interfacesFileContents[iLine];
        SimulatorsInterface networksInterface;
        for (int iLinkedInterface = 0; iLinkedInterface < 2; iLinkedInterface++)
        {
            bool bIsUpstreamInterface = iLinkedInterface == 0;
			int iSimulator = bIsUpstreamInterface ? networkInterface.first.first : networkInterface.second.first;
            if (iSimulator < 1 || iSimulator > listSimulators.size())
            {
                BOOST_LOG_TRIVIAL(error) << "The simulator number ID should be between 1 and the number of defined simulators in the SymuMaster XML scenario";
                return false;
            }
            TraficSimulatorHandler * pSimulator = listSimulators.at(iSimulator - 1);

			const std::string & interfaceName = bIsUpstreamInterface ? networkInterface.first.second : networkInterface.second.second;

            AbstractSimulatorInterface * pInterface = pSimulator->GetOrCreateInterface(interfaceName, bIsUpstreamInterface);

            if (!pInterface)
            {
                BOOST_LOG_TRIVIAL(error) << "The interface '" << interfaceName << "' has not been found in the graph for simulator " << iSimulator;
                return false;
            }

            if (bIsUpstreamInterface)
            {
                networksInterface.SetUpstreamInterface(pInterface);
            }
            else
            {
                networksInterface.SetDownstreamInterface(pInterface);

                simulationDescriptor.GetSimulatorsInterfaces()[networksInterface.GetDownstreamInterface()->GetSimulator()][networksInterface.GetUpstreamInterface()->GetSimulator()].push_back(networksInterface);
            }
        }

        // Check interfaces constraints
		//This test is no longueur used since we don't have to check if one Route is connected to only one SymuVia interface
		/*for (size_t iSimulator = 0; iSimulator < listSimulators.size(); iSimulator++)
        {
            if (!listSimulators[iSimulator]->CheckInterfaces(simulationDescriptor.GetSimulatorsInterfaces()))
            {
                return false;
            }
        }*/
	}

	bOk = doLinks(simulationDescriptor);

    return bOk;
}

bool NetworkInterfacesFileHandler::doLinks(const SimulationDescriptor & simulationDescriptor)
{
    // Determine all downstream interfaces
    std::map<AbstractSimulatorInterface*, std::set<AbstractSimulatorInterface*> > downstreamInterfaces;
    const std::map<TraficSimulatorHandler*, std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> > > & interfaces = simulationDescriptor.GetSimulatorsInterfaces();
    for (std::map<TraficSimulatorHandler*, std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> > >::const_iterator iterUpstream = interfaces.begin(); iterUpstream != interfaces.end(); ++iterUpstream)
    {
        for (std::map<TraficSimulatorHandler*, std::vector<SimulatorsInterface> >::const_iterator iterDownstream = iterUpstream->second.begin(); iterDownstream != iterUpstream->second.end(); ++iterDownstream)
        {
            for (size_t iInterface = 0; iInterface < iterDownstream->second.size(); iInterface++)
            {
                const SimulatorsInterface & interface = iterDownstream->second[iInterface];

                downstreamInterfaces[interface.GetDownstreamInterface()].insert(interface.GetUpstreamInterface());
            }
        }
    }

    // Create a virtual node for each downstreaminterface
    for (std::map<AbstractSimulatorInterface*, std::set<AbstractSimulatorInterface*> >::const_iterator iterDown = downstreamInterfaces.begin(); iterDown != downstreamInterfaces.end(); ++iterDown)
    {
        AbstractSimulatorInterface * pDownInterface = iterDown->first;
        Node * pDownNode = pDownInterface->GetNode();
        Graph * pParentGraph = pDownNode->getParent();

        // virtual interface node creation
		if (pParentGraph->GetNode("HYBRID_NODE_" + pDownInterface->getStrName())){
			continue;
		}
        Node * pVirtualInterfaceNode = pParentGraph->CreateAndAddNode("HYBRID_NODE_" + pDownInterface->getStrName(), SymuCore::NT_NetworksInterface, pDownNode->getCoordinates());

        // downstream virtual lien creation
        Link * pVirtualLinkFromVirtualNode = pParentGraph->CreateAndAddLink(pVirtualInterfaceNode, pDownNode);

        // downstream virtual pattern creation
        NullPattern* newPatternFromVirtualNode = new NullPattern(pVirtualLinkFromVirtualNode);
        pVirtualLinkFromVirtualNode->getListPatterns().push_back(newPatternFromVirtualNode);

        // build the penalties at the downstream node level (depend on the interface) :
        pDownInterface->BuildDownstreamPenalties(newPatternFromVirtualNode);

        // For each linked upstream interface :
        for (std::set<AbstractSimulatorInterface*>::const_iterator iterUp = iterDown->second.begin(); iterUp != iterDown->second.end(); ++iterUp)
        {
            AbstractSimulatorInterface * pUpInterface = *iterUp;
            Node * pUpNode = pUpInterface->GetNode();

            // upstream link creation
			Link * pVirtualLinkToVirtualNode = pParentGraph->GetLink(pUpNode->getStrName(), pVirtualInterfaceNode->getStrName());
			if (!pVirtualLinkToVirtualNode){
				pVirtualLinkToVirtualNode = pParentGraph->CreateAndAddLink(pUpNode, pVirtualInterfaceNode);
			}
			else{
				continue;
			}

            // upstream virtual pattern creation
            NullPattern* newPatternToVirtualNode = new NullPattern(pVirtualLinkToVirtualNode);
            pVirtualLinkToVirtualNode->getListPatterns().push_back(newPatternToVirtualNode);

            // build the penalties at the upstream node level (depend on the interface) :
            pUpInterface->BuildUpstreamPenalties(newPatternToVirtualNode);

            // Add penalty at the virtual node level 
            AbstractPenalty * pPenalty = new NullPenalty(pVirtualInterfaceNode, PatternsSwitch(newPatternToVirtualNode, newPatternFromVirtualNode));
            pVirtualInterfaceNode->getMapPenalties()[newPatternToVirtualNode][newPatternFromVirtualNode] = pPenalty;
        }
    }
    return true;
}
