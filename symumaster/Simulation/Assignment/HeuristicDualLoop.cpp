#include "HeuristicDualLoop.h"

#include "Simulation/Simulators/SimulationDescriptor.h"
#include "Simulation/Simulators/TraficSimulatorHandler.h"
#include "Simulation/Simulators/TraficSimulators.h"
#include "Simulation/Config/SimulationConfiguration.h"

#include "IO/XMLStringUtils.h"

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

std::mt19937 rng;

// TODO - output those parameters in XML
#define GA_SOLUTION_SIZE 10
int sizeForReplcament = GA_SOLUTION_SIZE;

int SymuMaster::RandomInteger(int min, int max)
{
    if (max < min) { max = min; }

    std::uniform_int_distribution<int> uni(min, max); // guaranteed unbiased

    auto output = uni(rng);
    return output;
}


// Creation of the classes (default)
HeuristicDualLoop::HeuristicDualLoop() : MSADualLoop()
{
    std::random_device rd;     // only used once to initialise (seed) engine
    unsigned int seed = rd();
    BOOST_LOG_TRIVIAL(debug) << "Using seed " << seed << " for HeuristicDualLoop model" << std::endl;
    rng.seed(seed);
    m_ThisOD = true;
    m_T0 = 3000;
    m_SA_iter = -1;
    m_IsUpdated = -1;
    m_statu_SA = 0;
    m_isGa = false;
}

HeuristicDualLoop::~HeuristicDualLoop()
{
}

bool HeuristicDualLoop::NeedsParallelSimulationInstances()
{
    return true;
}

int HeuristicDualLoop::GetMaxSimulationInstancesNumber()
{
    if (m_MHType == MHT_SimulatedAnnealing)
    {
        return 3;
    }
    else
    {
        assert(m_MHType == MHT_GeneticAlgorithm);
        return std::max<int>(GA_SOLUTION_SIZE, sizeForReplcament) * (2 * 2 * 2); // in GeneticAlgorithm(), we have CrossOver(), CrossOver_Inner() and Mutation() that can each double the solution size, at most.
    }
}

bool HeuristicDualLoop::GetNeededSimulationInstancesNumber(const std::vector<SymuCore::Trip*> &listSortedTrip, const boost::posix_time::ptime & startPeriodTime, const boost::posix_time::time_duration &PeriodTimeDuration, const boost::posix_time::time_duration& travelTimesUpdatePeriodDuration, int iCurrentIterationIndex, int iPeriod, int & nbSimulationInstances)
{
    if (m_MHType == MHT_SimulatedAnnealing)
    {
        // rmk. can't use m_statu_SA for this test cause m_statu_SA is updated after, in AssignPathsToTrips.
        if (m_iInnerloopIndex < (0.3)*m_iMaxInnerloopIteration)
        {
            nbSimulationInstances = 1;
        }
        else if (m_iInnerloopIndex < (0.6)*m_iMaxInnerloopIteration)
        {
            nbSimulationInstances = 3;
        }
        else
        {
            nbSimulationInstances = 2;
        }
    }
    else
    {
        assert(m_MHType == MHT_GeneticAlgorithm);
        if (iCurrentIterationIndex == 0)
        {
            nbSimulationInstances = 1;
        }
        else
        {
            // Build the solutions set
            if (!m_bChangeAssignment)
            {
                GeneticAlgorithm();
            }

            nbSimulationInstances = popuDnaa.Size();
        }
    }
   
    return true;
}


std::string HeuristicDualLoop::GetAlgorithmTypeNodeName() const
{
    return "MH_TYPE";
}

bool HeuristicDualLoop::readAlgorithmNode(DOMNode * pChildren)
{
    string MHName;
    XMLStringUtils::GetAttributeValue(pChildren, "mh_type", MHName);
    if (MHName == "SIMULATED_ANNEALING")
    {
        m_MHType = MHT_SimulatedAnnealing;
    }
    else if (MHName == "GENETIC_ALGORITHM")
    {
        m_MHType = MHT_GeneticAlgorithm;
    }
    else
    {
        assert(false);
    }
    
    if (!XMLStringUtils::GetAttributeValue(pChildren, "default_logit", m_dbDefaultTetaLogit))
    {
        m_dbDefaultTetaLogit = -1;
    }

    XMLStringUtils::GetAttributeValue(pChildren, "is_initialization", m_bInitialization);
    XMLStringUtils::GetAttributeValue(pChildren, "is_initial_stepsize_outerloop_index", m_bIsInitialStepSizeOuterloopIndex);

    return true;
}

// Main function to optimize
bool HeuristicDualLoop::AssignPathsToTrips(const vector<Trip *> &listSortedTrip, const boost::posix_time::ptime &startPeriodTime, const boost::posix_time::time_duration & PeriodTimeDuration,
    const boost::posix_time::time_duration &travelTimesUpdatePeriodDuration, int& iCurrentIterationIndex, int iPeriod, int iSimulationInstance, int iSimulationInstanceForBatch, bool bAssignAllToFinalSolution)
{
    //Y :: End ----------------------------------------------
    // if first inner loop , initialize temperature.
    if (m_iInnerloopIndex == 0) { m_T0 = 3000; }

    if (m_bChangeAssignment)
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
    if (!m_mapPredefinedPaths.empty())//Always iCurrentIterationIndex for assignment with predefined paths
    {
        m_iValueOfK = iCurrentIterationIndex;
    }
    else{
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

    if (m_MHType == MHT_GeneticAlgorithm && iCurrentIterationIndex == 0)
    {
        // reset best solution at the beginning of each assignment period, with an infinite cost so the first solution with non infinite gap takes the best slot.
        bestSol = DNAForSubpopsAndFitness();
        bestSol.fitness = std::numeric_limits<double>::infinity();

        // reset solutions
        popuDnaa.Clear();
    }

    // prepare assignment on first iteration :
    if (iCurrentIterationIndex == 0 && !bAssignAllToFinalSolution)
    {
        m_mapImprovedStepSize.clear();
        m_DNA_OriginalSolution.clear(); // we have to clear thise from an assignment period to an other
        BOOST_LOG_TRIVIAL(debug) << "Compute first shortest path";
        // Compute the first shortest path(s)
        bool bOk = ComputeFirstShortestPaths(listSortedTrip, dbDepartureTime, dbEndPeriodTime, travelTimesUpdatePeriodDuration, iSimulationInstance, iSimulationInstanceForBatch);
        if (!bOk)
            return false;
    }

    //Y : -----------------------------------------------------------
    // save original solution + compute fitness of original solution.
    //InitDNA(m_mapAffectedTrips[iSimulationInstanceForBatch], m_DNA_OriginalSolution);
    if (m_MHType == MHT_SimulatedAnnealing)
    {
        DNAtoMap(iSimulationInstanceForBatch, m_mapAffectedTrips[iSimulationInstanceForBatch], m_DNA_OriginalSolution);
        if (iSimulationInstance == 0 && !bAssignAllToFinalSolution)
        {
            m_ffS1 = ComputeGlobalGap(m_mapActualGlobalGap, m_mapOriginalShortestPathResults, iSimulationInstanceForBatch);
        }
    }
    //Y : 

    // Can't do this since here for heuristicDualLoop since the GroupCloseShortestPath method changes the global m_mapOriginalShortestPathResults.
	// => So, we just call the function after any update ot the m_mapOriginalShortestPathResults variable :
	// - when updating the m_mapOriginalShortestPathResults in the Decision function (when going to the next innerloop)
	// - when computing a new shortest path (when going to next outerloop)
    /*if (!bAssignAllToFinalSolution)
    {
        GroupCloseShortestPath(iSimulationInstanceForBatch);
    }*/

    // First loop on SubPopulations
    for (mapBySubPopulationOriginDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath> > >::iterator itShortestPathSubPopulation = m_mapOriginalShortestPathResults.begin(); itShortestPathSubPopulation != m_mapOriginalShortestPathResults.end(); ++itShortestPathSubPopulation)
    {
        SubPopulation* pSubPopulation = itShortestPathSubPopulation->first;
        mapByOriginDestination< vector<Trip*> >& TripsByODAtSubPopulation = m_mapTripsByOD[pSubPopulation];
        mapByOriginDestination <int>& StepsizeByODAtSubPopulation = m_mapImprovedStepSize[pSubPopulation];
        mapByOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& AffectedTripsAtSubPopulation = m_mapAffectedTrips[iSimulationInstanceForBatch][pSubPopulation];

        DNA & DNA_OriginalSolutionForSubpopulation = m_DNA_OriginalSolution[pSubPopulation];
        DNA & DNA_RandSoluForSubpopulation = m_DNA_RandSolu[pSubPopulation];
        DNA & DNA_MSA_SolutionForSubpopulation = m_DNA_MSA_Solution[pSubPopulation];
        DNA & DNA_GB_SolutionForSubpopulation = m_DNA_GB_Solution[pSubPopulation];
        DNA & DNA_Randomize_MSAForSubpopulation = m_DNA_Randomize_MSA[pSubPopulation];
        DNA & DNA_Randomize_GBForSubpopulation = m_DNA_Randomize_GB[pSubPopulation];

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
                const vector<boost::shared_ptr<SymuCore::ValuetedPath> > & listPathsFoundOriginal = itShortestPathDestination->second;

                map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath = AffectedTripsAtOrigin[pDestination];
                map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > newMapValuetedPath;

                // rmk. create a copy of ValuetedPaths here so we keep the original ValuetedPaths with original cost (if not, we share ValuetedPaths with all simulation instances,
                // and end with the cost of the last simulation instance in m_mapOriginalShortestPathResults).
                vector<boost::shared_ptr<SymuCore::ValuetedPath> > listPathsFound;
                for (size_t iPath = 0; iPath < listPathsFoundOriginal.size(); iPath++)
                {
                    boost::shared_ptr<ValuetedPath> pPath = listPathsFoundOriginal.at(iPath);
                    boost::shared_ptr<ValuetedPath> pPathCopy = boost::make_shared<ValuetedPath>(*pPath);

                    map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterMapValuetedPath = mapValuetedPath.find(pPath);
                    if (iterMapValuetedPath != mapValuetedPath.end())
                    {
                        newMapValuetedPath[pPathCopy] = iterMapValuetedPath->second;
                        mapValuetedPath.erase(iterMapValuetedPath);
                    }

                    listPathsFound.push_back(pPathCopy);
                }

                for (map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterMapValuetedPath = mapValuetedPath.begin(); iterMapValuetedPath != mapValuetedPath.end(); ++iterMapValuetedPath)
                {
                    boost::shared_ptr<ValuetedPath> pPathCopy = boost::make_shared<ValuetedPath>(*iterMapValuetedPath->first);
                    newMapValuetedPath[pPathCopy] = iterMapValuetedPath->second;
                }
                mapValuetedPath = newMapValuetedPath;

                vector<Trip*>& listTripsByOD = TripsByODAtOrigin[pDestination];

                //Step size method
                Stepsize = m_iValueOfK;

                if (iCurrentIterationIndex != 0)
                {
                    // MSA switch
                    switch (m_MHType)
                    {
                        case MHT_SimulatedAnnealing:
                        {
                            if (bAssignAllToFinalSolution)
                            {
                                // we have to apply the chosen solution to all cores to resync the simulators
                                GetMapValutedFromChr(iSimulationInstanceForBatch, DNA_OriginalSolutionForSubpopulation.chromosomes[pOrigin][pDestination], mapValuetedPath, listPathsFound);
                            }
                            else
                            {
                                // here we decide which segment // for begin we divide based on iterations , later we divide based on temperature.
                                // Gas case.
                                if (m_iInnerloopIndex < (0.3)*m_iMaxInnerloopIteration)
                                {
                                    m_statu_SA = 0;

                                    // if first inner loop , rn MSA (aim is to force process to get the shortest path. // to define it more.
                                    // TODO - Mostafa : may be remove the ClassicMSA ?
                                    if (m_iInnerloopIndex == 0) { ClassicMSAByYounes(iSimulationInstanceForBatch, mapValuetedPath, listPathsFound, listTripsByOD, Stepsize, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop); }
                                    else						{ Randomize_Method(iSimulationInstanceForBatch, mapValuetedPath, listPathsFound); }

                                    Chromosome chr; chr.SetTrips(mapValuetedPath); DNA_RandSoluForSubpopulation.AddChromosome(pOrigin, pDestination, chr);
                                }
                                else
                                if (m_iInnerloopIndex < (0.6)*m_iMaxInnerloopIteration)
                                {
                                    // liquid
                                    m_statu_SA = 1;

                                    if (iSimulationInstance == 0)
                                    {
                                        // fill mapValuetedPath with the original solution
                                        GetMapValutedFromChr(iSimulationInstanceForBatch, DNA_OriginalSolutionForSubpopulation.chromosomes[pOrigin][pDestination], mapValuetedPath, listPathsFound);

                                        //-----Randomize !------------------
                                        Randomize_Method(iSimulationInstanceForBatch, mapValuetedPath, listPathsFound);
                                        Chromosome chr_Rand; chr_Rand.SetTrips(mapValuetedPath); DNA_RandSoluForSubpopulation.AddChromosome(pOrigin, pDestination, chr_Rand);
                                    }
                                    else if (iSimulationInstance == 1)
                                    {
                                        // Get back the random mapValuetedPath
                                        GetMapValutedFromChr(iSimulationInstanceForBatch, DNA_RandSoluForSubpopulation.chromosomes[pOrigin][pDestination], mapValuetedPath, listPathsFound);

                                        // apply MSA (swapping).
                                        ClassicMSAByYounes(iSimulationInstanceForBatch, mapValuetedPath, listPathsFound, listTripsByOD, Stepsize, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop);
                                        Chromosome chr_RandMSA; chr_RandMSA.SetTrips(mapValuetedPath); DNA_Randomize_MSAForSubpopulation.AddChromosome(pOrigin, pDestination, chr_RandMSA);
                                    }
                                    else if (iSimulationInstance == 2)
                                    {
                                        // Get back the random mapValuetedPath
                                        GetMapValutedFromChr(iSimulationInstanceForBatch, DNA_RandSoluForSubpopulation.chromosomes[pOrigin][pDestination], mapValuetedPath, listPathsFound);

                                        // apply GB.
                                        GapBasedMSAByYounes(iSimulationInstanceForBatch, mapValuetedPath, listPathsFound, listTripsByOD, Stepsize, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop);
                                        Chromosome chr_RandGB; chr_RandGB.SetTrips(mapValuetedPath); DNA_Randomize_GBForSubpopulation.AddChromosome(pOrigin, pDestination, chr_RandGB);
                                    }
                                }
                                else
                                {
                                    // solid
                                    m_statu_SA = 2;

                                    if (iSimulationInstance == 0)
                                    {
                                        // Get back the original mapValuetedPath
                                        GetMapValutedFromChr(iSimulationInstanceForBatch, DNA_OriginalSolutionForSubpopulation.chromosomes[pOrigin][pDestination], mapValuetedPath, listPathsFound);

                                        // apply MSA (swapping).
                                        ClassicMSAByYounes(iSimulationInstanceForBatch, mapValuetedPath, listPathsFound, listTripsByOD, Stepsize, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop);
                                        Chromosome chr_RandMSA; chr_RandMSA.SetTrips(mapValuetedPath); DNA_MSA_SolutionForSubpopulation.AddChromosome(pOrigin, pDestination, chr_RandMSA);
                                    }
                                    else if (iSimulationInstance == 1)
                                    {
                                        // Get back the original mapValuetedPath
                                        GetMapValutedFromChr(iSimulationInstanceForBatch, DNA_OriginalSolutionForSubpopulation.chromosomes[pOrigin][pDestination], mapValuetedPath, listPathsFound);

                                        // apply GB.
                                        GapBasedMSAByYounes(iSimulationInstanceForBatch, mapValuetedPath, listPathsFound, listTripsByOD, Stepsize, bIsBRUE, paramsForSubpop.dbSigmaConvenience, paramsForSubpop.dbMaxRelativeCostThresholdOuterloop);
                                        Chromosome chr_RandGB; chr_RandGB.SetTrips(mapValuetedPath); DNA_GB_SolutionForSubpopulation.AddChromosome(pOrigin, pDestination, chr_RandGB);
                                    }
                                }
                            }

                            break;
                        }
                        case MHT_GeneticAlgorithm:
                        {
                            if (bAssignAllToFinalSolution)
                            {
                                GetMapValutedFromChr(iSimulationInstanceForBatch, bestSol.m_DNABySubPop[pSubPopulation].chromosomes[pOrigin][pDestination], mapValuetedPath, listPathsFound);
                            }
                            else
                            {
                                GetMapValutedFromChr(iSimulationInstanceForBatch, popuDnaa.GetSolution(iSimulationInstance).m_DNABySubPop[pSubPopulation].chromosomes[pOrigin][pDestination], mapValuetedPath, listPathsFound);
                            }
                            break;
                        }
                    }
                }
            }
        }  
    }

    // the iCurrentIterationIndex != 0 case is handled before.
    if (m_MHType == MHT_SimulatedAnnealing && iCurrentIterationIndex == 0)
    {
         InitDNA(m_mapAffectedTrips[iSimulationInstanceForBatch], m_DNA_OriginalSolution);
    }
    //ShowResult(listSortedTrip);

    return true;
}

bool KeepBestSol(HeuristicDualLoop::DNAForSubpopsAndFitness& bestSol, const HeuristicDualLoop::DNAForSubpopsAndFitness & temp, int iSimulationInstance)
{
    if (temp.fitness < bestSol.fitness) { bestSol = temp; return true; }
    return false;
}

bool HeuristicDualLoop::onSimulationInstanceDone(const std::vector<Trip *> &listTrip, const boost::posix_time::ptime & endPeriodTime, int iSimulationInstance, int iSimulationInstanceForBatch, int iCurrentIterationIndex)
{
    // Update path costs
    bool bOk = MSADualLoop::onSimulationInstanceDone(listTrip, endPeriodTime, iSimulationInstance, iSimulationInstanceForBatch, iCurrentIterationIndex);
    if (!bOk)
    {
        return false;
    } 

    if (m_MHType == MHT_SimulatedAnnealing)
    {
        // save gap : 
        // statu ! // 0 : gas, 1:liquid, 2 solid
        if (!(m_statu_SA == 0 && iCurrentIterationIndex == 0)) // in this case, m_ffS1 has been computed in AssignPathToTrips
        {
            double dbGlobalGap = ComputeGlobalGap(m_mapActualGlobalGap, m_mapNewShortestPathResults[iSimulationInstance], iSimulationInstanceForBatch);

            if (m_statu_SA == 0)
            {
                // useless to store the gap here do since we compute the gap directly in the decision function
            }
            else if (m_statu_SA == 1)
            {
                switch (iSimulationInstance)
                {
                case 0:
                    m_ffRandSol = dbGlobalGap;
                    break;
                case 1:
                    m_ffRandMSA = dbGlobalGap;
                    break;
                case 2:
                    m_ffRandGB = dbGlobalGap;
                    break;
                }
            }
            else
            {
                assert(m_statu_SA == 2);

                switch (iSimulationInstance)
                {
                case 0:
                    m_ffMSASol = dbGlobalGap;
                    break;
                case 1:
                    m_ffGBSol = dbGlobalGap;
                    break;
                }

            }

            if (dbGlobalGap < m_dbBestQuality)
            {
                m_dbBestQuality = dbGlobalGap;
                SaveCurrentAssignment(listTrip, iSimulationInstanceForBatch);
            }
        }
    }
    else
    {
        assert(m_MHType == MHT_GeneticAlgorithm);

        double fit = ComputeGlobalGap(m_mapActualGlobalGap, m_mapNewShortestPathResults[iSimulationInstance], iSimulationInstanceForBatch);

        if (popuDnaa.Size() != 0) // we don't want to create an empty solution with this until we have geenrated the solutions in GeneticALgorithm()
        {
            popuDnaa.SetFtiness(iSimulationInstance, fit);

            // keep best here !
            if (KeepBestSol(bestSol, popuDnaa.GetSolution(iSimulationInstance), iSimulationInstance))
            {
                bestsolKey = iSimulationInstance;
                SaveCurrentAssignment(listTrip, iSimulationInstanceForBatch);
            }
        }
    }

    // Max swapping reached management :
    if (iSimulationInstance == 0)
    {
        m_bMaximumSwappingReached = true;
    }

    if (!(m_iNbMaxTotalIterations != -1 && iCurrentIterationIndex >= m_iNbMaxTotalIterations) && m_bMaximumSwappingReached)
    {
        // First loop on SubPopulations
        for (mapBySubPopulationOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator itAffectedSubPopulation = m_mapAffectedTrips[iSimulationInstanceForBatch].begin(); itAffectedSubPopulation != m_mapAffectedTrips[iSimulationInstanceForBatch].end(); ++itAffectedSubPopulation)
        {
            SubPopulation* pSubPopulation = itAffectedSubPopulation->first;
            mapByOriginDestination <int>& StepsizeByODAtSubPopulation = m_mapImprovedStepSize[pSubPopulation];
            // Second loop on Origins
            for (mapByOriginDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator itAffectedOrigin = itAffectedSubPopulation->second.begin(); itAffectedOrigin != itAffectedSubPopulation->second.end(); ++itAffectedOrigin)
            {
                Origin* pOrigin = itAffectedOrigin->first;
                mapByDestination <int>& StepsizeByODAtOrigin = StepsizeByODAtSubPopulation[pOrigin];
                // Assignment loop
                for (mapByDestination< map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath > >::iterator itAffectedDestination = itAffectedOrigin->second.begin(); itAffectedDestination != itAffectedOrigin->second.end(); ++itAffectedDestination)
                {
                    Destination* pDestination = itAffectedDestination->first;
                    int &Stepsize = StepsizeByODAtOrigin[pDestination];

                    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = itAffectedDestination->second.begin(); iterPath != itAffectedDestination->second.end(); ++iterPath)
                    {
                        if ((iterPath->second.size() * (1.0 / (Stepsize + 1.0))) >= m_iMinSwapValue)
                        {
                            m_bMaximumSwappingReached = false;
                            return true;
                        }
                    }
                }
            }
        }
    }

    return true;
}


void HeuristicDualLoop::Randomize_Method(int iSimulationInstanceForBatch, map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath, vector<boost::shared_ptr<SymuCore::ValuetedPath>> &listPathFound)
{
    int listPathIndex = 0;

    // for each path
    for (map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
    {
        //current shortest path !
        boost::shared_ptr<SymuCore::ValuetedPath> currentShortestPath = listPathFound[listPathIndex % listPathFound.size()];

        // the current path.
        boost::shared_ptr<SymuCore::ValuetedPath> oldPath = iterPath->first;
        if (oldPath == currentShortestPath) { continue; }

        double CostPath = iterPath->first->GetCost();

        // list of trips of current path
        vector<Trip *>& vecTrip = iterPath->second;

        if (vecTrip.size() <= 1) { continue; }

        // how much we have to swap !
        int nbSwap = RandomInteger(1, ((int)vecTrip.size() - 1));

        int i = 0;
        for (vector<Trip *>::iterator itTrip = vecTrip.begin(); itTrip != vecTrip.end() && i<nbSwap;)
        {
            (*itTrip)->SetPath(iSimulationInstanceForBatch, currentShortestPath->GetPath());
            mapValuetedPath[currentShortestPath].push_back(*itTrip);
            itTrip = vecTrip.erase(itTrip);
            i++; listPathIndex++;
            currentShortestPath = listPathFound[listPathIndex % listPathFound.size()];
            if (oldPath == currentShortestPath)
            {
                listPathIndex++;
                currentShortestPath = listPathFound[listPathIndex % listPathFound.size()];

                assert(oldPath != currentShortestPath); // should not happen !
            }
        }
    }
}

void HeuristicDualLoop::UnifSwapByYounes(int iSimulationInstanceForBatch, int nbSwap, map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip *>, CompareValuetedPath> &userDistribution, boost::shared_ptr<SymuCore::ValuetedPath> actualPath, vector<boost::shared_ptr<SymuCore::ValuetedPath>> &listPathFound, const vector<Trip *> &listUsersOD, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop)
{
    //Y: get trips of the actual path.
    int sizeOD = (int)listUsersOD.size();

    vector<Trip *>& listAffectedUsersActualPath = userDistribution.at(actualPath);

    assert(nbSwap <= listAffectedUsersActualPath.size()); //the nbSwap should always be equal or less than the number of users on the path

    vector<Trip *> listAffectedUsersNextPath;

    bool notEnoughSwap = false;
    for (size_t i = 0; i < listPathFound.size(); i++)
    {
        if (!notEnoughSwap && (int)(i) >= nbSwap){
            notEnoughSwap = true;
            double minShortestPathCost = listPathFound[0]->GetCost();


            if (bIsBRUE) //find the minimum BR and add it to the shortestpath cost
            {
                double minSigma = numeric_limits<double>::infinity();
                for (size_t j = 0; j < userDistribution[actualPath].size(); j++)
                {
                    double currentSigma = userDistribution[actualPath][j]->GetSigmaConvenience(dbDefaultSigmaConvenience);
                    if (currentSigma < minSigma)
                        minSigma = currentSigma;
                }
                minShortestPathCost += minSigma;
            }

            if ((actualPath->GetCost() - minShortestPathCost) / minShortestPathCost > dbMaxRelativeCostThresholdOuterloop){ // check if the actual path is close to the minimum shortest path
                if (!userDistribution[actualPath].empty())
                {
                    Trip* pTrip = userDistribution[actualPath][0];
                    if (nbSwap == 0)
                        BOOST_LOG_TRIVIAL(warning) << "You can't swap enough user for the path from " << pTrip->GetOrigin()->getStrNodeName() << " to " << pTrip->GetDestination()->getStrNodeName() << " !!";

                    continue;//rmk: for now, we don't handle when there is too few user to swap. It's only treated if there is too few user to swap in all paths
                }
            }
            else{
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
    if ((*UpdateIt)->GetPath(iSimulationInstanceForBatch) == actualPath->GetPath())
    {
        valuesMap[UpdateIt] = floor(listAffectedUsersNextPath.size() * listAffectedUsersNextPath.size()) + 1.0;
    }

    //will give a value to identify if the trip is close to a nextPath trip
    for (vector<Trip*>::const_iterator itTrip = listUsersOD.begin(); itTrip != listUsersOD.end(); itTrip++)
    {
        bool isShortestPath = false;
        for (size_t i = 0; i< listPathFound.size(); i++)
        {
            if ((*itTrip)->GetPath(iSimulationInstanceForBatch) == listPathFound[i]->GetPath())
            {
                isShortestPath = true;
                break;
            }
        }
        //----
        if (isShortestPath)
        {
            UpdateIt = itTrip;
            for (int i = 0; i < bestMinSpace; i++)
            {
                if (UpdateIt == listUsersOD.begin())
                {
                    break;
                }

                UpdateIt--;
                if ((*UpdateIt)->GetPath(iSimulationInstanceForBatch) == actualPath->GetPath())
                    valuesMap[UpdateIt] += ((bestMinSpace - i) * (bestMinSpace - i));
            }

            UpdateIt = itTrip;
            for (int i = 0; i < bestMinSpace; i++)
            {
                UpdateIt++;
                if (UpdateIt == listUsersOD.end())
                {
                    break;
                }

                if ((*UpdateIt)->GetPath(iSimulationInstanceForBatch) == actualPath->GetPath())
                    valuesMap[UpdateIt] += ((bestMinSpace - i) * (bestMinSpace - i));
            }

        }
        //----

        if ((*itTrip)->GetPath(iSimulationInstanceForBatch) == actualPath->GetPath())
        {

            //test if the user can change (only in BRE mode)
            if (bIsBRUE && (max(0.0, ((actualPath->GetCost() - minCostPath) / minCostPath)) <= (*itTrip)->GetSigmaConvenience(dbDefaultSigmaConvenience)))
            {
                valuesMap[itTrip] = numeric_limits<double>::infinity();
            }
            else{
                valuesMap[itTrip]; // create int = 0 if not exist
            }

        }
        //----
    }
    //-------------------------
    vector<Trip*> TripToSwap;
    double minValue = numeric_limits<double>::infinity();
    map<vector<Trip*>::const_iterator, double>::iterator minIt = valuesMap.end();
    for (int i = 0; i < nbSwap - 1; i++)
    {
        //find the actual minimum value
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

        if (minIt == valuesMap.end())
            continue;


        vector<Trip*>::const_iterator UpdateIt = minIt->first;
        TripToSwap.push_back(*UpdateIt);
        minValue = numeric_limits<double>::infinity();

        //update the values on the map as if this trip will be change
        //-->
        UpdateIt = minIt->first;
        for (int i = 0; i < bestMinSpace; i++)
        {
            if (UpdateIt == listUsersOD.begin())
            {
                break;
            }

            UpdateIt--;
            if ((*UpdateIt)->GetPath(iSimulationInstanceForBatch) == actualPath->GetPath())
            {
                map<vector<Trip*>::const_iterator, double>::iterator valueFound = valuesMap.find(UpdateIt);
                if (valueFound != valuesMap.end())
                    valueFound->second += ((bestMinSpace - i) * (bestMinSpace - i));
            }
        }
        //--------
        UpdateIt = minIt->first;
        for (int i = 0; i < bestMinSpace; i++)
        {
            UpdateIt++;
            if (UpdateIt == listUsersOD.end())
            {
                break;
            }

            if ((*UpdateIt)->GetPath(iSimulationInstanceForBatch) == actualPath->GetPath())
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
            // remove actual trip from listAffectedUsersActualPath
            if ((*itFoundTrip) == pTrip)
            {
                listAffectedUsersActualPath.erase(itFoundTrip);
                break;
            }
        }

        if (listPathFound[ShortestPathIndex]->GetPath() == pTrip->GetPath(iSimulationInstanceForBatch))
        {
            continue;
        }
        else
        {
            // check with demand !

            int si = Counter(userDistribution);

            if ((si + 1) > sizeOD)
            {
                // print error
                BOOST_LOG_TRIVIAL(info) << " New - Demand will be : " + to_string(si + 1) << endl;
                //cin.get();
            }
            else
            {
                // accept !
                pTrip->SetPath(iSimulationInstanceForBatch, listPathFound[ShortestPathIndex]->GetPath());
                // add actual trip to shortest path
                userDistribution[listPathFound[ShortestPathIndex]].push_back(pTrip);
            }
        }
    }
}


void HeuristicDualLoop::KeepBestAssignment(const vector<Trip*> &listTrip, bool IsInnerLoopConverged)
{
    if (m_iInnerloopIndex == m_iMaxInnerloopIteration - 1 || IsInnerLoopConverged)
    {
        m_bChangeAssignment = true;
        BOOST_LOG_TRIVIAL(info) << "Skip assignment to simulate traffic";
    }
}

void HeuristicDualLoop::GroupCloseShortestPath(const std::map<SymuCore::SubPopulation*, DNA> & shortestpathCandidates)
{
	// First loop on SubPopulations
	for (map<SubPopulation*, DNA>::const_iterator itAffectedTripsSubPopulation = shortestpathCandidates.begin(); itAffectedTripsSubPopulation != shortestpathCandidates.end(); ++itAffectedTripsSubPopulation)
	{
		SubPopulation* pSubPopulation = itAffectedTripsSubPopulation->first;
		mapByOriginDestination< vector<boost::shared_ptr<ValuetedPath>> >& mapShortestPathAtSubPopulation = m_mapOriginalShortestPathResults[pSubPopulation];

		// Second loop on Origins
		for (map<Origin*, map<Destination*, Chromosome> >::const_iterator itAffectedTripsOrigin = itAffectedTripsSubPopulation->second.chromosomes.begin(); itAffectedTripsOrigin != itAffectedTripsSubPopulation->second.chromosomes.end(); ++itAffectedTripsOrigin)
		{
			Origin * pOrigin = itAffectedTripsOrigin->first;
			mapByDestination< vector<boost::shared_ptr<ValuetedPath>> >& mapShortestPathAtOrigin = mapShortestPathAtSubPopulation[pOrigin];

			// Assignment loop
			for (map<Destination*, Chromosome>::const_iterator itAffectedTripsDestination = itAffectedTripsOrigin->second.begin(); itAffectedTripsDestination != itAffectedTripsOrigin->second.end(); ++itAffectedTripsDestination)
			{
				Destination* pDestination = itAffectedTripsDestination->first;
				vector<boost::shared_ptr<ValuetedPath>>& listPathsFound = mapShortestPathAtOrigin[pDestination];

				if (listPathsFound.size() > 0)
				{
					double ShortestPathCost = listPathsFound[0]->GetCost();
					const Chromosome & chromosome = itAffectedTripsDestination->second;

					//Identify all Shortest Paths
					for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*> >::const_iterator itCurrentValuetedPath = chromosome.trips.begin(); itCurrentValuetedPath != chromosome.trips.end(); itCurrentValuetedPath++)
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

void HeuristicDualLoop::Decision()
{
    if (m_MHType == MHT_SimulatedAnnealing)
    {
        string s = "\n";

        // decision -----------------------------------------------------------------------------
        // no solution is generate (so all solutions were simulate) / and we are not in gas case.
        double ffS2;

        if (m_statu_SA != 0)
        {
			if (m_statu_SA == 1) { Decision_LIQUID_Case(ffS2); }
            else if (m_statu_SA == 2)	{ Decision_SOLID_Case(ffS2); }
        }
        else
        {
            // just one parallel instance in this case
            m_mapOriginalShortestPathResults = m_mapNewShortestPathResults[0];
			GroupCloseShortestPath(m_DNA_RandSolu);

            // Gas. * develop that later too.
            ffS2 = ComputeGlobalGap(m_mapActualGlobalGap, m_mapOriginalShortestPathResults, 0); // 0 because in gas, we just have one simulation instance

            bool bAccept = false;
            if (ffS2 > m_ffS1)
            {
                double dE = ffS2 - m_ffS1; double T = m_T0 / (log(1.0 + (m_iInnerloopIndex + 1.0))); double P = exp(-dE / T); // not need to use ;
                double r = (double)RandomInteger(0, 1000) / (1000.0); s += "P : " + to_string(P) + " / r : " + to_string(r) + " / dE  : " + to_string(dE) + " T :  " + to_string(T) + "\n";

                //Y: compare two solutions
                bAccept = r <= P;
            }
            else
            {
                bAccept = true;
            }

            if (bAccept)
            {
                s += "we accept the new Solution ! \n";
                m_DNA_OriginalSolution = m_DNA_RandSolu;
            }
        }

        s += "-------------------------------------\n";
        s += "fitness of solution S1 is " + to_string(m_ffS1) + "\n";	s += "fitness of solution S2 is " + to_string(ffS2) + "\n";
        s += "-------------------------------------\n";

        BOOST_LOG_TRIVIAL(info) << s << endl;
        //BOOST_LOG_TRIVIAL(info) << to_string(m_iInnerloopIndex) + "/" + to_string(m_iOuterloopIndex) + " / " << endl;

        // cleaning the DNAs
        m_DNA_GB_Solution.clear();
        m_DNA_MSA_Solution.clear();
        m_DNA_Randomize_GB.clear();
        m_DNA_Randomize_MSA.clear();
        m_DNA_RandSolu.clear();
    }
	else
	{
		m_mapOriginalShortestPathResults = m_mapNewShortestPathResults[bestsolKey];
		GroupCloseShortestPath(popuDnaa.GetSolution(bestsolKey).m_DNABySubPop);
	}
}

void HeuristicDualLoop::ClassicMSAByYounes(int iSimulationInstanceForBatch, std::map<boost::shared_ptr<SymuCore::ValuetedPath>, std::vector<SymuCore::Trip *>, CompareValuetedPath> &mapValuetedPath, std::vector<boost::shared_ptr<SymuCore::ValuetedPath>>& listPathFound, const std::vector<SymuCore::Trip*>& listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop)
{
    for (map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
    {
        if (find(listPathFound.begin(), listPathFound.end(), iterPath->first) == listPathFound.end()) //not on a shortest path
        {
            size_t NumUser = iterPath->second.size();
            int NumSwap;

            if (iterPath->first->GetCost() == numeric_limits<double>::infinity())//handle path that became impossible
            {
                NumSwap = (int)NumUser;
            }
            else
            {
                NumSwap = (int)floor((NumUser * (StepSize / (2.0 + 1.0))));
            }

            UnifSwapByYounes(iSimulationInstanceForBatch, NumSwap, mapValuetedPath, iterPath->first, listPathFound, listTripsByOD, bIsBRUE, dbDefaultSigmaConvenience, dbMaxRelativeCostThresholdOuterloop);
        }
    }
}

void HeuristicDualLoop::GapBasedMSAByYounes(int iSimulationInstanceForBatch, map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath, vector<boost::shared_ptr<ValuetedPath>> &listPathFound, vector<Trip*>& listTripsByOD, int StepSize, bool bIsBRUE, double dbDefaultSigmaConvenience, double dbMaxRelativeCostThresholdOuterloop)
{
    boost::shared_ptr<ValuetedPath> minusShortestPath = listPathFound[0];//the first is always the minimum shortest path
    double NormValue = 0.0;

    int NumSwap;
    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = mapValuetedPath.begin(); iterPath != mapValuetedPath.end(); ++iterPath)
    {
        boost::shared_ptr<ValuetedPath> currentValuetedPath = iterPath->first;
        if (find(listPathFound.begin(), listPathFound.end(), currentValuetedPath) != listPathFound.end()) //already on a shortest path
        {
            continue;
        }
        if (currentValuetedPath->GetCost() == numeric_limits<double>::infinity())//handle path that became impossible
        {
            NumSwap = (int)iterPath->second.size();
        }
        else
        {
            NumSwap = min((int)iterPath->second.size(), (int)floor((iterPath->second.size()) * ((1.0 / (StepSize + 1.0))*((currentValuetedPath->GetCost() - minusShortestPath->GetCost()) / currentValuetedPath->GetCost()))));
        }
        NumSwap = max(0, (int)(NumSwap));

        UnifSwapByYounes(iSimulationInstanceForBatch, NumSwap, mapValuetedPath, currentValuetedPath, listPathFound, listTripsByOD, bIsBRUE, dbDefaultSigmaConvenience, dbMaxRelativeCostThresholdOuterloop);
    }
}

//Y :: -------------- basic methods : ---------------------------
// Y: ------------------------------

//------------------------------

int MinInteger(int a, int b)
{
    if (a < b) return a;
    return b;
}

vector<int> GenerateRandomVect(int l, int val)
{
    vector<int> V = vector<int>();

    for (int i = 0; i < (l - 1); i++)
    {
        int v = RandomInteger(0, val); V.push_back(v); val -= v;
    }

    V.push_back(val);

    return V;
}

int GetIndex(int v, vector<int> V)
{
    int idx = -1;
    int bound = 0;

    for (int i = 0; i < V.size(); i++)
    {
        if (v >= bound && v < (bound + V[i])) { return i; }

        bound += V[i];
    }

    if (v == bound) return ((int)V.size() - 1);

    return idx;
}

int GetIndex(float f, int l)
{
    for (int i = 1; i <= l; i++)
    {
        float bound = ((float)i / (float)l);
        if (f < bound) return (i - 1);
    }

    return (l - 1);
}

int GetIndexShrtPath(vector<boost::shared_ptr<SymuCore::ValuetedPath>> listPathFound)
{
    int pMin = -1;
    double minValue = DBL_MAX;
    for (int i = 0; i < listPathFound.size(); i++)
    {
        boost::shared_ptr<SymuCore::ValuetedPath> vP = listPathFound[i];
        double value = vP->GetCost();
        if (value < minValue)
        {
            minValue = value;
            pMin = i;
        }
    }

    return pMin;
}

int bestOf(double a, double b, double c)
{
    //Y try to optimize that later !
    if (a <= b && a <= c) return 0;
    if (b <= a && b <= c) return 1;
    if (c <= a && c <= b) return 2;

    return -1;
}

int bestOf(double a, double b, double c, double d)
{
    //Y try to optimize that later !
    if (a <= b && a <= c && a <= d) return 0;
    if (b <= a && b <= c && b <= d) return 1;
    if (c <= a && c <= b && c <= d) return 2;
    if (d <= a && d <= b && d <= c) return 3;

    return -1;
}

int HeuristicDualLoop::Counter(map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > m_mapValPath)
{
    int counter = 0;
    for (map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >::iterator iterPath = m_mapValPath.begin(); iterPath != m_mapValPath.end(); ++iterPath)
    {
        int NumUser = (int)iterPath->second.size();
        counter += NumUser;
    }

    return counter;
}

void HeuristicDualLoop::InitDNA(mapBySubPopulationOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > > mapAffectedTrips, std::map<SubPopulation*, DNA> & DNA_Solution)
{
    for (mapBySubPopulationOriginDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >::iterator itShortestPathSubPopulation = m_mapOriginalShortestPathResults.begin(); itShortestPathSubPopulation != m_mapOriginalShortestPathResults.end(); ++itShortestPathSubPopulation)
    {
        DNA & DNA_SolutionForSubpop = DNA_Solution[itShortestPathSubPopulation->first];
        DNA_SolutionForSubpop.Reset();

        SubPopulation* pSubPopulation = itShortestPathSubPopulation->first;
        mapByOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& AffectedTripsAtSubPopulation = mapAffectedTrips[pSubPopulation];

        for (mapByOriginDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >::iterator itShortestPathOrigin = itShortestPathSubPopulation->second.begin(); itShortestPathOrigin != itShortestPathSubPopulation->second.end(); ++itShortestPathOrigin)
        {
            Origin * pOrigin = itShortestPathOrigin->first;
            mapByDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& AffectedTripsAtOrigin = AffectedTripsAtSubPopulation[pOrigin];

            for (mapByDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >::iterator itShortestPathDestination = itShortestPathOrigin->second.begin(); itShortestPathDestination != itShortestPathOrigin->second.end(); ++itShortestPathDestination)
            {
                Destination* pDestination = itShortestPathDestination->first;
                map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > mapValuetedPath = AffectedTripsAtOrigin[pDestination];

                // create a chromosome.
                Chromosome chr; chr.SetTrips(mapValuetedPath); DNA_SolutionForSubpop.AddChromosome(pOrigin, pDestination, chr);

            }
        }
    }
}

void HeuristicDualLoop::GetMapValutedFromChr(int iSimulationInstanceForBatch, Chromosome chr, map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath, const vector<boost::shared_ptr<SymuCore::ValuetedPath> > & listPathsFound)
{
    const map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > mapCopy = mapValuetedPath;
    mapValuetedPath.clear();

    for (map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>>::iterator iterPath = chr.trips.begin(); iterPath != chr.trips.end(); ++iterPath)
    {
        boost::shared_ptr<SymuCore::ValuetedPath> vP;
        for (map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>>::const_iterator iterPathOriginal = mapCopy.begin(); iterPathOriginal != mapCopy.end(); ++iterPathOriginal)
        {
            if (iterPathOriginal->first->GetPath() == iterPath->first->GetPath())
            {
                vP = iterPathOriginal->first;
                break;
            }
        }

        if (!vP)
        {
            // Case mapCopy.empty() where we fill mapValuetedPath for the first time (can happen when the number of parallel simulation instances increases from one iteration to the next)
            // Other case : new shortest path in GA (can't happen in SA since we don't add new paths inside the innerloop in SA)

            // we copy the ValuetedPaths from the mapValuetedPath, unless the path is in the shortest paths list

            assert(mapCopy.empty() || m_MHType == MHT_GeneticAlgorithm);

            for (size_t iPath = 0; iPath < listPathsFound.size(); iPath++)
            {
                if (listPathsFound[iPath]->GetPath() == iterPath->first->GetPath())
                {
                    vP = listPathsFound[iPath];
                    break;
                }
            }
            if (!vP)
            {
                vP = boost::make_shared<ValuetedPath>(*iterPath->first);
            }
        }

        vector<Trip*> VT = iterPath->second;
        mapValuetedPath[vP] = VT;

        for (size_t iTrip = 0; iTrip < VT.size(); iTrip++)
        {
            VT[iTrip]->SetPath(iSimulationInstanceForBatch, vP->GetPath());
        }
    }
}

void HeuristicDualLoop::DNAtoMap(int iSimulationInstanceForBatch, mapBySubPopulationOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& mapValuetedPath, std::map<SubPopulation*, DNA> DNA_Solution)
{
    for (std::map<SubPopulation*, DNA>::const_iterator iterSubpop = DNA_Solution.begin(); iterSubpop != DNA_Solution.end(); ++iterSubpop)
    {
        mapByOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > > & mapForSubpop = mapValuetedPath[iterSubpop->first];

        if (mapForSubpop.size() > 0 && iterSubpop->second.chromosomes.size()>0) { mapForSubpop.clear(); }

        for (std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, Chromosome> >::const_iterator it = iterSubpop->second.chromosomes.begin(); it != iterSubpop->second.chromosomes.end(); ++it)
        {
            Origin* pOrigin = it->first;
            map<SymuCore::Destination*, Chromosome> MapDestChrs = it->second;
            for (map<SymuCore::Destination*, Chromosome>::iterator itDest = MapDestChrs.begin(); itDest != MapDestChrs.end(); ++itDest)
            {
                Destination* pDest = itDest->first;
                Chromosome chr = itDest->second;
                map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath >& mapValuetedPath = mapForSubpop[pOrigin][pDest];

                GetMapValutedFromChr(iSimulationInstanceForBatch, chr, mapValuetedPath, m_mapOriginalShortestPathResults[iterSubpop->first][pOrigin][pDest]);
            }
        }
    }
}

int GetRandomElement(vector<int>  L)
{
    if (L.size() == 0) { return -1; }
    int p = RandomInteger(0, (int)L.size() - 1);
    return p;
}


//Y :: --------------End of basic Methods ---------------------------------------------
//--------------------------------------------------------------------------------------

void HeuristicDualLoop::Decision_LIQUID_Case(double& ffS2)
{
    int a = bestOf(m_ffS1, m_ffRandMSA, m_ffRandGB);

    if (a > 0)
    {
        // accept new solution
        if (a == 1)
        {
            // rand + msa
            ffS2 = m_ffRandMSA; //DNAtoMap(m_mapAffectedTrips, m_DNA_Randomize_MSA);
            m_DNA_OriginalSolution = m_DNA_Randomize_MSA;
            m_mapOriginalShortestPathResults = m_mapNewShortestPathResults[1];
			GroupCloseShortestPath(m_DNA_Randomize_MSA);
        }
        else
        {
            //rand+ gb
            ffS2 = m_ffRandGB; //DNAtoMap(m_mapAffectedTrips, m_DNA_Randomize_GB);
            m_DNA_OriginalSolution = m_DNA_Randomize_GB;
            m_mapOriginalShortestPathResults = m_mapNewShortestPathResults[2];
			GroupCloseShortestPath(m_DNA_Randomize_GB);
        }
    }
    else // previouse one is best. decision
    {
        // decision
        bool RdmMsaIsBetter = (m_ffRandMSA <= m_ffRandGB);
        if (RdmMsaIsBetter)	{ ffS2 = m_ffRandMSA; }
        else { ffS2 = m_ffRandGB; }
        double dE = ffS2 - m_ffS1; double T = m_T0 / (log(1.0 + (m_iInnerloopIndex + 1.0))); double P = exp(-dE / T); // not need to use ;
        double r = (double)RandomInteger(0, 1000) / (1000.0); //s += "P : " + to_string(P) + " / r : " + to_string(r) + " / dE  : " + to_string(dE) + " T :  " + to_string(T) + "\n";

        //Y: compare two solutions
        if (r <= P)
        {
            // change that.
            if (RdmMsaIsBetter)	{
                m_DNA_OriginalSolution = m_DNA_Randomize_MSA; /*DNAtoMap(m_mapAffectedTrips, m_DNA_Randomize_MSA);*/ 
                m_mapOriginalShortestPathResults = m_mapNewShortestPathResults[1];
				GroupCloseShortestPath(m_DNA_Randomize_MSA);
            }
            else
            {
                m_DNA_OriginalSolution = m_DNA_Randomize_GB; /*DNAtoMap(m_mapAffectedTrips, m_DNA_Randomize_GB);*/ 
                m_mapOriginalShortestPathResults = m_mapNewShortestPathResults[2];
				GroupCloseShortestPath(m_DNA_Randomize_GB);
            }
        }
        else
        {
            //get original
            //PrintMapPointers(m_mapAffectedTrips);

            // copy from map to dna
            //DNAtoMap(m_mapAffectedTrips, m_DNA_OriginalSolution);
            //PrintMapPointers(m_mapAffectedTrips);
            m_mapOriginalShortestPathResults = m_mapNewShortestPathResults[0];
			GroupCloseShortestPath(m_DNA_OriginalSolution);
        }
    }
}

void HeuristicDualLoop::Decision_SOLID_Case(double& ffS2)
{
    int a = bestOf(m_ffS1, m_ffMSASol, m_ffGBSol);
    if (a > 0)
    {
        // accept new solution
        if (a == 1)
        {
            // msa // change that . (copy)
            ffS2 = m_ffMSASol; //DNAtoMap(m_mapAffectedTrips, m_DNA_MSA_Solution);
            m_DNA_OriginalSolution = m_DNA_MSA_Solution;
            m_mapOriginalShortestPathResults = m_mapNewShortestPathResults[0];
			GroupCloseShortestPath(m_DNA_MSA_Solution);
        }
        else
        {
            // gb // change that . (copy)
            ffS2 = m_ffGBSol; //DNAtoMap(m_mapAffectedTrips, m_DNA_GB_Solution);
            m_DNA_OriginalSolution = m_DNA_GB_Solution;
            m_mapOriginalShortestPathResults = m_mapNewShortestPathResults[1];
			GroupCloseShortestPath(m_DNA_GB_Solution);
        }
    }
    else // previouse one is best. decision
    {
        // decision
        bool RdmMsaIsBetter = (m_ffMSASol <= m_ffGBSol);
        if (RdmMsaIsBetter)	{ ffS2 = m_ffMSASol; }
        else { ffS2 = m_ffGBSol; }
        double dE = ffS2 - m_ffS1; double T = m_T0 / (log(1.0 + (m_iInnerloopIndex + 1.0))); double P = exp(-dE / T); // not need to use ;

        //Y: to excute the else part
        double r = (double)RandomInteger(0, 1000) / (1000.0); //s += "P : " + to_string(P) + " / r : " + to_string(r) + " / dE  : " + to_string(dE) + " T :  " + to_string(T) + "\n";

        //Y: compare two solutions
        if (r <= P)
        {
            if (RdmMsaIsBetter)
            {
                m_DNA_OriginalSolution = m_DNA_MSA_Solution;/*DNAtoMap(m_mapAffectedTrips, m_DNA_MSA_Solution);*/
                m_mapOriginalShortestPathResults = m_mapNewShortestPathResults[0];
				GroupCloseShortestPath(m_DNA_MSA_Solution);
            }
            else	 
            {
                m_DNA_OriginalSolution = m_DNA_GB_Solution;/*DNAtoMap(m_mapAffectedTrips, m_DNA_GB_Solution);*/
                m_mapOriginalShortestPathResults = m_mapNewShortestPathResults[1];
				GroupCloseShortestPath(m_DNA_GB_Solution);
            }
        }
    }
}


// GA added functions
/////////////////////////////////

void Print__(map<boost::shared_ptr<ValuetedPath>, vector<Trip*>> M)
{
    string s = "" + to_string(M.size());
    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>>::iterator iterPath = M.begin(); iterPath != M.end(); ++iterPath)
    {
        s += "{" + to_string(iterPath->second.size()) + "} ";
    }

    BOOST_LOG_TRIVIAL(info) << s << endl;
}


void PrintDNAA(HeuristicDualLoop::DNAForSubpopsAndFitness dnaA, int m)
{
    string s = "";
    // for each chromosome print.
    for (map<SubPopulation*, HeuristicDualLoop::DNA>::iterator iterSubPop = dnaA.m_DNABySubPop.begin(); iterSubPop != dnaA.m_DNABySubPop.end(); ++iterSubPop)
    {
        for (map<Origin*, map<Destination*, HeuristicDualLoop::Chromosome> >::iterator itOrigin = iterSubPop->second.chromosomes.begin(); itOrigin != iterSubPop->second.chromosomes.end(); ++itOrigin)
        {
            Origin * pOrigin = itOrigin->first;

            for (map<Destination*, HeuristicDualLoop::Chromosome> ::iterator itDestination = itOrigin->second.begin(); itDestination != itOrigin->second.end(); ++itDestination)
            {
                Destination* pDestination = itDestination->first;
                HeuristicDualLoop::Chromosome chrA = itDestination->second;
                Print__(chrA.trips);
            }
        }
    }
}

map<int, int> VectorToMap(vector<int> T)
{
    map<int, int> M = map<int, int>();
    for (int val : T) { M[val] = -1; }
    return M;
}

int IdxWorstPath(vector<boost::shared_ptr<ValuetedPath>> V)
{
    int idx_worst_path = -1;
    double worstcost = -1.0;
    for (int i = 0; i < V.size(); i++)
    {
        boost::shared_ptr<ValuetedPath> vP = V[i];
        double cost = vP->GetCost();
        if (worstcost < cost)
        {
            idx_worst_path = i; worstcost = cost;
        }
    }

    return idx_worst_path;
}

int BetterPath(int i, int j, vector<boost::shared_ptr<ValuetedPath> > V)
{
    if (V[i]->GetCost() <= V[j]->GetCost()) { return i; }
    else								  { return j; }
}

vector<boost::shared_ptr<ValuetedPath>> GetValuetedPaths(map<boost::shared_ptr<ValuetedPath>, vector<Trip*>> M)
{
    vector<boost::shared_ptr<ValuetedPath>> Keys;
    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>>::iterator itPaths = M.begin(); itPaths != M.end(); ++itPaths) { Keys.push_back(itPaths->first); }
    return Keys;
}

vector<int> GetIDs(vector<Trip*> T)
{
    vector<int> V;
    for (int i = 0; i < T.size(); i++)
    {
        V.push_back(T[i]->GetID());
    }

    return V;
}

void PrintVectorIntegers(vector<int> T, string sep)
{
    string s = "";
    for (int i = 0; i < T.size(); i++)
    {
        int val = T[i];
        s += to_string(val) + "" + sep;
    }

    BOOST_LOG_TRIVIAL(info) << s << endl;
}

void HeuristicDualLoop::PrintChromosomeA(Chromosome chrA, int m)
{
    Print__(chrA.trips);
}

void HeuristicDualLoop::PrintChromosomeAWithDetails(Chromosome chrA)
{
    for (map <boost::shared_ptr<ValuetedPath>, vector<Trip*>>::iterator it = chrA.trips.begin(); it != chrA.trips.end(); ++it)
    {
        PrintVectorIntegers(GetIDs(it->second), " ; ");
    }
}

void GetSetIDsTripsNotAffected(map<int, int> M, vector<int>& TripsNotAffected)
{
    TripsNotAffected.clear();
    for (map<int, int>::iterator it = M.begin(); it != M.end(); ++it)
    {
        if (it->second == -1) TripsNotAffected.push_back(it->first);
    }
}


void HeuristicDualLoop::RemoveDuplicateTrips(Chromosome& f1, vector<boost::shared_ptr<ValuetedPath> > P, vector<int> IDTrips, vector<int>& TripsNotAffected, map<int, int>& counter)
{
    // we need here list of all trips by this OD.
    // generate the equivalent map; [key = id of trip ; value = id of Valueted_path]
    map<int, int> M = VectorToMap(IDTrips);

    // set all trips of paths idx in map
    for (int i = 0; i < P.size(); i++)
    {
        vector<Trip*> T = f1.trips[P[i]];

        for (Trip * t : T)
        {
            // check if trip eist before with value <> -1
            int value = M[t->GetID()];
            if (value == -1) { value = i; }
            else			 { value = BetterPath(i, value, P); }

            M[t->GetID()] = value;
        }
    }

    //	BOOST_LOG_TRIVIAL(info) << " MAP TRIPS : "; PrintMapInt(M,"#");

    // print chromosome here.
    //BOOST_LOG_TRIVIAL(info) << " Before Remove Duplication : "; PrintChromosomeA(f1, -1);

    // Update (Remove Duplication)
    // For-Loop
    for (int i = 0; i < P.size(); i++)
    {
        boost::shared_ptr<ValuetedPath> vP = P[i]; vector<Trip*> T = f1.trips[vP]; counter[i] = 0;

        for (int j = 0; j < T.size(); j++)
        {
            Trip * t = T[j];
            int idxAffectedPath = M[t->GetID()];
            // remove
            if (idxAffectedPath != i)
            {
                //BOOST_LOG_TRIVIAL(info) << " Remove it ! "+to_string(t.GetID()); 
                T.erase(T.begin() + j); j--;
                counter[i]++;
            }
        }
        f1.trips[vP].clear(); f1.SetTrips(vP, T);
    }

    BOOST_LOG_TRIVIAL(info) << " After Remove Duplication : "; PrintChromosomeA(f1, -1);
    // get set of trips not affected.
    // use M[key] = -1;

    GetSetIDsTripsNotAffected(M, TripsNotAffected);
    BOOST_LOG_TRIVIAL(info) << " Trips Not Affected : "; PrintVectorIntegers(TripsNotAffected, " ; ");

    // check point ! 
    //cin.get();
}

void SetTripsInVector(map<int, Trip*> M, vector<int> NotAffected, vector<Trip*>& R)
{
    // we need to check here (if t is null or no)

    for (int key : NotAffected)
    {
        Trip *t = M[key]; R.push_back(t);
    }
}

void SetTripsDependOnMapAndRemove(vector<Trip*>& R, map<int, int> counter, map<int, vector<Trip*>>& SelectedTrips)
{
    for (map<int, int>::iterator it = counter.begin(); it != counter.end(); ++it)
    {
        int pathIdx = it->first;
        int numberTripsOfPath = it->second;
        SelectedTrips[pathIdx] = vector<Trip*>();

        while (numberTripsOfPath > 0 && R.size()>0)
        {
            Trip* t = R[0]; SelectedTrips[pathIdx].push_back(t);
            //BOOST_LOG_TRIVIAL(info) << "Trip " + to_string(t.GetID()) + " is now in " + to_string(pathIdx);
            R.erase(R.begin() + 0);
            numberTripsOfPath--;
        }
    }
}

void HeuristicDualLoop::SetSelectedTripsDependOnMap(Chromosome& f, vector<boost::shared_ptr<ValuetedPath>> VPs, map<int, vector<Trip*>> SelectedTrips)
{
    for (map<int, vector<Trip*>>::iterator it = SelectedTrips.begin(); it != SelectedTrips.end(); ++it)
    {
        int idx = it->first; boost::shared_ptr<ValuetedPath> vp = VPs[idx]; vector<Trip*> T = it->second;
        for (Trip * t : T) { f.SetTrip(vp, t); }
    }
}


void HeuristicDualLoop::FixDNAA(Chromosome c, Chromosome& f, vector<boost::shared_ptr<ValuetedPath> > VPaths, boost::shared_ptr<ValuetedPath> worstPath, vector<int>& IDTrips)
{
    // for 1st solution . {change that to a method.}
    vector<int> NotAffected = vector<int>();

    // add here a counter
    map<int, int> counter = map<int, int>();

    //Generate the Map of trips by Idx of Path
    RemoveDuplicateTrips(f, VPaths, IDTrips, NotAffected, counter);

    // add rest of trips to worst path
    vector<Trip*> R; SetTripsInVector(c.GetAllTrips(), NotAffected, R);

    // size of R.
    //BOOST_LOG_TRIVIAL(info) << "Size of R is : " + to_string(R.size());

    // print counter here !
    //PrintMapInt(counter, " # ");

    // get Selected trips to set them depend on counter.
    map<int, vector<Trip*>> SelectedTrips;
    SetTripsDependOnMapAndRemove(R, counter, SelectedTrips);

    // print these trips
    //BOOST_LOG_TRIVIAL(info) << "Size of SelectedTrips is : " + to_string(SelectedTrips.size());
    SetSelectedTripsDependOnMap(f, VPaths, SelectedTrips);

    // put Trips of R in worst path.
    f.trips[worstPath].clear(); f.SetTrips(worstPath, R); BOOST_LOG_TRIVIAL(info) << "Chromosome f after add worst path. "; PrintChromosomeA(f, -1);
    PrintChromosomeAWithDetails(f);

    //cin.get();
}

void HeuristicDualLoop::CrossOver_PairChromosomes(Chromosome c1, Chromosome c2, Chromosome& f1, Chromosome& f2, vector<boost::shared_ptr<ValuetedPath>> V, vector<bool> B)
{
    for (int i = 0; i < V.size(); i++)
    {
        boost::shared_ptr<ValuetedPath> vP = V[i];
        //BOOST_LOG_TRIVIAL(info) << " PATH  " + to_string(i) + " : " + to_string(B[i]);
        // get trips of 
        vector<Trip*> T1 = c1.trips[vP];
        vector<Trip*> T2 = c2.trips[vP];
        // if its true :
        if (B[i])
        {
            f1.SetTrips(vP, T1); f2.SetTrips(vP, T2);
        }
        else
        {
            f1.SetTrips(vP, T2); f2.SetTrips(vP, T1);
        }

    }
    // print here .
    //PrintChromosomeA(f1, -1); PrintChromosomeA(f2, -1);

}

vector<bool> BinaryMask(int l)
{
    vector<bool> B = vector<bool>();

    for (int i = 0; i < l; i++)
    {
        int v = RandomInteger(0, 100);
        B.push_back(v <= 50);
    }

    return B;
}

void HeuristicDualLoop::CrossOver_PairChromosomes(Chromosome c1, Chromosome c2, Chromosome& f1, Chromosome& f2, vector<int>& IDTrips)
{
    f1.trips.clear(); f2.trips.clear();

    // do crossover and store them in f1,f2

    // get set of valuated paths.
    //map<ValuetedPath, vector<Trip>> trips = map<ValuetedPath, vector<Trip>>();
    vector<boost::shared_ptr<ValuetedPath>> VPaths = GetValuetedPaths(c1.trips);

    if (VPaths.size() <= 2)
    {
        // return same 
        f1 = c1; f2 = c2;
    }
    else
    {
        //BOOST_LOG_TRIVIAL(info) << " VPATHS > 2 : " + VPaths.size();
        // get worst path. 
        int indWorstPath = IdxWorstPath(VPaths);

        boost::shared_ptr<ValuetedPath> worstPath = VPaths[indWorstPath];
        //BOOST_LOG_TRIVIAL(info) << " WORST PATH : ";

        // remove it.
        VPaths.erase(VPaths.begin() + indWorstPath);
        //BOOST_LOG_TRIVIAL(info) << " WORST WAS REMOVED : ";

        // binary mask as vector.
        vector<bool> B = BinaryMask((int)VPaths.size());
        //BOOST_LOG_TRIVIAL(info) << " BINARY MASK GENERATED : ";

        // do crossover using binarymask.
        //BOOST_LOG_TRIVIAL(info) << "Before Crossover Pair Chromosome : ";
        //PrintChromosomeA(c1, -1); PrintChromosomeA(c2, -1);

        //BOOST_LOG_TRIVIAL(info) << "After Crossover Pair Chromosome : ";
        CrossOver_PairChromosomes(c1, c2, f1, f2, VPaths, B);

        //------------------------------------------------

        FixDNAA(c1, f1, VPaths, worstPath, IDTrips);
        FixDNAA(c1, f2, VPaths, worstPath, IDTrips);

        //------------------------------------------------

    }

}

void HeuristicDualLoop::CrossOver_PairDNAA(DNAForSubpopsAndFitness s1, DNAForSubpopsAndFitness s2, vector<DNAForSubpopsAndFitness>& Childs)
{
    // do crossover 
    DNAForSubpopsAndFitness NewSol_1, NewSol_2;
    BOOST_LOG_TRIVIAL(info) << "Crossover Pair DNAA : ";
    for (std::map<SymuCore::SubPopulation*, DNA>::iterator iterSubPop = s1.m_DNABySubPop.begin(); iterSubPop != s1.m_DNABySubPop.end(); ++iterSubPop)
    {
        for (map<Origin*, map<Destination*, Chromosome>>::iterator itOrg = iterSubPop->second.chromosomes.begin(); itOrg != iterSubPop->second.chromosomes.end(); ++itOrg)
        {
            Origin* pOrigin = itOrg->first;
            for (map<Destination*, Chromosome>::iterator itDest = itOrg->second.begin(); itDest != itOrg->second.end(); ++itDest)
            {
                Destination* pDestination = itDest->first;

                // get chromosomes
                Chromosome c1 = itDest->second;
                Chromosome c2 = s2.m_DNABySubPop[iterSubPop->first].chromosomes[pOrigin][pDestination];

                //PrintChromosomeA(c1, -1); PrintChromosomeA(c2, -1);

                // do crossover here !

                if (c1.trips.size() <= 2)
                {
                    // do nothing . [add each chromosome to New Solution]
                    NewSol_1.m_DNABySubPop[iterSubPop->first].AddChromosome(pOrigin, pDestination, c1);
                    NewSol_2.m_DNABySubPop[iterSubPop->first].AddChromosome(pOrigin, pDestination, c2);
                }
                else
                {
                    //BOOST_LOG_TRIVIAL(info) << "[" + pOrigin->getStrNodeName() + " ; "+pDestination->getStrNodeName()+"]";
                    // we need here list of all trips by this OD.
                    vector<int> TripsIDs = c1.GetAllTripsIDs();
                    // print here to be sure .
                    //BOOST_LOG_TRIVIAL(info) << "Trips IDs "; PrintVectorIntegers(TripsIDs, " ; ");	cin.get();

                    Chromosome f1, f2;
                    CrossOver_PairChromosomes(c1, c2, f1, f2, TripsIDs);

                    // add chromosome to solutions.
                    NewSol_1.m_DNABySubPop[iterSubPop->first].AddChromosome(pOrigin, pDestination, f1);
                    NewSol_2.m_DNABySubPop[iterSubPop->first].AddChromosome(pOrigin, pDestination, f2);
                }
            }
        }
    }

    // print the new solutions ..
    /*
    BOOST_LOG_TRIVIAL(info) << "Solution 1 : "; PrintDNAA(s1, -1);
    BOOST_LOG_TRIVIAL(info) << "Solution 2 : "; PrintDNAA(s2, -1);

    BOOST_LOG_TRIVIAL(info) << "Child 1 : "; PrintDNAA(NewSol_1, -1);
    BOOST_LOG_TRIVIAL(info) << "Child 2 : "; PrintDNAA(NewSol_2, -1);
    */

    if (!NewSol_1.IsAvailable()) { BOOST_LOG_TRIVIAL(info) << "Solution 1 is not available"; cin.get(); }
    if (!NewSol_2.IsAvailable()) { BOOST_LOG_TRIVIAL(info) << "Solution 2 is not available"; cin.get(); }


    //add new childs to the vector CHILDS
    Childs.push_back(NewSol_1); Childs.push_back(NewSol_2);
}

vector<int> HeuristicDualLoop::KeysOfPopu(PopuDNA P)
{
    vector<int> keys;
    for (map<int, DNAForSubpopsAndFitness>::iterator it = P.Solutions.begin(); it != P.Solutions.end(); ++it)
    {
        int key = it->first;
        keys.push_back(key);
    }

    return keys;
}

void HeuristicDualLoop::CrossOver_Inner(int to_Select)
{
    // the dnaa solutions of the crossover Inner.
    vector<DNAForSubpopsAndFitness> Childs;

    // i need to put here a selected population. (they are stored in : SeletedPopuDnaa)
    Selection(to_Select);

    // let say it is 4 elemets of Population.

    // for each two solution
    int m = SeletedPopuDnaa.Size() / 2; //BOOST_LOG_TRIVIAL(info) << "m  =  "+to_string(m); 

    // get set of solution 
    // for each pair :
    vector<int> K = KeysOfPopu(SeletedPopuDnaa); //BOOST_LOG_TRIVIAL(info) << "K size  =  " + to_string(K.size());
    for (int i = 0; i < m; i++)
    {
        // select first solution : 
        int r = RandomInteger(0, (int)K.size() - 1); /* BOOST_LOG_TRIVIAL(info) << "r  =  " + to_string(r); */int key = K[r]; DNAForSubpopsAndFitness s1 = popuDnaa.GetSolution(key); K.erase(K.begin() + r);
        // select second solution :
        r = RandomInteger(0, (int)K.size() - 1);	/* BOOST_LOG_TRIVIAL(info) << "r  =  " + to_string(r); */	key = K[r]; DNAForSubpopsAndFitness s2 = popuDnaa.GetSolution(key); K.erase(K.begin() + r);

        CrossOver_PairDNAA(s1, s2, Childs);
    }

    //BOOST_LOG_TRIVIAL(info) << " Childs size = " + to_string(Childs.size());
    popuDnaa.AddSolutions(Childs);

    // print Ga Population.
    //BOOST_LOG_TRIVIAL(info) << "POPULATION GA INNER "; PrintPopu(popuDnaa);

    //cin.get();


}

void HeuristicDualLoop::Replacement(int max_)
{
    // max size is max of ((max_, popuDnaa.Size())
    max_ = min(max_, popuDnaa.Size());
    //BOOST_LOG_TRIVIAL(info) << "max_  = " + to_string(max_);

    // print population before 
    //BOOST_LOG_TRIVIAL(info) << "Before Sorting : size = " +to_string(popuDnaa.Size()); PrintPopu(popuDnaa);
    // get ranked map.
    map<double, int> InverseMap = map<double, int>();
    for (map<int, DNAForSubpopsAndFitness>::iterator it = popuDnaa.Solutions.begin(); it != popuDnaa.Solutions.end(); ++it)  { InverseMap[it->second.fitness] = it->first; }

    //BOOST_LOG_TRIVIAL(info) << "Map Inverse !  = " + to_string(InverseMap.size());

    // loop in inverse map until max_
    int i = 0;

    // FIRST STEP IS TO KEEP ONLY ELEMENT OF IVERSE MAP.
    vector<int> ListKeys = vector<int>();
    for (map<double, int>::iterator it = InverseMap.begin(); it != InverseMap.end(); ++it)  { ListKeys.push_back(it->second); }
    int diff = popuDnaa.Size();
    popuDnaa.KeepOnlyKeys(ListKeys);

    //BOOST_LOG_TRIVIAL(info) << "diff " +to_string((diff - popuDnaa.Size()));

    for (map<double, int>::iterator it = InverseMap.begin(); it != InverseMap.end(); ++it)
    {
        // remove from PopuDnaa.
        //BOOST_LOG_TRIVIAL(info) << "Removed // " + to_string(it->first);
        if (i >= max_) { popuDnaa.RemoveByCost(it->first); BOOST_LOG_TRIVIAL(info) << "SIZE POPU : " + to_string(popuDnaa.Size()) + " / i = " + to_string(i); } i++;
    }

    //BOOST_LOG_TRIVIAL(info) << "Map Inverse / " + to_string(InverseMap.size()) + " / Popu = " + to_string(popuDnaa.Size());

    //BOOST_LOG_TRIVIAL(info) << "After Sorting and Remove ! ; new size = "+to_string(popuDnaa.Size()); PrintPopu(popuDnaa);
}

void HeuristicDualLoop::GenerateRandomPopu(int n)
{
    popuDnaa.Clear();
    // fiill the PopuDNAA

    for (int i = 0; i < n; i++)
    {
        // generate solution and add it to the population.
        RandomSolution();
    }
}

vector<Trip*>  HeuristicDualLoop::GetSetTrips(vector<Trip*> listTripsByOD)
{
    vector<Trip*> L;
    for (Trip* tr : listTripsByOD)
    {
        L.push_back(tr);
    }

    return L;
}

HeuristicDualLoop::Chromosome RandomChromosomeA(vector<Trip*> T, vector<boost::shared_ptr<ValuetedPath>> P)
{
    map<boost::shared_ptr<ValuetedPath>, vector<Trip*>> map_ValutedPathEmpty;

    // fill this map.
    // for each Trip
    HeuristicDualLoop::Chromosome chrA;
    for (int i = 0; i < T.size(); i++)
    {
        Trip *t = T[i];

        // choose randomly a valuated path.
        int r = RandomInteger(0, (int)P.size() - 1);

        //BOOST_LOG_TRIVIAL(info) << "we choosed the path :  = " + to_string(r)+" / size = "+to_string(P.size()) << endl;
        boost::shared_ptr<ValuetedPath> vP = P[r];

        chrA.SetTrip(vP, t);
    }

    //chrA.SetTrips(map_ValutedPathEmpty);
    //PrintChromosomeA(chrA,-1); cin.get();

    return chrA;
}

vector<boost::shared_ptr<ValuetedPath> > HeuristicDualLoop::GetSetPaths(vector<boost::shared_ptr<ValuetedPath>> V)
{
    vector<boost::shared_ptr<ValuetedPath>> P = vector<boost::shared_ptr<ValuetedPath>>();
    for (boost::shared_ptr<ValuetedPath> path : V)
    {
        //ValuetedPath* path = V[i]; 
        P.push_back(path);
    }

    return P;
}

vector<boost::shared_ptr<ValuetedPath>> HeuristicDualLoop::GetValutedPointerPaths(map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath> mapValuetedPath)
{
    vector<boost::shared_ptr<ValuetedPath>> listPathsFound;
    for (map<boost::shared_ptr<ValuetedPath>, vector<Trip*>, CompareValuetedPath>::iterator itPaths = mapValuetedPath.begin(); itPaths != mapValuetedPath.end(); ++itPaths)
    {
        boost::shared_ptr<ValuetedPath> vP = itPaths->first;
        listPathsFound.push_back(vP);
    }

    return listPathsFound;
}

void HeuristicDualLoop::RandomSolution()
{
    // creat a DNA

    DNAForSubpopsAndFitness s;

    for (mapBySubPopulationOriginDestination< std::vector<boost::shared_ptr<SymuCore::ValuetedPath>> >::iterator itShortestPathSubPopulation = m_mapOriginalShortestPathResults.begin(); itShortestPathSubPopulation != m_mapOriginalShortestPathResults.end(); ++itShortestPathSubPopulation)
    {
        SubPopulation* pSubPopulation = itShortestPathSubPopulation->first;

        mapByOriginDestination< vector<Trip*> >& TripsByODAtSubPopulation = m_mapTripsByOD[pSubPopulation];
        mapByOriginDestination <int>& StepsizeByODAtSubPopulation = m_mapImprovedStepSize[pSubPopulation];
        mapByOriginDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& AffectedTripsAtSubPopulation = m_mapAffectedTrips[DEFAULT_SIMULATION_INSTANCE][pSubPopulation];

        // Second loop on Origins
        for (mapByOriginDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >::iterator itShortestPathOrigin = itShortestPathSubPopulation->second.begin(); itShortestPathOrigin != itShortestPathSubPopulation->second.end(); ++itShortestPathOrigin)
        {
            Origin * pOrigin = itShortestPathOrigin->first;
            string NomOrg = pOrigin->getStrNodeName();

            mapByDestination< vector<Trip*> >& TripsByODAtOrigin = TripsByODAtSubPopulation[pOrigin];
            mapByDestination< map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath > >& AffectedTripsAtOrigin = AffectedTripsAtSubPopulation[pOrigin];

            // Assignment loop
            for (mapByDestination< vector<boost::shared_ptr<SymuCore::ValuetedPath>> >::iterator itShortestPathDestination = itShortestPathOrigin->second.begin(); itShortestPathDestination != itShortestPathOrigin->second.end(); ++itShortestPathDestination)
            {
                Destination* pDestination = itShortestPathDestination->first;
                string NomDest = pDestination->getStrNodeName();

                map<boost::shared_ptr<SymuCore::ValuetedPath>, vector<Trip*>, CompareValuetedPath> mapValuetedPath = AffectedTripsAtOrigin[pDestination];

                // this one to update
                vector<boost::shared_ptr<SymuCore::ValuetedPath>> listPathsFound = GetValutedPointerPaths(mapValuetedPath);

                // add the shortest paths if needed
                for (size_t iSP = 0; iSP < itShortestPathDestination->second.size(); iSP++)
                {
                    bool bAlreadyHere = false;
                    for (size_t i = 0; i < listPathsFound.size() && !bAlreadyHere; i++)
                    {
                        if (listPathsFound[i]->GetPath() == itShortestPathDestination->second[iSP]->GetPath())
                        {
                            bAlreadyHere = true;
                        }
                    }
                    if (!bAlreadyHere)
                    {
                        listPathsFound.push_back(itShortestPathDestination->second[iSP]);
                    }
                }

                // get just list of paths.

                vector<boost::shared_ptr<SymuCore::ValuetedPath>> LPaths = GetSetPaths(listPathsFound);

                vector<Trip*>& listTripsByOD = TripsByODAtOrigin[pDestination];
                vector<Trip*> LTrips = GetSetTrips(listTripsByOD);

                // generate here a chromosome A:
                //Print_PathSetOf_Map();
                //BOOST_LOG_TRIVIAL(info) << "Chromosome " + NomOrg + "/" + NomDest + "/" + to_string(listPathsFound.size()) + "/" + to_string(LPaths.size());

                Chromosome chrA = RandomChromosomeA(LTrips, LPaths);
                //cin.get();

                // set chromosome A to the solution.
                s.m_DNABySubPop[pSubPopulation].AddChromosome(pOrigin, pDestination, chrA);

                // --- End of generation
                //listPathsFound = itShortestPathDestination->second;
                //BOOST_LOG_TRIVIAL(info) << "afte : set of paths : " + to_string(listPathsFound.size());
                //Print_PathSetOf_Map();
                //cin.get();
            }
        }
    }

    // print solution.

    //BOOST_LOG_TRIVIAL(info) << " Solution Print Begin "; PrintDNAA(s, -1); BOOST_LOG_TRIVIAL(info) << " Solution Print End ";

    // set s to map path
    //--- Do Process here !

    //BOOST_LOG_TRIVIAL(warning) << " print set path before copy ! ";
    //Print_PathSetOf_Map();

    popuDnaa.AddSolution(s);
}

map<Origin*, map<Destination*, bool>> BinaryMask(HeuristicDualLoop::DNA s1)
{
    map<Origin*, map<Destination*, bool>> B = map<Origin*, map<Destination*, bool>>();

    for (map<Origin*, map<Destination*, HeuristicDualLoop::Chromosome>>::iterator itOrg = s1.chromosomes.begin(); itOrg != s1.chromosomes.end(); ++itOrg)
    {
        Origin* pOrigin = itOrg->first;
        for (map<Destination*, HeuristicDualLoop::Chromosome>::iterator itDest = itOrg->second.begin(); itDest != itOrg->second.end(); ++itDest)
        {
            Destination* pDestination = itDest->first;

            int v = RandomInteger(0, 100);
            if (v <= 50) { B[pOrigin][pDestination] = true; }
            else		 { B[pOrigin][pDestination] = false; }
        }
    }

    return B;
}

void CrossOver_(HeuristicDualLoop::DNAForSubpopsAndFitness s1, HeuristicDualLoop::DNAForSubpopsAndFitness s2, vector<HeuristicDualLoop::DNAForSubpopsAndFitness>& childs)
{
    HeuristicDualLoop::DNAForSubpopsAndFitness c1, c2;

    // crossover.
    for (map<SubPopulation*, HeuristicDualLoop::DNA>::iterator iterSubPop = s1.m_DNABySubPop.begin(); iterSubPop != s1.m_DNABySubPop.end(); ++iterSubPop)
    {
        // generate binary mask.
        map<Origin*, map<Destination*, bool>> B = BinaryMask(iterSubPop->second);

        for (map<Origin*, map<Destination*, HeuristicDualLoop::Chromosome>>::iterator itOrg = iterSubPop->second.chromosomes.begin(); itOrg != iterSubPop->second.chromosomes.end(); ++itOrg)
        {
            Origin* pOrigin = itOrg->first;
            for (map<Destination*, HeuristicDualLoop::Chromosome>::iterator itDest = itOrg->second.begin(); itDest != itOrg->second.end(); ++itDest)
            {
                Destination* pDestination = itDest->first;

                bool parent = B[pOrigin][pDestination];

                HeuristicDualLoop::Chromosome chrA_1 = itDest->second;
                HeuristicDualLoop::Chromosome chrA_2 = s2.m_DNABySubPop[iterSubPop->first].chromosomes[pOrigin][pDestination];

                if (!chrA_1.IsAvailable()) { BOOST_LOG_TRIVIAL(info) << "Chr_1 is not available"; }
                if (!chrA_2.IsAvailable()) { BOOST_LOG_TRIVIAL(info) << "Chr_2 is not available"; }


                if (parent) { c1.m_DNABySubPop[iterSubPop->first].AddChromosome(pOrigin, pDestination, chrA_1); c2.m_DNABySubPop[iterSubPop->first].AddChromosome(pOrigin, pDestination, chrA_2); }
                else        { c2.m_DNABySubPop[iterSubPop->first].AddChromosome(pOrigin, pDestination, chrA_1); c1.m_DNABySubPop[iterSubPop->first].AddChromosome(pOrigin, pDestination, chrA_2); }
            }
        }
    }

    // print childs.

    /*
    BOOST_LOG_TRIVIAL(info) << "------ Parent 1 ------------------"; PrintDNAA(s1, -1);
    BOOST_LOG_TRIVIAL(info) << "------ Parent 2 ------------------"; PrintDNAA(s2, -1);
    BOOST_LOG_TRIVIAL(info) << "------ Child 1 -------------------"; PrintDNAA(c1,-1);
    BOOST_LOG_TRIVIAL(info) << "------ Child 2 -------------------"; PrintDNAA(c2,-1);
    */

    // check solution !

    if (!c1.IsAvailable()) { BOOST_LOG_TRIVIAL(info) << "C_1 is not available"; cin.get(); }
    if (!c2.IsAvailable()) { BOOST_LOG_TRIVIAL(info) << "C_2 is not available"; cin.get(); }

    childs.push_back(c1); childs.push_back(c2);


}

void HeuristicDualLoop::CrossOver(PopuDNA popu, double crossRate)
{
    // number of pairs
    int siz = (int)(crossRate*popu.Size());
    if (siz % 2 == 1) siz++;
    int m = (popu.Size() / 2);

    vector<int> keys = KeysOfPopu(popu);
    // print
    BOOST_LOG_TRIVIAL(info) << "Keys : "; PrintVectorIntegers(keys, " # ");

    // take two by two and crossover them.
    for (int i = 0; i < m; i++)
    {
        // change key (to get randomly)
        int p1 = GetRandomElement(keys); int key1 = keys[p1]; DNAForSubpopsAndFitness s1 = popu.GetSolution(key1);  keys.erase(keys.begin() + p1);
        int p2 = GetRandomElement(keys); int key2 = keys[p2]; DNAForSubpopsAndFitness s2 = popu.GetSolution(key2);  keys.erase(keys.begin() + p2);

        // crossover here !
        BOOST_LOG_TRIVIAL(info) << "p1 / p2 = (" + to_string(p1) + "/" + to_string(p2) + ")";
        BOOST_LOG_TRIVIAL(info) << "key1 / key 2 = (" + to_string(key1) + "/" + to_string(key2) + ")";

        if (s1.chromosomeSize() != s2.chromosomeSize())
        {
            //BOOST_LOG_TRIVIAL(info) << "S1 :"; PrintDNAA(s1, -1); BOOST_LOG_TRIVIAL(info) << "End - S1 :";
            //BOOST_LOG_TRIVIAL(info) << "S2 :"; PrintDNAA(s2, -1); BOOST_LOG_TRIVIAL(info) << "End - S2 :";

            BOOST_LOG_TRIVIAL(info) << "Waiiiiiiiiiiiii " + to_string(s1.chromosomeSize()) + "/" + to_string(s2.chromosomeSize());
            cin.get();
        }

        vector<DNAForSubpopsAndFitness> childs;
        CrossOver_(s1, s2, childs);

        // add childs to popu
        popuDnaa.AddSolutions(childs);
    }

}

Origin* RandomOrg(map<Origin*, map<Destination*, HeuristicDualLoop::Chromosome>> A)
{
    int i = 0;
    Origin* pStand = NULL;
    int k = RandomInteger(0, (int)A.size() - 1);
    for (map<Origin*, map<Destination*, HeuristicDualLoop::Chromosome>>::iterator itOrg = A.begin(); itOrg != A.end(); ++itOrg)
    {
        Origin* pOrg = itOrg->first;
        if (i == k)
        {
            return pOrg;
        }

        i++;
    }

    return pStand;
}

Destination* RandomDest(Origin* pOrigine, map<Origin*, map<Destination*, HeuristicDualLoop::Chromosome>> A)
{
    Destination* pStand = NULL;

    map<Origin*, map<Destination*, HeuristicDualLoop::Chromosome>>::iterator itOrg = A.find(pOrigine);
    if (itOrg != A.end())
    {
        int k = RandomInteger(0, (int)itOrg->second.size() - 1);

        int i = 0;
        for (map<Destination*, HeuristicDualLoop::Chromosome>::iterator itDest = itOrg->second.begin(); itDest != itOrg->second.end(); ++itDest)
        {
            Destination* pDest = itDest->first;
            if (i == k)
            {
                return pDest;
            }

            i++;
        }
    }

    return pStand;
}

void mutation_Dnaa(HeuristicDualLoop::DNAForSubpopsAndFitness& s, HeuristicDualLoop::PopuDNA popu)
{
    int tm = 20;
    int v = RandomInteger(0, 100);
    if (v <= tm)
    {
        // do mutation.

        // get random DNAA from population 
        HeuristicDualLoop::DNAForSubpopsAndFitness sRand = popu.GetRandom();

        for (map<SubPopulation*, HeuristicDualLoop::DNA>::iterator iterSubPop = s.m_DNABySubPop.begin(); iterSubPop != s.m_DNABySubPop.end(); ++iterSubPop)
        {
            //select random (origin, destination);
            Origin* pOrg = RandomOrg(iterSubPop->second.chromosomes);
            Destination* pDest = RandomDest(pOrg, iterSubPop->second.chromosomes);

            //BOOST_LOG_TRIVIAL(info) << " OD to Mutate  : [" + pOrg->getStrNodeName() + ";"+pDest->getStrNodeName()+"]";
            
            HeuristicDualLoop::Chromosome ChrA = sRand.m_DNABySubPop[iterSubPop->first].chromosomes[pOrg][pDest];
            //BOOST_LOG_TRIVIAL(info) << " Chromosome to swap "; PrintChromosomeA(ChrA, -2);

            // update of chromosome
            if (ChrA.trips.size() == 0)
            {
                BOOST_LOG_TRIVIAL(info) << " chrA is null !"; cin.get();
            }
            //else
            iterSubPop->second.AddChromosome(pOrg, pDest, ChrA);
        }

        //BOOST_LOG_TRIVIAL(info) << " After Mutation : "; PrintDNAA(s, -1);
    }
}

void Mutation(HeuristicDualLoop::PopuDNA& popu, double mutRate)
{
    int tm = (int)(mutRate * 100);
    int v = tm;

    // for each element
    vector<HeuristicDualLoop::DNAForSubpopsAndFitness> mutat;
    for (map<int, HeuristicDualLoop::DNAForSubpopsAndFitness>::iterator itSol = popu.Solutions.begin(); itSol != popu.Solutions.end(); ++itSol)
    {
        // do mutation
        HeuristicDualLoop::DNAForSubpopsAndFitness s = itSol->second;

        // create a copy.
        HeuristicDualLoop::DNAForSubpopsAndFitness copyS = s;
        v = RandomInteger(0, 100);
        if (v <= tm)
        {
            mutation_Dnaa(copyS, popu);
            mutat.push_back(copyS);
        }
    }

    popu.AddSolutions(mutat);

}

void PrintPopu(HeuristicDualLoop::PopuDNA popu)
{
    for (map<int, HeuristicDualLoop::DNAForSubpopsAndFitness>::iterator itDnaa = popu.Solutions.begin(); itDnaa != popu.Solutions.end(); ++itDnaa)
    {
        BOOST_LOG_TRIVIAL(info) << "---------------------------------";
        // get solution
        HeuristicDualLoop::DNAForSubpopsAndFitness s = itDnaa->second;
        PrintDNAA(s, -1);
    }
}


void HeuristicDualLoop::Selection(int max_)
{
    // take initial population {we have to be sure that the population is ranked}
    //PopuDNAA popuSelected;


    // clear the selected population.
    SeletedPopuDnaa.Clear();

    // get set of keys of Popu.
    vector<int> keys = KeysOfPopu(popuDnaa);

    int size = min(popuDnaa.Size(), max_);
    // select min(max,size)

    for (int i = 0; i < size; i++)
    {
        // get key random key 
        // -- in begin we will just get keys i order then we will use random.
        int key = keys[i];

        // get element.
        DNAForSubpopsAndFitness s = popuDnaa.GetSolution(key);
        //add it to Select Population.
        SeletedPopuDnaa.SetSolution(key, s);
    }

    // print selected one
    //PrintPopu(SeletedPopuDnaa);
    //cin.get();
}

void HeuristicDualLoop::GeneticAlgorithm()
{
    // TODO - output those as parameters
    int sizeSelectedSolutions = 10;
    int to_Select = 6; // for GA
    double crossRate = 0.5;
    double mutRate = 0.4;

    // if population is empty (generate a random one ! )
    if (popuDnaa.Size() != 0)
    {
        BOOST_LOG_TRIVIAL(info) << " ----- Replacement ----- ";
        Replacement(sizeForReplcament);
        //BOOST_LOG_TRIVIAL(info) << " after replccment : "+to_string(popuDnaa.Size());cin.get();
    }
    else { GenerateRandomPopu(GA_SOLUTION_SIZE); }

    //if (!popuDnaa.IsAvailable()) { BOOST_LOG_TRIVIAL(info) << " after replacment "; cin.get(); }

    BOOST_LOG_TRIVIAL(info) << " ----- Selection ----- ";
    Selection(sizeSelectedSolutions);
    //if (!popuDnaa.IsAvailable()) { BOOST_LOG_TRIVIAL(info) << " after selection "; cin.get(); }

    //BOOST_LOG_TRIVIAL(info) << " SIZE OF WHOLE POPULATION AFTER SELECTION IS : " + to_string(popuDnaa.Size()); cin.get();

    BOOST_LOG_TRIVIAL(info) << " ----- Crossover ----- ";
    CrossOver(popuDnaa, crossRate);
    //if (!popuDnaa.IsAvailable()) { BOOST_LOG_TRIVIAL(info) << " after crossover "; cin.get(); }

    //BOOST_LOG_TRIVIAL(info) << " SIZE OF WHOLE POPULATION AFTER CROSSOVER IS : " + to_string(popuDnaa.Size()); cin.get();

    BOOST_LOG_TRIVIAL(info) << " ----- Mutation ----- ";
    Mutation(popuDnaa, mutRate);
    //if (!popuDnaa.IsAvailable()) { BOOST_LOG_TRIVIAL(info) << " after mutation "; cin.get(); }

    //BOOST_LOG_TRIVIAL(info) << " SIZE OF WHOLE POPULATION AFTER MUTATION IS : " + to_string(popuDnaa.Size()); cin.get();

    // replacement (check it later ) , it is about remove
    BOOST_LOG_TRIVIAL(info) << " ----- [GA INNER - Crossover Inner ] ----- ";
    CrossOver_Inner(to_Select);
    //if (!popuDnaa.IsAvailable()) { BOOST_LOG_TRIVIAL(info) << " after Inner Crossover "; cin.get(); }

    //BOOST_LOG_TRIVIAL(info) << " SIZE OF WHOLE POPULATION AFTER GA_INNER IS : " + to_string(popuDnaa.Size()); cin.get();

    // Remove Empty Solutions and not availble solutions.
    popuDnaa.RemoveNotAvailaleSolutions();

    //if (!popuDnaa.IsAvailable()) { cin.get(); }

    //BOOST_LOG_TRIVIAL(info) << " ----- [PRINT THE PPULATION ] ----- ";
    //popuDnaa.Print("\n", "\n#", "\n;","\n:");

    BOOST_LOG_TRIVIAL(info) << " SIZE OF WHOLE POPULATION  IS : " + to_string(popuDnaa.Size()); //cin.get();
    //PrintPopu(popuDnaa); 

    // check that the GetMaxSimulationInstancesNumber method is correct (or not too wrong)
    if (popuDnaa.Size() > GetMaxSimulationInstancesNumber())
    {
        BOOST_LOG_TRIVIAL(error) << "SIZE OF WHOLE POPULATION IS GREATER THAN THE MAXIMUM NUMBER OF SIMULATIONS FOR THIS ITERATION COMPUTED IN GetMaxSimulationInstancesNumber()";
    }
}

