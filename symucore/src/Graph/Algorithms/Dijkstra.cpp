#include "Dijkstra.h"
#include "Graph/Model/Node.h"
#include "Graph/Model/Link.h"
#include "Graph/Model/Graph.h"
#include "Graph/Model/Pattern.h"
#include "Demand/Origin.h"
#include "Demand/Destination.h"
#include "Demand/Population.h"
#include "Demand/SubPopulation.h"
#include "Demand/Path.h"
#include "Graph/Model/Cost.h"
#include "Utils/Point.h"
#include "Demand/ValuetedPath.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <vector>
#include <set>

using namespace SymuCore;
using namespace std;
using namespace boost::posix_time;

Dijkstra::Dijkstra()
{
    m_eHeuristic = SPH_NONE;
    m_dbAStarBeta = 0;
    m_dbHeuristicGamma = 0;
    m_bFromOriginToDestination = false;
}

Dijkstra::Dijkstra(bool bFromOriginToDestination)
{
    m_eHeuristic = SPH_NONE;
    m_dbAStarBeta = 0;
    m_dbHeuristicGamma = 0;
    m_bFromOriginToDestination = bFromOriginToDestination;
}

Dijkstra::~Dijkstra()
{
}

void Dijkstra::SetHeuristic(ShortestPathHeuristic eHeuristic, double dbAStarBeta, double dbHeuristicGamma)
{
    m_eHeuristic = eHeuristic;
    m_dbAStarBeta = dbAStarBeta;
    m_dbHeuristicGamma = dbHeuristicGamma;
}

struct TargetPoint {

    TargetPoint(Node* pNode, Pattern * pPattern, size_t index)
    {
        m_pNode = pNode;
        m_pPattern = pPattern;
        m_Index = index;
    }

    Node * m_pNode;
    Pattern * m_pPattern;
    size_t m_Index;
};

class fromOriginToDestinations {
public:
    fromOriginToDestinations(Origin * pOrigin, const std::vector<Destination*> & lstDestinations) :
        m_pOrigin(pOrigin),
        m_lstDestinations(lstDestinations)
    {
    }

    bool isFromOriginToDestination() const
    {
        return true;
    }

    Node * getInitialNode() const
    {
        return m_pOrigin->getNode();
    }

    Pattern * getInitialPattern() const
    {
        return m_pOrigin->getPattern();
    }

    std::vector<TargetPoint> buildFinalPoints() const
    {
        std::vector<TargetPoint> result;
        for (size_t iDestination = 0; iDestination < m_lstDestinations.size(); iDestination++)
        {
            Destination * pDest = m_lstDestinations.at(iDestination);
            result.push_back(TargetPoint(pDest->getNode(), pDest->getPattern(), iDestination));
        }
        return std::move(result);
    }

    const std::vector<Link*> & getNextLinks(Node * pNode) const {
        return pNode->getDownstreamLinks();
    }

    Node * getNextNode(Link * pLink) const {
        return pLink->getDownstreamNode();
    }

    double getNextTime(double dbStartTime, double dbAdditionalTime) const
    {
        return dbStartTime + dbAdditionalTime;
    }

    Cost * getPenaltyCost(int iSimulationInstance, Node * pNode, Pattern* nextPattern, Pattern* previousPattern, double t, SubPopulation* pSubPopulation) const
    {
        return pNode->getPenaltyCost(iSimulationInstance, previousPattern, nextPattern, t, pSubPopulation);
    }

private:
    Origin * m_pOrigin;
    const std::vector<Destination*> & m_lstDestinations;
};

class fromDestinationToOrigins {
public:
    fromDestinationToOrigins(Destination * pDest, const std::vector<Origin*> & lstOrigins) :
        m_pDestination(pDest),
        m_lstOrigins(lstOrigins)
    {
    }

    bool isFromOriginToDestination() const
    {
        return false;
    }

    Node * getInitialNode() const
    {
        return m_pDestination->getNode();
    }

    Pattern * getInitialPattern() const
    {
        return m_pDestination->getPattern();
    }

    std::vector<TargetPoint> buildFinalPoints() const
    {
        std::vector<TargetPoint> result;
        for (size_t iOrigin = 0; iOrigin < m_lstOrigins.size(); iOrigin++)
        {
            Origin * pOrigin = m_lstOrigins.at(iOrigin);
            result.push_back(TargetPoint(pOrigin->getNode(), pOrigin->getPattern(), iOrigin));
        }
        return std::move(result);
    }

    const std::vector<Link*> & getNextLinks(Node * pNode) const {
        return pNode->getUpstreamLinks();
    }

    Node * getNextNode(Link * pLink) const {
        return pLink->getUpstreamNode();
    }

    double getNextTime(double dbStartTime, double dbAdditionalTime) const
    {
        return dbStartTime - dbAdditionalTime;
    }

    Cost * getPenaltyCost(int iSimulationInstance, Node * pNode, Pattern* nextPattern, Pattern* previousPattern, double t, SubPopulation* pSubPopulation) const
    {
        return pNode->getPenaltyCost(iSimulationInstance, nextPattern, previousPattern, t, pSubPopulation);
    }

private:
    Destination * m_pDestination;
    const std::vector<Origin*> & m_lstOrigins;
};


map<Origin*, map<Destination*, vector<ValuetedPath*> > > Dijkstra::getShortestPaths(int iSimulationInstance, const vector<Origin*>& originNodes, const vector<Destination*>& destinationNodes, SubPopulation *pSubPopulation, double dbStartingPointTime, bool bUseCostsForTemporalAlgorithm)
{
	map<Origin*, map<Destination*, vector<ValuetedPath*> > > result;
    if (m_bFromOriginToDestination)
    {
        for (size_t iOrigin = 0; iOrigin < originNodes.size(); iOrigin++)
        {
            Origin * pOrigin = originNodes.at(iOrigin);
            fromOriginToDestinations graphTraverser(pOrigin, destinationNodes);
            const std::vector<vector<ValuetedPath*> > & resultsForOrigin = getShortestPaths(iSimulationInstance, pSubPopulation, dbStartingPointTime, graphTraverser, bUseCostsForTemporalAlgorithm);
            for (size_t iDestination = 0; iDestination < destinationNodes.size(); iDestination++)
            {
                result[pOrigin][destinationNodes.at(iDestination)] = std::move(resultsForOrigin.at(iDestination));
            }
        }
    }
    else
    {
        for (size_t iDestination = 0; iDestination < destinationNodes.size(); iDestination++)
        {
            Destination * pDest = destinationNodes.at(iDestination);
            fromDestinationToOrigins graphTraverser(pDest, originNodes);
            const std::vector<vector<ValuetedPath*> > & resultsForDest = getShortestPaths(iSimulationInstance, pSubPopulation, dbStartingPointTime, graphTraverser, bUseCostsForTemporalAlgorithm);
            for (size_t iOrigin = 0; iOrigin < originNodes.size(); iOrigin++)
            {
                result[originNodes.at(iOrigin)][pDest] = std::move(resultsForDest.at(iOrigin));
            }
        }
    }
    return result;
}

template<class T>
std::vector<vector<ValuetedPath*> > Dijkstra::getShortestPaths(int iSimulationInstance, SubPopulation *pSubPopulation, double dbStartingPointTime, const T & graphTraverser, bool bUseCostsForTemporalAlgorithm)
{
    map<double, vector<TimedNode*> > mapWaitingNodes; //map of all nodes that have to be checked, sorted by cost
    vector<TimedNode*> listTimedNodes; //list of all timedNode (used for clear)
    vector<TargetPoint> finalNodes = graphTraverser.buildFinalPoints();
    std::vector<vector<ValuetedPath*> > listResultsPaths(finalNodes.size());
    Population* pPopulation = pSubPopulation->GetPopulation();
    Node * pNextNode;

    Point * pOnlyEndPosition = NULL;
    if (m_eHeuristic == SPH_EUCLIDIAN)
    {
        assert(finalNodes.size() == 1); // can do A* with only 1 origin and 1 destination

        pOnlyEndPosition = finalNodes.front().m_pNode->getCoordinates();
    }

    // fast access to node's current weight
	std::map<std::string, std::map<int, double> > mapWeights;

    TimedNode* timedInitialNode = new TimedNode(graphTraverser.getInitialNode(), NULL, NULL, 0.0, dbStartingPointTime);
    listTimedNodes.push_back(timedInitialNode);

    // particular case of destinations that are the origin (a path from a zone to the same zone) :
    for (vector<TargetPoint>::iterator iterDest = finalNodes.begin(); iterDest != finalNodes.end();)
    {
        if (iterDest->m_pNode == graphTraverser.getInitialNode() && !iterDest->m_pPattern && !graphTraverser.getInitialPattern())
        {
            ValuetedPath* newValuetedPath = new ValuetedPath(timedInitialNode, graphTraverser.isFromOriginToDestination());
            listResultsPaths[iterDest->m_Index].push_back(newValuetedPath);

            iterDest = finalNodes.erase(iterDest);
        }
        else
        {
            ++iterDest;
        }
    }


    if (!finalNodes.empty())
    {
        //initialization of the map listWaitingNodes for the first iteration
		const std::vector<Link*> & nextLinks = graphTraverser.getNextLinks(timedInitialNode->m_pNode);
		for (size_t iLink = 0; iLink < nextLinks.size(); iLink++)
        {
			Link * pLink = nextLinks.at(iLink);
            pNextNode = graphTraverser.getNextNode(pLink);

            if (!(pPopulation->hasAccess(pNextNode->getParent()->GetServiceType())))
            {
                continue;
            }

			const std::vector<Pattern*> & listPatterns = pLink->getListPatterns();
            for (size_t iPattern = 0; iPattern < listPatterns.size(); iPattern++)
            {
				Pattern * pPattern = listPatterns.at(iPattern);
                // If a destination pattern is set, add only this pattern as available starting TimedNode.
                if (graphTraverser.getInitialPattern() == NULL || pPattern == graphTraverser.getInitialPattern())
                {
                    Cost pPatternCost = GetTotalPatternCost(iSimulationInstance, pPattern, dbStartingPointTime, pSubPopulation);

                    double dbCostValue = pPatternCost.getCostValue();

                    if (pPatternCost.getCostValue() >= 0.0)
                    {
                        TimedNode* newTimedNode = new TimedNode(pNextNode, timedInitialNode, pPattern, dbCostValue, graphTraverser.getNextTime(dbStartingPointTime, pPatternCost.getTravelTime()));
                        listTimedNodes.push_back(newTimedNode);

                        double dbCostValueWithHeuristic = dbCostValue;
                        if (m_eHeuristic == SPH_EUCLIDIAN)
                        {
                            dbCostValueWithHeuristic += m_dbHeuristicGamma * pOnlyEndPosition->distanceTo(pNextNode->getCoordinates());
                        }
                        mapWaitingNodes[dbCostValueWithHeuristic].push_back(newTimedNode);
						mapWeights[newTimedNode->m_pNode->getStrName()][pPattern->getUniqueID()] = dbCostValueWithHeuristic;
                    }
                }
            }
        }
    }


    while (!mapWaitingNodes.empty() && !finalNodes.empty())
    {
		vector<TimedNode*>& listOfNodes = mapWaitingNodes.begin()->second;
        TimedNode* currentTimedNode = listOfNodes[0];  // Timed node with the smaller cost
        //remove it from the map
        listOfNodes.erase(listOfNodes.begin());
        if (listOfNodes.empty())
            mapWaitingNodes.erase(mapWaitingNodes.begin());

		const std::vector<Link*> & nextLinks = graphTraverser.getNextLinks(currentTimedNode->m_pNode);
		for (size_t iLink = 0; iLink < nextLinks.size(); iLink++)
        {
			Link * pLink = nextLinks.at(iLink);
            pNextNode = graphTraverser.getNextNode(pLink);

            // for performance reasons, we ignore links leading to areas if the area is not in the origin list (takes significant time for nothing 
            // for networks with areas which is the critical case)
			// 20190625 : added the isNetworksInterface condition to make network interfaces by areas work.
			if ((pNextNode->getNodeType() == NT_Area || pNextNode->getNodeType() == NT_SubArea) && !pNextNode->isSimulatorsInterface())
            {
                bool bLeadsToTarget = false;
                for (size_t iTarget = 0; iTarget < finalNodes.size(); iTarget++)
                {
                    const TargetPoint & target = finalNodes.at(iTarget);
                    if (target.m_pNode == pNextNode)
                    {
                        bLeadsToTarget = true;
                        break;
                    }
                    else if ((target.m_pNode->getNodeType() == NT_InterLayerOrigin || target.m_pNode->getNodeType() == NT_InterLayerDestination)
                        && target.m_pNode->getStrName() == pNextNode->getStrName())
                    {
                        // case of the origin as a interlayer origin/destination : have to check the next node or the node name...
                        bLeadsToTarget = true;
                        break;
                    }
                }
                if (!bLeadsToTarget)
                {
                    continue;
                }
            }

            if (!(pPopulation->hasAccess(pNextNode->getParent()->GetServiceType())))
            {
                continue;
            }

			const std::vector<Pattern*> & listPatterns = pLink->getListPatterns();
            for (size_t iPattern = 0; iPattern < listPatterns.size(); iPattern++)
            {
				Pattern * pPattern = listPatterns.at(iPattern);
                Cost* currentAbstractPenaltyCost = graphTraverser.getPenaltyCost(iSimulationInstance, currentTimedNode->m_pNode, pPattern, currentTimedNode->m_pPredecossorPattern, bUseCostsForTemporalAlgorithm ? graphTraverser.getNextTime(dbStartingPointTime, currentTimedNode->m_dTotalCost) : currentTimedNode->m_dTotalTime, pSubPopulation);
				if (currentAbstractPenaltyCost == NULL || currentAbstractPenaltyCost->getCostValue() == numeric_limits<double>::infinity()){
					continue;
				}

                Cost patternCost = GetTotalPatternCost(iSimulationInstance, pPattern, graphTraverser.getNextTime(bUseCostsForTemporalAlgorithm ? graphTraverser.getNextTime(dbStartingPointTime, currentTimedNode->m_dTotalCost) : currentTimedNode->m_dTotalTime, bUseCostsForTemporalAlgorithm ? currentAbstractPenaltyCost->getCostValue() : currentAbstractPenaltyCost->getTravelTime()), pSubPopulation);

                if (patternCost.getCostValue() >= 0.0 && patternCost.getCostValue() < numeric_limits<double>::infinity())
                {
                    double currentCostValue = currentTimedNode->m_dTotalCost
                        + patternCost.getCostValue()
                        + currentAbstractPenaltyCost->getCostValue();

                    double dbCurrentCostValueWithHeuristic = currentCostValue;
                    if (m_eHeuristic == SPH_EUCLIDIAN)
                    {
                        dbCurrentCostValueWithHeuristic += m_dbHeuristicGamma * pOnlyEndPosition->distanceTo(pNextNode->getCoordinates());
                    }

                    // get the current target node weight
					std::map<std::string, std::map<int, double> >::iterator iterCurrentWeight = mapWeights.find(pNextNode->getStrName());
                    if (iterCurrentWeight == mapWeights.end())
                    {
                        // first occurence of the target node : add it to the map
                        TimedNode* newTimedNode = new TimedNode(pNextNode, currentTimedNode, pPattern, currentCostValue, graphTraverser.getNextTime(currentTimedNode->m_dTotalTime, patternCost.getTravelTime() + currentAbstractPenaltyCost->getTravelTime()));
						listTimedNodes.push_back(newTimedNode);
                        mapWaitingNodes[dbCurrentCostValueWithHeuristic].push_back(newTimedNode);
						mapWeights[pNextNode->getStrName()][pPattern->getUniqueID()] = dbCurrentCostValueWithHeuristic;
                    }
                    else
                    {
						std::map<int, double>::iterator iterCurrentPattern = iterCurrentWeight->second.find(pPattern->getUniqueID());

                        if (iterCurrentPattern == iterCurrentWeight->second.end())
                        {
                            // first occurence of the target node : add it to the map
                            TimedNode* newTimedNode = new TimedNode(pNextNode, currentTimedNode, pPattern, currentCostValue, graphTraverser.getNextTime(currentTimedNode->m_dTotalTime, patternCost.getTravelTime() + currentAbstractPenaltyCost->getTravelTime()));
							listTimedNodes.push_back(newTimedNode);
                            mapWaitingNodes[dbCurrentCostValueWithHeuristic].push_back(newTimedNode);
                            iterCurrentWeight->second[pPattern->getUniqueID()] = dbCurrentCostValueWithHeuristic;
                        }
                        else if (dbCurrentCostValueWithHeuristic < iterCurrentPattern->second)
                        {
                            TimedNode * pNewTimedNode = NULL;
                            // update the cost :
                            std::vector<TimedNode*> & previousCostTimedNodes = mapWaitingNodes.at(iterCurrentPattern->second);
                            for (size_t iNode = 0; iNode < previousCostTimedNodes.size(); iNode++)
                            {
                                // remove TimedNode from the old cost map :
                                TimedNode * pTimedNode = previousCostTimedNodes[iNode];
                                if (pTimedNode->m_pPredecossorPattern == pPattern)
                                {
                                    pNewTimedNode = pTimedNode;

                                    assert(pTimedNode->m_pNode == pNextNode);

                                    // swap with last element and pop_back to avoid memory reallocation
                                    previousCostTimedNodes[iNode] = previousCostTimedNodes.back();
                                    previousCostTimedNodes.pop_back();
                                    break;
                                }
                            }
                            if (previousCostTimedNodes.empty())
                            {
                                mapWaitingNodes.erase(iterCurrentPattern->second);
                            }

                            assert(pNewTimedNode);

                            // update cost in the TimedNode object :
                            pNewTimedNode->m_dTotalCost = currentCostValue;
                            pNewTimedNode->m_pPredecossorNode = currentTimedNode;
                            pNewTimedNode->m_dTotalTime = graphTraverser.getNextTime(currentTimedNode->m_dTotalTime, patternCost.getTravelTime() + currentAbstractPenaltyCost->getTravelTime());

                            // update cost in the mapWeights container :
                            iterCurrentPattern->second = dbCurrentCostValueWithHeuristic;

                            // update cost in the mapWaitingNodes container :
                            mapWaitingNodes[dbCurrentCostValueWithHeuristic].push_back(pNewTimedNode);
                        }
                    }
                }
            }
        } // end list Link
        //find if the node is an end node
        for (vector<TargetPoint>::iterator itOrigin = finalNodes.begin(); itOrigin != finalNodes.end();)
        {
			if ((*itOrigin).m_pNode == currentTimedNode->m_pNode
				&& ((*itOrigin).m_pPattern == NULL || (*itOrigin).m_pPattern == currentTimedNode->m_pPredecossorPattern))
            {
                //Take only the time along the pathway
                currentTimedNode->m_dTotalTime = graphTraverser.isFromOriginToDestination() ? currentTimedNode->m_dTotalTime - dbStartingPointTime : dbStartingPointTime - currentTimedNode->m_dTotalTime;

                ValuetedPath* newValuetedPath = new ValuetedPath(currentTimedNode, graphTraverser.isFromOriginToDestination());
                listResultsPaths[(*itOrigin).m_Index].push_back(newValuetedPath);

                itOrigin = finalNodes.erase(itOrigin);

                // rmk : no break here cause we can have multiple origins with the same node, one with no constraint on the downstream pattern and one with,
                // so both origins can be achieved at once.
                //break;
            }
            else
            {
                ++itOrigin;
            }
        }
    }//end while


    //Clear memory
    for (size_t iTimedNode = 0; iTimedNode < listTimedNodes.size(); iTimedNode++)
    {
        delete listTimedNodes[iTimedNode];
    }

    return listResultsPaths; // all end nodes that have been found
}

//Add the penalization of the cost (only in k-shortest path)
Cost Dijkstra::GetTotalPatternCost(int iSimulationInstance, Pattern* pPattern, double t, SubPopulation* pSubPopulation)
{
    Cost* pCost = pPattern->getPatternCost(iSimulationInstance, t, pSubPopulation);

    if (pCost != NULL)
    {
        // unpenalized cost
        Cost resCost = *pCost;

        // apply the penalization factor to the cost
        std::map<Pattern*, double>::iterator itMap = m_mapPenalizedPatterns.find(pPattern);
        if (itMap != m_mapPenalizedPatterns.end())
        {
            if (pCost->getCostValue() == 0)
            {
                // Special case of null cost patterns (touching areas with null area costs used by SymuVia for example),
                // we use directly the percentage as the penalized cost to avoid infinite loops when computing k-shortest paths
                resCost.setUsedCostValue(itMap->second);
            }
            else
            {
                resCost.setUsedCostValue(pCost->getCostValue() * (1 + itMap->second));
            }
        }

        // if euclidian heuristic, add the functional class related term :
        if (m_eHeuristic == SPH_EUCLIDIAN)
        {
            resCost.setUsedCostValue(resCost.getCostValue() + pCost->getCostValue() * m_dbAStarBeta * pPattern->getFuncClass());
        }

        return resCost;
    }
    else
    {
        return Cost(-1.0, -1.0);
    }
}

map<Pattern *, double>& Dijkstra::GetMapPenalizedPatterns()
{
    return m_mapPenalizedPatterns;
}
