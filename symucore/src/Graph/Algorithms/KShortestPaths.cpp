#include "KShortestPaths.h"
#include "Dijkstra.h"
#include "Graph/Model/Node.h"
#include "Graph/Model/Link.h"
#include "Graph/Model/Pattern.h"
#include "Demand/Origin.h"
#include "Demand/Destination.h"
#include "Demand/SubPopulation.h"
#include "Demand/Population.h"
#include "Demand/Path.h"
#include "Graph/Model/Cost.h"
#include "Demand/ValuetedPath.h"
#include "Demand/PredefinedPath.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <vector>
#include <set>

using namespace SymuCore;
using namespace std;
using namespace boost::posix_time;

KShortestPaths::KShortestPaths()
{
    m_pMapPredefinedPaths = NULL;
    m_pListCommonalityFactorParametersByOD = NULL;
    m_nMethod = 0;
    m_eHeuristic = SPH_NONE;
    m_dbAStarBeta = 0.0;
    m_dbAStarGamma = 0.0;
}

KShortestPaths::KShortestPaths(double dbAssignementAlpha, double dbAssignementBeta, double dbAssignementGamma, bool CommonalityFilter, std::vector<double> listCommonalityFactorParameters, double dbKShortestPathsThreshold, const FromSymuViaParameters &KByOD,
    std::map<Origin*, std::map<Destination*, std::vector<PredefinedPath> > > * pPredefinedPaths, std::map<Origin*, std::map<Destination*, std::vector<double> > > * pListCommonalityFactorParametersByOD)
{
    m_dbKShortestPathsThreshold = dbKShortestPathsThreshold;
    m_KParameters = KByOD;
    m_dbAssignementAlpha = dbAssignementAlpha;
    m_dbAssignementBeta = dbAssignementBeta;
    m_dbAssignementGamma = dbAssignementGamma;
    m_bCommonalityFilter = CommonalityFilter;
    m_listCommonalityFactorParameters = listCommonalityFactorParameters;
    m_pMapPredefinedPaths = pPredefinedPaths;
    m_pListCommonalityFactorParametersByOD = pListCommonalityFactorParametersByOD;
    m_nMethod = 0;
    m_eHeuristic = SPH_NONE;
    m_dbAStarBeta = 0.0;
    m_dbAStarGamma = 0.0;
}

KShortestPaths::~KShortestPaths()
{
}

const std::map<Origin*, std::map<Destination*, std::vector<std::pair<Path, double> > > > & KShortestPaths::getFilteredPaths() const
{
    return m_listFilteredPaths;
}

void KShortestPaths::SetSimulationGameMethod(int nMethod, const Path & path)
{
    m_nMethod = nMethod;
    m_nMethodRefPath = path;
}

void KShortestPaths::SetHeuristicParams(ShortestPathHeuristic eHeuristic, double dbAStarBeta, double dbAStarGamma)
{
    m_eHeuristic = eHeuristic;
    m_dbAStarBeta = dbAStarBeta;
    m_dbAStarGamma = dbAStarGamma;
}

std::vector<Pattern*> & KShortestPaths::GetPatternsToAvoid()
{
    return m_PatternsToAvoid;
}

bool itinerarySorter(const ValuetedPath* i, const ValuetedPath* j)
{
    return *i < *j;
}

map<Origin*, map<Destination*, vector<ValuetedPath*> > > KShortestPaths::getShortestPaths(int iSimulationInstance, const vector<Origin*>& originNodes, const vector<Destination*>& destinationNodes, SubPopulation* pSubPopulation, double dbArrivalTime, bool bUseCostsForTemporalAlgorithm)
{
    Dijkstra OneShortestPath;
    OneShortestPath.SetHeuristic(m_eHeuristic, m_dbAStarBeta, m_dbAStarGamma);
    map<Origin*, map<Destination*, vector<ValuetedPath*> > > listResultsPaths; // list of path for all destination Nodes found
    map<Origin*, map<Destination*, vector<ValuetedPath*> > > currentPathFound;
    const std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, SymuCore::ListTimeFrame<double> > > & KParametersBySubPopulation = m_KParameters.at(pSubPopulation);

    map<Pattern *, double> initialPatternPenalization;
    for (size_t iPatternToAvoid = 0; iPatternToAvoid < m_PatternsToAvoid.size(); iPatternToAvoid++)
    {
        initialPatternPenalization[m_PatternsToAvoid.at(iPatternToAvoid)] = std::numeric_limits<double>::infinity();
    }

    m_listFilteredPaths.clear();

    for(size_t iOrigin = 0; iOrigin < originNodes.size(); iOrigin++)
    {
        Origin* pOrigin = originNodes[iOrigin];
        for(size_t iDestination = 0; iDestination < destinationNodes.size(); iDestination++)
        {
            Destination* pDestination = destinationNodes[iDestination];
            std::vector<std::pair<Path, double> > & filteredPathForOD = m_listFilteredPaths[pOrigin][pDestination];

            int nNbPlusCourtChemin = (int)(*KParametersBySubPopulation.at(pOrigin).at(pDestination).getData(dbArrivalTime));

            // Si on demande 0 chemins (a priori l'appelant devrait le vérifier mais bon...), il arrive qu'on en renvoie malgré tout en cas de chemin de référence par exemple
            // (cf. cas méthode 1) ou qu'on fasse des calculs pour rien (cf. méthodes 2 et 3 pour lesquelles on incrémente nNbPlusCourtChemin).
            // Du coup, on court-circuite la boucle dans ce cas.
            if (nNbPlusCourtChemin <= 0) {
                listResultsPaths[pOrigin][pDestination] = vector<ValuetedPath*>();
                continue;
            }

            
            std::vector<double> * pCFParams = m_pListCommonalityFactorParametersByOD == NULL ? &m_listCommonalityFactorParameters : &m_pListCommonalityFactorParametersByOD->at(pOrigin).at(pDestination);

            vector<Origin*> OneOrigin;
			OneOrigin.push_back(pOrigin);
            vector<Destination*> OneDestination;
			OneDestination.push_back(pDestination);
            vector<ValuetedPath*> alreadyFoundPaths;
            ValuetedPath* pCurrentPenalizedPath;
            double      dbCoutMin = std::numeric_limits<double>::infinity();
            double      dbMinPenalizedCost = std::numeric_limits<double>::infinity();
            bool        bFin, bTrouve;
            double      dbCost, dbPenalizedCost;

            bool bCommonalityFilter = m_bCommonalityFilter;
            double dbCommonalityAlpha = pCFParams->at(0);
            switch (m_nMethod)
            {
            case 3:
                nNbPlusCourtChemin++;
                dbCommonalityAlpha = -1;
                bCommonalityFilter = true;
                break;
            case 2:
                nNbPlusCourtChemin++;
                dbCommonalityAlpha = 0;
                bCommonalityFilter = true;
                break;
            case 1:
                bCommonalityFilter = false;
                break;
            case 0:
            default:
                break;
            }

            std::vector<ValuetedPath*> refPath;
            if (m_nMethod != 0)
            {
                const Cost & refCost = m_nMethodRefPath.GetPathCost(iSimulationInstance, dbArrivalTime, pSubPopulation, true);
                ValuetedPath* pRefpath = new ValuetedPath(m_nMethodRefPath, refCost.getCostValue(), refCost.getTravelTime());
                refPath.push_back(pRefpath);
            }

            // dealing with predefined paths :
            bool bDoCompute = true;
            int nbPredefPaths = 0;
            vector<ValuetedPath*> predefinedPaths;
            if (m_pMapPredefinedPaths)
            {
                double dbRemainingCoeff = 1.0;
                const std::vector<PredefinedPath> & predefinedPathsForOD = m_pMapPredefinedPaths->at(pOrigin).at(pDestination);
                for (size_t iPath = 0; iPath < predefinedPathsForOD.size(); iPath++) {
                    const PredefinedPath & predef = predefinedPathsForOD.at(iPath);
                    dbRemainingCoeff -= predef.getCoeff();

                    const Cost & predefCost = predef.getPath().GetPathCost(iSimulationInstance, dbArrivalTime, pSubPopulation, true);
                    ValuetedPath * pPredefinedPath = new ValuetedPath(predef.getPath(), predefCost.getCostValue(), predefCost.getTravelTime());
                    pPredefinedPath->setIsPredefined(true);
                    pPredefinedPath->setStrName(predef.getStrName());
                    pPredefinedPath->setPredefinedJunctionNode(predef.getJunction());
                    predefinedPaths.push_back(pPredefinedPath);
                    nbPredefPaths++;
                }

                // rmk. : using epsilon * number of predefined paths to handle numeric precision issues
                bDoCompute = dbRemainingCoeff > (std::numeric_limits<double>::epsilon()*nbPredefPaths) && nbPredefPaths < nNbPlusCourtChemin;
            }


            if (bDoCompute)
            {
                OneShortestPath.GetMapPenalizedPatterns() = initialPatternPenalization;

                bFin = false;
                while (!bFin)
                {
                    currentPathFound = OneShortestPath.getShortestPaths(iSimulationInstance, OneOrigin, OneDestination, pSubPopulation, dbArrivalTime, bUseCostsForTemporalAlgorithm);

                    const vector<ValuetedPath*> & currentPathFoundForOD = currentPathFound[pOrigin][pDestination];
                    if (!currentPathFoundForOD.empty())
                    {
                        pCurrentPenalizedPath = currentPathFoundForOD.front();
                        dbMinPenalizedCost = std::min<double>(pCurrentPenalizedPath->GetCost(), dbMinPenalizedCost);
                        dbPenalizedCost = pCurrentPenalizedPath->GetCost();

                        // Cost of the path without penalization
                        dbCost = pCurrentPenalizedPath->GetPath().GetPathCost(iSimulationInstance, dbArrivalTime, pSubPopulation, true).getCostValue();

                        // penalization of patterns
                        map<Pattern*, double>& penalizedPatterns = OneShortestPath.GetMapPenalizedPatterns();
                        vector<Pattern*>& listPatterns = pCurrentPenalizedPath->GetPath().GetlistPattern();
                        for (size_t i = 0; i < listPatterns.size(); i++)
                        {
                            penalizedPatterns[listPatterns[i]] += m_dbAssignementAlpha;
                        }

                        dbCoutMin = std::min<double>(dbCost, dbCoutMin);

                        // Is this itinary already detected ?
                        bTrouve = false;
                        for (size_t i = 0; i < alreadyFoundPaths.size(); i++)
                        {
                            if (alreadyFoundPaths[i]->GetPath() == pCurrentPenalizedPath->GetPath())
                            {
                                delete pCurrentPenalizedPath;
                                bTrouve = true;
                                break;
                            }
                        }

                        // It's a new path, verify if we have to stop
                        if (!bTrouve)
                        {
                            pCurrentPenalizedPath->setCost(dbCost);
                            // rmq. : not strict inequalities so it works with nul path cost.
                            if (dbCost <= m_dbAssignementBeta * dbCoutMin && pCurrentPenalizedPath->GetCost() <= m_dbAssignementGamma * dbMinPenalizedCost)
                            {
                                double dbCommonalityFactor = 0;
                                if (bCommonalityFilter)
                                {
                                    if (m_nMethod == 2 || m_nMethod == 3)
                                    {
                                        // For method 2 and 3, we compute the CFk between the reference path and the candidate path only
                                        dbCommonalityFactor = ComputeCommonalityFactor(iSimulationInstance, refPath, pCurrentPenalizedPath, dbArrivalTime, pSubPopulation, pCFParams);
                                    }
                                    else
                                    {
                                        dbCommonalityFactor = ComputeCommonalityFactor(iSimulationInstance, alreadyFoundPaths, pCurrentPenalizedPath, dbArrivalTime, pSubPopulation, pCFParams);
                                    }
                                }

                                if (!bCommonalityFilter || dbCommonalityFactor <= dbCommonalityAlpha)
                                {
                                    if (alreadyFoundPaths.size() == 0 || m_dbKShortestPathsThreshold == -1 || (dbCost / alreadyFoundPaths[0]->GetCost()) < (1 + m_dbKShortestPathsThreshold))
                                    {
                                        // we add the path
                                        pCurrentPenalizedPath->setCommonalityFactor(dbCommonalityFactor);
                                        pCurrentPenalizedPath->setPenalizedCost(dbPenalizedCost);
                                        alreadyFoundPaths.push_back(pCurrentPenalizedPath);
                                    }
                                    else{
                                        delete pCurrentPenalizedPath;
                                        bFin = true;
                                    }
                                }
                                else
                                {
                                    filteredPathForOD.push_back(std::make_pair(pCurrentPenalizedPath->GetPath(), dbCommonalityFactor));
                                    delete pCurrentPenalizedPath;
                                }
                            }
                            else
                            {
                                delete pCurrentPenalizedPath;
                            }
                        }

                        // should we continue ?
                        if (bFin || (int)alreadyFoundPaths.size() >= nNbPlusCourtChemin || dbPenalizedCost > (1 + m_dbAssignementGamma) * dbMinPenalizedCost)
                            bFin = true;
                    }
                    else
                    {
                        bFin = true;
                    }
                }
            }

            // removing computed paths equals to predefined ones
            for (size_t iPath = 0; iPath < predefinedPaths.size(); iPath++)
            {
                ValuetedPath * pPredefPath = predefinedPaths.at(iPath);
                for (vector<ValuetedPath*>::iterator iterComputedPath = alreadyFoundPaths.begin(); iterComputedPath != alreadyFoundPaths.end();)
                {
                    ValuetedPath * pComputedPath = *iterComputedPath;
                    if (pComputedPath->GetPath() == pPredefPath->GetPath())
                    {
                        // computed path already is in the predefined paths :
                        delete pComputedPath;
                        iterComputedPath = alreadyFoundPaths.erase(iterComputedPath);
                        break;
                    }
                    ++iterComputedPath;
                }
            }

            // remove computedpaths equals to the reference path
            if (m_nMethod == 2 || m_nMethod == 3)
            {
                nNbPlusCourtChemin--;
                ValuetedPath * pRefPath = refPath.front();
                for (vector<ValuetedPath*>::iterator iterComputedPath = alreadyFoundPaths.begin(); iterComputedPath != alreadyFoundPaths.end();)
                {
                    ValuetedPath * pComputedPath = *iterComputedPath;
                    if (pComputedPath->GetPath() == pRefPath->GetPath())
                    {
                        delete pComputedPath;
                        iterComputedPath = alreadyFoundPaths.erase(iterComputedPath);
                        break;
                    }
                    ++iterComputedPath;
                }
            }

            // removing the extra paths
            unsigned int nbComputedItinerariesToKeep = std::max<int>(0, (nNbPlusCourtChemin - nbPredefPaths));
            std::sort(alreadyFoundPaths.begin(), alreadyFoundPaths.end(), itinerarySorter);
            for (size_t iPath = 0; iPath < alreadyFoundPaths.size(); iPath++)
            {
                ValuetedPath * pComputedPath = alreadyFoundPaths.at(iPath);
                if (iPath < nbComputedItinerariesToKeep)
                {
                    predefinedPaths.push_back(pComputedPath);
                }
                else
                {
                    delete pComputedPath;
                }
            }

            // for method 1, the reference path must appear :
            if (m_nMethod == 1)
            {
                ValuetedPath * pRefPath = refPath.front();
                bool bRefPathIsComputed = false;
                for (size_t iIti = 0; iIti < predefinedPaths.size(); iIti++)
                {
                    if (predefinedPaths[iIti]->GetPath() == pRefPath->GetPath())
                    {
                        bRefPathIsComputed = true;
                        break;
                    }
                }
                if (!bRefPathIsComputed)
                {
                    const Cost & predefCost = pRefPath->GetPath().GetPathCost(iSimulationInstance, dbArrivalTime, pSubPopulation, true);
                    ValuetedPath * pRefPathResult = new ValuetedPath(pRefPath->GetPath(), predefCost.getCostValue(), predefCost.getTravelTime());
                    pRefPathResult->setIsPredefined(true);
                    pRefPathResult->setPredefinedJunctionNode(NULL);
                    pRefPathResult->setPenalizedCost(pRefPathResult->GetCost());

                    // we don't wan't the reference path if we try to avoid one of its links:
                    bool bAvoidRefPath = pRefPathResult->GetCost() >= std::numeric_limits<double>::infinity();
                    if (!bAvoidRefPath)
                    {
                        for (size_t iPatternToAvoid = 0; iPatternToAvoid < m_PatternsToAvoid.size(); iPatternToAvoid++)
                        {
                            if (std::find(pRefPath->GetPath().GetlistPattern().begin(), pRefPath->GetPath().GetlistPattern().end(), m_PatternsToAvoid.at(iPatternToAvoid)) != pRefPath->GetPath().GetlistPattern().end())
                            {
                                bAvoidRefPath = true;
                                break;
                            }
                        }
                    }

                    if (!bAvoidRefPath)
                    {
                        if (predefinedPaths.empty() || (int)predefinedPaths.size() < nbComputedItinerariesToKeep)
                        {
                            predefinedPaths.push_back(pRefPathResult);
                        }
                        else
                        {
                            // replacing the least good path by the reference path (which should have a higher cost value so no need to sort again)
                            delete predefinedPaths.back();
                            predefinedPaths.back() = pRefPathResult;
                        }
                    }
                    else
                    {
                        delete pRefPathResult;
                    }
                }
            }

            for (size_t iRefPath = 0; iRefPath < refPath.size(); iRefPath++)
            {
                delete refPath[iRefPath];
            }

            listResultsPaths[pOrigin][pDestination] = predefinedPaths;
        }
    }

    return listResultsPaths; // all end nodes that have been found
}

double KShortestPaths::ComputeCommonalityFactor(int iSimulationInstance, vector<ValuetedPath*>& alreadyFoundPaths, ValuetedPath* pCurrentPath, double dbArrivalTime, SubPopulation* pSubPopulation, std::vector<double> * pCFParams)
{
    double dbResult = 0;
    double dbTravelTimePath = pCurrentPath->GetTime();

    // For every paths Found
    for(size_t iFoundPaths = 0; iFoundPaths < alreadyFoundPaths.size(); iFoundPaths++)
    {
        // Calcul travel time for each path found and travel time for links shared with current path
        ValuetedPath* pCurrentFoundPath = alreadyFoundPaths[iFoundPaths];
        vector<Pattern*>& listPatternPathFound = pCurrentFoundPath->GetPath().GetlistPattern();
        vector<Pattern*>& listPatternCurrentPath = pCurrentPath->GetPath().GetlistPattern();
        double dbTTPathFound = pCurrentFoundPath->GetTime();
        double dbTTShared = 0.0;
        Pattern* pCurrentPattern = NULL;

        for(size_t iPattern = 0; iPattern < listPatternPathFound.size(); iPattern++)
        {
            pCurrentPattern = listPatternPathFound[iPattern];
            vector<Pattern*>::iterator itSamePattern = find(listPatternCurrentPath.begin(), listPatternCurrentPath.end(), pCurrentPattern);
            if(itSamePattern != listPatternCurrentPath.end())
            {
                Pattern* pPattern = (*itSamePattern);
                dbTTShared += pPattern->getPatternCost(iSimulationInstance, dbArrivalTime, pSubPopulation)->getTravelTime();

                itSamePattern++;
                if((iPattern+1 < listPatternPathFound.size()) && (itSamePattern != listPatternCurrentPath.end()) && (listPatternPathFound[iPattern+1] == (*itSamePattern)))
                {
                    //add the time for Penalty
                    dbTTShared += pPattern->getLink()->getDownstreamNode()->getPenaltyCost(iSimulationInstance, pPattern, (*itSamePattern), dbArrivalTime, pSubPopulation)->getTravelTime();
                }
            }
        }

        dbResult += pow(dbTTShared / (sqrt(dbTravelTimePath) * sqrt(dbTTPathFound)), pCFParams->at(2));
    }

    dbResult = pCFParams->at(1) * log(dbResult);

    return dbResult;
}
