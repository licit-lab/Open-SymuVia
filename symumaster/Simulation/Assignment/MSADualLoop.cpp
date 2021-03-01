#include "MSADualLoop.h"

#include "Simulation/Simulators/SimulationDescriptor.h"
#include "Simulation/Simulators/TraficSimulatorHandler.h"
#include "Simulation/Simulators/TraficSimulators.h"
#include "Simulation/Config/SimulationConfiguration.h"

#include "IO/XMLStringUtils.h"

#include "HeuristicDualLoop.h"
#include <Demand/Trip.h>
#include <Demand/Path.h>
#include <Demand/Origin.h>
#include <Demand/Destination.h>
#include <Demand/Population.h>
#include <Demand/VehicleType.h>
#include <Demand/MacroType.h>
#include <Demand/Population.h>
#include <Demand/SubPopulation.h>
#include <Graph/Model/Cost.h>
#include <Graph/Model/Pattern.h>
#include <Graph/Algorithms/Dijkstra.h>
#include <Graph/Algorithms/KShortestPaths.h>
#include <Graph/Algorithms/ShortestPathsComputer.h>
#include <Graph/Model/Link.h>

#include <SymExp.h>

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNodeList.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/make_shared.hpp>

#include <set>

using namespace XERCES_CPP_NAMESPACE;
using namespace SymuMaster;
using namespace SymuCore;
using namespace std;

// Creation of the classes (default)
MSADualLoop::MSADualLoop() : m_iMaxInnerloopIteration(1), m_iMaxOuterloopIteration(2), m_dbShortestPathsThreshold(0.02), m_bIsFillFromPreviousPeriod(true), m_iUpdateCostPeriodsNumber(5),
    m_iUpdateCostNumber(6), m_bIterationIsKShortestPath(false), m_iKVariable(3), m_dbKShortestPathsThreshold(1.0), m_MSAType(MSA_GapBased), m_dbDefaultTetaLogit(3.5),
    m_InnerloopTestType(CostConvergence), m_dbInnerloopGapThreshold(34), m_dbInnerloopRelativeODViolation(0.06), m_dbInnerloopRelativeUserViolation(0.06),
	m_dbRelativeSwappingThreshold(0.8), m_dbOuterloopRelativeODViolation(0.02), m_dbOuterloopRelativeUserViolation(0.04),
	m_bInitialization(false), m_bIsInitialStepSizeOuterloopIndex(true), m_bUsePredefinedPathsOnly(true)
{
    m_iInnerloopIndex = 0;
    m_iOuterloopIndex = 0;
    m_ShortestPathsMethod = NULL;
    m_InitialShortestPathsMethod = NULL;
}

// Constructor with arguments
MSADualLoop::MSADualLoop(int iMaxInnerloopIteration, int iMaxOuterloopIteration, double dbShortestPathsThreshold, bool bIsFillFromPreviousPeriod, int iUpdateCostPeriodsNumber, int iUpdateCostNumber,
                         bool bIsKShortestPath, int iKVariable, double dbKShortestPathsThreshold, MSAType eMSAType, double dbDefaultLogit, InnerloopTestType eInnerloopTestType,
                         double dbInnerloopGapThreshold, double dbInnerloopMaxODViolation, double dbInnerloopCostByODThreshold, double dbUserSwapByODThreshold,
                         double dbOuterloopMaxODViolation, double dbOuterloopCostByODThreshold, bool bInitialization, bool bIsInitialStepSizeOuterloopIndex)
    : m_iMaxInnerloopIteration(iMaxInnerloopIteration), m_iMaxOuterloopIteration(iMaxOuterloopIteration), m_dbShortestPathsThreshold(dbShortestPathsThreshold), m_bIsFillFromPreviousPeriod(bIsFillFromPreviousPeriod),
      m_iUpdateCostPeriodsNumber(iUpdateCostPeriodsNumber), m_iUpdateCostNumber(iUpdateCostNumber), m_bIterationIsKShortestPath(bIsKShortestPath), m_iKVariable(iKVariable), m_dbKShortestPathsThreshold(dbKShortestPathsThreshold),
      m_MSAType(eMSAType), m_dbDefaultTetaLogit(dbDefaultLogit), m_InnerloopTestType(eInnerloopTestType), m_dbInnerloopGapThreshold(dbInnerloopGapThreshold),
      m_dbInnerloopRelativeODViolation(dbInnerloopMaxODViolation), m_dbInnerloopRelativeUserViolation(dbInnerloopCostByODThreshold),
      m_dbRelativeSwappingThreshold(dbUserSwapByODThreshold), m_dbOuterloopRelativeODViolation(dbOuterloopMaxODViolation), m_dbOuterloopRelativeUserViolation(dbOuterloopCostByODThreshold),
	  m_bInitialization(bInitialization), m_bIsInitialStepSizeOuterloopIndex(bIsInitialStepSizeOuterloopIndex), m_bUsePredefinedPathsOnly(true)
{
    m_iInnerloopIndex = 0;
    m_iOuterloopIndex = 0;
    m_ShortestPathsMethod = NULL;
    m_InitialShortestPathsMethod = NULL;
}

MSADualLoop::~MSADualLoop()
{
    ClearMemory();

    if(m_ShortestPathsMethod)
        delete m_ShortestPathsMethod;

    if (m_InitialShortestPathsMethod)
        delete m_InitialShortestPathsMethod;
}

void MSADualLoop::ClearMemory()
{
    m_mapAffectedTrips.clear();

}


std::string MSADualLoop::GetAlgorithmTypeNodeName() const
{
    return "MSA_TYPE";
}

bool MSADualLoop::readAlgorithmNode(DOMNode * pChildren)
{
    string MSAName;
    XMLStringUtils::GetAttributeValue(pChildren, "msa_type", MSAName);
    if (MSAName == "SIMPLE")
    {
        m_MSAType = MSA_Simple;
    }
    else if (MSAName == "SMART")
    {
        m_MSAType = MSA_Smart;
    }
    else if (MSAName == "GAP_BASED")
    {
        m_MSAType = MSA_GapBased;
    }
    else if (MSAName == "GAP_BASED_PROB")
    {
        m_MSAType = MSA_GapBasedProb;
    }
    else if (MSAName == "GAP_LUDO")
    {
        m_MSAType = MSA_GapLudo;
    }
    else if (MSAName == "WITHIN_DAY_FPA")
    {
        m_MSAType = MSA_WithinDayFPA;
    }
    else if (MSAName == "INITIAL_BASED_WITHIN_DAY_FPA")
    {
        m_MSAType = MSA_InitialBasedWDFPA;
    }
    else if (MSAName == "INITIAL_BASED_SIMPLE")
    {
        m_MSAType = MSA_InitialBasedSimple;
    }
    else if (MSAName == "GAP_BASED_NORMALIZE")
    {
        m_MSAType = MSA_GapBasedNormalize;
    }
    else if (MSAName == "PROBABILISTIC")
    {
        m_MSAType = MSA_Probabilistic;
    }
    else if (MSAName == "STEPSIZE_PROB")
    {
        m_MSAType = MSA_StepSizeProb;
    }
    else if (MSAName == "LOGIT")
    {
        bool bHasOtherThingThanLogitInitialization = false;
        for (std::map<std::string, ParametersForSubpopulation>::const_iterator iterSubpop = m_mapParametersBySubpopulationName.begin(); iterSubpop != m_mapParametersBySubpopulationName.end(); ++iterSubpop)
        {
            if (!iterSubpop->second.bInitialIsLogitAffectation)
            {
                bHasOtherThingThanLogitInitialization = true;
                break;
            }
        }
        if (bHasOtherThingThanLogitInitialization)
        {
            BOOST_LOG_TRIVIAL(error) << "Having MSA_Logit without having Logit for initial affectaion is not possible !";
            return false;
        }
        m_MSAType = MSA_Logit;
    }

    if (!XMLStringUtils::GetAttributeValue(pChildren, "default_logit", m_dbDefaultTetaLogit))
    {
        m_dbDefaultTetaLogit = -1;
    }

    if (!XMLStringUtils::GetAttributeValue(pChildren, "alpha", m_dbAlpha))
    {
        m_dbAlpha = -1;
        if (m_MSAType == MSA_InitialBasedWDFPA || m_MSAType == MSA_WithinDayFPA || m_MSAType == MSA_InitialBasedSimple)
        {
            BOOST_LOG_TRIVIAL(error) << "Please determine the proper alpha value and set it in the SymuMaster XML input file";
        }
    }

    XMLStringUtils::GetAttributeValue(pChildren, "is_initialization", m_bInitialization);
    XMLStringUtils::GetAttributeValue(pChildren, "is_initial_stepsize_outerloop_index", m_bIsInitialStepSizeOuterloopIndex);

    return true;
}

// Load each parameter from XML file
bool MSADualLoop::LoadFromXML(DOMNode * pSimulationConfigNode, string& predefinedPathsFilePath)
{
    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "sigma_close_shortest_paths", m_dbShortestPathsThreshold);

    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "paths_update_nb_periods", m_iUpdateCostPeriodsNumber);

    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "paths_update_nb_instants", m_iUpdateCostNumber);

    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "minimum_swapping_value", m_iMinSwapValue);

    //test the correctness of the aggregation values
    if(m_iUpdateCostNumber > m_iUpdateCostPeriodsNumber+1)
    {
        BOOST_LOG_TRIVIAL(warning) << "You want to update the travel cost using the " << m_iUpdateCostPeriodsNumber << " last periods but using " << m_iUpdateCostNumber <<
                                      "different instants. We will use " << (m_iUpdateCostPeriodsNumber+1) << " instead, that is the maximum number of instants possible in this configuration.";
        m_iUpdateCostNumber = m_iUpdateCostPeriodsNumber+1;
    }

    string pathFile;
    if(!XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "predefined_paths_file_path", pathFile))
        pathFile = "";
    predefinedPathsFilePath = pathFile;

	XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "predefined_paths_only", m_bUsePredefinedPathsOnly);

    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "fill_from_previous_assignment_period", m_bIsFillFromPreviousPeriod);

    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "improvement_dependant_step_size", m_bUseStepSizeImprovement);
	
	if (!XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "max_iterations_number", m_iNbMaxTotalIterations))
	{
		m_iNbMaxTotalIterations = -1;
	}

    XMLStringUtils::GetAttributeValue(pSimulationConfigNode, "print_affectation", m_bPrintAffectation);

    DOMNodeList * pMSADualLoopChildren = pSimulationConfigNode->getChildNodes();
    for (XMLSize_t iChild = 0; iChild < pMSADualLoopChildren->getLength(); iChild++)
    {
        DOMNode * pChildren = pMSADualLoopChildren->item(iChild);

        const string & nodeName = US(pChildren->getNodeName());
        if (nodeName == "SHORTEST_PATH")
        {

            XMLStringUtils::GetAttributeValue(pChildren, "is_k_shortest_path_iteration", m_bIterationIsKShortestPath);

            XMLStringUtils::GetAttributeValue(pChildren, "is_k_shortest_path_initial", m_bInitialIsKShortestPath);

            bool hasKSHortestPath = m_bIterationIsKShortestPath || m_bInitialIsKShortestPath;

            //if(!XMLStringUtils::GetAttributeValue(pChildren, "k", m_bIsFillFromPreviousPeriod) && hasKSHortestPath)
			int nTmp;
			if (!XMLStringUtils::GetAttributeValue(pChildren, "k", nTmp) && hasKSHortestPath)
            {
                m_iKVariable = -1;
            }

            if(!XMLStringUtils::GetAttributeValue(pChildren, "threshold_close_shortest_path", m_dbKShortestPathsThreshold) && hasKSHortestPath)
		      {
                m_dbKShortestPathsThreshold = -1;
            }

            if(!XMLStringUtils::GetAttributeValue(pChildren, "assignment_alpha", m_dbAssignementAlpha) && hasKSHortestPath)
            {
                m_dbAssignementAlpha = -1;
            }

            if(!XMLStringUtils::GetAttributeValue(pChildren, "assignment_beta", m_dbAssignementBeta) && hasKSHortestPath)
            {
                m_dbAssignementBeta = -1;
            }

            if(!XMLStringUtils::GetAttributeValue(pChildren, "assignment_gamma", m_dbAssignementGamma) && hasKSHortestPath)
            {
                m_dbAssignementGamma = -1;
            }

            if(!XMLStringUtils::GetAttributeValue(pChildren, "wardrop_tolerance", m_dbWardropTolerance) && hasKSHortestPath)
            {
                m_dbAssignementGamma = -1;
            }

            XMLStringUtils::GetAttributeValue(pChildren, "CFK_filter", m_bCommonalityFilter);

            if(m_bCommonalityFilter)
            {
				double dbVal;
				m_listCommonalityFactorParameters.resize(3);
				if (!XMLStringUtils::GetAttributeValue(pChildren, "CFK_alpha", dbVal))
				{
					m_listCommonalityFactorParameters[0] = -1;
				}
				else
					m_listCommonalityFactorParameters[0] = dbVal;

                if(!XMLStringUtils::GetAttributeValue(pChildren, "CFK_beta", dbVal))
                {
                    m_listCommonalityFactorParameters[1] = -1;
                }
				else
					m_listCommonalityFactorParameters[1] = dbVal;

                if(!XMLStringUtils::GetAttributeValue(pChildren, "CFK_gamma", dbVal))
                {
                    m_listCommonalityFactorParameters[2] = -1;
                }
				else
					m_listCommonalityFactorParameters[2] = dbVal;
            }
        }
        else if(nodeName == "OPTIMIZER")
        {
            ParametersForSubpopulation defaultOptimizerParams;
            LoadFromXML(pChildren, defaultOptimizerParams);

            // rmq. using empty string as key for the global default parameters
            m_mapParametersBySubpopulationName[std::string()] = defaultOptimizerParams;

            DOMNodeList * pOptimizerChildren = pChildren->getChildNodes();
            for (XMLSize_t iOptimizerChild = 0; iOptimizerChild < pOptimizerChildren->getLength(); iOptimizerChild++)
            {
                DOMNode * pOptimizerChild = pOptimizerChildren->item(iOptimizerChild);

                const string & optNodeName = US(pOptimizerChild->getNodeName());
                if (optNodeName == "SUBPOPULATION_SPECIALIZATION")
                {
                    ParametersForSubpopulation specializedOptimizerParams;
                    LoadFromXML(pOptimizerChild, specializedOptimizerParams, &defaultOptimizerParams);

                    DOMNodeList * pOptimizerSpecChildren = pOptimizerChild->getChildNodes();
                    for (XMLSize_t iOptimizerSpecChild = 0; iOptimizerSpecChild < pOptimizerSpecChildren->getLength(); iOptimizerSpecChild++)
                    {
                        DOMNode * pOptimizerSpecChild = pOptimizerSpecChildren->item(iOptimizerSpecChild);

                        const string & optSpecNodeName = US(pOptimizerSpecChild->getNodeName());
                        if (optSpecNodeName == "SUBPOPULATION")
                        {
                            string subpopName;
                            XMLStringUtils::GetAttributeValue(pOptimizerSpecChild, "id", subpopName);
                            m_mapParametersBySubpopulationName[subpopName] = specializedOptimizerParams;
                        }
                    }
                }
            }
        }
        else if (nodeName == GetAlgorithmTypeNodeName())
        {
            if (!readAlgorithmNode(pChildren))
            {
                return false;
            }
        }
        else if (nodeName == "GAP_CONVERGENCE")
        {
            m_bIsGapBasedConvergenceOuterloop = true;

            DOMNodeList * pGapConvergenceChildren = pChildren->getChildNodes();
            for (XMLSize_t iChild = 0; iChild < pGapConvergenceChildren->getLength(); iChild++)
            {
                DOMNode * pLoopTypeChild = pGapConvergenceChildren->item(iChild);

                const string & nodeName = US(pLoopTypeChild->getNodeName());
                if (nodeName == "OUTERLOOP_GAP_CONVERGENCE")
                {
                    XMLStringUtils::GetAttributeValue(pLoopTypeChild, "max_outerloop_iterations", m_iMaxOuterloopIteration);

                    XMLStringUtils::GetAttributeValue(pLoopTypeChild, "use_relative_gap_threshold_convergence", m_bUseRelativeGapThreshold);

                    if (!XMLStringUtils::GetAttributeValue(pLoopTypeChild, "max_gap_threshold_outerloop", m_mapParametersBySubpopulationName[std::string()].dbMaxGapThresholdOuterloop) && !m_bUseRelativeGapThreshold)
                    {
                        BOOST_LOG_TRIVIAL(error) << "max_gap_threshold_outerloop can't be empty if you use the fix gap threshold method !";
                        return false;
                    }

                    if(!XMLStringUtils::GetAttributeValue(pLoopTypeChild, "max_relative_gap_difference_outerloop", m_dbOuterloopRelativeGapThreshold) && m_bUseRelativeGapThreshold)
                    {
                        BOOST_LOG_TRIVIAL(error) << "max_relative_gap_difference_outerloop can't be empty if you use the relative gap convergence method !";
                        return false;
                    }

                    XMLStringUtils::GetAttributeValue(pLoopTypeChild, "max_relative_cost_threshold_outerloop", m_mapParametersBySubpopulationName[std::string()].dbMaxRelativeCostThresholdOuterloop);

                    for (std::map<std::string, ParametersForSubpopulation>::iterator iterParamsForSubpop = m_mapParametersBySubpopulationName.begin(); iterParamsForSubpop != m_mapParametersBySubpopulationName.end(); ++iterParamsForSubpop)
                    {
                        iterParamsForSubpop->second.dbMaxGapThresholdOuterloop = m_mapParametersBySubpopulationName[std::string()].dbMaxGapThresholdOuterloop;
                        iterParamsForSubpop->second.dbMaxRelativeCostThresholdOuterloop = m_mapParametersBySubpopulationName[std::string()].dbMaxRelativeCostThresholdOuterloop;
                    }

                    XMLStringUtils::GetAttributeValue(pLoopTypeChild, "max_relative_user_violation_outerloop", m_dbOuterloopRelativeUserViolation);

                    // read specializations by subpopulation
                    DOMNodeList * pOuterLoopGapChildren = pLoopTypeChild->getChildNodes();
                    for (XMLSize_t iOuterLoopGapChild = 0; iOuterLoopGapChild < pOuterLoopGapChildren->getLength(); iOuterLoopGapChild++)
                    {
                        DOMNode * pOuterLoopGapChild = pOuterLoopGapChildren->item(iOuterLoopGapChild);

                        const string & outerloopChildNodeName = US(pOuterLoopGapChild->getNodeName());
                        if (outerloopChildNodeName == "SUBPOPULATION_SPECIALIZATION")
                        {
                            DOMNodeList * pOuterLoopGapSpecChildren = pOuterLoopGapChild->getChildNodes();
                            for (XMLSize_t iOuterLoopGapSpecChild = 0; iOuterLoopGapSpecChild < pOuterLoopGapSpecChildren->getLength(); iOuterLoopGapSpecChild++)
                            {
                                DOMNode * pOuterLoopGapSpecChild = pOuterLoopGapSpecChildren->item(iOuterLoopGapSpecChild);

                                const string & subpopSpecnodeName = US(pOuterLoopGapSpecChild->getNodeName());
                                if (subpopSpecnodeName == "SUBPOPULATION")
                                {
                                    string subpopName;
                                    XMLStringUtils::GetAttributeValue(pOuterLoopGapSpecChild, "id", subpopName);

                                    std::map<std::string, ParametersForSubpopulation>::iterator iterParamsForSubpop = m_mapParametersBySubpopulationName.find(subpopName);
                                    if (iterParamsForSubpop == m_mapParametersBySubpopulationName.end())
                                    {
                                        m_mapParametersBySubpopulationName[subpopName] = m_mapParametersBySubpopulationName[std::string()];
                                    }
                                    if (!XMLStringUtils::GetAttributeValue(pOuterLoopGapChild, "max_relative_cost_threshold_outerloop", m_mapParametersBySubpopulationName[subpopName].dbMaxRelativeCostThresholdOuterloop))
                                    {
                                        m_mapParametersBySubpopulationName[subpopName].dbMaxRelativeCostThresholdOuterloop = m_mapParametersBySubpopulationName[std::string()].dbMaxRelativeCostThresholdOuterloop;
                                    }
                                    if (!XMLStringUtils::GetAttributeValue(pOuterLoopGapChild, "max_gap_threshold_outerloop", m_mapParametersBySubpopulationName[subpopName].dbMaxGapThresholdOuterloop))
                                    {
                                        m_mapParametersBySubpopulationName[subpopName].dbMaxGapThresholdOuterloop = m_mapParametersBySubpopulationName[std::string()].dbMaxGapThresholdOuterloop;
                                    }
                                }
                            }
                        }
                    }
                }
                else if(nodeName == "INNERLOOP_GAP_CONVERGENCE")
                {
                    m_InnerloopTestType = GapConvergence;

                    XMLStringUtils::GetAttributeValue(pLoopTypeChild, "max_innerloop_iterations", m_iMaxInnerloopIteration);

                    XMLStringUtils::GetAttributeValue(pLoopTypeChild, "max_relative_gap_difference_innerloop", m_dbInnerloopGapThreshold);
                }
            }
        }
        else if (nodeName == "COST_CONVERGENCE")
        {
            m_bIsGapBasedConvergenceOuterloop = false;

            DOMNodeList * pCostConvergenceChildren = pChildren->getChildNodes();
            for (XMLSize_t iChild = 0; iChild < pCostConvergenceChildren->getLength(); iChild++)
            {
                DOMNode * pLoopTypeChild = pCostConvergenceChildren->item(iChild);

                const string & nodeName = US(pLoopTypeChild->getNodeName());
                if (nodeName == "OUTERLOOP_COST_CONVERGENCE")
                {
                    XMLStringUtils::GetAttributeValue(pLoopTypeChild, "max_outerloop_iterations", m_iMaxOuterloopIteration);

                    XMLStringUtils::GetAttributeValue(pLoopTypeChild, "max_relative_cost_threshold_outerloop", m_mapParametersBySubpopulationName[std::string()].dbMaxRelativeCostThresholdOuterloop);
                    m_mapParametersBySubpopulationName[std::string()].dbMaxRelativeCostThresholdInnerloop = m_mapParametersBySubpopulationName[std::string()].dbMaxRelativeCostThresholdOuterloop;

                    for (std::map<std::string, ParametersForSubpopulation>::iterator iterParamsForSubpop = m_mapParametersBySubpopulationName.begin(); iterParamsForSubpop != m_mapParametersBySubpopulationName.end(); ++iterParamsForSubpop)
                    {
                        iterParamsForSubpop->second.dbMaxRelativeCostThresholdOuterloop = m_mapParametersBySubpopulationName[std::string()].dbMaxRelativeCostThresholdOuterloop;
                        iterParamsForSubpop->second.dbMaxRelativeCostThresholdInnerloop = m_mapParametersBySubpopulationName[std::string()].dbMaxRelativeCostThresholdInnerloop;
                    }

                    XMLStringUtils::GetAttributeValue(pLoopTypeChild, "max_relative_user_violation_outerloop", m_dbOuterloopRelativeUserViolation);

                    XMLStringUtils::GetAttributeValue(pLoopTypeChild, "max_relative_OD_violation_outerloop", m_dbOuterloopRelativeODViolation);

                    // read specializations by subpopulation
                    DOMNodeList * pOuterLoopCostChildren = pLoopTypeChild->getChildNodes();
                    for (XMLSize_t iOuterLoopCostChild = 0; iOuterLoopCostChild < pOuterLoopCostChildren->getLength(); iOuterLoopCostChild++)
                    {
                        DOMNode * pOuterLoopCostChild = pOuterLoopCostChildren->item(iOuterLoopCostChild);

                        const string & outerloopChildNodeName = US(pOuterLoopCostChild->getNodeName());
                        if (outerloopChildNodeName == "SUBPOPULATION_SPECIALIZATION")
                        {
                            DOMNodeList * pOuterLoopCostSpecChildren = pOuterLoopCostChild->getChildNodes();
                            for (XMLSize_t iOuterLoopCostSpecChild = 0; iOuterLoopCostSpecChild < pOuterLoopCostSpecChildren->getLength(); iOuterLoopCostSpecChild++)
                            {
                                DOMNode * pOuterLoopCostSpecChild = pOuterLoopCostSpecChildren->item(iOuterLoopCostSpecChild);

                                const string & subpopSpecnodeName = US(pOuterLoopCostSpecChild->getNodeName());
                                if (subpopSpecnodeName == "SUBPOPULATION")
                                {
                                    string subpopName;
                                    XMLStringUtils::GetAttributeValue(pOuterLoopCostSpecChild, "id", subpopName);

                                    std::map<std::string, ParametersForSubpopulation>::iterator iterParamsForSubpop = m_mapParametersBySubpopulationName.find(subpopName);
                                    if (iterParamsForSubpop == m_mapParametersBySubpopulationName.end())
                                    {
                                        m_mapParametersBySubpopulationName[subpopName] = m_mapParametersBySubpopulationName[std::string()];
                                    }
                                    if (!XMLStringUtils::GetAttributeValue(pOuterLoopCostChild, "max_relative_cost_threshold_outerloop", m_mapParametersBySubpopulationName[subpopName].dbMaxRelativeCostThresholdOuterloop))
                                    {
                                        m_mapParametersBySubpopulationName[subpopName].dbMaxRelativeCostThresholdOuterloop = m_mapParametersBySubpopulationName[std::string()].dbMaxRelativeCostThresholdOuterloop;
                                    }
                                    m_mapParametersBySubpopulationName[subpopName].dbMaxRelativeCostThresholdInnerloop = m_mapParametersBySubpopulationName[std::string()].dbMaxRelativeCostThresholdOuterloop;
                                }
                            }
                        }
                    }

                }
                else if(nodeName == "INNERLOOP_COST_CONVERGENCE")
                {
                    XMLStringUtils::GetAttributeValue(pLoopTypeChild, "max_innerloop_iterations", m_iMaxInnerloopIteration);

                    DOMNodeList * pInnerloopConvergenceTestNodes = pLoopTypeChild->getChildNodes();
                    for (XMLSize_t iChild = 0; iChild < pInnerloopConvergenceTestNodes->getLength(); iChild++)
                    {
                        DOMNode * pInnerLoopChild = pInnerloopConvergenceTestNodes->item(iChild);

                        const string & innerLoopNodeName = US(pInnerLoopChild->getNodeName());
                        if (innerLoopNodeName == "TIME_BASED_TEST")
                        {
                            m_InnerloopTestType = CostConvergence;

                            double dbInnerloopRelativeCostThreshold;
                            if (XMLStringUtils::GetAttributeValue(pInnerLoopChild, "max_relative_cost_threshold_innerloop", dbInnerloopRelativeCostThreshold))
                            {
                                for (std::map<std::string, ParametersForSubpopulation>::iterator iterParamsBySubpop = m_mapParametersBySubpopulationName.begin(); iterParamsBySubpop != m_mapParametersBySubpopulationName.end(); ++iterParamsBySubpop)
                                {
                                    iterParamsBySubpop->second.dbMaxRelativeCostThresholdInnerloop = dbInnerloopRelativeCostThreshold;
                                }
                            }

                            if (!XMLStringUtils::GetAttributeValue(pInnerLoopChild, "max_relative_user_violation_innerloop", m_dbInnerloopRelativeUserViolation))
                            {
                                m_dbInnerloopRelativeUserViolation = m_dbOuterloopRelativeUserViolation;
                            }

                            if (!XMLStringUtils::GetAttributeValue(pInnerLoopChild, "max_relative_OD_violation_innerloop", m_dbInnerloopRelativeODViolation))
                            {
                                m_dbInnerloopRelativeODViolation = m_dbOuterloopRelativeODViolation;
                            }
                        }
                        else if(innerLoopNodeName == "SWAP_BASED_TEST")
                        {
                            m_InnerloopTestType = SwapConvergence;

                            XMLStringUtils::GetAttributeValue(pInnerLoopChild, "max_relative_swapping_threshold", m_dbRelativeSwappingThreshold);

                            if(!XMLStringUtils::GetAttributeValue(pInnerLoopChild, "max_relative_OD_violation_innerloop", m_dbInnerloopRelativeODViolation))
                            {
                                m_dbInnerloopRelativeODViolation = m_dbOuterloopRelativeODViolation;
                            }
                         }
                    }
                }
            }
        }
    }

    return true;
}


void MSADualLoop::LoadFromXML(XERCES_CPP_NAMESPACE::DOMNode * pOptimizerConfigNode, ParametersForSubpopulation & paramsToFill, ParametersForSubpopulation * pDefaultValues)
{
    string optimizerName;
    if (XMLStringUtils::GetAttributeValue(pOptimizerConfigNode, "equilibrium_type", optimizerName))
    {
        if (optimizerName == "UE")
        {
            paramsToFill.eOptimizerType = Optimizer_UE;
        }
        else if (optimizerName == "BRUE")
        {
            paramsToFill.eOptimizerType = Optimizer_BRUE;
        }
        else if (optimizerName == "SO")
        {
            paramsToFill.eOptimizerType = Optimizer_SO;
        }
		else if (optimizerName == "SSO")
		{
			paramsToFill.eOptimizerType = Optimizer_SSO;
		}
        else
        {
            assert(false);
        }
    }
    else
    {
        assert(pDefaultValues);
        paramsToFill.eOptimizerType = pDefaultValues->eOptimizerType;
    }

    if (!XMLStringUtils::GetAttributeValue(pOptimizerConfigNode, "is_logit_initialize", paramsToFill.bInitialIsLogitAffectation))
    {
        paramsToFill.bInitialIsLogitAffectation = pDefaultValues->bInitialIsLogitAffectation;
    }

    if (!XMLStringUtils::GetAttributeValue(pOptimizerConfigNode, "is_uniform_initialization", paramsToFill.bIsUniformInitialization))
    {
        paramsToFill.bIsUniformInitialization = pDefaultValues->bIsUniformInitialization;
    }

    if (!XMLStringUtils::GetAttributeValue(pOptimizerConfigNode, "is_smart_stepsize", paramsToFill.bStepSizeAffectation))
    {
        paramsToFill.bStepSizeAffectation = pDefaultValues->bStepSizeAffectation;
    }
    if (NeedsParallelSimulationInstances() && paramsToFill.bStepSizeAffectation)
    {
        BOOST_LOG_TRIVIAL(error) << "is_smart_stepsize is not available for Heuristic dual loop :(";
        exit(-1);
    }

    if (!XMLStringUtils::GetAttributeValue(pOptimizerConfigNode, "sigma_convenience", paramsToFill.dbSigmaConvenience))
    {
        paramsToFill.dbSigmaConvenience = pDefaultValues->dbSigmaConvenience;
    }

    if (!XMLStringUtils::GetAttributeValue(pOptimizerConfigNode, "value_of_time", paramsToFill.dbValueOfTime))
    {
        paramsToFill.dbValueOfTime = pDefaultValues->dbValueOfTime;
    }
}

bool MSADualLoop::Initialize(const boost::posix_time::ptime & startTime, const boost::posix_time::ptime & endTime, vector<Population *> &listPopulations, std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, bool bDayToDayAffectation, bool bUseCostsForTemporalShortestPath)
{
    // convert map of parameters by subpopulation name to map of parameters by subpopulation pointer to avoid a lot of string comparisons later.
    m_dbPreviousOuterLoopGap = numeric_limits<double>::infinity();
    for (size_t iPop = 0; iPop < listPopulations.size(); iPop++)
    {
        for (size_t iSubPop = 0; iSubPop < listPopulations[iPop]->GetListSubPopulations().size(); iSubPop++)
        {
            SubPopulation * pSubPop = listPopulations[iPop]->GetListSubPopulations().at(iSubPop);

            m_mapPreviousGap[pSubPop] = numeric_limits<double>::infinity();
            m_mapPreviousOuterLoopGap[pSubPop] = numeric_limits<double>::infinity();

            bool bSubPopFound = false;
            for (std::map<std::string, ParametersForSubpopulation>::const_iterator iter = m_mapParametersBySubpopulationName.begin(); iter != m_mapParametersBySubpopulationName.end(); ++iter)
            {
                if (pSubPop->GetStrName() == iter->first)
                {
                    m_mapParametersBySubpopulation[pSubPop] = iter->second;
                    bSubPopFound = true;
                    break;
                }
            }
            if (!bSubPopFound)
            {
                m_mapParametersBySubpopulation[pSubPop] = m_mapParametersBySubpopulationName[std::string()];
            }
        }
    }

    // times-->numbers
    double dbDepartureTime = 0.0;
    double dbEndSimulationTime = (((double)(endTime - startTime).total_microseconds()) / 1000000);
    SetStartSimulationTime(startTime);
    SetEndSimulationTime(endTime);

    // OTK - TODO - if we want to do it like this, we should output the random seed user in order to be able to reproduce the simulation...
    // Maybe it should be good to do something similar to SymuVia and use a global seed for SymuMaster : we should have juste one srand call for SymuMaster
    //to have different rand between each execution
    unsigned int iSeed = (unsigned int)time(0);
    BOOST_LOG_TRIVIAL(info) << "Using seed " << iSeed << " for probabilistic method";
    srand(iSeed);

    m_bDayToDayAffectation = bDayToDayAffectation;
    m_bUseCostsForTemporalShortestPath = bUseCostsForTemporalShortestPath;

    SubPopulation* currentSubPopulation;
    Trip* currentTrip;
    bool bOk = true;
    bool needKParameters = ((m_bIterationIsKShortestPath || m_bInitialIsKShortestPath) && (m_iKVariable == -1 || m_bCommonalityFilter || m_dbWardropTolerance == -1));
    bool bNeedLogitParamsForAtLeastOneSubpopulation = false;

    for (size_t iPopulation = 0; iPopulation < listPopulations.size(); iPopulation++)
    {
        Population* currentPopulation = listPopulations[iPopulation];
        for (size_t iSubPopulation = 0; iSubPopulation < currentPopulation->GetListSubPopulations().size(); iSubPopulation++)
        {
            currentSubPopulation = currentPopulation->GetListSubPopulations()[iSubPopulation];

            const ParametersForSubpopulation & subpopulationParams = m_mapParametersBySubpopulation.at(currentSubPopulation);

            mapCostFunctions[currentSubPopulation] = (subpopulationParams.eOptimizerType == Optimizer_SO) || (subpopulationParams.eOptimizerType == Optimizer_SSO) ? CF_Marginals : CF_TravelTime;

            bool needLogitParameters = ((m_MSAType == MSA_Logit || subpopulationParams.bInitialIsLogitAffectation) && m_dbDefaultTetaLogit == -1);
            if (needLogitParameters)
            {
                bNeedLogitParamsForAtLeastOneSubpopulation = true;
            }

            map<Origin*, map<Destination*, ListTimeFrame<double> > >& KParametersBySubPopulation = m_KParameters[currentSubPopulation];
            map<Origin*, map<Destination*, ListTimeFrame<double> > >& LogitParametersBySubPopulation = m_LogitParameters[currentSubPopulation];
            for(size_t iTrip = 0; iTrip < currentSubPopulation->GetListUsers().size(); iTrip++)
            {
                currentTrip = currentSubPopulation->GetListUsers().at(iTrip);

                //check if this trip hasn't already a predefined path
                if(currentTrip->isPredefinedPath())
                {
                    continue;
                }

                if(needKParameters)
                {
                    KParametersBySubPopulation[currentTrip->GetOrigin()][currentTrip->GetDestination()];
                }else if(m_bIterationIsKShortestPath || m_bInitialIsKShortestPath){
                    double kValue = m_iKVariable;
                    KParametersBySubPopulation[currentTrip->GetOrigin()][currentTrip->GetDestination()].addTimeFrame(dbDepartureTime, dbEndSimulationTime, boost::make_shared<double>(kValue));
                }

                if(needLogitParameters)
                {
                    LogitParametersBySubPopulation[currentTrip->GetOrigin()][currentTrip->GetDestination()];
                }
                else if (m_MSAType == MSA_Logit || subpopulationParams.bInitialIsLogitAffectation)
                {
                    LogitParametersBySubPopulation[currentTrip->GetOrigin()][currentTrip->GetDestination()].addTimeFrame(dbDepartureTime, dbEndSimulationTime, boost::make_shared<double>(m_dbDefaultTetaLogit));
                }
            }
        }
    }

    if (m_bIterationIsKShortestPath || m_bInitialIsKShortestPath)
    {
        bOk = SymFillShortestPathParameters(needKParameters, m_bCommonalityFilter, m_dbAssignementAlpha, m_dbAssignementBeta, m_dbAssignementGamma, m_listCommonalityFactorParameters, m_dbWardropTolerance, m_KParameters);
    }

    if (bOk && bNeedLogitParamsForAtLeastOneSubpopulation)
    {
        bOk = SymFillLogitParameters(m_LogitParameters);
    }

    if(m_bIterationIsKShortestPath)
    {
        m_ShortestPathsMethod = new KShortestPaths(m_dbAssignementAlpha, m_dbAssignementBeta, m_dbAssignementGamma, m_bCommonalityFilter, m_listCommonalityFactorParameters, m_dbKShortestPathsThreshold, m_KParameters);
    }else{
        m_ShortestPathsMethod = new Dijkstra();
    }
    if(m_bInitialIsKShortestPath)
    {
        m_InitialShortestPathsMethod = new KShortestPaths(m_dbAssignementAlpha, m_dbAssignementBeta, m_dbAssignementGamma, m_bCommonalityFilter, m_listCommonalityFactorParameters, m_dbKShortestPathsThreshold, m_KParameters);
    }else{
        m_InitialShortestPathsMethod = new Dijkstra();
    }

    m_bChangeAssignment = false;
	m_bChangeInitialization = false;

    return bOk;
}

// Main function to optimize
bool MSADualLoop::AssignPathsToTrips(const vector<Trip *> &listSortedTrip, const boost::posix_time::ptime &startPeriodTime, const boost::posix_time::time_duration & PeriodTimeDuration,
    const boost::posix_time::time_duration &travelTimesUpdatePeriodDuration, int& iCurrentIterationIndex, int iPeriod, int iSimulationInstance, int iSimulationInstanceForBatch, bool bAssignAllToFinalSolution)
{
    assert(!bAssignAllToFinalSolution);

    if(m_bChangeAssignment)
    {
		if (!m_bChangeInitialization)
			AffectBestAssignment(listSortedTrip, iSimulationInstanceForBatch);
        if (iSimulationInstance == 0)
        {
            iCurrentIterationIndex--; //this is not a specific iteration of OuterLoop neither InnerLoop
        }
        return true; // skip other assignment method and only do the traffic simulation
    }

    //Determine the Value Of K for the period
    if(!m_mapPredefinedPaths.empty() && m_bUsePredefinedPathsOnly)//Always iCurrentIterationIndex for assignment with predefined paths only
    {
        m_iValueOfK = iCurrentIterationIndex;
    }else{
        if (m_bIsInitialStepSizeOuterloopIndex)
        {
            m_iValueOfK = m_iOuterloopIndex;
        }
        else
        {
            m_iValueOfK = m_iInnerloopIndex + 1;
        }
    }

    // times-->numbers
    double dbDepartureTime = (((double)(startPeriodTime - m_dtStartSimulationTime).total_microseconds()) / 1000000);
    double dbEndPeriodTime = (((double)(PeriodTimeDuration).total_microseconds()) / 1000000) + dbDepartureTime;

    m_iCurrentPeriod = iPeriod;

    // prepare assignment on first iteration :
    if (iCurrentIterationIndex == 0)
    {
        m_mapImprovedStepSize.clear();
        BOOST_LOG_TRIVIAL(debug) << "Compute first shortest path";
		// Compute the first shortest path(s)
        bool bOk = ComputeFirstShortestPaths(listSortedTrip, dbDepartureTime, dbEndPeriodTime, travelTimesUpdatePeriodDuration, iSimulationInstance, iSimulationInstanceForBatch);
        if(!bOk)
            return false;
    }

    GroupCloseShortestPath(iSimulationInstanceForBatch);

    // First loop on SubPopulations
    for (mapBySubPopulationOriginDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath> > >::iterator itShortestPathSubPopulation = m_mapOriginalShortestPathResults.begin(); itShortestPathSubPopulation != m_mapOriginalShortestPathResults.end(); ++itShortestPathSubPopulation)
    {
        SubPopulation* pSubPopulation = itShortestPathSubPopulation->first;

        mapByOriginDestination< vector<Trip*> >& TripsByODAtSubPopulation = m_mapTripsByOD[pSubPopulation];
		mapByOriginDestination <int>& StepsizeByODAtSubPopulation = m_mapImprovedStepSize[pSubPopulation];
        mapByOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& AffectedTripsAtSubPopulation = m_mapAffectedTrips[iSimulationInstance][pSubPopulation];

        const ParametersForSubpopulation & paramsForSubpop = m_mapParametersBySubpopulation.at(pSubPopulation);
        bool bIsBRUE = paramsForSubpop.eOptimizerType == Optimizer_BRUE;

        // Second loop on Origins
        for (mapByOriginDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath> > >::iterator itShortestPathOrigin = itShortestPathSubPopulation->second.begin(); itShortestPathOrigin != itShortestPathSubPopulation->second.end(); ++itShortestPathOrigin)
        {
            Origin * pOrigin = itShortestPathOrigin->first;
            mapByDestination< vector<Trip*> >& TripsByODAtOrigin = TripsByODAtSubPopulation[pOrigin];
			mapByDestination <int>& StepsizeByODAtOrigin = StepsizeByODAtSubPopulation[pOrigin];
            mapByDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& AffectedTripsAtOrigin = AffectedTripsAtSubPopulation[pOrigin];

            // Assignment loop
            for (mapByDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath> > >::iterator itShortestPathDestination = itShortestPathOrigin->second.begin(); itShortestPathDestination != itShortestPathOrigin->second.end(); ++itShortestPathDestination)
            {
                Destination* pDestination = itShortestPathDestination->first;
				int &Stepsize = StepsizeByODAtOrigin[pDestination];
                vector<boost::shared_ptr<SymuCore::ValuetedPath> > & listPathsFound = itShortestPathDestination->second;
                vector<Trip*>& listTripsByOD = TripsByODAtOrigin[pDestination];
                map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath = AffectedTripsAtOrigin[pDestination];
				//Step size method
                if (paramsForSubpop.bStepSizeAffectation)
				{
					if ((iCurrentIterationIndex == 0) || ((m_iInnerloopIndex == 0) && (m_bInitialization)))
					{
						Stepsize = 1;
					}
					else if (m_iInnerloopIndex == 0 && m_iOuterloopIndex > 1)
					{
						Stepsize = m_iOuterloopIndex;
					}
					else if (m_mapPrGapByOD[pSubPopulation][pOrigin][pDestination] - m_mapGapByOD[pSubPopulation][pOrigin][pDestination] <= 0)
					{
						Stepsize++;
					}
					m_mapPrGapByOD[pSubPopulation][pOrigin][pDestination] = m_mapGapByOD[pSubPopulation][pOrigin][pDestination];
				}
				else
				{
					Stepsize = m_iValueOfK;
				}
					
                if (iCurrentIterationIndex != 0)
                {
				    // MSA switch
				    switch(m_MSAType)
				    {
					    case MSA_Simple: // V.1: Classic MSA
					    {
                            ClassicMSA(mapValuetedPath, listPathsFound, listTripsByOD, Stepsize, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop);
                            break;
					    }
					    case MSA_Smart: // V.2: Smart MSA (ranking)
					    {
							BOOST_LOG_TRIVIAL(warning) << "SMART MSA " << pSubPopulation->GetStrName() << " " << pOrigin->getStrNodeName() << "->" << pDestination->getStrNodeName();
						    SmartMSA(mapValuetedPath, listPathsFound, listTripsByOD, Stepsize, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop);
                            break;
					    }
					    case MSA_GapBased: // V.3: Smart MSA (gap based)
                        case MSA_GapBasedProb: // TODO Mostafa
					    case MSA_GapBasedNormalize: // V.4: Gap based approach with normalization
					    {
						    GapBasedMSA(mapValuetedPath, listPathsFound, listTripsByOD, Stepsize, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop);
                            break;
					    }
					    case MSA_GapLudo: // V.3.5: Smart MSA (gap based with recovery)
					    {
						    GapBasedLudoMSA(mapValuetedPath, listPathsFound, listTripsByOD, Stepsize, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop);
                            break;
					    }
					    case MSA_Probabilistic: // V.5: New version swap with probability of improvement and random number
                        case MSA_StepSizeProb: // TODO Mostafa
					    {
                            ProbabilisticMSA(mapValuetedPath, listPathsFound, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop, Stepsize);
                            break;
					    }
                        case MSA_WithinDayFPA:
                        {
                            WithinDayFPAMSA(mapValuetedPath, listPathsFound, listTripsByOD, Stepsize, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop);
                            break;
                        }
                        case MSA_InitialBasedWDFPA:
                        {
                            InitialBasedWDFPAMSA(mapValuetedPath, listPathsFound, listTripsByOD, Stepsize, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop);
                            break;
                        }
                        case MSA_InitialBasedSimple:
                        {
                            InitialBasedSimpleMSA(mapValuetedPath, listPathsFound, listTripsByOD, Stepsize, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop);
                            break;
                        }
					    case MSA_Logit: // V.6: New version of classic MSA but we also take in account the coeff of logit
					    {
                            double dbTetaLogit = *m_LogitParameters.at(pSubPopulation).at(pOrigin).at(pDestination).getData(dbEndPeriodTime);
                            LogitMSA(mapValuetedPath, listPathsFound, listTripsByOD, dbTetaLogit, Stepsize, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop);
                            break;
                        }//end switch
                    }
                }
            }
        }
    }
    //ShowResult(listSortedTrip);

    return true;
}


bool MSADualLoop::onSimulationInstanceDone(const std::vector<Trip *> &listTrip, const boost::posix_time::ptime & endPeriodTime, int iSimulationInstance, int iSimulationInstanceForBatch, int iCurrentIterationIndex)
{
    UpdatePathCostAndShortestPath(m_mapNewShortestPathResults[iSimulationInstance], iSimulationInstanceForBatch);
    return true;
}


void MSADualLoop::Decision()
{
    m_mapOriginalShortestPathResults = m_mapNewShortestPathResults[DEFAULT_SIMULATION_INSTANCE];
}

void MSADualLoop::SaveCurrentAssignment(const std::vector<SymuCore::Trip*> & listTrip, int iSimulationInstanceForBatch)
{
    m_listPreviousAffectationPaths.clear();
    m_listPreviousAffectationUnassignedPaths.clear();
    for (size_t i = 0; i < listTrip.size(); i++)
    {
        m_listPreviousAffectationPaths.push_back(listTrip[i]->GetPath(iSimulationInstanceForBatch));
    }
    for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterSubPopulation = m_mapAffectedTrips[iSimulationInstanceForBatch].begin(); iterSubPopulation != m_mapAffectedTrips[iSimulationInstanceForBatch].end(); ++iterSubPopulation)
    {
        for (mapByOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
        {
            for (mapByDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                for (map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = iterDestination->second.begin(); iterPath != iterDestination->second.end(); ++iterPath)
                {
                    if (iterPath->second.empty())
                    {
                        m_listPreviousAffectationUnassignedPaths[iterSubPopulation->first][iterOrigin->first][iterDestination->first].push_back(iterPath->first->GetPath());
                    }
                }
            }
        }
    }
}


bool MSADualLoop::IsRollbackNeeded(const std::vector<Trip *> &listTrip, const boost::posix_time::ptime & startPeriodTime, const boost::posix_time::time_duration &PeriodTimeDuration, int iCurrentIterationIndex, int nbParallelSimulationMaxNumber, bool& bOk)
{

    //times
    double dbDepartureTime = ((double)(startPeriodTime - m_dtStartSimulationTime).total_microseconds()) / 1000000;
    double dbEndPeriodTime = (((double)(PeriodTimeDuration).total_microseconds()) / 1000000) + dbDepartureTime;

    bool bStopIteration = false;

	if (MaximumSwappingReached(iCurrentIterationIndex))
    {
        if(!m_bChangeAssignment)
        {
            BOOST_LOG_TRIVIAL(info) << "The maximum number of swapping possible is reached for all OD !" << endl;
            //BOOST_LOG_TRIVIAL(warning) << "Final Gap: " << m_dbBestQuality;
			m_iInnerloopIndex = m_iMaxInnerloopIteration; // to force the roolback to OuterLoop at next iteration
		}
		else{
			PrintOutputData();
		}
        if(iCurrentIterationIndex == 0)//special case to print even if maximum swapping is reached at first iteration
            PrintOutputData();

        bStopIteration = true; // no need to continue
    }

    //check for best solution quality
    if((iCurrentIterationIndex != 0))
    {
        if(m_bChangeAssignment)
        {
            m_bChangeAssignment = false;
            if (!m_bChangeInitialization)
            {
                m_dbActualGlobalGap = ComputeGlobalGap(m_mapActualGlobalGap, m_mapNewShortestPathResults[DEFAULT_SIMULATION_INSTANCE], DEFAULT_SIMULATION_INSTANCE);
            }
        }else{
            KeepBestAssignment(listTrip, bStopIteration);
            if(m_bChangeAssignment)
                return true;
        }
    }

    //--->check if we iterate through inner Loop
    if (!bStopIteration && iCurrentIterationIndex != 0 && IsInnerLoopIterationNeeded(m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE]) && m_iInnerloopIndex < m_iMaxInnerloopIteration)
    {
        //iterate through innerLoop
        m_iInnerloopIndex++;
        BOOST_LOG_TRIVIAL(info) << "RollBack needed for InnerLoop " << m_iInnerloopIndex;
        if (m_iInnerloopIndex >= m_iMaxInnerloopIteration)
        {
            // OTK - TODO - question : du coup en l'état, on gardait la dernière simulation faite pour passer à la prochaine outerloop ? (pas de decision ? quel intérêt de cette dernière itération ?)
            // if maximum inner loop iteration reached, iterate through outer loop instead
            BOOST_LOG_TRIVIAL(info) << "Maximum Innerloop reached !"; 
			BOOST_LOG_TRIVIAL(warning) << "Final Gap for the outer loop: " << m_dbBestQuality;
        }else
		{
            Decision();
			return true;
        }
    }
	else if (iCurrentIterationIndex != 0 && !bStopIteration && m_iInnerloopIndex < m_iMaxInnerloopIteration)
    {
        KeepBestAssignment(listTrip, true);
        if (m_bChangeAssignment)
        {
            m_iInnerloopIndex = m_iMaxInnerloopIteration; // to force the roolback to OuterLoop at next iteration
            return true;
        }
    }
    //<---

    //calcul shortest path
    if(!bStopIteration)
    {
        bOk = CalculateShortestPaths(listTrip, dbEndPeriodTime, false, m_mapOriginalShortestPathResults, DEFAULT_SIMULATION_INSTANCE);
		if (!bOk)
		{
            //BOOST_LOG_TRIVIAL(warning) << "OuterLoop finish by bOK";
            //BOOST_LOG_TRIVIAL(warning) << "Final Gap: " << m_dbBestQuality;
			return false;
		}
		// For heuristic case only, we call the groupShortestpaths method since it is not done in the next assignpathtotrips for heuristic method (see comment there)
		if (dynamic_cast<HeuristicDualLoop*>(this)) {
			GroupCloseShortestPath(DEFAULT_SIMULATION_INSTANCE);
		}
        PrintOutputData();
    }

    if (iCurrentIterationIndex == 0)
    {
        std::map<SubPopulation*, double> placeHolder;
        m_dbBestQuality = ComputeGlobalGap(placeHolder, m_mapOriginalShortestPathResults, DEFAULT_SIMULATION_INSTANCE);
        SaveCurrentAssignment(listTrip, DEFAULT_SIMULATION_INSTANCE);
    }
            
    //--->check if we stop iteration
    bool bHasNewShortestPathDiscovered = HasNewShortestPathDiscovered();

	// Modification for Lukas - 2018-04-11
	//bHasNewShortestPathDiscovered = false;
	// End of modification

    if (IsOuterLoopIterationNeeded(iCurrentIterationIndex, (int)listTrip.size(), bHasNewShortestPathDiscovered) && !bStopIteration)
    {

		//Initialization
		if (m_bInitialization)
		{
			if (m_bChangeInitialization)
				m_bChangeInitialization = false;
			else if (!bStopIteration && m_iOuterloopIndex > 0)
			{
                //BOOST_LOG_TRIVIAL(info) << "Here in Initialization";
                Initialization(nbParallelSimulationMaxNumber);
				m_bChangeAssignment = true;
				m_bChangeInitialization = true;
				return true;
			}
		}
        //iterate through outerLoop
        m_iInnerloopIndex = 0;
        //BOOST_LOG_TRIVIAL(warning) << "Final Gap for outer loop " << m_iOuterloopIndex << ": " << m_dbBestQuality;
        m_iOuterloopIndex++;
        BOOST_LOG_TRIVIAL(info) << "RollBack needed for OuterLoop " << m_iOuterloopIndex;

        if (m_iOuterloopIndex > m_iMaxOuterloopIteration)
        {
            BOOST_LOG_TRIVIAL(info) << "Maximum Outerloop reachead !";
            //BOOST_LOG_TRIVIAL(warning) << "Final Gap: " << m_dbBestQuality;
            bStopIteration = true;  //reach the maximum convergence allowed
        }
		
		BOOST_LOG_TRIVIAL(warning) << "Outer Loop is started";

        if (bHasNewShortestPathDiscovered)
        {
            m_dbBestQuality = m_dbPreviousOuterLoopGap;
        }

    }else{
		bStopIteration = true;
    }

	//BOOST_LOG_TRIVIAL(warning) << "before fill previous assignment";

    if(bStopIteration){

        for (std::map<SubPopulation*, double>::iterator iterGap = m_mapPreviousGap.begin(); iterGap != m_mapPreviousGap.end(); ++iterGap)
        {
            iterGap->second = numeric_limits<double>::infinity();
        }
        for (std::map<SubPopulation*, double>::iterator iterGap = m_mapPreviousOuterLoopGap.begin(); iterGap != m_mapPreviousOuterLoopGap.end(); ++iterGap)
        {
            iterGap->second = numeric_limits<double>::infinity();
        }
        m_dbPreviousOuterLoopGap = numeric_limits<double>::infinity();

        if(m_bIsFillFromPreviousPeriod)
        {
			//BOOST_LOG_TRIVIAL(warning) << "fill previous assignment";

            //keep curent affectation for next Period
            m_mapPreviousAffectationRatio.clear();
            // rmq. here in outerloop, all simulation instances should have been resynced, so we can use DEFAULT_SIMULATION_INSTANCE
            for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterSubPopulation = m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].begin(); iterSubPopulation != m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].end(); ++iterSubPopulation)
            {
				SubPopulation* pSubPopulation = iterSubPopulation->first;

				mapByOriginDestination< vector<Trip*> >& TripsByODAtSubPopulation = m_mapTripsByOD[pSubPopulation];

                mapByOriginDestination< multimap<double, boost::shared_ptr<SymuCore::ValuetedPath>> >& AffectationRatioAtSubPopulation = m_mapPreviousAffectationRatio[iterSubPopulation->first];
                for (mapByOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
                {
					Origin* pOrigin = iterOrigin->first;
					mapByDestination< vector<Trip*> >& TripsByODAtOrigin = TripsByODAtSubPopulation[pOrigin];


                    mapByDestination< multimap<double, boost::shared_ptr<SymuCore::ValuetedPath>> >& AffectationRatioAtOrigin = AffectationRatioAtSubPopulation[iterOrigin->first];
                    for (mapByDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
                    {
						Destination* pDestination = iterDestination->first;
						vector<Trip*>& listTripsByOD = TripsByODAtOrigin[pDestination];

                        multimap<double, boost::shared_ptr<SymuCore::ValuetedPath>>& mapAffectationRatio = AffectationRatioAtOrigin[iterDestination->first];
                        for (map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = iterDestination->second.begin(); iterPath != iterDestination->second.end(); ++iterPath)
                        {
							//BOOST_LOG_TRIVIAL(warning) << "fill previous assignment " << iterOrigin->first->getStrNodeName() << " " << iterDestination->first->getStrNodeName() << " " << iterPath->second.size() << " " << listTripsByOD.size() << " " << ((double)(iterPath->second.size())) / ((double)(listTripsByOD.size()));
                            //mapAffectationRatio.insert(make_pair((double)(iterPath->second.size() / listTrip.size()), iterPath->first));
							mapAffectationRatio.insert(make_pair( ((double)(iterPath->second.size())) / ((double)(listTripsByOD.size())), iterPath->first));
                        }
                    }
                }
            }
        }else{
            ClearMemory();
        }
		BOOST_LOG_TRIVIAL(warning) << "Final Gap : " << m_dbBestQuality;
        return false;
    }
    //<---

    return true;
}


bool MSADualLoop::ComputeFirstShortestPaths(const vector<Trip*>& listTrips, double dbDepartureTime, double dbEndPeriodTime, const boost::posix_time::time_duration& travelTimesUpdatePeriodDuration, int iSimulationInstance, int iSimulationInstanceForBatch)
{
    if (iSimulationInstance == 0)
    {
        m_mapOriginalShortestPathResults.clear();
        m_mapTripsByOD.clear();
        m_iInnerloopIndex = 0;
        m_iOuterloopIndex = 0;

        //---> calculate the different times for the aggregation
        m_listEndPeriods.clear();
        m_listEndPeriods.push_back(dbEndPeriodTime);

        if (m_iUpdateCostNumber > 1)
        {
            double dbTravelTimeUpdateDuration = ((double)(travelTimesUpdatePeriodDuration).total_microseconds()) / 1000000;
            double dbAggregationDepartureTime = max(dbDepartureTime, dbEndPeriodTime - (dbTravelTimeUpdateDuration * m_iUpdateCostPeriodsNumber));
            double dbIntervalToWorkOn = dbEndPeriodTime - dbAggregationDepartureTime;
            double dbDividedStep = dbIntervalToWorkOn / (m_iUpdateCostNumber - 1);
            for (int i = 0; i < m_iUpdateCostNumber - 1; i++)
            {
                int indexPeriod = (int)((dbDividedStep * i) / dbTravelTimeUpdateDuration);
                m_listEndPeriods.push_back((indexPeriod * dbTravelTimeUpdateDuration) + dbAggregationDepartureTime);
            }
        }
        assert(m_iUpdateCostNumber == m_listEndPeriods.size());

        Trip * pTrip;
        for (size_t i = 0; i < listTrips.size(); i++)
        {
            pTrip = listTrips[i];
            m_mapTripsByOD[pTrip->GetSubPopulation()][pTrip->GetOrigin()][pTrip->GetDestination()].push_back(pTrip);
        }
    }

    if (iSimulationInstanceForBatch == 0)
    {
        m_mapAffectedTrips.clear();
    }

    mapBySubPopulationOriginDestination< vector<Trip*> > partialMapTripsByOD = m_mapTripsByOD;
    //handle predefined affectation (remove them from partialMapTripsByOD)
    CheckPredefinedODAffectation(partialMapTripsByOD, iSimulationInstanceForBatch);

    vector<Trip*> listNotAffectedUsers;
    //fill from previous iteration results (will remove OD found from partialMapTripsByOD and add users not affected to listNotAffectedUsers)
    FillFromPreviousAffectationRatio(partialMapTripsByOD, listNotAffectedUsers, dbEndPeriodTime, iSimulationInstanceForBatch);

    //all users are affected
    if(listNotAffectedUsers.empty())
        return true;

	//BOOST_LOG_TRIVIAL(warning) << "Remains not affected trip: " << listNotAffectedUsers.size();

    // we do it before CalculateShortestPaths to avoid unnecessary processing in CalculateShortestPaths -> CompareExistingValuetedPath
    m_lInitialBest.clear();

    //if there is still user not affected in listTrips
    bool bOk = CalculateShortestPaths(listNotAffectedUsers, dbEndPeriodTime, true, m_mapOriginalShortestPathResults, iSimulationInstanceForBatch);
    if(!bOk)
        return false;

    //affect them with all or nothing affectation
    bOk = InitialAffectation(partialMapTripsByOD, dbEndPeriodTime, iSimulationInstanceForBatch);
    
    if (m_MSAType == MSA_InitialBasedWDFPA || m_MSAType == MSA_InitialBasedSimple)
    {
        assert(iSimulationInstanceForBatch == 0); // MSA_InitialBasedWDFPA and MSA_InitialBasedSimple are for MSADualLoop only

        for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterSubPopulation = m_mapAffectedTrips[0].begin(); iterSubPopulation != m_mapAffectedTrips[0].end(); ++iterSubPopulation)
        {
            for (mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
            {
                for (mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
                {
                    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = iterDestination->second.begin(); iterPath != iterDestination->second.end(); ++iterPath)
                    {
                        m_mapIntialAssignment[iterPath->first] = (int)iterPath->second.size();
                    }
                }
            }
        }
    }

    return bOk;
}


struct PathSorter {
    bool operator()(const std::pair<Path, double> & a, const std::pair<Path, double> & b) const
    {
        return a.second > b.second;
    }
};


void MSADualLoop::CheckPredefinedODAffectation(mapBySubPopulationOriginDestination< vector<Trip*> >& partialMapTripsByOD, int iSimulationInstanceForBatch)
{
    for (mapBySubPopulationOriginDestination< map<Path, vector<double> > >::iterator iterSubPopulation = m_mapPredefinedPaths.begin(); iterSubPopulation != m_mapPredefinedPaths.end(); ++iterSubPopulation)
    {
        mapByOriginDestination< vector<Trip*> >& mapTripAtSubPopulation = partialMapTripsByOD[iterSubPopulation->first];
        for (mapByOriginDestination< map<Path, vector<double> > >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
        {
            mapByDestination< vector<Trip*> >& mapTripAtOrigin = mapTripAtSubPopulation[iterOrigin->first];
            for (mapByDestination< map<Path, vector<double> > >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                const vector<double> & testAffectationValues = iterDestination->second.begin()->second;
                if(m_iCurrentPeriod >= (int)(testAffectationValues.size()) || testAffectationValues[m_iCurrentPeriod] == -1) // ???
                    continue;

                vector<Trip*>& listTrip = mapTripAtOrigin[iterDestination->first];
                int nbTotalUsers = (int)listTrip.size();
                int iNbUsersToAffect;

                // we sort paths with decreasing coefficient so the highest coefficient get assigned first (with small number of users, the ceiling of the number of users to swap
                // can make the users assigned to a path that has not the highest coefficient)
                std::vector<std::pair<Path, double>> paths;
                for (map<Path, vector<double> >::iterator iterPath = iterDestination->second.begin(); iterPath != iterDestination->second.end(); ++iterPath)
                {
                    paths.push_back(std::pair<Path, double>(iterPath->first, iterPath->second[m_iCurrentPeriod]));
                }
                std::sort(paths.begin(), paths.end(), PathSorter());

                Path lastPath;
				int nbAffUsers=0; // total numbers of assigned vehicles
                for (size_t iPath = 0; iPath < paths.size(); iPath++)
                {
                    const std::pair<Path, double> & pathCoeff = paths[iPath];
                    lastPath = pathCoeff.first;
					iNbUsersToAffect = (int)floor(pathCoeff.second * nbTotalUsers);

					// Last vehicle to be assigned
					if(iNbUsersToAffect==0 && pathCoeff.second>0 && nbAffUsers<nbTotalUsers)
					{
						iNbUsersToAffect = 1;
					}
					
                    int index = 0;
					nbAffUsers += iNbUsersToAffect;

                    // we affect trips uniformly :
                    std::vector<size_t> usersToAffect;
                    for (int iTrip = 0; iTrip < iNbUsersToAffect; iTrip++)
                    {
                        double dbUserIndex = ((double)listTrip.size() / iNbUsersToAffect) * iTrip;
                        size_t userIndex = (size_t)floor(dbUserIndex);

                        if (userIndex >= listTrip.size()) break;

                        usersToAffect.push_back(userIndex);
                    }
                    for (vector<size_t>::reverse_iterator itTrip = usersToAffect.rbegin(); itTrip != usersToAffect.rend(); ++itTrip)
                    {
                        listTrip[*itTrip]->SetPath(iSimulationInstanceForBatch, lastPath);
                        if (!m_bDayToDayAffectation && m_bUsePredefinedPathsOnly)
                        {
                            listTrip.erase(listTrip.begin() + *itTrip);
                        }
                        else
                        {
                            listTrip[*itTrip]->SetIsPredefinedPath(true);
                        }
                    }
                }

                for(vector<Trip*>::iterator itTrip = listTrip.begin(); itTrip != listTrip.end();) // assign all left users
                {
					// if isPredefinedPath, a path has already been set to the trip : don't override it with lastPath !
					if (!(*itTrip)->isPredefinedPath())
					{
						(*itTrip)->SetPath(iSimulationInstanceForBatch, lastPath);
					}

					if (!m_bDayToDayAffectation && m_bUsePredefinedPathsOnly)
					{
						itTrip = listTrip.erase(itTrip);
					}
					else
					{
						(*itTrip)->SetIsPredefinedPath(true);
						++itTrip;
					}
                }
                if(listTrip.empty())
                {
                    mapTripAtOrigin.erase(iterDestination->first);
                }
            }
            if(mapTripAtOrigin.empty())
            {
                mapTripAtSubPopulation.erase(iterOrigin->first);
            }
        }
        if(mapTripAtSubPopulation.empty())
        {
            partialMapTripsByOD.erase(iterSubPopulation->first);
        }
    }
}

struct SortByOriginDestination {
    bool operator()(const SymuCore::Trip* p1, const SymuCore::Trip* p2) const
    {
        return p1->GetOrigin()->getNode()->getStrName() < p2->GetOrigin()->getNode()->getStrName()
            || (p1->GetOrigin()->getNode()->getStrName() == p2->GetOrigin()->getNode()->getStrName() && p1->GetDestination()->getNode()->getStrName() < p2->GetDestination()->getNode()->getStrName());
    }
};

bool MSADualLoop::CalculateShortestPaths(const vector<Trip*> &ListTrips, double dbEndPeriodTime, bool isInitialization, mapBySubPopulationOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> > & shortestPaths, int iSimulationInstanceForBatch)
{
    //Computing shortest paths from destinations to origins
    if (!isInitialization)
    {
        shortestPaths.clear();
    }
    //BOOST_LOG_TRIVIAL(warning) << "Shortest path computation";
    //map of trips sorted by SubPopulation
    Trip * pTrip;
    map<SubPopulation*, vector<Trip*> > TripsBySubPopulation;
    for (size_t i = 0; i < ListTrips.size(); i++)
    {
        pTrip = ListTrips[i];
        TripsBySubPopulation[pTrip->GetSubPopulation()].push_back(pTrip);
    }

    //sort the users by OD so the next algorithm works (it assumes trips sorted by OD)
    for (map<SubPopulation*, vector<Trip*> >::iterator iterSubPopulation = TripsBySubPopulation.begin(); iterSubPopulation != TripsBySubPopulation.end(); ++iterSubPopulation)
    {
        std::sort(iterSubPopulation->second.begin(), iterSubPopulation->second.end(), SortByOriginDestination());
    }

    map<Origin*, set<Destination*> > sortedLstDestinations;
    map<set<Destination*>, vector<Origin*> > mapConnectedDestinationOrigin;
    bool hasPredefinedPaths = false;
    Origin* pLastOrigin;
    Destination* pLastDestination;

    for (map<SubPopulation*, vector<Trip*> >::iterator iterSubPopulation = TripsBySubPopulation.begin(); iterSubPopulation != TripsBySubPopulation.end(); ++iterSubPopulation)
    {
        SubPopulation* pSubPopulation = iterSubPopulation->first;
        mapByOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip *>, CompareValuetedPath> > affectedTripsAtSubPopulation = m_mapAffectedTrips[iSimulationInstanceForBatch][pSubPopulation];

        pLastOrigin = NULL;
        pLastDestination = NULL;

        //for all trip, we group destination by same origin
        //trip are implicitly sorted by OD, so we can't return one a already seen OD (same goes for subPopulation)
        for (vector<Trip*>::const_iterator itTrip = iterSubPopulation->second.begin(); itTrip != iterSubPopulation->second.end(); ++itTrip)
        {
            Origin* pOrigin = (*itTrip)->GetOrigin();
            Destination* pDestination = (*itTrip)->GetDestination();

            //optimize process for checking every OD
            if(pOrigin == pLastOrigin && pDestination == pLastDestination)
                continue;

            pLastOrigin = pOrigin;
            pLastDestination = pDestination;

			//verify if has predefined paths
			if (PredefinedPaths(pOrigin, pDestination, pSubPopulation, dbEndPeriodTime, shortestPaths, iSimulationInstanceForBatch, hasPredefinedPaths))
			{
				// if no predefined paths, or if predefined paths but we allow to compute new paths starting from the second iteration :
				if (!hasPredefinedPaths || (!m_bUsePredefinedPathsOnly && !isInitialization))
					sortedLstDestinations[pOrigin].insert(pDestination);
			}
			else
			{
				return false;
			}
        }

        //group all origins that have the same list of destinations
        for (map<Origin*, set<Destination*> >::iterator itLstDestination = sortedLstDestinations.begin(); itLstDestination != sortedLstDestinations.end(); ++itLstDestination)
        {
            mapConnectedDestinationOrigin[itLstDestination->second].push_back(itLstDestination->first);
        }

        // compute new shortest paths
        map<Origin *, map<Destination *, vector<ValuetedPath*> > > tempResultsPaths;
        for (map<set<Destination*>, vector<Origin*> >::iterator itConnectedCouples = mapConnectedDestinationOrigin.begin(); itConnectedCouples != mapConnectedDestinationOrigin.end(); itConnectedCouples++)
        {
            mapByOriginDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath> > > bestResultsPaths;
            vector<Destination*> LstDestination(itConnectedCouples->first.begin(), itConnectedCouples->first.end());
            vector<Origin*> lstOrigin = itConnectedCouples->second;

            //compute shortest path for each period of aggregation
            for(size_t i = 0; i< m_listEndPeriods.size(); i++)
			{
                if(isInitialization)
                {
                    tempResultsPaths = m_InitialShortestPathsMethod->getShortestPaths(iSimulationInstanceForBatch, lstOrigin, LstDestination, pSubPopulation, m_listEndPeriods[i], m_bUseCostsForTemporalShortestPath);
                }else{
                    tempResultsPaths = m_ShortestPathsMethod->getShortestPaths(iSimulationInstanceForBatch, lstOrigin, LstDestination, pSubPopulation, m_listEndPeriods[i], m_bUseCostsForTemporalShortestPath);
                }

                //keep only the best Shortest path found for each OD
                //for each origin found
                for(map<Origin *, map<Destination *, vector<ValuetedPath*> > >::iterator itOrigin = tempResultsPaths.begin(); itOrigin != tempResultsPaths.end(); itOrigin++)
                {
                    Origin* pOrigin = itOrigin->first;
                    mapByDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >& bestResultsAtOrigin = bestResultsPaths[pOrigin];
                    map<Destination *, vector<SymuCore::ValuetedPath *> >& tempResultsAtOrigin = itOrigin->second;
                    //for each destination found
                    for (mapByDestination< vector<SymuCore::ValuetedPath *> >::iterator itDestination = tempResultsPaths[pOrigin].begin(); itDestination != tempResultsPaths[pOrigin].end(); itDestination++)
                    {
                        vector<boost::shared_ptr<SymuCore::ValuetedPath>>& bestUpdatedResultsAtDestination = bestResultsAtOrigin[itDestination->first];
                        vector<SymuCore::ValuetedPath *>& tempResultsAtDestination = tempResultsAtOrigin[itDestination->first];
                        vector<boost::shared_ptr<SymuCore::ValuetedPath>> sortedUpdatedResultsAtDestination;
                        for(size_t j = 0; j< tempResultsAtDestination.size(); j++)
                        {
                            UpdatePathCostFromMultipleInstants(tempResultsAtDestination[j], pSubPopulation, iSimulationInstanceForBatch);
                            //sort the shortestPaths found
                            vector<boost::shared_ptr<SymuCore::ValuetedPath>>::iterator it = sortedUpdatedResultsAtDestination.begin();
                            for(; it != sortedUpdatedResultsAtDestination.end(); it++)
                            {
                                if(tempResultsAtDestination[j]->GetCost() <= (*it)->GetCost())
                                {
                                    sortedUpdatedResultsAtDestination.insert(it, boost::shared_ptr<ValuetedPath>(tempResultsAtDestination[j]));
                                    break;
                                }
                            }
                            if(it == sortedUpdatedResultsAtDestination.end())
                            {
                                sortedUpdatedResultsAtDestination.push_back(boost::shared_ptr<ValuetedPath>(tempResultsAtDestination[j]));
                            }
                        }
						if(sortedUpdatedResultsAtDestination.size()>0)
							if(bestUpdatedResultsAtDestination.empty() || bestUpdatedResultsAtDestination[0]->GetCost() > sortedUpdatedResultsAtDestination[0]->GetCost())
							{
								bestUpdatedResultsAtDestination = sortedUpdatedResultsAtDestination;
							}
                    }
                }
            }

            mapByOriginDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >& mapShortestPathBySubPopulation = shortestPaths[pSubPopulation];

            //for each origin found
            for (mapByOriginDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >::iterator itOrigin = bestResultsPaths.begin(); itOrigin != bestResultsPaths.end(); itOrigin++)
            {
                Origin* pOrigin = itOrigin->first;
                mapByDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip *>, CompareValuetedPath> > affectedTripsAtOrigin = affectedTripsAtSubPopulation[pOrigin];
                mapByDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >& mapShortestPathByOrigin = mapShortestPathBySubPopulation[pOrigin];
                //for each destination found
                for (mapByDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >::iterator itDestination = itOrigin->second.begin(); itDestination != itOrigin->second.end(); itDestination++)
                {
                    map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip *>, CompareValuetedPath> mapValuetedPaths = affectedTripsAtOrigin[itDestination->first];
                    vector<boost::shared_ptr<SymuCore::ValuetedPath>>& resultsPath = itDestination->second;
					if(resultsPath.size()>0)
						if(!mapValuetedPaths.empty() && mapValuetedPaths.begin()->first->GetCost() < resultsPath[0]->GetCost())
						{
							resultsPath.pop_back();
							resultsPath.insert(resultsPath.begin(), mapValuetedPaths.begin()->first);
						}
					mapShortestPathByOrigin[itDestination->first] = resultsPath;
                }
            }
        }
        sortedLstDestinations.clear();
        mapConnectedDestinationOrigin.clear();
    }

    CompareExistingValuetedPath(shortestPaths, iSimulationInstanceForBatch);
    return true;
}


bool MSADualLoop::PredefinedPaths(Origin* pOrigin, Destination* pDestination, SubPopulation* pSubPopulation, double dbEndPeriodTime, mapBySubPopulationOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> > & shortestPaths, int iSimulationInstanceForBatch, bool & hasPredefinedPath)
{
    hasPredefinedPath = false;
    mapBySubPopulationOriginDestination< map<Path, vector<double> > >::iterator itSubPopulation = m_mapPredefinedPaths.find(pSubPopulation);
    if(itSubPopulation == m_mapPredefinedPaths.end())
        return true;

    mapByOriginDestination< vector< boost::shared_ptr<SymuCore::ValuetedPath> > >& ShortestPathAtSubPopulation = shortestPaths[pSubPopulation];

    mapByOriginDestination< map<Path, vector<double> > >::iterator itPredefinedValuetedPathByOrigin = itSubPopulation->second.find(pOrigin);
    if(itPredefinedValuetedPathByOrigin != itSubPopulation->second.end())
    {
        mapByDestination< map<Path, vector<double> > >::iterator itPredefinedValuetedPathByDestination = itPredefinedValuetedPathByOrigin->second.find(pDestination);
        if(itPredefinedValuetedPathByDestination != itPredefinedValuetedPathByOrigin->second.end())
        {
            hasPredefinedPath = true;
            vector< boost::shared_ptr<SymuCore::ValuetedPath> >& listShortestPath = ShortestPathAtSubPopulation[pOrigin][pDestination];
            if(!listShortestPath.empty())//already encounter this triplet SubPopulation/origin/destination
                return true;

            boost::shared_ptr<SymuCore::ValuetedPath> pValuetedPath;
            //keep all path sorted by their costs
            map<double, set<boost::shared_ptr<SymuCore::ValuetedPath>, CompareValuetedPath> > mapMinCostPath;

            //update the values for the paths
            map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<SymuCore::Trip*>, CompareValuetedPath >& mapValuetedPath = m_mapAffectedTrips[iSimulationInstanceForBatch][pSubPopulation][pOrigin][pDestination];
            bool firstTime = mapValuetedPath.size() == 0;
            for (map<Path, vector<double> >::iterator itMapPath = itPredefinedValuetedPathByDestination->second.begin(); itMapPath != itPredefinedValuetedPathByDestination->second.end(); itMapPath++)
            {
                //the ratio values for users affectation was already treated before. Here, it's always empty or fill with -1.
                Path path(itMapPath->first);
                pValuetedPath = boost::make_shared<ValuetedPath>(path);
                UpdatePathCostFromMultipleInstants(pValuetedPath.get(), pSubPopulation, iSimulationInstanceForBatch);
                mapMinCostPath[pValuetedPath->GetCost()].insert(pValuetedPath);
                if (firstTime)
                    mapValuetedPath.insert(make_pair(pValuetedPath, vector<Trip*>()));
            }

            //Identify all Shortest Paths
            double ShortestPathCost = mapMinCostPath.begin()->first;
            double CurrentPathCost;
            for (map<double, set<boost::shared_ptr<ValuetedPath>, CompareValuetedPath> >::iterator itMapPath = mapMinCostPath.begin(); itMapPath != mapMinCostPath.end();)
            {
                CurrentPathCost = itMapPath->first;
				if ((ShortestPathCost == 0 && CurrentPathCost == 0) // to avoid division by zero if the shortestpath has a null cost
					|| (ShortestPathCost != 0 && (((CurrentPathCost / ShortestPathCost) - 1.0) <= m_dbKShortestPathsThreshold)))
                {
                    listShortestPath.insert(listShortestPath.end(), itMapPath->second.begin(), itMapPath->second.end());
                    itMapPath = mapMinCostPath.erase(itMapPath);
                    if(!m_bIterationIsKShortestPath || (int)listShortestPath.size() >= *m_KParameters.at(pSubPopulation).at(pOrigin).at(pDestination).getData(dbEndPeriodTime))
                        break;
                }else{
                    break; //no other shortest path
                }
            }
            mapMinCostPath.clear();

            if (listShortestPath.empty())
            {
                BOOST_LOG_TRIVIAL(error) << "None of the predefined paths defined from '" << pOrigin->getStrNodeName() << "' to '" << pDestination->getStrNodeName() << "' are possible !";
                return false;
            }

        }
    }
    return true;
}


//Verify if ValuetedPath pointer in the m_mapShortestPathResults already exist in the m_mapAffectedTrips. If it does, delete it and replace it by the old one
void MSADualLoop::CompareExistingValuetedPath(mapBySubPopulationOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> > & shortestPaths, int iSimulationInstanceForBatch)
{
    // First loop on SubPopulations
    for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator itAffectedTripsSubPopulation = m_mapAffectedTrips[iSimulationInstanceForBatch].begin(); itAffectedTripsSubPopulation != m_mapAffectedTrips[iSimulationInstanceForBatch].end(); ++itAffectedTripsSubPopulation)
    {
        SubPopulation* pSubPopulation = itAffectedTripsSubPopulation->first;
        mapByOriginDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >& mapShortestPathAtSubPopulation = shortestPaths[pSubPopulation];

        // Second loop on Origins
        for (mapByOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator itAffectedTripsOrigin = itAffectedTripsSubPopulation->second.begin(); itAffectedTripsOrigin != itAffectedTripsSubPopulation->second.end(); ++itAffectedTripsOrigin)
        {
            Origin * pOrigin = itAffectedTripsOrigin->first;
            mapByDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >& mapShortestPathAtOrigin = mapShortestPathAtSubPopulation[pOrigin];

            // Assignment loop
            for (mapByDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator itAffectedTripsDestination = itAffectedTripsOrigin->second.begin(); itAffectedTripsDestination != itAffectedTripsOrigin->second.end(); ++itAffectedTripsDestination)
            {
                Destination* pDestination = itAffectedTripsDestination->first;
                vector<boost::shared_ptr<SymuCore::ValuetedPath>>& listPathsFound = mapShortestPathAtOrigin[pDestination];
                map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath = itAffectedTripsDestination->second;

                for (map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator itOld = mapValuetedPath.begin(); itOld != mapValuetedPath.end(); itOld++)
                {
                    for (vector<boost::shared_ptr<SymuCore::ValuetedPath> >::iterator itNew = listPathsFound.begin(); itNew != listPathsFound.end(); itNew++)
                    {
                        if ((*itOld->first == *(*itNew)) && (itOld->first != (*itNew)))
                        {
                            *itNew = itOld->first;
                        }
                    }
                }
            }
        }
    }

    for (mapBySubPopulationOriginDestination<std::vector<boost::shared_ptr<SymuCore::ValuetedPath> > >::iterator itInitialbestSubPopulation = m_lInitialBest.begin(); itInitialbestSubPopulation != m_lInitialBest.end(); ++itInitialbestSubPopulation)
    {
        SubPopulation* pSubPopulation = itInitialbestSubPopulation->first;
        mapByOriginDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >& mapShortestPathAtSubPopulation = shortestPaths[pSubPopulation];

        // Second loop on Origins
        for (mapByOriginDestination<std::vector<boost::shared_ptr<SymuCore::ValuetedPath> > >::iterator itInitialBestOrigin = itInitialbestSubPopulation->second.begin(); itInitialBestOrigin != itInitialbestSubPopulation->second.end(); ++itInitialBestOrigin)
        {
            Origin * pOrigin = itInitialBestOrigin->first;
            mapByDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >& mapShortestPathAtOrigin = mapShortestPathAtSubPopulation[pOrigin];

            // Assignment loop
            for (mapByDestination<std::vector<boost::shared_ptr<SymuCore::ValuetedPath> > >::iterator itInitialBestDestination = itInitialBestOrigin->second.begin(); itInitialBestDestination != itInitialBestOrigin->second.end(); ++itInitialBestDestination)
            {
                Destination* pDestination = itInitialBestDestination->first;

                mapByDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >::iterator iterShortestPaths = mapShortestPathAtOrigin.find(pDestination);
                if (iterShortestPaths != mapShortestPathAtOrigin.end()) // test to prevent adding empty path list to m_mapShortestPathResults (crashes in some cases)
                {
                    vector<boost::shared_ptr<SymuCore::ValuetedPath>>& listPathsFound = iterShortestPaths->second;
                    std::vector<boost::shared_ptr<SymuCore::ValuetedPath> >& lstInitialBest = itInitialBestDestination->second;

                    for (size_t iPath = 0; iPath < lstInitialBest.size(); iPath++)
                    {
                        boost::shared_ptr<ValuetedPath> initialBestPath = lstInitialBest.at(iPath);
                        for (vector<boost::shared_ptr<SymuCore::ValuetedPath> >::iterator itNew = listPathsFound.begin(); itNew != listPathsFound.end(); itNew++)
                        {
                            if ((*initialBestPath == *(*itNew)) && (initialBestPath != (*itNew)))
                            {
                                *itNew = initialBestPath;
                            }
                        }
                    }
                }
            }
        }
    }
}


void MSADualLoop::ClassicMSA(map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath, vector<boost::shared_ptr<SymuCore::ValuetedPath>> &listPathFound, const vector<Trip*>& listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop)
{
    for (map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
    {
        if (find(listPathFound.begin(), listPathFound.end(), iterPath->first) == listPathFound.end()) //not on a shortest path
        {
            size_t NumUser = iterPath->second.size();
            int NumSwap;

            if(iterPath->first->GetCost() == numeric_limits<double>::infinity())//handle path that became impossible
            {
                NumSwap = (int)NumUser;
            }
            else
            {
                NumSwap = (int)floor((NumUser * (1.0 / (StepSize + 1.0))));
            }

            UniformlySwap(NumSwap, mapValuetedPath, iterPath->first, listPathFound, listTripsByOD, bIsBRUE, dbDefaultSigmaConvenience, dbMaxRelativeCostThresholdOuterloop);
        }
    }
}


void MSADualLoop::SmartMSA(map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath, vector<boost::shared_ptr<ValuetedPath>> &listPathFound, vector<Trip*>& listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop)
{
    map<double, vector<Trip*> > mapRanking;
	// Sort trips by cost value
    for (size_t iTrip = 0; iTrip < listTripsByOD.size(); ++iTrip)
    {
        Trip* currentTrip = listTripsByOD[iTrip];
        double dbDepartureTripTime = ((double)(currentTrip->GetDepartureTime() - m_dtStartSimulationTime).total_microseconds()) / 1000000;
        //we always start at the origin and use the departure time of the trip
        assert(!NeedsParallelSimulationInstances()); // rmq. we can use DEFAULT_SIMULATION_INSTANCE here since we dont do parallel simulation
        mapRanking[currentTrip->GetPath(DEFAULT_SIMULATION_INSTANCE).GetPathCost(DEFAULT_SIMULATION_INSTANCE, dbDepartureTripTime, currentTrip->GetSubPopulation(), false).getCostValue()].push_back(currentTrip);
    }


    int NumSwap = (int)floor(listTripsByOD.size() * (1.0 / (StepSize + 1.0)));
	//BOOST_LOG_TRIVIAL(warning) << NumSwap;
	
    map<double, vector<Trip*> >::reverse_iterator itMap = mapRanking.rbegin();
    boost::shared_ptr<ValuetedPath> newShortestPath;
    boost::shared_ptr<ValuetedPath> oldShortestPath;
    double minCostPath;
    int indexInVector = 0;

	for (int i = 0; i < NumSwap && itMap != mapRanking.rend();)
    {
        Trip * pTrip = itMap->second[indexInVector];
        newShortestPath = listPathFound[i % listPathFound.size()];
        minCostPath = listPathFound[0]->GetCost();
        //CPQ - TODO - Peut etre il pourrait être plus interressant de construire une map<Trip, ValuetedPath> specifique à smartMSA pour eviter cette boucle
        for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
        {
            if (pTrip->GetPath(DEFAULT_SIMULATION_INSTANCE) == iterPath->first->GetPath())
            {
                oldShortestPath = iterPath->first;
                break;
            }
        }
        //test if the user can change (only in BRUE mode)
        if(bIsBRUE && (max(0.0 , ((oldShortestPath->GetCost() - minCostPath) / minCostPath)) <= pTrip->GetSigmaConvenience(dbDefaultSigmaConvenience)))
        {
            indexInVector++;
            if(indexInVector == (int)itMap->second.size())
            {
                indexInVector = 0;
                itMap++;
            }
            continue;
        }

		//Note: it is possible that some users are already on the shortest path but they are taken into account in the swapped users 
		// => OK but it misses the case where there is more than one shortest path
		if (!(pTrip->GetPath(DEFAULT_SIMULATION_INSTANCE) == newShortestPath->GetPath()))
		{
			vector<Trip*>& vecTrip = mapValuetedPath[oldShortestPath];
			vecTrip.erase(find(vecTrip.begin(), vecTrip.end(), pTrip));
			pTrip->SetPath(DEFAULT_SIMULATION_INSTANCE, newShortestPath->GetPath());
			mapValuetedPath[newShortestPath].push_back(pTrip);
			indexInVector++;	// Next trip
			if(indexInVector == (int)itMap->second.size())
			{
				indexInVector = 0;
				itMap++;	// Next path
			}
			i++;	// Next swap
		}
		else
		{
			BOOST_LOG_TRIVIAL(warning) << "Pas assez de véhicule à swapper";
			break;

			/*indexInVector++;	// Next trip
			if (indexInVector == (int)itMap->second.size())
			{
				indexInVector = 0;
				itMap++;	// Next path
			}*/
		}
    }
}


void MSADualLoop::GapBasedMSA(map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath, vector<boost::shared_ptr<ValuetedPath>> &listPathFound, vector<Trip*>& listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop)
{
    boost::shared_ptr<ValuetedPath> minusShortestPath = listPathFound[0];//the first is always the minimum shortest path
    double NormValue = 0.0;
    if (m_MSAType == MSA_GapBasedNormalize)
    {
        for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
        {
            if(iterPath->second.empty())
                continue;

            NormValue += iterPath->first->GetCost() - minusShortestPath->GetCost();
        }
    }
    int NumSwap;
    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
    {
        boost::shared_ptr<ValuetedPath> currentValuetedPath = iterPath->first;
        if (find(listPathFound.begin(), listPathFound.end(), currentValuetedPath) != listPathFound.end()) //already on a shortest path
        {
            continue;
        }
        if(currentValuetedPath->GetCost() == numeric_limits<double>::infinity())//handle path that became impossible
        {
            NumSwap = (int)iterPath->second.size();
        }
        else
        {
            if (m_MSAType == MSA_GapBasedNormalize)
            {
                NumSwap = min((int)iterPath->second.size(), (int)floor((iterPath->second.size()) * ((1.0 / (StepSize + 1.0))*((currentValuetedPath->GetCost() - minusShortestPath->GetCost()) / NormValue))));
                //BOOST_LOG_TRIVIAL(warning) << "NumUser: " << (int)iterPath->second.size();
                //BOOST_LOG_TRIVIAL(warning) << "NumSwap: " << NumSwap;
			}
            else
            {
                NumSwap = min((int)iterPath->second.size(), (int)floor((iterPath->second.size()) * ((1.0 / (StepSize + 1.0))*((currentValuetedPath->GetCost() - minusShortestPath->GetCost()) / currentValuetedPath->GetCost()))));
                //BOOST_LOG_TRIVIAL(warning) << "NumUser: " << (int)iterPath->second.size();
                //BOOST_LOG_TRIVIAL(warning) << "NumSwap: " << NumSwap;
                //BOOST_LOG_TRIVIAL(warning) << "1 / K: " << (1.0 / (StepSize + 1.0));
                //BOOST_LOG_TRIVIAL(warning) << "Gap value: " << ((currentValuetedPath->GetCost() - minusShortestPath->GetCost()) / currentValuetedPath->GetCost());
			}
        }
        NumSwap = max(0, (int)(NumSwap));
        if (m_MSAType == MSA_GapBasedProb)
        {
            ProbabilisticSwap(NumSwap, mapValuetedPath, currentValuetedPath, listPathFound, listTripsByOD, bIsBRUE, dbDefaultSigmaConvenience, dbMaxRelativeCostThresholdOuterloop);
        }
        else
        {
            UniformlySwap(NumSwap, mapValuetedPath, currentValuetedPath, listPathFound, listTripsByOD, bIsBRUE, dbDefaultSigmaConvenience, dbMaxRelativeCostThresholdOuterloop);
        }
    }
}

void MSADualLoop::GapBasedLudoMSA(map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath, vector<boost::shared_ptr<ValuetedPath>> &listPathFound, vector<Trip*>& listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop)
{
    boost::shared_ptr<ValuetedPath> minusShortestPath = listPathFound[0];//the first is always the minimum shortest path

    int NumSwap;
    int SumGapNumSwap = 0;
    int SumClassicNumSwap = 0;
    vector<int> listNumSwap;
    vector<boost::shared_ptr<ValuetedPath>> listAvoidPath;
    int indexNbPath = 0;
    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
    {
        boost::shared_ptr<ValuetedPath> currentValuetedPath = iterPath->first;
        if (find(listPathFound.begin(), listPathFound.end(), currentValuetedPath) != listPathFound.end()) //already on a shortest path
        {
            //SumClassicNumSwap += (int)floor(((int)iterPath->second.size() * (1.0 / (StepSize + 1.0))));
            listAvoidPath.push_back(currentValuetedPath);
            continue;
        }
        if(currentValuetedPath->GetCost() == numeric_limits<double>::infinity())//handle path that became impossible
        {
            listAvoidPath.push_back(currentValuetedPath);
            NumSwap = (int)iterPath->second.size();
            listNumSwap.push_back(NumSwap);
            SumGapNumSwap += NumSwap;
            SumClassicNumSwap += NumSwap;
        }
        else
        {
            NumSwap = (int)floor((iterPath->second.size()) * ((1.0 / (StepSize + 1.0))*((currentValuetedPath->GetCost() - minusShortestPath->GetCost()) / currentValuetedPath->GetCost())));
			NumSwap = (max(NumSwap, 1));
			SumGapNumSwap += NumSwap;
			listNumSwap.push_back(NumSwap);
            SumClassicNumSwap += (int)floor(((int)iterPath->second.size() * (1.0 / (StepSize + 1.0))));
		}
	}
	if (SumGapNumSwap == 0)
	{
		/*if (SumClassicNumSwap == 0)
		{
			return;
		}
		else
			SumGapNumSwap = 1;*/
        return;
	}
    double BoostUpValue = SumClassicNumSwap / SumGapNumSwap;
    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
	{
        boost::shared_ptr<ValuetedPath> currentValuetedPath = iterPath->first;
        if (find(listAvoidPath.begin(), listAvoidPath.end(), currentValuetedPath) != listAvoidPath.end()) //Not interrested in this path
            continue;

        NumSwap = min((int)iterPath->second.size(), (int)(listNumSwap[indexNbPath] * BoostUpValue));
        NumSwap = max(0, NumSwap);
        UniformlySwap(NumSwap, mapValuetedPath, currentValuetedPath, listPathFound, listTripsByOD, bIsBRUE, dbDefaultSigmaConvenience, dbMaxRelativeCostThresholdOuterloop);
		indexNbPath++;
    }
}


void MSADualLoop::ProbabilisticMSA(map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath, vector<boost::shared_ptr<ValuetedPath>> &listPathFound, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop, int StepSize)
{
    double SwapProb;
    double DecisionNum;
    int listPathIndex = 0;
    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
    {
        boost::shared_ptr<ValuetedPath> currentShortestPath = listPathFound[listPathIndex % listPathFound.size()];
        boost::shared_ptr<ValuetedPath> oldPath = iterPath->first;
        if (oldPath == currentShortestPath)
        {
            continue;
        }
        double minCostPath = listPathFound[0]->GetCost();
        double dbConvenienceValue;

        // Compute probability of improvement
        if(iterPath->first->GetCost() == numeric_limits<double>::infinity())//handle path that became impossible
        {
            SwapProb = numeric_limits<double>::infinity();
            dbConvenienceValue = numeric_limits<double>::infinity();
        }else{
            SwapProb = (m_MSAType == MSA_StepSizeProb ? (1.0 / StepSize) : 1) * (oldPath->GetCost() - minCostPath) / oldPath->GetCost();
            dbConvenienceValue = max(0.0 , ((oldPath->GetCost() - minCostPath) / minCostPath));
        }

		vector<Trip *>& vecTrip = iterPath->second;
		//BOOST_LOG_TRIVIAL(warning) << "NumUser: " << (int)iterPath->second.size();
        for (vector<Trip *>::iterator itTrip = vecTrip.begin(); itTrip != vecTrip.end();)
        {

            //test if the user can change (only in BRE mode)
            if (bIsBRUE && (dbConvenienceValue <= (*itTrip)->GetSigmaConvenience(dbDefaultSigmaConvenience)))
            {
                continue;
            }

            DecisionNum = ((double)rand() / (double)(RAND_MAX));
            if (SwapProb > DecisionNum)
            {
                assert(!NeedsParallelSimulationInstances()); // rmq. we can use DEFAULT_SIMULATION_INSTANCE here since we dont do parallel simulation
                (*itTrip)->SetPath(DEFAULT_SIMULATION_INSTANCE, currentShortestPath->GetPath());
                mapValuetedPath[currentShortestPath].push_back(*itTrip);
                itTrip = vecTrip.erase(itTrip);

                // go to the next shorest path
                listPathIndex++;
                currentShortestPath = listPathFound[listPathIndex % listPathFound.size()];
                if (oldPath == currentShortestPath)
                {
                    listPathIndex++;
                    currentShortestPath = listPathFound[listPathIndex % listPathFound.size()];

                    assert(oldPath != currentShortestPath); // should not happen !
                }
            }
            else
                itTrip++;
         }
    }
}

void MSADualLoop::WithinDayFPAMSA(map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath, vector<boost::shared_ptr<ValuetedPath>> &listPathFound, vector<Trip*>& listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop)
{
    boost::shared_ptr<ValuetedPath> minusShortestPath = listPathFound[0];//the first is always the minimum shortest path
    vector<boost::shared_ptr<ValuetedPath>> listShortestPath(1, minusShortestPath);
    double MeanTT = 0.0;
    double Alpha = 0.0;
    int NumPath = 0;
    int NumUser = 0;
    int NumSwap;

    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
    {
        MeanTT += iterPath->first->GetCost();
        NumUser += (int)iterPath->second.size();
        NumPath++;
    }
    for (size_t i = 0; i < listPathFound.size(); ++i)
    {
        if (mapValuetedPath.find(listPathFound[i]) == mapValuetedPath.end()) //already on a shortest path
        {
            MeanTT += listPathFound[i]->GetCost();
            NumPath++;
        }
    }


    MeanTT = MeanTT / NumPath;
    if (m_dbAlpha != -1)
    {
        Alpha = m_dbAlpha;
    }
    else
    {
        Alpha = NumUser / 100.0;
    }
    BOOST_LOG_TRIVIAL(warning) << "Alpha: " << Alpha;

    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::const_reverse_iterator iterPath = mapValuetedPath.rbegin(); iterPath != mapValuetedPath.rend(); ++iterPath)
    {
        boost::shared_ptr<ValuetedPath> currentValuetedPath = iterPath->first;

        if (currentValuetedPath == minusShortestPath) //already on a shortest path
        {
            continue;
        }
        if (currentValuetedPath->GetCost() == numeric_limits<double>::infinity())//handle path that became impossible
        {
            NumSwap = (int)iterPath->second.size();
        }
        else
        {
            NumSwap = min((NumUser - (int)iterPath->second.size()), (int)(Alpha * (MeanTT - currentValuetedPath->GetCost())));
        }
        if (NumSwap <= 0)
        {
            NumSwap = abs(NumSwap);
            if (NumSwap >= (int)iterPath->second.size())
            {
                BOOST_LOG_TRIVIAL(error) << "The path will be empty for next iteration with current size: " << (int)iterPath->second.size();
                if (NumSwap > NumUser)
                {
                    BOOST_LOG_TRIVIAL(error) << "Please revised the Alpha value! Because the number of swap is more than demand of OD";
                    BOOST_LOG_TRIVIAL(error) << "NumSwap: " << NumSwap;
                    BOOST_LOG_TRIVIAL(error) << "NumUser: " << NumUser;
                }
            }
            NumSwap = min((int)iterPath->second.size(), NumSwap);

            UniformlySwap(NumSwap, mapValuetedPath, currentValuetedPath, listShortestPath, listTripsByOD, bIsBRUE, dbDefaultSigmaConvenience, dbMaxRelativeCostThresholdOuterloop);
        }
        else
        {
            vector<boost::shared_ptr<ValuetedPath>> listCurrenttPath(1, currentValuetedPath);
            UniformlySwap(NumSwap, mapValuetedPath, minusShortestPath, listCurrenttPath, listTripsByOD, bIsBRUE, dbDefaultSigmaConvenience, dbMaxRelativeCostThresholdOuterloop);
        }
    }
}

void MSADualLoop::InitialBasedWDFPAMSA(map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath, vector<boost::shared_ptr<ValuetedPath>> &listPathFound, vector<Trip*>& listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop)
{
    boost::shared_ptr<ValuetedPath> minusShortestPath = listPathFound[0];//the first is always the minimum shortest path
    vector<boost::shared_ptr<ValuetedPath>> listShortestPath(1, minusShortestPath);
    double MeanTT = 0.0;
    double Alpha = 0.0;
    int NumPath = 0;
    int NumUser = 0;
    int NumSwap;
    double q = 0.5;
    double Beta = pow((1.0 / (StepSize + 1.0)), q);

    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
    {
        MeanTT += iterPath->first->GetCost();
        NumUser += (int)iterPath->second.size();
        NumPath++;
    }
    for (size_t i = 0; i < listPathFound.size(); ++i)
    {
        if (mapValuetedPath.find(listPathFound[i]) == mapValuetedPath.end()) //already on a shortest path
        {
            MeanTT += listPathFound[i]->GetCost();
            NumPath++;
        }
    }


    MeanTT = MeanTT / NumPath;
    Alpha = NumUser / 100.0;

    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::const_reverse_iterator iterPath = mapValuetedPath.rbegin(); iterPath != mapValuetedPath.rend(); ++iterPath)
    {
        boost::shared_ptr<ValuetedPath> currentValuetedPath = iterPath->first;

        if (currentValuetedPath == minusShortestPath) //already on a shortest path
        {
            continue;
        }
        if (currentValuetedPath->GetCost() == numeric_limits<double>::infinity())//handle path that became impossible
        {
            NumSwap = (int)iterPath->second.size();
        }
        else
        {
            NumSwap = (int)(((Beta * m_mapIntialAssignment[currentValuetedPath]) + ((1.0 - Beta)*((int)iterPath->second.size() + min((NumUser - (int)iterPath->second.size()), (int)(Alpha * (MeanTT - currentValuetedPath->GetCost())))))) - (int)iterPath->second.size());
        }
        if (NumSwap <= 0)
        {
            NumSwap = abs(NumSwap);
            if (NumSwap >= (int)iterPath->second.size())
            {
                BOOST_LOG_TRIVIAL(error) << "The path will be empty for next iteration" << (int)iterPath->second.size();
                if (NumSwap > NumUser)
                {
                    BOOST_LOG_TRIVIAL(error) << "Please revised the Alpha value!";
                }
            }
            NumSwap = min((int)iterPath->second.size(), NumSwap);

            UniformlySwap(NumSwap, mapValuetedPath, currentValuetedPath, listShortestPath, listTripsByOD, bIsBRUE, dbDefaultSigmaConvenience, dbMaxRelativeCostThresholdOuterloop);
        }
        else
        {
            vector<boost::shared_ptr<ValuetedPath>> listCurrenttPath(1, currentValuetedPath);
            UniformlySwap(NumSwap, mapValuetedPath, minusShortestPath, listCurrenttPath, listTripsByOD, bIsBRUE, dbDefaultSigmaConvenience, dbMaxRelativeCostThresholdOuterloop);
        }
    }
}

void MSADualLoop::InitialBasedSimpleMSA(map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath, vector<boost::shared_ptr<ValuetedPath>> &listPathFound, vector<Trip*>& listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop)
{
    boost::shared_ptr<ValuetedPath> minusShortestPath = listPathFound[0];//the first is always the minimum shortest path
    vector<boost::shared_ptr<ValuetedPath>> listShortestPath(1, minusShortestPath);
    double MeanTT = 0.0;
    double Alpha = 0.0;
    int NumPath = 0;
    int NumUser = 0;
    int NumSwap;
    double q = 0.5;
    double Beta = pow((1.0 / (StepSize + 1.0)), q);

    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
    {
        MeanTT += iterPath->first->GetCost();
        NumUser += (int)iterPath->second.size();
        NumPath++;
    }
    for (size_t i = 0; i < listPathFound.size(); ++i)
    {
        if (mapValuetedPath.find(listPathFound[i]) == mapValuetedPath.end()) //already on a shortest path
        {
            MeanTT += listPathFound[i]->GetCost();
            NumPath++;
        }
    }


    MeanTT = MeanTT / NumPath;
    Alpha = NumUser / 100.0;

    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::const_reverse_iterator iterPath = mapValuetedPath.rbegin(); iterPath != mapValuetedPath.rend(); ++iterPath)
    {
        boost::shared_ptr<ValuetedPath> currentValuetedPath = iterPath->first;

        if (currentValuetedPath == minusShortestPath) //already on a shortest path
        {
            continue;
        }
        if (currentValuetedPath->GetCost() == numeric_limits<double>::infinity())//handle path that became impossible
        {
            NumSwap = (int)iterPath->second.size();
        }
        else
        {
            NumSwap = (int)(((Beta * m_mapIntialAssignment[currentValuetedPath]) + ((1.0 - Beta)*((int)iterPath->second.size()*(1.0 - (1.0 / (StepSize + 1.0)))))) - (int)iterPath->second.size());
        }
        if (NumSwap <= 0)
        {
            NumSwap = abs(NumSwap);
            if (NumSwap >= (int)iterPath->second.size())
            {
                BOOST_LOG_TRIVIAL(error) << "The path will be empty for next iteration" << (int)iterPath->second.size();
                if (NumSwap > NumUser)
                {
                    BOOST_LOG_TRIVIAL(error) << "Please revised the Alpha value!";
                }
            }
            NumSwap = min((int)iterPath->second.size(), NumSwap);

            UniformlySwap(NumSwap, mapValuetedPath, currentValuetedPath, listShortestPath, listTripsByOD, bIsBRUE, dbDefaultSigmaConvenience, dbMaxRelativeCostThresholdOuterloop);
        }
        else
        {
            vector<boost::shared_ptr<ValuetedPath>> listCurrenttPath(1, currentValuetedPath);
            UniformlySwap(NumSwap, mapValuetedPath, minusShortestPath, listCurrenttPath, listTripsByOD, bIsBRUE, dbDefaultSigmaConvenience, dbMaxRelativeCostThresholdOuterloop);
        }
    }
}

void MSADualLoop::LogitMSA(map<boost::shared_ptr<ValuetedPath>, vector<Trip *>, MSADualLoop::CompareValuetedPath> &mapValuetedPath, vector<boost::shared_ptr<ValuetedPath>> &listPathFound, vector<Trip *> &listTripsByOD, double dbTetaLogit, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop)
{
    vector<boost::shared_ptr<ValuetedPath>> alreadyAssignedPaths;
    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
    {
        alreadyAssignedPaths.push_back(iterPath->first);
    }

    map<boost::shared_ptr<ValuetedPath>, double> mapCoeffs = GetCoeffsFromLogit(alreadyAssignedPaths, 1.0, dbTetaLogit);

    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
    {
        if (find(listPathFound.begin(), listPathFound.end(), iterPath->first) == listPathFound.end()) //not on a shortest path
        {
            size_t NumUser = iterPath->second.size();

            int NumSwap;
            if(iterPath->first->GetCost() == numeric_limits<double>::infinity())//handle path that became impossible
            {
                NumSwap = (int)iterPath->second.size();
            }else{
                NumSwap = (int)floor(((NumUser * (1.0 / (StepSize + 1.0)))) * mapCoeffs[iterPath->first]);
				//BOOST_LOG_TRIVIAL(warning) << "NumUsers: " << iterPath->second.size();
				//BOOST_LOG_TRIVIAL(warning) << "1 / k: " << 1.0 / (m_iValueOfK + 1.0);
				//BOOST_LOG_TRIVIAL(warning) << "Logit: " << mapCoeffs[iterPath->first];
				//BOOST_LOG_TRIVIAL(warning) << "NumSwap: " << NumSwap;
            }

            UniformlySwap(NumSwap, mapValuetedPath, iterPath->first, listPathFound, listTripsByOD, bIsBRUE, dbDefaultSigmaConvenience, dbMaxRelativeCostThresholdOuterloop);
        }
    }
}

void MSADualLoop::AffectBestAssignment(const vector<Trip *> &listSortedTrip, int iSimulationInstanceForBatch)
{
    for(size_t i=0; i<min(listSortedTrip.size(), m_listPreviousAffectationPaths.size()); i++)
    {
        listSortedTrip[i]->SetPath(iSimulationInstanceForBatch, m_listPreviousAffectationPaths[i]);
    }

    for (mapBySubPopulationOriginDestination< vector<Trip*> >::iterator iterSubPopulation = m_mapTripsByOD.begin(); iterSubPopulation != m_mapTripsByOD.end(); ++iterSubPopulation)
    {
        bool isSubPopPredefined = false;
        mapBySubPopulationOriginDestination< map<Path, vector<double> > >::iterator itPredefinedBySubPopulation = m_mapPredefinedPaths.find(iterSubPopulation->first);
        if(itPredefinedBySubPopulation != m_mapPredefinedPaths.end()) //test if we have a predefined path for this SubPopulation
        {
            isSubPopPredefined = true;
        }
        mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& TripsByPathAtSubPopulation = m_mapAffectedTrips[iSimulationInstanceForBatch][iterSubPopulation->first];
        for (mapByOriginDestination< vector<Trip*> >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
        {
            bool isOriginPredefined = false;
            mapByOriginDestination< map<Path, vector<double> > >::iterator itPredefinedByOrigin;
            if(isSubPopPredefined)
            {
                itPredefinedByOrigin = itPredefinedBySubPopulation->second.find(iterOrigin->first);
                if(itPredefinedByOrigin != itPredefinedBySubPopulation->second.end()) //test if we have a predefined path for this Origin
                {
                    isOriginPredefined = true;
                }
            }
            mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& TripsByPathAtOrigin = TripsByPathAtSubPopulation[iterOrigin->first];
            for (mapByDestination< vector<Trip*> >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                map<Path, vector<double> > predefinedPaths;
                if(isOriginPredefined)
                {
                    mapByDestination< map<Path, vector<double> > >::iterator itPredefinedByDestination = itPredefinedByOrigin->second.find(iterDestination->first);
                    if(itPredefinedByDestination != itPredefinedByOrigin->second.end()) //test if we have a predefined path for this Destination
                    {
                        predefinedPaths = itPredefinedByDestination->second;
                    }
                }

                map< boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath = TripsByPathAtOrigin[iterDestination->first];
                vector<Trip*>& listTripByOD = iterDestination->second;
                mapValuetedPath.clear();

                map<Path, vector<Trip*> > mapPath;
                for(size_t i = 0; i<listTripByOD.size(); i++)
                {
                    Trip* pTrip = listTripByOD[i];
                    mapPath[pTrip->GetPath(iSimulationInstanceForBatch)].push_back(pTrip);
                }

                //add predefined paths with empty users
                for(map<Path, vector<double> >::iterator itPath = predefinedPaths.begin(); itPath != predefinedPaths.end(); itPath++)
                {
                    if(mapPath.find(itPath->first) == mapPath.end())
                    {
                        boost::shared_ptr<ValuetedPath> newValuetedPath = GetExistingValuetedPathFromBestAssignment(iterSubPopulation->first, iterOrigin->first, iterDestination->first, itPath->first);
                        if (!newValuetedPath)
                        {
                            newValuetedPath = boost::make_shared<ValuetedPath>(itPath->first);
                        }
                        mapValuetedPath.insert(make_pair(newValuetedPath, vector<Trip*>()));
                    }
                }

                for(map<Path, vector<Trip*> >::iterator itPath = mapPath.begin(); itPath != mapPath.end(); itPath++)
                {
                    boost::shared_ptr<ValuetedPath> newValuetedPath = GetExistingValuetedPathFromBestAssignment(iterSubPopulation->first, iterOrigin->first, iterDestination->first, itPath->first);
                    if (!newValuetedPath)
                    {
                        newValuetedPath = boost::make_shared<ValuetedPath>(itPath->first);
                    }
                    mapValuetedPath[newValuetedPath] = itPath->second;
                }
            }
        }
    }

    // add empty paths from previous best assignment (if not, the computed gap will be different redoing the best assignment)
    for (std::map<SymuCore::SubPopulation*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, std::vector<SymuCore::Path> > > >::const_iterator iterSubpopulation = m_listPreviousAffectationUnassignedPaths.begin(); iterSubpopulation != m_listPreviousAffectationUnassignedPaths.end(); ++iterSubpopulation)
    {
        for (std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, std::vector<SymuCore::Path> > >::const_iterator iterOrigin = iterSubpopulation->second.begin(); iterOrigin != iterSubpopulation->second.end(); ++iterOrigin)
        {
            for (std::map<SymuCore::Destination*, std::vector<SymuCore::Path> >::const_iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                const std::vector<SymuCore::Path> & emptyPaths = iterDestination->second;
                map< boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath = m_mapAffectedTrips[iSimulationInstanceForBatch][iterSubpopulation->first][iterOrigin->first][iterDestination->first];
                
                for (size_t iEmptyPath = 0; iEmptyPath < emptyPaths.size(); iEmptyPath++)
                {
                    const Path & emptyPath = emptyPaths.at(iEmptyPath);
                    bool bIsPathAlreadyHere = false;
                    for (map< boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterMapValuetedPath = mapValuetedPath.begin(); iterMapValuetedPath != mapValuetedPath.end(); ++iterMapValuetedPath)
                    {
                        if (iterMapValuetedPath->first->GetPath() == emptyPath)
                        {
                            bIsPathAlreadyHere = true;
                            break;
                        }
                    }
                    if (!bIsPathAlreadyHere)
                    {
                        boost::shared_ptr<ValuetedPath> newValuetedPath = GetExistingValuetedPathFromBestAssignment(iterSubpopulation->first, iterOrigin->first, iterDestination->first, emptyPath);
                        if (!newValuetedPath)
                        {
                            newValuetedPath = boost::make_shared<ValuetedPath>(emptyPath);
                        }
                        mapValuetedPath.insert(make_pair(newValuetedPath, vector<Trip*>()));
                    }
                }
            }
        }
    }
}

// Method used to avoid an issue where we get different ValuetedPath objects for the same path when using m_lInitialBest (produces crashes)
boost::shared_ptr<ValuetedPath> MSADualLoop::GetExistingValuetedPathFromBestAssignment(SubPopulation * pSubPop, Origin * pOrigin, Destination * pDestination, const Path & path)
{
    mapBySubPopulationOriginDestination<std::vector<boost::shared_ptr<SymuCore::ValuetedPath> > >::const_iterator iterSubPop = m_lInitialBest.find(pSubPop);
    if (iterSubPop != m_lInitialBest.end())
    {
        mapByOriginDestination<std::vector<boost::shared_ptr<SymuCore::ValuetedPath> > >::const_iterator iterOrigin = iterSubPop->second.find(pOrigin);
        if (iterOrigin != iterSubPop->second.end())
        {
            mapByDestination<std::vector<boost::shared_ptr<SymuCore::ValuetedPath> > >::const_iterator iterDestination = iterOrigin->second.find(pDestination);
            if (iterDestination != iterOrigin->second.end())
            {
                for (size_t iPath = 0; iPath < iterDestination->second.size(); iPath++)
                {
                    if (iterDestination->second[iPath]->GetPath() == path)
                    {
                        return iterDestination->second[iPath];
                    }
                }
            }
        }
    }
    return boost::shared_ptr<ValuetedPath>();
}


void MSADualLoop::UpdatePathCostAndShortestPath(mapBySubPopulationOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> > & shortestPaths, int iSimulationInstanceForBatch)
{
    shortestPaths.clear();

    //update all the values for mapTripsByPath
    for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterSubPopulation = m_mapAffectedTrips[iSimulationInstanceForBatch].begin(); iterSubPopulation != m_mapAffectedTrips[iSimulationInstanceForBatch].end(); ++iterSubPopulation)
    {
        SubPopulation* pSubPopulation = iterSubPopulation->first;
        mapByOriginDestination< vector<boost::shared_ptr<ValuetedPath> > >& ShortestPathAtSubPopulation = shortestPaths[pSubPopulation];

        for (mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
        {
            Origin* pOrigin = iterOrigin->first;
            mapByDestination< vector<boost::shared_ptr<ValuetedPath> > >& ShortestPathAtOrigin = ShortestPathAtSubPopulation[pOrigin];

            for (mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                Destination* pDestination = iterDestination->first;
                map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > newMapValuetedPath;
                map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& oldMapValuetedPath = iterDestination->second;
                vector<boost::shared_ptr<ValuetedPath>>& listShortestPath = ShortestPathAtOrigin[pDestination];

                boost::shared_ptr<ValuetedPath> pValuetedPath;
                vector<Trip*> currentTrips;
                for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = oldMapValuetedPath.begin(); iterPath != oldMapValuetedPath.end();)
                {

                    pValuetedPath = iterPath->first;
                    currentTrips = iterPath->second;
                    iterPath = oldMapValuetedPath.erase(iterPath);

                    UpdatePathCostFromMultipleInstants(pValuetedPath.get(), pSubPopulation, iSimulationInstanceForBatch);
                    newMapValuetedPath.insert(make_pair(pValuetedPath, currentTrips));

                }
                //replace data
                oldMapValuetedPath = newMapValuetedPath;

                //Identify the Shortest Path
				if(newMapValuetedPath.size()>0)
					listShortestPath.push_back(newMapValuetedPath.begin()->first);
            }
        }
    }
}

void MSADualLoop::UpdatePathCostFromMultipleInstants(ValuetedPath* pValuetedPath, SubPopulation *pSubPopulation, int iSimulationInstanceForBatch)
{
    //compute the path cost for different time trough the path and get the mean
    double costValueSum = 0.0;
    double timeValueSum = 0.0;

    double dbEmptyPathCost = pValuetedPath->GetPath().GetPathCost(iSimulationInstanceForBatch, 0.0, pSubPopulation, true).getCostValue();

    const ParametersForSubpopulation & paramsForSubpop = m_mapParametersBySubpopulation.at(pSubPopulation);

    for (size_t iAggregate = 0; iAggregate < m_iUpdateCostNumber; iAggregate++)
    {
        Cost currentCost = pValuetedPath->GetPath().GetPathCost(iSimulationInstanceForBatch, m_listEndPeriods[iAggregate], pSubPopulation, true);

        //treshold for the cost of the path
        if( (paramsForSubpop.eOptimizerType != Optimizer_SO ) && (paramsForSubpop.eOptimizerType != Optimizer_SSO))
        {
            double dbTreshold = max(m_listEndPeriods[iAggregate], dbEmptyPathCost);
            currentCost.setUsedCostValue(min(currentCost.getCostValue(), dbTreshold));
        }

        costValueSum += currentCost.getCostValue();
        timeValueSum += currentCost.getTravelTime();
    }
    pValuetedPath->setCost(costValueSum / m_iUpdateCostNumber);
    pValuetedPath->setTime(timeValueSum / m_iUpdateCostNumber);

}


bool MSADualLoop::IsInnerLoopIterationNeeded(mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& prevMapTripsByPath)
{
    bool isConvergence = false;
    // Convergence test switch
    BOOST_LOG_TRIVIAL(debug) << "Here in InnerLoop Gap is " << m_dbActualGlobalGap << " for innerLoop " << m_iInnerloopIndex << " and outerLoop " << m_iOuterloopIndex;
    PrintOutputGapIteration(m_dbActualGlobalGap, true);

    switch(m_InnerloopTestType)
    {
        case CostConvergence: // Convergence test based on the cost
        {
            isConvergence = CostBasedConvergenceTest(false);
            break;
        }
        case SwapConvergence: // Convergence test based on the swap
        {
            isConvergence = SwapBasedConvergenceTest(prevMapTripsByPath);
            break;
        }
        case GapConvergence: // Convergence test based on the gap
        {
            isConvergence = true;
            for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::const_iterator iter = prevMapTripsByPath.begin();
                iter != prevMapTripsByPath.end();
                ++iter)
            {
                double dbPreviousGap = m_mapPreviousGap.at(iter->first);
                if (dbPreviousGap != 0)
                {
                    if ((abs(m_mapActualGlobalGap.at(iter->first) - dbPreviousGap) / dbPreviousGap) >= m_dbInnerloopGapThreshold)
                    {
                        isConvergence = false;
                        break;
                    }
                }
            }
            m_mapPreviousGap = m_mapActualGlobalGap;
            break;
        }//end switch
     }

	if (isConvergence)
		BOOST_LOG_TRIVIAL(debug) << "Convergence found for innerLoop " << m_iInnerloopIndex;

    return !isConvergence;
}

bool MSADualLoop::IsOuterLoopIterationNeeded(int iCurrentIterationIndex, int iTotalNbUsers, bool bHasNewShortestPathDiscovered)
{
    if (iTotalNbUsers == 0) // No users left
        return false;

    bool isConvergence = false;
    std::map<SymuCore::SubPopulation*, double> gapBySubPopulation;
	double dbGlobalGap = ComputeGlobalGap(gapBySubPopulation, m_mapOriginalShortestPathResults, DEFAULT_SIMULATION_INSTANCE);

    BOOST_LOG_TRIVIAL(debug) << "Here in OuterLoop Gap is " << dbGlobalGap << " for innerLoop " << m_iInnerloopIndex << " and outerLoop " << m_iOuterloopIndex;
    PrintOutputGapIteration(dbGlobalGap, false);

    if(m_bIsGapBasedConvergenceOuterloop)
    {
        m_mapPreviousGap = gapBySubPopulation; // for InnerLoop test
    }

    if (!bHasNewShortestPathDiscovered)
    {
		BOOST_LOG_TRIVIAL(debug) << "No new shortest path";
		if(m_bIsGapBasedConvergenceOuterloop)
        {
            if (m_bUseRelativeGapThreshold && iCurrentIterationIndex != 0) // can't use relative gap for the first iteration since we have infinite values which lead to indeterminate values in the convergence test below
            {
                isConvergence = true;
                for (std::map<SubPopulation*, double>::const_iterator iterGap = m_mapPreviousOuterLoopGap.begin(); iterGap != m_mapPreviousOuterLoopGap.end(); ++iterGap)
                {
                    double dbPrevGap = m_mapPreviousOuterLoopGap.at(iterGap->first);
                    if (dbPrevGap != 0)
                    {
                        if ((abs(iterGap->second - dbPrevGap) / dbPrevGap) >= m_dbOuterloopRelativeGapThreshold)
                        {
                            isConvergence = false;
                            break;
                        }
                    }
                }
            }
            else
            {
                isConvergence = true;
                // we converge only if the gap for every subpopulation is inferior to the threshold specified for the subpopulation
                for (std::map<SubPopulation*, double>::const_iterator iterGapBySubpop = gapBySubPopulation.begin(); iterGapBySubpop != gapBySubPopulation.end(); ++iterGapBySubpop)
                {
                    if (iterGapBySubpop->second > m_mapParametersBySubpopulation.at(iterGapBySubpop->first).dbMaxGapThresholdOuterloop)
                    {
                        isConvergence = false;
                        break;
                    }
                }
            }
                
        }else{
            // Check a variante of gap function as quality of solution
            isConvergence = CostBasedConvergenceTest(true);
        }
    }

    m_mapPreviousOuterLoopGap = gapBySubPopulation;
    m_dbPreviousOuterLoopGap = dbGlobalGap;

	if (isConvergence)
		BOOST_LOG_TRIVIAL(debug) << "Convergence found for OuterLoop " << m_iOuterloopIndex;

    return !isConvergence;
}


bool MSADualLoop::SwapBasedConvergenceTest(mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& prevMapTripsByPath)
{
    double dbMaxODViolation = (m_dbInnerloopRelativeODViolation < 0 ? m_dbOuterloopRelativeODViolation : m_dbInnerloopRelativeODViolation);
    int nbUserSwap = 0;
    int nbOfOD = 0;
    int NbODSignificantSwap = 0;
    int totalNbUsersByOD = 0;

    //Calcul number of violations
    
    assert(!NeedsParallelSimulationInstances()); // rmk. : using DEFAULT_SIMULATION_INSTANCE since we only come here for MSADualLoop

    for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterSubPopulation = m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].begin(); iterSubPopulation != m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].end(); ++iterSubPopulation)
    {
        SubPopulation* pSubPopulation = iterSubPopulation->first;
        mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& prevTripsByPathAtSubPopulation = prevMapTripsByPath[pSubPopulation];
        for (mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
        {
            Origin* pOrigin = iterOrigin->first;
            mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& prevTripsByPathAtOrigin = prevTripsByPathAtSubPopulation[pOrigin];
            for (mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                Destination* pDestination = iterDestination->first;
                nbUserSwap = 0;
                totalNbUsersByOD = 0;
                map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& prevTripsByPathAtDestination = prevTripsByPathAtOrigin[pDestination];
                for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = iterDestination->second.begin(); iterPath != iterDestination->second.end(); ++iterPath)
                {
                    nbUserSwap += abs((int)prevTripsByPathAtDestination[iterPath->first].size() - (int)iterPath->second.size());
                    totalNbUsersByOD += (int)iterPath->second.size();
                }
                nbOfOD++;

                if ((nbUserSwap/(double)totalNbUsersByOD) >= m_dbRelativeSwappingThreshold)
                {
                    NbODSignificantSwap++;
                }
            }
        }
    }
    return !((NbODSignificantSwap / (double)nbOfOD) >= dbMaxODViolation);
}


//This function also remove no longer used shortest paths
bool MSADualLoop::HasNewShortestPathDiscovered()
{
    // Check if new shortest paths are found
    bool hasOneNewPath = false;

    //rmk. using DEFAULT_SIMULATION_INSTANCE since we are in outerloop convergence test here, so the parallel simulation instances for heuristic dual loop innerloop have been resynced here and all m_mapAffectedTrips should be the same.
    for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterSubPopulation = m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].begin(); iterSubPopulation != m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].end(); ++iterSubPopulation)
    {
        SubPopulation* pSubPopulation = iterSubPopulation->first;
        mapByOriginDestination< vector<boost::shared_ptr<ValuetedPath>> >& shortestPathAtSubPopulation = m_mapOriginalShortestPathResults[pSubPopulation];
        for (mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
        {
            Origin* pOrigin = iterOrigin->first;
            mapByDestination< vector<boost::shared_ptr<ValuetedPath>> >& shortestPathAtOrigin = shortestPathAtSubPopulation[pOrigin];
            for (mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                Destination* pDestination = iterDestination->first;
                vector<boost::shared_ptr<ValuetedPath> >& listPathFound = shortestPathAtOrigin[pDestination];
                vector<bool> isAlreadyFound(listPathFound.size(), false);

                //don't check for new path if we use predefined paths only and have predefined path for this Subpop + OD
                bool bCheckForNewPaths = true;
				// Pour test de découverte de nouveaux plus courts chemins malgré les chemins prédéfinis
				// No need if we want t compute new shortests paths
				if (m_bUsePredefinedPathsOnly)
				{
					mapBySubPopulationOriginDestination< map<Path, vector<double> > >::iterator itPredefinedBySubPopulation = m_mapPredefinedPaths.find(pSubPopulation);
					if (itPredefinedBySubPopulation != m_mapPredefinedPaths.end()) //test if we have a predefined path for this SubPopulation
					{
						mapByOriginDestination< map<Path, vector<double> > >::iterator itPredefinedByOrigin = itPredefinedBySubPopulation->second.find(pOrigin);
						if (itPredefinedByOrigin != itPredefinedBySubPopulation->second.end()) //test if we have a predefined path for this Origin
						{
							mapByDestination< map<Path, vector<double> > >::iterator itPredefinedByDestination = itPredefinedByOrigin->second.find(pDestination);
							if (itPredefinedByDestination != itPredefinedByOrigin->second.end()) //test if we have a predefined path for this Destination
							{
								if (itPredefinedByDestination->second.size() > 0)
								{
									bCheckForNewPaths = false;		// CSQ: If there is no predefined path, paths are calculated even if m_bUsePredefinedPathsOnly is true
								}
							}
						}
					}
				}

				if (bCheckForNewPaths)
                {
                    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = iterDestination->second.begin(); iterPath != iterDestination->second.end();)
                    {
                        bool isShortestPath = false;
                        for(size_t i = 0; i < listPathFound.size(); i++)
                        {
                            if (iterPath->first == listPathFound[i])
                            {
                                isAlreadyFound[i] = true;
                                isShortestPath = true;
                                break;
                            }
                        }

                        //If we swap all users of a path, remove it (no longer a shortest path) /!\ only if not a predefined path
                        if (iterPath->second.empty() && !isShortestPath && !m_bChangeInitialization)
                        {
                            boost::shared_ptr<ValuetedPath> pValuetedPath = iterPath->first;
                            iterPath = iterDestination->second.erase(iterPath);
                        }else{
                            iterPath++;
                        }

                    }

                    for(size_t i = 0; i < isAlreadyFound.size(); i++)
                    {
                        if (!isAlreadyFound[i])
                        {
                            hasOneNewPath = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    return hasOneNewPath;
}


bool MSADualLoop::CostBasedConvergenceTest(bool isOuterLoop)
{
    double dbMaxODViolation;
    double dbUserCostThreshold;
    double dbUserByODThreshold;

    if(isOuterLoop)
    {
        dbMaxODViolation = m_dbOuterloopRelativeODViolation;
        dbUserByODThreshold = m_dbOuterloopRelativeUserViolation;
    }else{
        dbMaxODViolation = (m_dbInnerloopRelativeODViolation < 0 ? m_dbOuterloopRelativeODViolation : m_dbInnerloopRelativeODViolation);
        dbUserByODThreshold = (m_dbInnerloopRelativeUserViolation < 0 ? m_dbOuterloopRelativeUserViolation : m_dbInnerloopRelativeUserViolation);
    }

    int nbODViolation = 0;
    int nbOfOD = 0;
    int nbViolation = 0;
    int nbOfUsers = 0;

    double shortestPathTravelCost;

    // rmk. using DEFAULT_SIMULATION_INSTANCE here since all instances have been resynced at the end of innerloop anyway.
    for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterSubPopulation = m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].begin(); iterSubPopulation != m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].end(); ++iterSubPopulation)
    {
        const ParametersForSubpopulation & paramsForSubpop = m_mapParametersBySubpopulation.at(iterSubPopulation->first);
        dbUserCostThreshold = isOuterLoop ? paramsForSubpop.dbMaxRelativeCostThresholdOuterloop : paramsForSubpop.dbMaxRelativeCostThresholdInnerloop;
        for (mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
        {
            for (mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                nbViolation = 0;
                nbOfUsers = 0;
                //the valueted paths in the last maps are ordered by costs
                map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPaths = iterDestination->second;
                for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPaths.begin(); iterPath != mapValuetedPaths.end(); ++iterPath)
                {
                    shortestPathTravelCost = mapValuetedPaths.begin()->first->GetCost();

                    for (vector<Trip*>::iterator iterTrip = iterPath->second.begin(); iterTrip != iterPath->second.end(); ++iterTrip)
                    {
                        Trip* currentTrip = (*iterTrip);
                        double shortestPathTravelCostModified = shortestPathTravelCost;

                        if (paramsForSubpop.eOptimizerType == Optimizer_BRUE)
                        {
                            shortestPathTravelCostModified += (shortestPathTravelCost * currentTrip->GetSigmaConvenience(paramsForSubpop.dbSigmaConvenience));
                        }

                        if (max(0.0 , ((iterPath->first->GetCost() - shortestPathTravelCostModified) / shortestPathTravelCostModified)) > dbUserCostThreshold)
                        {
                            nbViolation++;
                        }
                    }

                    nbOfUsers += (int)iterPath->second.size();
                }
                if((nbViolation / (double)nbOfUsers) >= dbUserByODThreshold)
                {
                    nbODViolation++;
                }
                nbOfOD++;
            }
        }
    }
    return !((nbODViolation / (double)nbOfOD) >= dbMaxODViolation);
}

void MSADualLoop::KeepBestAssignment(const vector<Trip*> &listTrip, bool IsInnerLoopConverged)
{
    assert(!NeedsParallelSimulationInstances()); // to be sure we can use DEFAULT_SIMULATION_INSTANCE here.

    m_dbActualGlobalGap = ComputeGlobalGap(m_mapActualGlobalGap, m_mapNewShortestPathResults[DEFAULT_SIMULATION_INSTANCE], DEFAULT_SIMULATION_INSTANCE);

    if (m_dbActualGlobalGap < m_dbBestQuality)
    {
        m_dbBestQuality = m_dbActualGlobalGap;
        SaveCurrentAssignment(listTrip, DEFAULT_SIMULATION_INSTANCE);
    }
    else if ((m_dbActualGlobalGap >= m_dbBestQuality) && (m_iInnerloopIndex == m_iMaxInnerloopIteration - 1 || IsInnerLoopConverged))
    {
        m_bChangeAssignment = true;
        BOOST_LOG_TRIVIAL(info) << "Skip assignment to simulate traffic";
		BOOST_LOG_TRIVIAL(info) << m_dbActualGlobalGap << " " << m_dbBestQuality << " " << IsInnerLoopConverged;
    }
}

double MSADualLoop::ComputeGlobalGap(std::map<SymuCore::SubPopulation*, double> & gapBySubPopulation, mapBySubPopulationOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> > & shortestPaths, int iSimulationInstanceForBatch)
{
    gapBySubPopulation.clear();

    double gap = 0.0;
    int nbUsers = 0;

    for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterSubPopulation = m_mapAffectedTrips[iSimulationInstanceForBatch].begin(); iterSubPopulation != m_mapAffectedTrips[iSimulationInstanceForBatch].end(); ++iterSubPopulation)
    {
        int nbUsersForSubPop = 0;
        double & gapForSubPop = gapBySubPopulation[iterSubPopulation->first];
        gapForSubPop = 0;
        mapByOriginDestination< vector< boost::shared_ptr<ValuetedPath> > >& ShortestPathAtSubPopulation = shortestPaths[iterSubPopulation->first];
        const ParametersForSubpopulation & subpopulationParams = m_mapParametersBySubpopulation.at(iterSubPopulation->first);
        for (mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
        {
            mapByDestination< vector< boost::shared_ptr<ValuetedPath> > >& ShortestPathAtOrigin = ShortestPathAtSubPopulation[iterOrigin->first];
            for (mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                int nbUsersForOD = 0;
                double ODGap = 0.0;
                vector< boost::shared_ptr<ValuetedPath> >& listShortestPath = ShortestPathAtOrigin[iterDestination->first];
                map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPaths = iterDestination->second;

				if (listShortestPath.size() == 0)
					continue;

                boost::shared_ptr<ValuetedPath> shortestPath = listShortestPath[0];
                //ValuetedPath* shortestPath = mapValuetedPaths.begin()->first;

                for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPaths.begin(); iterPath != mapValuetedPaths.end(); ++iterPath)
                {
                    nbUsersForOD += (int)iterPath->second.size();
                    if (subpopulationParams.eOptimizerType == Optimizer_BRUE)
                    {
                        for(size_t i=0; i<iterPath->second.size(); i++)
                        {
                            Trip* currentTrip = iterPath->second[i];
                            double shortestPathCost = shortestPath->GetCost() + (shortestPath->GetCost() * currentTrip->GetSigmaConvenience(subpopulationParams.dbSigmaConvenience));

                            ODGap += max(0.0 , (iterPath->first->GetCost() - shortestPathCost));
                        }
                    }else{

                        // Protection against infinite path costs with 0 users which leads to an undefined gap value
                        if (!iterPath->second.empty())
                        {
                            ODGap += ((int)iterPath->second.size()) * (iterPath->first->GetCost() - shortestPath->GetCost());
                        }
                    }
                }
                nbUsers += nbUsersForOD;
                nbUsersForSubPop += nbUsersForOD;
                gap += ODGap;
                gapForSubPop += ODGap;
                if (subpopulationParams.bStepSizeAffectation)
				{
					// Mostafa: change value of OD Gap
                    double dbODGap = ODGap;
                    if (nbUsersForOD > 0)
                    {
                        dbODGap /= nbUsersForOD;
                    }
                    m_mapGapByOD[iterSubPopulation->first][iterOrigin->first][iterDestination->first] = dbODGap;
				}
            }
        }

        if (nbUsersForSubPop > 0)
        {
            gapForSubPop /= nbUsersForSubPop;
        }
    }

    if (nbUsers > 0)
    {
        gap /= nbUsers;
    }

    //BOOST_LOG_TRIVIAL(warning) << "Gap: " << gap;
	return gap;
}

void MSADualLoop::GroupCloseShortestPath(int iSimulationInstanceForBatch)
{

    // First loop on SubPopulations
    for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator itAffectedTripsSubPopulation = m_mapAffectedTrips[iSimulationInstanceForBatch].begin(); itAffectedTripsSubPopulation != m_mapAffectedTrips[iSimulationInstanceForBatch].end(); ++itAffectedTripsSubPopulation)
    {
        SubPopulation* pSubPopulation = itAffectedTripsSubPopulation->first;
        mapByOriginDestination< vector<boost::shared_ptr<ValuetedPath>> >& mapShortestPathAtSubPopulation = m_mapOriginalShortestPathResults[pSubPopulation];

        // Second loop on Origins
        for (mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator itAffectedTripsOrigin = itAffectedTripsSubPopulation->second.begin(); itAffectedTripsOrigin != itAffectedTripsSubPopulation->second.end(); ++itAffectedTripsOrigin)
        {
            Origin * pOrigin = itAffectedTripsOrigin->first;
            mapByDestination< vector<boost::shared_ptr<ValuetedPath>> >& mapShortestPathAtOrigin = mapShortestPathAtSubPopulation[pOrigin];

            // Assignment loop
            for (mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator itAffectedTripsDestination = itAffectedTripsOrigin->second.begin(); itAffectedTripsDestination != itAffectedTripsOrigin->second.end(); ++itAffectedTripsDestination)
            {
                Destination* pDestination = itAffectedTripsDestination->first;
                vector<boost::shared_ptr<ValuetedPath>>& listPathsFound = mapShortestPathAtOrigin[pDestination];

				if (listPathsFound.size() > 0)
				{
					double ShortestPathCost = listPathsFound[0]->GetCost();
                    map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath = itAffectedTripsDestination->second;

					//Identify all Shortest Paths
                    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator itCurrentValuetedPath = mapValuetedPath.begin(); itCurrentValuetedPath != mapValuetedPath.end(); itCurrentValuetedPath++)
					{
						bool isAlreadyShortestPath = false;
                        for (vector<boost::shared_ptr<ValuetedPath>>::iterator itShortestPath = listPathsFound.begin(); itShortestPath != listPathsFound.end(); itShortestPath++)
						{
							if ((*itShortestPath) == itCurrentValuetedPath->first)
							{
								isAlreadyShortestPath = true;
								break;
							}
						}
						if (isAlreadyShortestPath)
							continue;

						if (((double)(itCurrentValuetedPath->first->GetCost() / ShortestPathCost) - 1.0) <= m_dbShortestPathsThreshold)
						{
							listPathsFound.push_back(itCurrentValuetedPath->first);
							//BOOST_LOG_TRIVIAL(warning) << "Shortest path is grouped by other for origin " << pOrigin->getStrNodeName() << " and Destination " << pDestination->getStrNodeName();
						}
						else {
							break; //no other shortest path
						}
					}
				}
            }
        }
    }


}


void MSADualLoop::FillFromPreviousAffectationRatio(mapBySubPopulationOriginDestination< vector<Trip*> > &partialMapTripsByOD, vector<Trip *> &listNotAffectedUsers, double dbEndPeriodTime, int iSimulationInstanceForBatch)
{
    // First loop on SubPopulations
    for (mapBySubPopulationOriginDestination< multimap<double, boost::shared_ptr<ValuetedPath>> >::iterator iterSubPopulation = m_mapPreviousAffectationRatio.begin(); iterSubPopulation != m_mapPreviousAffectationRatio.end(); ++iterSubPopulation)
    {
        SubPopulation * pSubPopulation = iterSubPopulation->first;
        mapBySubPopulationOriginDestination< vector<Trip*> >::iterator itTripsByODAtSubPopulation = partialMapTripsByOD.find(pSubPopulation);
        if(itTripsByODAtSubPopulation == partialMapTripsByOD.end()) //The SubPopulation exist no more for the current iteration
            continue;

        mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& TripsByPathAtSubPopulation = m_mapAffectedTrips[iSimulationInstanceForBatch][pSubPopulation];
        mapByOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath> > > & shortestPathsForSubPop = m_mapOriginalShortestPathResults[pSubPopulation];

        // Second loop on Origins
        for (mapByOriginDestination< multimap<double, boost::shared_ptr<ValuetedPath>> >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
        {
            Origin * pOrigin = iterOrigin->first;
            mapByOriginDestination< vector<Trip*> >::iterator itTripsByODAtOrigin = itTripsByODAtSubPopulation->second.find(pOrigin);
            if(itTripsByODAtOrigin == itTripsByODAtSubPopulation->second.end()) //The Origin exist no more for the current iteration
                continue;

            mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& TripsByPathAtOrigin = TripsByPathAtSubPopulation[pOrigin];
            mapByDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath> > > & shortestPathsForOrigin = shortestPathsForSubPop[pOrigin];

            // Third loop on destination
            for (mapByDestination< multimap<double, boost::shared_ptr<ValuetedPath>> >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                Destination * pDestination = iterDestination->first;
                mapByDestination< vector<Trip*> >::iterator itTripsByODAtDestination = itTripsByODAtOrigin->second.find(pDestination);
                if(itTripsByODAtDestination == itTripsByODAtOrigin->second.end()) //The Destination doesn't exist anymore for the current iteration
                    continue;

                map< boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath = TripsByPathAtOrigin[pDestination];
                std::vector<boost::shared_ptr<SymuCore::ValuetedPath> > & shortestPathsForDestination = shortestPathsForOrigin[pDestination];

                multimap<double, boost::shared_ptr<ValuetedPath>>& currentMapRatio = iterDestination->second; // Previous ratios to apply

                //Verify if the path is possible for this iteration
                double notAffectedRatio = 0;
                for (multimap<double, boost::shared_ptr<ValuetedPath>>::iterator iterPath = currentMapRatio.begin(); iterPath != currentMapRatio.end();)
                {
                    boost::shared_ptr<ValuetedPath> pValuetedPath = iterPath->second;
                    UpdatePathCostFromMultipleInstants(pValuetedPath.get(), pSubPopulation, iSimulationInstanceForBatch);
                    if(pValuetedPath->GetCost() == numeric_limits<double>::infinity())
                    {
                        notAffectedRatio += iterPath->first;
                        iterPath = currentMapRatio.erase(iterPath);
                        continue;
                    }
                    iterPath++;
                }

                std::set<boost::shared_ptr<ValuetedPath>, CompareValuetedPath> shortestPaths;
                assert(shortestPathsForDestination.empty());

                //attach user based on precedent iteration ratio
                int currentNbOfUsers;
                double currentRatioValue;
                int nbUsersEnconters = 0;
                int indexIteration = 1;
                boost::shared_ptr<ValuetedPath> pathFound;
                int totalNumberOfUsers = (int)itTripsByODAtDestination->second.size();

				//BOOST_LOG_TRIVIAL(warning) << pSubPopulation->GetStrName() << " " << pOrigin->getStrNodeName() << " " << pDestination->getStrNodeName() << " " << totalNumberOfUsers << " " << notAffectedRatio;

                for (multimap<double, boost::shared_ptr<ValuetedPath>>::iterator iterPath = currentMapRatio.begin(); iterPath != currentMapRatio.end(); ++iterPath)
                {
                    currentRatioValue = (iterPath->first * (1-notAffectedRatio) );
                    pathFound = iterPath->second;
                    if(indexIteration == (int)currentMapRatio.size()) // Last iteration
                    {
                        currentNbOfUsers = totalNumberOfUsers - nbUsersEnconters;
                    }else{
                        currentNbOfUsers = (int)(totalNumberOfUsers * currentRatioValue);
                    }

					//BOOST_LOG_TRIVIAL(warning) << iterPath->first << " " << currentRatioValue << " " << currentNbOfUsers;

                    shortestPaths.insert(pathFound);
                    vector<Trip*>& listTrip = mapValuetedPath[pathFound];
                    //for (int i = nbUsersEnconters; i < max(totalNumberOfUsers, currentNbOfUsers + nbUsersEnconters); i++)
					for (int i = nbUsersEnconters; i < min(totalNumberOfUsers, currentNbOfUsers + nbUsersEnconters); i++)
                    {
                        Trip * pTrip = itTripsByODAtDestination->second[i];
                        pTrip->SetPath(iSimulationInstanceForBatch, pathFound->GetPath());
                        listTrip.push_back(pTrip);
                    }
                    nbUsersEnconters += currentNbOfUsers;
                    indexIteration++;
                }

                if (!shortestPaths.empty())
                {
                    shortestPathsForDestination.push_back(*shortestPaths.begin());
                }

                //Those trips are affected
                itTripsByODAtOrigin->second.erase(itTripsByODAtDestination);
            }

            if(itTripsByODAtOrigin->second.empty())
                itTripsByODAtSubPopulation->second.erase(itTripsByODAtOrigin);
        }

        if(itTripsByODAtSubPopulation->second.empty())
            partialMapTripsByOD.erase(itTripsByODAtSubPopulation);
    }

    //Compute the list of users that are still not affected
    for (mapBySubPopulationOriginDestination< vector<Trip*> >::iterator iterSubPopulation = partialMapTripsByOD.begin(); iterSubPopulation != partialMapTripsByOD.end(); ++iterSubPopulation)
    {
        for (mapByOriginDestination< vector<Trip*> >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
        {
            for (mapByDestination< vector<Trip*> >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                listNotAffectedUsers.insert(listNotAffectedUsers.end(), iterDestination->second.begin(), iterDestination->second.end());
				//BOOST_LOG_TRIVIAL(warning) << "Not affected trip " << iterSubPopulation->first->GetPopulation()->GetStrName() << " " << iterOrigin->first->getStrNodeName() << " " << iterDestination->first->getStrNodeName();
            }
        }
    }
}

bool MSADualLoop::InitialAffectation(mapBySubPopulationOriginDestination<vector<Trip *> > &mapTripsByOD, double dbArrivalTime, int iSimulationInstanceForBatch)
{
    // First loop on SubPopulations
    for (mapBySubPopulationOriginDestination< vector<Trip*> >::iterator iterSubPopulation = mapTripsByOD.begin(); iterSubPopulation != mapTripsByOD.end(); ++iterSubPopulation)
    {
        SubPopulation * pSubPopulation = iterSubPopulation->first;
        mapBySubPopulationOriginDestination< vector<boost::shared_ptr<ValuetedPath>> >::iterator itPathBySubPopulation = m_mapOriginalShortestPathResults.find(pSubPopulation);
        if (itPathBySubPopulation == m_mapOriginalShortestPathResults.end()) //Make sur we have a path for this SubPopulation
        {
            BOOST_LOG_TRIVIAL(warning) << "No Paths available with SubPopulation " << pSubPopulation->GetStrName();
            continue;
        }
        mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& TripsByPathAtSubPopulation = m_mapAffectedTrips[iSimulationInstanceForBatch][pSubPopulation];

        const ParametersForSubpopulation & subpopulationParams = m_mapParametersBySubpopulation.at(iterSubPopulation->first);

        // Second loop on Origins
        for (mapByOriginDestination< vector<Trip*> >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
        {
            Origin * pOrigin = iterOrigin->first;
            mapByOriginDestination< vector<boost::shared_ptr<ValuetedPath>> >::iterator itPathByOrigin = itPathBySubPopulation->second.find(pOrigin);
            if(itPathByOrigin == itPathBySubPopulation->second.end()) //Make sur we have a path for this Origin
            {
                BOOST_LOG_TRIVIAL(warning) << "No Paths available from " << pOrigin->getStrNodeName() << " with SubPopulation " << pSubPopulation->GetStrName();
                continue;
            }
            mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& TripsByPathAtOrigin = TripsByPathAtSubPopulation[pOrigin];

            // Third loop on destination
            for (mapByDestination< vector<Trip*> >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                Destination * pDestination = iterDestination->first;
                mapByDestination< vector<boost::shared_ptr<ValuetedPath>> >::iterator itPathByDestination = itPathByOrigin->second.find(pDestination);
                if(itPathByDestination == itPathByOrigin->second.end()) //Make sur we have a path for this Destination
                {
                    BOOST_LOG_TRIVIAL(warning) << "No Paths available from " << pOrigin->getStrNodeName() << " to " << pDestination->getStrNodeName() << " with SubPopulation " << pSubPopulation->GetStrName();
                    continue;
                }
                map< boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath = TripsByPathAtOrigin[pDestination];

                const vector<boost::shared_ptr<ValuetedPath>> & listPathFound = itPathByDestination->second;

                if (m_bInitialization)
                {
                    m_lInitialBest[pSubPopulation][pOrigin][pDestination] = listPathFound;
                }
                    
                int numberOfUsers = (int)iterDestination->second.size();
				//BOOST_LOG_TRIVIAL(warning) << "InitialAffectation " << pSubPopulation->GetStrName() << " " << pOrigin->getStrNodeName() << " " << pDestination->getStrNodeName() << " " << numberOfUsers;
                vector<Trip*>& lstAllUserByOD = iterDestination->second;

                if(m_bDayToDayAffectation || !m_bUsePredefinedPathsOnly)
                {
                    for(size_t i=0; i<listPathFound.size(); i++)
                    {
                        mapValuetedPath[listPathFound[i]] = vector<Trip*>();
                    }

                    vector<Trip*>::iterator itUser;
                    Trip* currentTrip;
                    bool pathFound;
                    for(itUser=lstAllUserByOD.begin(); itUser!=lstAllUserByOD.end();itUser++)
                    {
                        currentTrip = (*itUser);
                        if(currentTrip->isPredefinedPath())
                        {
                            pathFound = false;
                            map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath>::iterator itMapValueted;
                            for(itMapValueted= mapValuetedPath.begin(); itMapValueted != mapValuetedPath.end(); itMapValueted++)
                            {
                                if (itMapValueted->first->GetPath() == currentTrip->GetPath(iSimulationInstanceForBatch))
                                {
                                    itMapValueted->second.push_back(currentTrip);
                                    pathFound = true;
                                    break;
                                }
                            }

                            if(!pathFound)
                            {
                                boost::shared_ptr<ValuetedPath> pValuetedPath = boost::make_shared<ValuetedPath>(currentTrip->GetPath(iSimulationInstanceForBatch));
                                UpdatePathCostFromMultipleInstants(pValuetedPath.get(), pSubPopulation, iSimulationInstanceForBatch);
                                mapValuetedPath[pValuetedPath].push_back(currentTrip);
                            }

                            itUser = lstAllUserByOD.erase(itUser);
                        }else
                            itUser++;
                    }
                }

                if (listPathFound.size() > 1 && subpopulationParams.bInitialIsLogitAffectation)
                {
                    //attach user on each new trips (uniformly based on coeffs found with logit)
                    map<boost::shared_ptr<ValuetedPath>, double> mapCoeffsNotSorted = GetCoeffsFromLogit(listPathFound, 1.0, *m_LogitParameters[pSubPopulation][pOrigin][pDestination].getData(dbArrivalTime));
                    map<boost::shared_ptr<ValuetedPath>, double, CompareValuetedPath> mapCoeffs;
                    for (map<boost::shared_ptr<ValuetedPath>, double, CompareValuetedPath>::iterator itSortMap = mapCoeffsNotSorted.begin(); itSortMap != mapCoeffsNotSorted.end(); itSortMap++)
                    {
                        mapCoeffs.insert(make_pair(itSortMap->first, itSortMap->second));
                    }
					mapCoeffsNotSorted.clear();

                    int numberOfUsersAssigned = 0;
                    //put the value of the map to the exact number of user, not only the coeff
                    map<boost::shared_ptr<ValuetedPath>, double, CompareValuetedPath>::iterator itMapCoeffs;
                    for(itMapCoeffs = mapCoeffs.begin(); itMapCoeffs != mapCoeffs.end(); itMapCoeffs++)
                    {
                        int CurrentUsersAssigned = (int)(itMapCoeffs->second * numberOfUsers);
                        itMapCoeffs->second = CurrentUsersAssigned;
                        numberOfUsersAssigned += CurrentUsersAssigned;
                    }
                    //assign all the users left
					itMapCoeffs = mapCoeffs.begin();
                    itMapCoeffs->second += numberOfUsers - numberOfUsersAssigned;

                    boost::shared_ptr<ValuetedPath> pathFound;
                    int nbIteration = 0;                            //just to be sure to never have an infinite loop (this should never happen
                    int nbIterationMax = (int)mapCoeffs.size() * 2; //because the sum of values in mapCoeffs is equal to the total number of users)
                    map<boost::shared_ptr<ValuetedPath>, double, CompareValuetedPath>::iterator itCoeff = mapCoeffs.begin();
                    for (size_t i = 0; i < numberOfUsers; i++)
                    {
                        bool stop = false;
                        while(!stop) // find one path to assign
                        {
                            if(itCoeff == mapCoeffs.end())
                                itCoeff = mapCoeffs.begin();

                            if(itCoeff->second > 0)
                            {
                                itCoeff->second--;
                                nbIteration = 0;
                                pathFound = itCoeff->first;
                                stop = true;
                            }else{
                                nbIteration++;
                                if(nbIteration == nbIterationMax)
                                {
                                    BOOST_LOG_TRIVIAL(error) << "Couldn't assign all the trips from " << pOrigin->getStrNodeName() << " to " << pDestination->getStrNodeName() << " with SubPopulation " << pSubPopulation->GetStrName();
                                    return false;
                                }
                            }
                            itCoeff++;
                        }
                        Trip * pTrip = lstAllUserByOD[i];
                        // All or nothing assignment for first itr.
                        pTrip->SetPath(iSimulationInstanceForBatch, pathFound->GetPath());
                        // Assign trips to path for first itr.
                        mapValuetedPath[pathFound].push_back(pTrip);
                    }

                }else{
                    //attach user on each new trips (all-or-nothing or uniformly if more than 1 path)
                    int nbPathFound = (int)listPathFound.size();
                    boost::shared_ptr<ValuetedPath> pathFound;

					if (nbPathFound>0)
					{
						for (size_t i = 0; i < numberOfUsers; i++)
						{
							pathFound = listPathFound[i % nbPathFound];

							if (lstAllUserByOD.size() > i)
							{
								Trip * pTrip = lstAllUserByOD[i];
								// All or nothing assignment for first itr.
                                pTrip->SetPath(iSimulationInstanceForBatch, pathFound->GetPath());
								// Assign trips to path for first itr.
								mapValuetedPath[pathFound].push_back(pTrip);
							}
						}
					}
                }

                //erase valuetedPath with no users on it if not in predefined path
                if(m_mapPredefinedPaths.empty())
                {
                    map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath>::iterator itMapValueted;
                    for(itMapValueted=mapValuetedPath.begin(); itMapValueted != mapValuetedPath.end();)
                    {
                        if(itMapValueted->second.empty())
                            itMapValueted = mapValuetedPath.erase(itMapValueted);
                        else
                            itMapValueted++;
                    }
                }

                // if nothing, don't put it in the map
                if (mapValuetedPath.empty())
                {
                    TripsByPathAtOrigin.erase(pDestination);
                }
            }
        }
    }

    m_dbBestQuality = numeric_limits<double>::infinity();

    return true;
}

void MSADualLoop::Initialization(int nbParallelSimulationMaxNumber)
{
    for (int iSimulationInstance = 0; iSimulationInstance < nbParallelSimulationMaxNumber; iSimulationInstance++)
    {
        for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterSubPopulation = m_mapAffectedTrips[iSimulationInstance].begin(); iterSubPopulation != m_mapAffectedTrips[iSimulationInstance].end(); ++iterSubPopulation)
        {
            for (mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
            {
                for (mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
                {
                    if (m_mapParametersBySubpopulation.at(iterSubPopulation->first).bIsUniformInitialization)
                    {
                        // Uniform initialization

                        map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath = iterDestination->second;

                        //MOSTAFA_Modification
                        // 1 - All trips are assigned on first path				
                        boost::shared_ptr<ValuetedPath> InitialBestPathInd = mapValuetedPath.begin()->first;
                        //ValuetedPath* InitialBestPathInd = m_lInitialBest[listPathIndex];

                        //listPathIndex++;
                        //NumUser = mapValuetedPath.begin()->second.size();

                        for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
                        {
                            //MOSTAFA_Modification
                            if (InitialBestPathInd == iterPath->first)
                                continue;

                            //NumUser = NumUser + iterPath->second.size();
                            vector<Trip *>& vecTrip = iterPath->second;
                            for (vector<Trip *>::iterator itTrip = vecTrip.begin(); itTrip != vecTrip.end();)
                            {
                                (*itTrip)->SetPath(iSimulationInstance, InitialBestPathInd->GetPath());
                                mapValuetedPath[InitialBestPathInd].push_back(*itTrip);
                                itTrip = vecTrip.erase(itTrip);
                            }

                        }

                        // 2 - Sort trips by departure time
                        std::sort(mapValuetedPath[InitialBestPathInd].begin(), mapValuetedPath[InitialBestPathInd].end(), SymuCore::Trip::DepartureTimePtrSorter);

                        // 2 - Paths are uniformly distributed
                        //NumPath = mapValuetedPath.size();
                        //NumUserPerPath = (int)(NumUser / NumPath);

                        map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator PathIt = mapValuetedPath.begin();
                        vector<Trip *>& vecTrips = mapValuetedPath[InitialBestPathInd];

                        for (vector<Trip *>::iterator itTrip = vecTrips.begin(); itTrip != vecTrips.end();)
                        {
                            if (PathIt == mapValuetedPath.end())
                            {
                                PathIt = mapValuetedPath.begin();
                            }

                            if (PathIt->first == InitialBestPathInd)
                            {
                                ++itTrip;
                            }
                            else
                            {
                                (*itTrip)->SetPath(iSimulationInstance, PathIt->first->GetPath());
                                mapValuetedPath[PathIt->first].push_back(*itTrip);

                                itTrip = vecTrips.erase(itTrip);
                            }
                            ++PathIt;
                        }
                        //MOSTAFA_Modification
                    }
                    else
                    {
                        // random all or nothing initialization
                        int listPathIndex = 0;
                        const std::vector<boost::shared_ptr<ValuetedPath>> & initialBestPaths = m_lInitialBest[iterSubPopulation->first][iterOrigin->first][iterDestination->first];

                        if (!initialBestPaths.empty())
                        {
                            map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath = iterDestination->second;
                            for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
                            {
                                boost::shared_ptr<ValuetedPath> IntialBestPath = initialBestPaths[listPathIndex % initialBestPaths.size()];
                                if (IntialBestPath->GetPath() == iterPath->first->GetPath())
                                    continue;
                                vector<Trip *>& vecTrip = iterPath->second;
                                if (!vecTrip.empty())
                                {
                                    listPathIndex++;
                                }
                                for (vector<Trip *>::iterator itTrip = vecTrip.begin(); itTrip != vecTrip.end();)
                                {
                                    (*itTrip)->SetPath(iSimulationInstance, IntialBestPath->GetPath());
                                    mapValuetedPath[IntialBestPath].push_back(*itTrip);
                                    itTrip = vecTrip.erase(itTrip);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

map<boost::shared_ptr<ValuetedPath>, double> MSADualLoop::GetCoeffsFromLogit(const vector<boost::shared_ptr<ValuetedPath> > & actualPaths, double dbRemainingCoeff, double actualTetaLogit)
{
    map<boost::shared_ptr<ValuetedPath>, double> mapResultsCoeffs;
    double dbSumCostLogit = 0.0;
    for(size_t i = 0; i < actualPaths.size(); i++) {
        dbSumCostLogit += exp( - actualTetaLogit * actualPaths[i]->GetCost() );
    }

    // We compute the correct coefficent for the path
    boost::shared_ptr<ValuetedPath> currentPath;
    for(size_t iPath = 0; iPath < actualPaths.size(); iPath++) {
        currentPath = actualPaths[iPath];
        double dbCoeff = dbRemainingCoeff * (exp( - actualTetaLogit * currentPath->GetCost() ) / dbSumCostLogit) ;
        mapResultsCoeffs[currentPath] = dbCoeff;
    }

    return mapResultsCoeffs;
}

void MSADualLoop::ProbabilisticSwap(int nbSwap, std::map<boost::shared_ptr<ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &userDistribution, boost::shared_ptr<ValuetedPath> actualPath, std::vector<boost::shared_ptr<ValuetedPath> > &listPathFound, const std::vector<SymuCore::Trip*>& listUsersOD, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop)
{
    double SwapProb;
    double DecisionNum;

    int listPathIndex = 0;
    boost::shared_ptr<ValuetedPath> currentShortestPath = listPathFound[listPathIndex % listPathFound.size()];
    boost::shared_ptr<ValuetedPath> oldPath = actualPath;
    if (oldPath == currentShortestPath)
    {
        return;
    }
    double minCostPath = listPathFound[0]->GetCost();
    double dbConvenienceValue;

    // Compute probability of improvement
    if (actualPath->GetCost() == numeric_limits<double>::infinity())//handle path that became impossible
    {
        SwapProb = numeric_limits<double>::infinity();
        dbConvenienceValue = numeric_limits<double>::infinity();
    }
    else{
        SwapProb = (oldPath->GetCost() - minCostPath) / oldPath->GetCost();
        dbConvenienceValue = max(0.0, ((oldPath->GetCost() - minCostPath) / minCostPath));
    }

    vector<Trip *>& vecTrip = userDistribution.at(actualPath);
    int iSwapDone = 0;
    //BOOST_LOG_TRIVIAL(warning) << "NumUser: " << (int)iterPath->second.size();
    for (vector<Trip *>::iterator itTrip = vecTrip.begin(); itTrip != vecTrip.end();)
    {

        //test if the user can change (only in BRE mode)
        if (bIsBRUE && (dbConvenienceValue <= (*itTrip)->GetSigmaConvenience(dbDefaultSigmaConvenience)))
        {
            continue;
        }

        DecisionNum = ((double)rand() / (double)(RAND_MAX));
        if (SwapProb > DecisionNum)
        {
            assert(!NeedsParallelSimulationInstances()); // so we can use DEFAULT_SIMULATION_INSTANCE
            (*itTrip)->SetPath(DEFAULT_SIMULATION_INSTANCE, currentShortestPath->GetPath());
            userDistribution[currentShortestPath].push_back(*itTrip);
            itTrip = vecTrip.erase(itTrip);

            iSwapDone++;
            if (iSwapDone >= nbSwap)
            {
                return;
            }

            // go to the next shorest path
            listPathIndex++;
            currentShortestPath = listPathFound[listPathIndex % listPathFound.size()];
            if (oldPath == currentShortestPath)
            {
                listPathIndex++;
                currentShortestPath = listPathFound[listPathIndex % listPathFound.size()];

                assert(oldPath != currentShortestPath); // should not happen !
            }
        }
        else
            itTrip++;
    }
}


void MSADualLoop::UniformlySwap(int nbSwap, map<boost::shared_ptr<ValuetedPath>, vector<Trip *>, CompareValuetedPath> &userDistribution, boost::shared_ptr<ValuetedPath> actualPath, vector<boost::shared_ptr<ValuetedPath> > &listPathFound, const vector<Trip *> &listUsersOD, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop)
{
    vector<Trip *>& listAffectedUsersActualPath = userDistribution.at(actualPath);

    assert(nbSwap <= listAffectedUsersActualPath.size()); //the nbSwap should always be equal or less than the number of users on the path

    vector<Trip *> listAffectedUsersNextPath;
    bool notEnoughSwap = false;
    for(size_t i = 0; i < listPathFound.size(); i++)
    {
        if(!notEnoughSwap && (int)(i) >= nbSwap){
            notEnoughSwap = true;
            double minShortestPathCost = listPathFound[0]->GetCost();
            if (bIsBRUE) //find the minimum BR and add it to the shortestpath cost
            {
                double minSigma = numeric_limits<double>::infinity();
                for(size_t j=0; j < userDistribution[actualPath].size(); j++)
                {
                    double currentSigma = userDistribution[actualPath][j]->GetSigmaConvenience(dbDefaultSigmaConvenience);
                    if(currentSigma < minSigma)
                        minSigma = currentSigma;
                }
                minShortestPathCost += minSigma;
            }

            if((actualPath->GetCost() - minShortestPathCost) / minShortestPathCost > dbMaxRelativeCostThresholdOuterloop){ // check if the actual path is close to the minimum shortest path
                if (!userDistribution[actualPath].empty())
                {
                    Trip* pTrip = userDistribution[actualPath][0];
                    if (nbSwap == 0)
                        BOOST_LOG_TRIVIAL(warning) << "You can't swap enough user for the path from " << pTrip->GetOrigin()->getStrNodeName() << " to " << pTrip->GetDestination()->getStrNodeName() << " !!";

                    continue;//rmk: for now, we don't handle when there is too few user to swap. It's only treated if there is too few user to swap in all paths
                }
            }else{
                break;
            }
        }
        vector<Trip *>& listTempUsers = userDistribution[listPathFound[i]];
        listAffectedUsersNextPath.insert(listAffectedUsersNextPath.end(), listTempUsers.begin(), listTempUsers.end());
    }

    if (nbSwap == 0)
        return;

    map<vector<Trip*>::const_iterator, double> valuesMap;

    int bestMinSpace = (int)((listUsersOD.size() - listAffectedUsersNextPath.size()) / nbSwap);
    vector<Trip*>::const_iterator UpdateIt = listUsersOD.begin();
    double minCostPath = userDistribution.begin()->first->GetCost();

    //add a penalty for the first user on the trip
    assert(!NeedsParallelSimulationInstances()); // so we can use DEFAULT_SIMULATION_INSTANCE
    if ((*UpdateIt)->GetPath(DEFAULT_SIMULATION_INSTANCE) == actualPath->GetPath())
    {
        valuesMap[UpdateIt] = floor(listAffectedUsersNextPath.size() * listAffectedUsersNextPath.size()) + 1.0;
    }

    //will give a value to identify if the trip is close to a nextPath trip
    for(vector<Trip*>::const_iterator itTrip = listUsersOD.begin(); itTrip != listUsersOD.end(); itTrip++)
    {
        bool isShortestPath = false;
        for(size_t i = 0; i< listPathFound.size(); i++)
        {
            if ((*itTrip)->GetPath(DEFAULT_SIMULATION_INSTANCE) == listPathFound[i]->GetPath())
            {
                isShortestPath = true;
                break;
            }
        }

        if(isShortestPath)
        {
            UpdateIt = itTrip;
            for(int i = 0; i < bestMinSpace; i++)
            {
                if(UpdateIt == listUsersOD.begin())
                {
                    break;
                }

                UpdateIt--;
                if ((*UpdateIt)->GetPath(DEFAULT_SIMULATION_INSTANCE) == actualPath->GetPath())
                    valuesMap[UpdateIt] += ((bestMinSpace - i) * (bestMinSpace - i));
            }

            UpdateIt = itTrip;
            for(int i = 0; i < bestMinSpace; i++)
            {
                UpdateIt++;
                if(UpdateIt == listUsersOD.end())
                {
                    break;
                }

                if ((*UpdateIt)->GetPath(DEFAULT_SIMULATION_INSTANCE) == actualPath->GetPath())
                    valuesMap[UpdateIt] += ((bestMinSpace - i) * (bestMinSpace - i));
            }

        }


        if ((*itTrip)->GetPath(DEFAULT_SIMULATION_INSTANCE) == actualPath->GetPath())
        {

            //test if the user can change (only in BRE mode)
            if(bIsBRUE && (max(0.0 , ((actualPath->GetCost() - minCostPath) / minCostPath)) <= (*itTrip)->GetSigmaConvenience(dbDefaultSigmaConvenience)))
            {
                valuesMap[itTrip] = numeric_limits<double>::infinity();
            }else{
                valuesMap[itTrip]; // create int = 0 if not exist
            }

        }
    }

    vector<Trip*> TripToSwap;
    double minValue = numeric_limits<double>::infinity();
    map<vector<Trip*>::const_iterator, double>::iterator minIt = valuesMap.end();
    for (int i = 0; i < nbSwap-1; i++)
    {
        //find the actual minimum value
        for (map<vector<Trip*>::const_iterator, double>::iterator itTrip = valuesMap.begin(); itTrip != valuesMap.end(); itTrip++)
        {
            if(itTrip->second == 0)
            {
                minIt = itTrip;
                break;
            }
            if(itTrip->second < minValue)
            {
                minIt = itTrip;
                minValue = itTrip->second;
            }
        }

        if (minIt == valuesMap.end())
            continue;


        vector<Trip*>::const_iterator UpdateIt = minIt->first;
        TripToSwap.push_back(*UpdateIt);
        minValue = numeric_limits<double>::infinity();

        //update the values on the map as if this trip will be change
        //-->
        UpdateIt = minIt->first;
        for(int i = 0; i < bestMinSpace; i++)
        {
            if(UpdateIt == listUsersOD.begin())
            {
                break;
            }

            UpdateIt--;
            if ((*UpdateIt)->GetPath(DEFAULT_SIMULATION_INSTANCE) == actualPath->GetPath())
            {
                map<vector<Trip*>::const_iterator, double>::iterator valueFound = valuesMap.find(UpdateIt);
                if (valueFound != valuesMap.end())
                    valueFound->second += ((bestMinSpace - i) * (bestMinSpace - i));
            }
        }

        UpdateIt = minIt->first;
        for(int i = 0; i < bestMinSpace; i++)
        {
            UpdateIt++;
            if(UpdateIt == listUsersOD.end())
            {
                break;
            }

            if ((*UpdateIt)->GetPath(DEFAULT_SIMULATION_INSTANCE) == actualPath->GetPath())
            {
                map<vector<Trip*>::const_iterator, double>::iterator valueFound = valuesMap.find(UpdateIt);
                if (valueFound != valuesMap.end())
                    valueFound->second += ((bestMinSpace - i) * (bestMinSpace - i));
            }
        }
        //<--

        minIt = valuesMap.erase(minIt);
    }

    if (!valuesMap.empty())
    {
        for (map<vector<Trip*>::const_iterator, double>::iterator itTrip = valuesMap.begin(); itTrip != valuesMap.end(); itTrip++)
        {
            if (itTrip->second == 0)
            {
                minIt = itTrip;
                break;
            }
            if (itTrip->second < minValue)
            {
                minIt = itTrip;
                minValue = itTrip->second;
            }
        }
        TripToSwap.push_back(*(minIt->first));
    }


    for (size_t i = 0; i < TripToSwap.size(); i++)
    {
        int ShortestPathIndex = (int)(i % listPathFound.size());
        Trip * pTrip = TripToSwap[i];
        for (vector<Trip*>::iterator itFoundTrip = listAffectedUsersActualPath.begin(); itFoundTrip != listAffectedUsersActualPath.end(); itFoundTrip++)
        {
            if((*itFoundTrip) == pTrip)
            {
                listAffectedUsersActualPath.erase(itFoundTrip);
                break;
            }
        }
        pTrip->SetPath(DEFAULT_SIMULATION_INSTANCE, listPathFound[ShortestPathIndex]->GetPath());
        userDistribution[listPathFound[ShortestPathIndex]].push_back(pTrip);
    }
}

void MSADualLoop::PrintOutputGapIteration(double dbActualGap, bool bInInnerLoop)
{
	boost::filesystem::ofstream OutputStream;
	std::string fileName = m_SimulationConfiguration->GetStrOutputPrefix() + "_globalGap.csv";

	if (m_iCurrentPeriod == 0 && m_iOuterloopIndex == 0)
	{
		OutputStream.open(boost::filesystem::path(m_SimulationConfiguration->GetStrOutpuDirectoryPath()) / fileName, boost::filesystem::ofstream::trunc);
        OutputStream << "IsInnerloop;Period;OuterloopIteration;InnerLoopIteration;GlobalGap" << endl;
	}
	else{
		OutputStream.open(boost::filesystem::path(m_SimulationConfiguration->GetStrOutpuDirectoryPath()) / fileName, boost::filesystem::ofstream::app);
	}

	if (OutputStream.fail())
	{
		BOOST_LOG_TRIVIAL(error) << "Unable to open the file '" << fileName << "' !";
		return;
	}

    std::string strIsInnerLoop = bInInnerLoop?"yes;":"no;";

    OutputStream << strIsInnerLoop << to_string(m_iCurrentPeriod) << ";" << to_string(m_iOuterloopIndex) << ";" << to_string(m_iInnerloopIndex) << ";" << dbActualGap << endl;
	OutputStream.close();

	if (m_bPrintAffectation && ((m_iOuterloopIndex == 0 && m_iInnerloopIndex == 0) || bInInnerLoop))
    {
        fileName = m_SimulationConfiguration->GetStrOutputPrefix() + "_distribution.csv";

        if (m_iCurrentPeriod == 0 && m_iOuterloopIndex == 0)
        {
            OutputStream.open(boost::filesystem::path(m_SimulationConfiguration->GetStrOutpuDirectoryPath()) / fileName, boost::filesystem::ofstream::trunc);
            OutputStream << "Period;OuterloopIteration;InnerLoopIteration;SubPopulation;Origin;Destination;NbOfUser;TravelTime;Path" << endl;
        }
        else{
            OutputStream.open(boost::filesystem::path(m_SimulationConfiguration->GetStrOutpuDirectoryPath()) / fileName, boost::filesystem::ofstream::app);
        }

        if (OutputStream.fail())
        {
            BOOST_LOG_TRIVIAL(error) << "Unable to open the file '" << fileName << "' !";
            return;
        }


        // rmk. we use DEFAULT_SIMULATION_INSTANCE here because in outerloop, or if innerloop, not called for HeuristicDualLoop (IsIterationNeeded just returns true)
        for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterSubPopulation = m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].begin(); iterSubPopulation != m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].end(); ++iterSubPopulation)
        {
            mapByOriginDestination< vector<boost::shared_ptr<ValuetedPath> > >& ShortestPathAtSubPopulation = m_mapOriginalShortestPathResults[iterSubPopulation->first];
            for (mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
            {
                mapByDestination< vector<boost::shared_ptr<ValuetedPath> > >& ShortestPathAtOrigin = ShortestPathAtSubPopulation[iterOrigin->first];
                for (mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
                {
					if (ShortestPathAtOrigin[iterDestination->first].size() == 0)
						continue;

                    boost::shared_ptr<ValuetedPath> minShortestPath = ShortestPathAtOrigin[iterDestination->first][0];

                    //the valueted paths in the last maps are ordered by costs
                    map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPaths = iterDestination->second;
                    int sumOfUser = 0;

                    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPaths.begin(); iterPath != mapValuetedPaths.end(); ++iterPath)
                    {
                        OutputStream << to_string(m_iCurrentPeriod) << ";" << to_string(m_iOuterloopIndex) << ";" << to_string(m_iInnerloopIndex) << ";"
                                     << iterSubPopulation->first->GetStrName() << ";" << iterOrigin->first->getStrNodeName() << ";"
                                     << iterDestination->first->getStrNodeName() << ";" << iterPath->second.size() << ";" << iterPath->first->GetCost() << ";"
                                     << iterPath->first->GetPath() << ";" << endl;

                        sumOfUser += (int)iterPath->second.size();
                    }

                    OutputStream << to_string(m_iCurrentPeriod) << ";" << to_string(m_iOuterloopIndex) << ";" << to_string(m_iInnerloopIndex) << ";"
                                 << iterSubPopulation->first->GetStrName() << ";" << iterOrigin->first->getStrNodeName() << ";"
                                 << iterDestination->first->getStrNodeName() << ";" << sumOfUser << ";" << minShortestPath->GetCost() << ";"
                                 << "Sum of user and minimum travel time for the OD;" << endl;
                }
            }
        }

        OutputStream << endl << endl;
        OutputStream.close();

    }
}

void MSADualLoop::PrintOutputData()
{
    double GlobalGap = 0.0;
    double UserViolation = 0.0;
    double nbOfUsers = 0.0;
    double TotalNbOfODs = 0.0;
    double SumODViolation = 0.0;

    boost::filesystem::ofstream OutputStream;
    std::string fileName = m_SimulationConfiguration->GetStrOutputPrefix() + "_violation.csv";

    if(m_iCurrentPeriod == 0 && m_iOuterloopIndex == 0)
    {
        OutputStream.open(boost::filesystem::path(m_SimulationConfiguration->GetStrOutpuDirectoryPath()) / fileName, boost::filesystem::ofstream::trunc);
        OutputStream << "Period;OuterloopIteration;Population;SubPopulation;Origin;Destination;Violation" << endl;
    }else{
        OutputStream.open(boost::filesystem::path(m_SimulationConfiguration->GetStrOutpuDirectoryPath()) / fileName, boost::filesystem::ofstream::app);
    }

    if(OutputStream.fail())
    {
        BOOST_LOG_TRIVIAL(error) << "Unable to open the file '" << fileName << "' !";
        return;
    }

    for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterSubPopulation = m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].begin(); iterSubPopulation != m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].end(); ++iterSubPopulation)
    {
        const ParametersForSubpopulation & paramsForSubpop = m_mapParametersBySubpopulation.at(iterSubPopulation->first);
        mapByOriginDestination< vector< boost::shared_ptr<ValuetedPath> > >& ShortestPathAtSubPopulation = m_mapOriginalShortestPathResults[iterSubPopulation->first];
        for (mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterOrigin = iterSubPopulation->second.begin(); iterOrigin != iterSubPopulation->second.end(); ++iterOrigin)
        {
            mapByDestination< vector< boost::shared_ptr<ValuetedPath> > >& ShortestPathAtOrigin = ShortestPathAtSubPopulation[iterOrigin->first];
            for (mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                vector< boost::shared_ptr<ValuetedPath> >& listShortestPath = ShortestPathAtOrigin[iterDestination->first];

				if (listShortestPath.size() == 0)
					continue;

                boost::shared_ptr<ValuetedPath> minShortestPath = listShortestPath[0];
                //ValuetedPath* minShortestPath = iterDestination->second.begin()->first;
                UserViolation = 0.0;
                nbOfUsers = 0.0;

                //the valueted paths in the last maps are ordered by costs
                map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPaths = iterDestination->second;
                for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPaths.begin(); iterPath != mapValuetedPaths.end(); ++iterPath)
                {
                    //rmk : maybe it would be better to use the cost of the new shortest path discovered, even if there is still no user on it.
                    double minShortestPathCost = minShortestPath->GetCost();
                    if (paramsForSubpop.eOptimizerType == Optimizer_BRUE)
                    {
                        for(size_t i=0; i<iterPath->second.size(); i++)
                        {
                            minShortestPathCost = minShortestPath->GetCost();
                            Trip* currentTrip = iterPath->second[i];
                            minShortestPathCost = minShortestPathCost + (minShortestPathCost * currentTrip->GetSigmaConvenience(paramsForSubpop.dbSigmaConvenience));

                            GlobalGap += max(0.0 , (iterPath->first->GetCost() - minShortestPathCost));
                            if (max(0.0, ((iterPath->first->GetCost() - minShortestPathCost) / minShortestPathCost)) > paramsForSubpop.dbMaxRelativeCostThresholdOuterloop)
                                UserViolation++;

                            nbOfUsers++;
                        }
                    }else{
                        GlobalGap += ((int)iterPath->second.size()) * (iterPath->first->GetCost() - minShortestPathCost);
                        if (((iterPath->first->GetCost() - minShortestPathCost) / minShortestPathCost) > paramsForSubpop.dbMaxRelativeCostThresholdOuterloop)
                            UserViolation += (int)iterPath->second.size();

                        nbOfUsers += (int)iterPath->second.size();
                    }
                }

                OutputStream << to_string(m_iCurrentPeriod) <<
                                ";" << to_string(m_iOuterloopIndex) <<
                                ";" << iterSubPopulation->first->GetPopulation()->GetStrName() <<
                                ";" << iterSubPopulation->first->GetStrName() <<
                                ";" << iterOrigin->first->getStrNodeName() <<
                                ";" << iterDestination->first->getStrNodeName() <<
                                ";" << UserViolation/nbOfUsers << endl;
                TotalNbOfODs++;
                if(UserViolation/nbOfUsers >= m_dbOuterloopRelativeUserViolation)
                    SumODViolation++;
            }
        }
    }
    OutputStream << to_string(m_iCurrentPeriod) << ";" << to_string(m_iOuterloopIndex) << ";SumViolation;ALL;ALL;ALL;" << SumODViolation/TotalNbOfODs << endl;
    //OutputStream << to_string(m_iCurrentPeriod) << ";" << to_string(m_iOuterloopIndex) << ";GlobalGap;ALL;ALL;ALL;" << GlobalGap/TotalNbOfUsers << endl;
    OutputStream << to_string(m_iCurrentPeriod) << ";" << to_string(m_iOuterloopIndex) << ";GlobalGap;ALL;ALL;ALL;" << GlobalGap << endl; // it's better to output the exact global gap
    OutputStream.close();
}

bool MSADualLoop::MaximumSwappingReached(int iCurrentIterationIndex)
{
    bool isMaximumReached = true;

	if (m_iNbMaxTotalIterations != -1 && iCurrentIterationIndex >= m_iNbMaxTotalIterations)
		return isMaximumReached;

    assert(!NeedsParallelSimulationInstances()); // so we can use DEFAULT_SIMULATION_INSTANCE

    // First loop on SubPopulations
    for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator itAffectedSubPopulation = m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].begin(); itAffectedSubPopulation != m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE].end() && isMaximumReached; ++itAffectedSubPopulation)
    {
		SubPopulation* pSubPopulation = itAffectedSubPopulation->first;
        mapByOriginDestination <int>& StepsizeByODAtSubPopulation = m_mapImprovedStepSize[pSubPopulation];
		// Second loop on Origins
        for (mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator itAffectedOrigin = itAffectedSubPopulation->second.begin(); itAffectedOrigin != itAffectedSubPopulation->second.end() && isMaximumReached; ++itAffectedOrigin)
        {
			Origin* pOrigin = itAffectedOrigin->first;
			mapByDestination <int>& StepsizeByODAtOrigin = StepsizeByODAtSubPopulation[pOrigin];
			// Assignment loop
            for (mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator itAffectedDestination = itAffectedOrigin->second.begin(); itAffectedDestination != itAffectedOrigin->second.end() && isMaximumReached; ++itAffectedDestination)
            {
				Destination* pDestination = itAffectedDestination->first;
                int &Stepsize = StepsizeByODAtOrigin[pDestination];

                for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = itAffectedDestination->second.begin(); iterPath != itAffectedDestination->second.end(); ++iterPath)
                {            
					if((iterPath->second.size() * (1.0 / (Stepsize + 1.0))) >= m_iMinSwapValue)
                    {
                        isMaximumReached = false;
                        return false;
                    }
                }
            }
        }
    }

    return isMaximumReached;
}


void MSADualLoop::ShowResult(const vector<Trip *> &listSortedTrip)
{
    // for each Trip :
    for (vector<Trip*>::const_iterator itTrip = listSortedTrip.begin(); itTrip != listSortedTrip.end(); ++itTrip)
    {

        vector<Pattern*> listPatterns = (*itTrip)->GetPath(DEFAULT_SIMULATION_INSTANCE).GetlistPattern();
        string strRes = "From " + (*itTrip)->GetOrigin()->getNode()->getStrName() + " to " + (*itTrip)->GetDestination()->getNode()->getStrName() + " at Cost " + to_simple_string((*itTrip)->GetDepartureTime()) + " with vehc ";
        if(!(*itTrip)->GetSubPopulation()->GetPopulation()->GetMacroType()->getListVehicleTypes().empty())
        {
            strRes +=  (*itTrip)->GetSubPopulation()->GetPopulation()->GetMacroType()->getListVehicleTypes().at(0)->getStrName();
            for (unsigned int i = 1; i < (*itTrip)->GetSubPopulation()->GetPopulation()->GetMacroType()->getListVehicleTypes().size(); i++)
            {
                strRes += "->" + (*itTrip)->GetSubPopulation()->GetPopulation()->GetMacroType()->getListVehicleTypes().at(i)->getStrName();
            }
            strRes += "\n";
        }
        if(!listPatterns.empty())
        {
            strRes +=  "\n" + listPatterns.at(0)->toString();
            for (unsigned int i = 1; i < listPatterns.size(); i++)
            {
                strRes += "->" + listPatterns.at(i)->getLink()->getUpstreamNode()->getStrName();
                strRes += "->" + listPatterns.at(i)->toString();
            }
            strRes += "\n";
        }else
            strRes += "\n No path found !!";
        BOOST_LOG_TRIVIAL(debug) << strRes;

    }
}
