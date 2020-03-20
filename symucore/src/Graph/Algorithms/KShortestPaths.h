#pragma once

#ifndef SYMUCORE_KSHORTESTPATHS_H
#define SYMUCORE_KSHORTESTPATHS_H

#include "SymuCoreExports.h"
#include "ShortestPathsComputer.h"
#include "Graph/Model/ListTimeFrame.h"

#include "Utils/SymuCoreConstants.h"

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

    class Population;
    class Path;
    class Origin;
    class Destination;
    class ValuetedPath;
    class SubPopulation;
    class PredefinedPath;

class SYMUCORE_DLL_DEF KShortestPaths : public ShortestPathsComputer {

public:

	typedef std::map<SymuCore::SubPopulation*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, SymuCore::ListTimeFrame<double> > > > FromSymuViaParameters;

    KShortestPaths();
    KShortestPaths(double dbAssignementAlpha, double dbAssignementBeta, double dbAssignementGamma, bool CommonalityFilter,
                   std::vector<double> dbCommonalityFactorParameters, double dbKShortestPathsThreshold, const FromSymuViaParameters& KByOD,
                   std::map<Origin*, std::map<Destination*, std::vector<PredefinedPath> > > * pMapPredefinedPaths = NULL,
                   std::map<Origin*, std::map<Destination*, std::vector<double> > > * pListCommonalityFactorParametersByOD = NULL);
    virtual ~KShortestPaths();

    //rmk : for now, we only use kShortestPath for 1 origin to 1 destination. It may be improve in the future
    virtual std::map<Origin*, std::map<Destination*, std::vector<ValuetedPath*> > >  getShortestPaths(int iSimulationInstance, const std::vector<Origin*>& originNodes, const std::vector<Destination*>& destinationNodes, SubPopulation *pSubPopulation, double dbArrivalTime, bool bUseCostsForTemporalAlgorithm = false);

    void FillVariables();

    double ComputeCommonalityFactor(int iSimulationInstance, std::vector<ValuetedPath *> &alreadyFoundPaths, ValuetedPath* pCurrentPath, double dbArrivalTime, SubPopulation* pSubPopulation, std::vector<double> * pCFParams);

    const std::map<Origin*, std::map<Destination*, std::vector<std::pair<Path, double> > > > & getFilteredPaths() const;

    void SetSimulationGameMethod(int nMethod, const Path & path);
    void SetHeuristicParams(ShortestPathHeuristic eHeuristic, double dbAStarBeta, double dbAStarGamma);

    std::vector<Pattern*> & GetPatternsToAvoid();

private:

    double                              m_dbKShortestPathsThreshold;
    FromSymuViaParameters               m_KParameters;
    double                              m_dbAssignementAlpha;
    double                              m_dbAssignementBeta;
    double                              m_dbAssignementGamma;
    bool                                m_bCommonalityFilter;
    std::vector<double>                 m_listCommonalityFactorParameters;
    std::map<Origin*, std::map<Destination*, std::vector<double> > > * m_pListCommonalityFactorParametersByOD;
    std::map<Origin*, std::map<Destination*, std::vector<PredefinedPath> > > * m_pMapPredefinedPaths;
    int                                 m_nMethod; // K shortest pah method for the simulation game
    Path                                m_nMethodRefPath; // reference path related to m_nMethod, for the simulation game

    std::map<Origin*, std::map<Destination*, std::vector<std::pair<Path, double> > > > m_listFilteredPaths; // list of the filtered paths for the last call

    ShortestPathHeuristic               m_eHeuristic;
    double                              m_dbAStarBeta;
    double                              m_dbAStarGamma;

    std::vector<Pattern*>               m_PatternsToAvoid;

};
}

#pragma warning(pop)

#endif // SYMUCORE_KSHORTESTPATHS_H
