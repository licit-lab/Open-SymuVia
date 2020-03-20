#pragma once

#ifndef SYMUCORE_DIJKSTRA_H
#define SYMUCORE_DIJKSTRA_H

#include "SymuCoreExports.h"
#include "ShortestPathsComputer.h"
#include "Utils/SymuCoreConstants.h"


#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

    class SubPopulation;
    class Path;
    class Origin;
    class Destination;
    class ValuetedPath;

class SYMUCORE_DLL_DEF Dijkstra : public ShortestPathsComputer {

public:

    Dijkstra();
    Dijkstra(bool bFromOriginToDestination);
    virtual ~Dijkstra();

    virtual std::map<Origin*, std::map<Destination*, std::vector<ValuetedPath*> > >  getShortestPaths(int iSimulationInstance, const std::vector<Origin*>& originNodes, const std::vector<Destination*>& destinationNodes, SubPopulation* pSubPopulation, double dbStartingPointTime, bool bUseCostsForTemporalAlgorithm = false);

    std::map<Pattern *, double> &GetMapPenalizedPatterns();

    void SetHeuristic(ShortestPathHeuristic eHeuristic, double dbAStarBeta, double dbHeuristicGamma);

private:

    template<class T>
    std::vector<std::vector<ValuetedPath*> > getShortestPaths(int iSimulationInstance, SubPopulation* pSubPopulation, double dbStartingPointTime, const T & graphTraverser, bool bUseCostsForTemporalAlgorithm);

    Cost GetTotalPatternCost(int iSimulationInstance, Pattern *pPattern, double t, SubPopulation *pSubPopulation);

    std::map<Pattern*, double>  m_mapPenalizedPatterns;

    ShortestPathHeuristic       m_eHeuristic;
    double                      m_dbAStarBeta;
    double                      m_dbHeuristicGamma;

    bool                        m_bFromOriginToDestination;
};
}

#pragma warning(pop)

#endif // SYMUCORE_DIJKSTRA_H
