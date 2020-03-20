#pragma once

#ifndef SYMUCORE_SHORTESTPATHSCOMPUTER_H
#define SYMUCORE_SHORTESTPATHSCOMPUTER_H

#include "SymuCoreExports.h"

#include "Demand/Path.h"

#include <map>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class Destination;
class Origin;
class SubPopulation;
class ValuetedPath;

class SYMUCORE_DLL_DEF ShortestPathsComputer {

public:

    // Structure used to keep information about Nodes during the travel of the graph
    struct TimedNode {

        TimedNode()
        {
        }

        ~TimedNode()
        {
        }

        TimedNode(Node* pNode, TimedNode* pPredecossorNode, Pattern* pPredecossorPattern, double dTotalCost, double dTotalTime)
        {
            m_pNode = pNode;
            m_pPredecossorNode = pPredecossorNode;
            m_pPredecossorPattern = pPredecossorPattern;
            m_dTotalCost = dTotalCost;
            m_dTotalTime = dTotalTime;
        }

        inline bool operator==(TimedNode* otherTimedNode)
        {
            return otherTimedNode->m_pNode == m_pNode;
        }

        Node *							m_pNode; //pointer to the node
        TimedNode *						m_pPredecossorNode; //direct predecessor Node
        Pattern *						m_pPredecossorPattern; //direct predecessor Pattern
        double							m_dTotalCost; //total cost to reach this Node
        double                  		m_dTotalTime; // total time to reach this Node
    };

    ShortestPathsComputer();
	virtual ~ShortestPathsComputer();

    virtual std::map<Origin*, std::map<Destination*, std::vector<ValuetedPath*> > > getShortestPaths(int iSimulationInstance, const std::vector<Origin*>& originNodes, const std::vector<Destination*>& destinationNodes, SubPopulation* pSubPopulation, double dbArrivalTime, bool bUseCostsForTemporalAlgorithm = false) =0;
};
}

#pragma warning(pop)

#endif // SYMUCORE_SHORTESTPATHSCOMPUTER_H
