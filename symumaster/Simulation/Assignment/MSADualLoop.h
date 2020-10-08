#pragma once

#ifndef _SYMUMASTER_MSADUALLOOPASSIGNMENTMODEL_
#define _SYMUMASTER_MSADUALLOOPASSIGNMENTMODEL_

#include "SymuMasterExports.h"

#include "AssignmentModel.h"

#include <Graph/Algorithms/Dijkstra.h>
#include <Demand/ValuetedPath.h>
#include <Demand/SubPopulation.h>
#include <Demand/Destination.h>
#include <Demand/Origin.h>
#include <Graph/Model/ListTimeFrame.h>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/trivial.hpp>
#include <boost/shared_ptr.hpp>

#include <map>

#pragma warning(push)
#pragma warning(disable : 4251)

// used in order to track where the simulation instance identifier is used
#define DEFAULT_SIMULATION_INSTANCE 0

namespace SymuMaster {


class SYMUMASTER_EXPORTS_DLL_DEF MSADualLoop : public AssignmentModel {

public:
    enum OptimizerType { Optimizer_UE, Optimizer_BRUE, Optimizer_SO, Optimizer_SSO };

protected:
    struct ParametersForSubpopulation {
        OptimizerType eOptimizerType;
        bool bInitialIsLogitAffectation;
        bool bIsUniformInitialization;
        bool bStepSizeAffectation;
        double dbSigmaConvenience;
        double dbValueOfTime;

        double dbMaxGapThresholdOuterloop;
        double dbMaxRelativeCostThresholdOuterloop;
        double dbMaxRelativeCostThresholdInnerloop;
    };

public:

    //Enums
    enum MSAType { MSA_Simple, MSA_Smart, MSA_GapBased, MSA_GapBasedProb, MSA_GapLudo, MSA_GapBasedNormalize, MSA_Probabilistic, MSA_StepSizeProb, MSA_Logit, MSA_WithinDayFPA, MSA_InitialBasedWDFPA, MSA_InitialBasedSimple };
    enum InnerloopTestType {CostConvergence, SwapConvergence, GapConvergence};

    MSADualLoop();
    MSADualLoop(int iMaxInnerloopIteration, int iMaxOuterloopIteration, double dbShortestPathsThreshold, bool bIsFillFromPreviousPeriod, int iUpdateCostPeriodsNumber, int iUpdateCostNumber,
                bool bIsKShortestPath, int iKVariable, double dbKShortestPathsThreshold, MSAType eMSAType, double dbDefaultLogit, InnerloopTestType eInnerloopTestType,
                double dbInnerloopGapThreshold, double dbInnerloopMaxODViolation, double dbInnerloopCostByODThreshold, double dbUserSwapByODThreshold,
                double dbOuterloopMaxODViolation, double dbOuterloopCostByODThreshold, bool bInitialization, bool bIsInitialStepSizeOuterloopIndex);

    virtual ~MSADualLoop();

    virtual bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMNode * pAssignmentConfigNode, std::string &predefinedPathsFilePath);

    virtual bool Initialize(const boost::posix_time::ptime &startTime, const boost::posix_time::ptime &endTime, std::vector<SymuCore::Population*> &listPopulations, std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, bool bDayToDayAffectation, bool bUseCostsForTemporalTravelTimes);

    virtual bool AssignPathsToTrips(const std::vector<SymuCore::Trip*> &listSortedTrip, const boost::posix_time::ptime & startPeriodTime, const boost::posix_time::time_duration &PeriodTimeDuration, const boost::posix_time::time_duration& travelTimesUpdatePeriodDuration, int &iCurrentIterationIndex, int iPeriod, int iSimulationInstance, int iSimulationInstanceForBatch, bool bAssignAllToFinalSolution);

    virtual bool IsRollbackNeeded(const std::vector<SymuCore::Trip*> &listSortedTrip, const boost::posix_time::ptime & startPeriodTime, const boost::posix_time::time_duration &PeriodTimeDuration, int iCurrentIterationIndex, int nbParallelSimulationMaxNumber, bool &bOk);

    void ShowResult(const std::vector<SymuCore::Trip *> &listSortedTrip);

protected:

    void LoadFromXML(XERCES_CPP_NAMESPACE::DOMNode * pOptimizerConfigNode, ParametersForSubpopulation & paramsToFill, ParametersForSubpopulation * pDefaultValues = NULL);

    struct CompareValuetedPath{
        bool operator()(const boost::shared_ptr<SymuCore::ValuetedPath> p1, const boost::shared_ptr<SymuCore::ValuetedPath> p2) const { return *p1 < *p2; }
    };

   
    template<class T>
    // ToMostafa: mapBySubPopulationOriginDestination is one solution.
    using mapBySubPopulationOriginDestination = std::map<SymuCore::SubPopulation*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, T, SortDestination >, SortOrigin >, SortSubPopulation >;
    template<class T>
    using mapByOriginDestination = std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, T, SortDestination >, SortOrigin >;
    template<class T>
    using mapByDestination = std::map<SymuCore::Destination*, T, SortDestination >;

    using ODAndTimeDependent = std::map<SymuCore::SubPopulation*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, SymuCore::ListTimeFrame<double> > > >;


    //************************** Discover Methods **************************
    bool CalculateShortestPaths(const std::vector<SymuCore::Trip *> &ListTrips, double dbEndPeriodTime, bool isInitialization, mapBySubPopulationOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> > & shortestPaths, int iSimulationInstanceForBatch);

    bool ComputeFirstShortestPaths(const std::vector<SymuCore::Trip *> &listTrips, double dbDepartureTime, double dbEndPeriodTime, const boost::posix_time::time_duration& travelTimesUpdatePeriodDuration, int iSimulationInstance, int iSimulationInstanceForBatch);

    void CheckPredefinedODAffectation(mapBySubPopulationOriginDestination<std::vector<SymuCore::Trip *> >& partialMapTripsByOD, int iSimulationInstanceForBatch);

    bool PredefinedPaths(SymuCore::Origin* pOrigin, SymuCore::Destination* pDestination, SymuCore::SubPopulation *pSubPopulation, double dbEndPeriodTime, mapBySubPopulationOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> > & shortestPaths, int iSimulationInstanceForBatch, bool & hasPredefinedPath);

    bool InitialAffectation(mapBySubPopulationOriginDestination<std::vector<SymuCore::Trip *> > &mapTripsByOD, double dbArrivalTime, int iSimulationInstanceForBatch);

	//************************** Initialization *************************
    void Initialization(int nbParallelSimulationMaxNumber);

    mapBySubPopulationOriginDestination<std::vector<boost::shared_ptr<SymuCore::ValuetedPath> > > m_lInitialBest;

    bool                            m_bChangeInitialization; //boolean to skip assignment and only do the traffic simulation

	//************************** Affectation Methods **************************
    void ClassicMSA(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &mapValuetedPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath> >& listPathFound, const std::vector<SymuCore::Trip*>& listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop);

    void SmartMSA(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &mapValuetedPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath> >& listPathFound, std::vector<SymuCore::Trip *> &listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop);

    void GapBasedMSA(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &mapValuetedPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath> >& listPathFound, std::vector<SymuCore::Trip *> &listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop);

    void GapBasedLudoMSA(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &mapValuetedPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath> >& listPathFound, std::vector<SymuCore::Trip *> &listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop);
	
    void ProbabilisticMSA(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &mapValuetedPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath> >& listPathFound, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop, int StepSize);

    void WithinDayFPAMSA(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &mapValuetedPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath> >& listPathFound, std::vector<SymuCore::Trip *> &listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop);

    void InitialBasedWDFPAMSA(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &mapValuetedPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath> >& listPathFound, std::vector<SymuCore::Trip *> &listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop);

    void InitialBasedSimpleMSA(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &mapValuetedPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath> >& listPathFound, std::vector<SymuCore::Trip *> &listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop);

    void LogitMSA(std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &mapValuetedPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath> >& listPathFound, std::vector<SymuCore::Trip *> &listTripsByOD, double dbTetaLogit, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop);

    void AffectBestAssignment(const std::vector<SymuCore::Trip *> &listSortedTrip, int iSimulationInstanceForBatch);

    //************************** Convergence Methods **************************
    virtual bool IsInnerLoopIterationNeeded(mapBySubPopulationOriginDestination<std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> > &prevMapTripsByPath);

    bool IsOuterLoopIterationNeeded(int iCurrentIterationIndex, int iTotalNbUsers, bool bHasNewShortestPathDiscovered);

    bool SwapBasedConvergenceTest(mapBySubPopulationOriginDestination<std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> > &prevMapTripsByPath);

    bool HasNewShortestPathDiscovered();

    bool CostBasedConvergenceTest(bool isOuterLoop);

    double ComputeGlobalGap(std::map<SymuCore::SubPopulation*, double> & gapBySubPopulation, mapBySubPopulationOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> > & shortestPaths, int iSimulationInstanceForBatch);

    //************************** Other Methods **************************
    std::map<boost::shared_ptr<SymuCore::ValuetedPath>, double> GetCoeffsFromLogit(const std::vector<boost::shared_ptr<SymuCore::ValuetedPath> > &actualPaths, double dbRemainingCoeff, double actualTetaLogit);

    void UniformlySwap(int nbSwap, std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &userDistribution, boost::shared_ptr<SymuCore::ValuetedPath> actualPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> &listPathFound, const std::vector<SymuCore::Trip*>& listUsersOD, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop);
    void ProbabilisticSwap(int nbSwap, std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &userDistribution, boost::shared_ptr<SymuCore::ValuetedPath> actualPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> &listPathFound, const std::vector<SymuCore::Trip*>& listUsersOD, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop);

    virtual void KeepBestAssignment(const std::vector<SymuCore::Trip *> &listTrip, bool IsInnerLoopConverged);

    void PrintOutputData();

    void PrintOutputGapIteration(double dbActualGap, bool bInInnerLoop);
	
	virtual bool MaximumSwappingReached(int iCurrentIterationIndex);

    void UpdatePathCostFromMultipleInstants(SymuCore::ValuetedPath *, SymuCore::SubPopulation *pSubPopulation, int iSimulationInstanceForBatch);

    void UpdatePathCostAndShortestPath(mapBySubPopulationOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> > & shortestPaths, int iSimulationInstanceForBatch);

    void GroupCloseShortestPath(int iSimulationInstanceBatch);

    void CompareExistingValuetedPath(mapBySubPopulationOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> > & shortestPaths, int iSimulationInstanceForBatch);

    void FillFromPreviousAffectationRatio(mapBySubPopulationOriginDestination<std::vector<SymuCore::Trip *> > &partialMapTripsByOD, std::vector<SymuCore::Trip *> &listNotAffectedUsers, double dbEndPeriodTime, int iSimulationInstanceForBatch);

    void ClearMemory();

    boost::shared_ptr<SymuCore::ValuetedPath> GetExistingValuetedPathFromBestAssignment(SymuCore::SubPopulation * pSubPop, SymuCore::Origin * pOrigin, SymuCore::Destination * pDestination, const SymuCore::Path & path);

    void SaveCurrentAssignment(const std::vector<SymuCore::Trip*> & listTips, int iSimulationInstanceForBatch);


    //************************** Attributes **************************
    std::map<int, mapBySubPopulationOriginDestination< std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip*>, CompareValuetedPath > > > m_mapAffectedTrips; // Map of trips relation by path

    mapBySubPopulationOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> > m_mapOriginalShortestPathResults; // shortest path at the beginning of the current iteration
    std::map<int, mapBySubPopulationOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> > > m_mapNewShortestPathResults; // updated shortest paths at the end of the iteration for each parallel simulation instance

    mapBySubPopulationOriginDestination< std::vector<SymuCore::Trip*> > m_mapTripsByOD; //Trips sorted by SubPopulation, origin and destination

    mapBySubPopulationOriginDestination< std::multimap<double, boost::shared_ptr<SymuCore::ValuetedPath> > > m_mapPreviousAffectationRatio; //Trips sorted by SubPopulation, origin and destination

    std::map<std::string, ParametersForSubpopulation> m_mapParametersBySubpopulationName;
    std::map<SymuCore::SubPopulation*, ParametersForSubpopulation> m_mapParametersBySubpopulation;


    //variables for dual loop
    int                             m_iInnerloopIndex;
    int                             m_iOuterloopIndex;
    int                             m_iMaxInnerloopIteration;
    int                             m_iMaxOuterloopIteration;
    double                          m_dbShortestPathsThreshold; // if the percentage of cost of one path to the shortest path is less or equal this number, we assume that the path is also a shortest path
    bool                            m_bIsFillFromPreviousPeriod;
	bool                            m_bUseStepSizeImprovement;
    bool                            m_bIsGapBasedConvergenceOuterloop;
    bool                            m_bUseUserExperiencedCost;
    bool                            m_bPrintAffectation;
    int                             m_iMinSwapValue;
	int                             m_iNbMaxTotalIterations;
    int                             m_iCurrentPeriod;

    //Variables for aggregation
    int                             m_iUpdateCostPeriodsNumber;
    int                             m_iUpdateCostNumber;
    std::vector<double>             m_listEndPeriods;

    //Shortest path
    SymuCore::ShortestPathsComputer*m_ShortestPathsMethod;
    bool                            m_bIterationIsKShortestPath;
    int                             m_iKVariable;
    double                          m_dbKShortestPathsThreshold;
    double                          m_dbWardropTolerance;
    ODAndTimeDependent              m_KParameters;
    double                          m_dbAssignementAlpha;
    double                          m_dbAssignementBeta;
    double                          m_dbAssignementGamma;
    bool                            m_bCommonalityFilter;
    std::vector<double>             m_listCommonalityFactorParameters;
	bool							m_bUsePredefinedPathsOnly;

    //Initialization
    bool                            m_bInitialIsKShortestPath;
    SymuCore::ShortestPathsComputer*m_InitialShortestPathsMethod;
    bool							m_bInitialization;
    bool                            m_bIsInitialStepSizeOuterloopIndex;
    bool							m_bDayToDayAffectation;
    bool                            m_bUseCostsForTemporalShortestPath;

    //variables for MSA
	int								m_iValueOfK;
    MSAType                         m_MSAType;
	double                          m_dbDefaultTetaLogit;
    ODAndTimeDependent              m_LogitParameters;
    double                          m_dbAlpha;

    //Variables to assure MSA Quality
    double                          m_dbBestQuality;
    bool                            m_bChangeAssignment; //boolean to skip assignment and only do the traffic simulation
    std::vector<SymuCore::Path>     m_listPreviousAffectationPaths ; // list of path that correspond to the list of trips
    std::map<SymuCore::SubPopulation*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, std::vector<SymuCore::Path> > > > m_listPreviousAffectationUnassignedPaths; // list of previous best assignment paths with no assigned trips (have to keep them)

    std::map<boost::shared_ptr<SymuCore::ValuetedPath>, int>  m_mapIntialAssignment; // map of path and flow

    //innerloop
    InnerloopTestType               m_InnerloopTestType;
    std::map<SymuCore::SubPopulation*, double> m_mapPreviousGap;
    std::map<SymuCore::SubPopulation*, double> m_mapActualGlobalGap;
    double                          m_dbActualGlobalGap;
    double                          m_dbInnerloopGapThreshold;
    double                          m_dbInnerloopRelativeODViolation;
    double                          m_dbInnerloopRelativeUserViolation;
    double                          m_dbRelativeSwappingThreshold;

	//outerloop
	bool                            m_bUseRelativeGapThreshold;
	double                          m_dbOuterloopRelativeGapThreshold;
    std::map<SymuCore::SubPopulation*, double> m_mapPreviousOuterLoopGap;
    double                          m_dbPreviousOuterLoopGap;
	double                          m_dbOuterloopRelativeODViolation;
	double                          m_dbOuterloopRelativeUserViolation;

	//StepSize Improvement
	mapBySubPopulationOriginDestination< int > m_mapImprovedStepSize;
	mapBySubPopulationOriginDestination< double > m_mapGapByOD;
	mapBySubPopulationOriginDestination< double > m_mapPrGapByOD;

    protected:
        virtual std::string GetAlgorithmTypeNodeName() const;
        virtual bool readAlgorithmNode(XERCES_CPP_NAMESPACE::DOMNode * pChildren);

        virtual void Decision();

        virtual bool onSimulationInstanceDone(const std::vector<SymuCore::Trip *> &listTrip, const boost::posix_time::ptime & endPeriodTime, int iSimulationInstance, int iSimulationInstanceForBatch, int iCurrentIterationIndex);

};

}

#pragma warning(pop)

#endif // _SYMUMASTER_MSADUALLOOPASSIGNMENTMODEL_
