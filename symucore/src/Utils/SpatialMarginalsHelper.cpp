#include "SpatialMarginalsHelper.h"

#include <Graph/Model/Cost.h>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/math/special_functions.hpp>

#include <boost/make_shared.hpp>

#include <math.h>

using namespace SymuCore;

void SpatialMarginalsHelper::SetNbVehicles(size_t iTravelTimesUpdatePeriodIndex, double dbStartTime, double dbEndTime, MacroType * pMacroType, double dbNbVehiclesForMacroType,
    double dbNbVehiclesForAllMacroTypes, double dbTravelTimeForAllMacroTypes)
{
    if (iTravelTimesUpdatePeriodIndex >= m_mapTemporalNbVehicles.size())
    {
        m_mapTemporalNbVehicles.addTimeFrame(dbStartTime, dbEndTime, boost::make_shared<std::map<MacroType*, double>>());
        m_mapTravelTimesForAllMacroTypes.addTimeFrame(dbStartTime, dbEndTime, boost::make_shared<double>());
    }
    const TimeFrame<std::map<MacroType *, double>> & nbVehiclesVariation = m_mapTemporalNbVehicles.getTimeFrame((size_t)iTravelTimesUpdatePeriodIndex);
    nbVehiclesVariation.getData()->operator[](pMacroType) = dbNbVehiclesForMacroType;
    nbVehiclesVariation.getData()->operator[](NULL) = dbNbVehiclesForAllMacroTypes;
    const TimeFrame<double> & nbTravelTimeForAllMacroTypesVariation = m_mapTravelTimesForAllMacroTypes.getTimeFrame((size_t)iTravelTimesUpdatePeriodIndex);
    (*nbTravelTimeForAllMacroTypesVariation.getData()) = dbTravelTimeForAllMacroTypes;
}

double computeITTFromLinearRegresion(double dbTT, double dbNVeh, double ttNext, double dbNVehNext, bool & infMarginal)
{
    infMarginal = false;
    double dbITT;
    if (dbNVehNext - dbNVeh == 0)
    {
        // pas possible de calculer une pente : on prend 0
        dbITT = 0;
    }
    else
    {
        if (dbTT == std::numeric_limits<double>::infinity() && ttNext == std::numeric_limits<double>::infinity())
        {
            // Pour éviter le NaN : on met 0 : le marginal prendra la valeur du temps de parcours donc +INF.
            dbITT = 0;
            infMarginal = true;
        }
        else if ((dbTT == std::numeric_limits<double>::infinity() && ttNext == -std::numeric_limits<double>::infinity())
            || (dbTT == -std::numeric_limits<double>::infinity() && ttNext == std::numeric_limits<double>::infinity()))
        {
            dbITT = 0;
            infMarginal = true;
        }
        else
        {
            dbITT = (ttNext - dbTT) / (dbNVehNext - dbNVeh);
        }
    }
    return dbITT;
}

std::vector<double> polyfit(const std::vector<double>& oX,
    const std::vector<double>& oY, int nDegree)
{
    using namespace boost::numeric::ublas;

    assert(oX.size() == oY.size());

    // more intuative this way
    nDegree++;

    size_t nCount = oX.size();
    matrix<double> oXMatrix(nCount, nDegree);
    matrix<double> oYMatrix(nCount, 1);

    // copy y matrix
    for (size_t i = 0; i < nCount; i++)
    {
        oYMatrix(i, 0) = oY[i];
    }

    // create the X matrix
    for (size_t nRow = 0; nRow < nCount; nRow++)
    {
        double nVal = 1.0;
        for (int nCol = 0; nCol < nDegree; nCol++)
        {
            oXMatrix(nRow, nCol) = nVal;
            nVal *= oX[nRow];
        }
    }

    // transpose X matrix
    matrix<double> oXtMatrix(trans(oXMatrix));
    // multiply transposed X matrix with X matrix
    matrix<double> oXtXMatrix(prec_prod(oXtMatrix, oXMatrix));
    // multiply transposed X matrix with Y matrix
    matrix<double> oXtYMatrix(prec_prod(oXtMatrix, oYMatrix));

    // lu decomposition
    permutation_matrix<> pert(oXtXMatrix.size1());
    const matrix<int>::size_type singular = lu_factorize(oXtXMatrix, pert);

    // must be singular
    if (singular == 0)
    {
        // backsubstitution
        lu_substitute(oXtXMatrix, pert, oXtYMatrix);

        // copy the result to coeff
        return std::vector<double>(oXtYMatrix.data().begin(), oXtYMatrix.data().end());
    }

    // may happen if x-axis are close from each others (not detected for the equality test for now)
    return std::vector<double>();
}

double computeITTFromQuadraticRegresion(double dbPrevTT, double dbPrevNVeh, double dbTT, double dbNVeh, double ttNext, double dbNVehNext, bool & bInfMarginal)
{
    bInfMarginal = false;
    if (dbTT == std::numeric_limits<double>::infinity())
    {
        bInfMarginal = true;
        return 0;
    }

    if (dbPrevTT == std::numeric_limits<double>::infinity() || ttNext == std::numeric_limits<double>::infinity())
    {
        // double linear regression :
        bool bInfMarginalTmp1;
        double dbITT1 = computeITTFromLinearRegresion(dbPrevTT, dbPrevNVeh, dbTT, dbNVeh, bInfMarginalTmp1);
        bool bInfMarginalTmp2;
        double dbITT2 = computeITTFromLinearRegresion(dbTT, dbNVeh, ttNext, dbNVehNext, bInfMarginalTmp2);
        if (bInfMarginalTmp1 || bInfMarginalTmp2)
        {
            bInfMarginal = true;
            return 0;
        }
        else
        {
            double dbITT = (dbITT1 + dbITT2) / 2.0;
            // Case of NaN (one is +INF and the other is -INF, we replace by +INF) :
            if (boost::math::isnan(dbITT))
            {
                bInfMarginal = true;
                dbITT = 0;
            }
            return dbITT;
        }
    }

    // at this point, we don't have infinite values. we still have to treat case 2 : identical x-axis :
    // if doubloon, we keep the last point in temporal order
    std::vector<std::pair<double, double> > uniqueAbscissaPoints;
    uniqueAbscissaPoints.push_back(std::make_pair(dbPrevNVeh, dbPrevTT));
    if (dbNVeh != uniqueAbscissaPoints.front().first)
    {
        uniqueAbscissaPoints.push_back(std::make_pair(dbNVeh, dbTT));
    }
    else
    {
        uniqueAbscissaPoints.front().second = dbTT;
    }
    bool bAlreadyHereAbscissa = false;
    for (size_t iPt = 0; iPt < uniqueAbscissaPoints.size(); iPt++)
    {
        std::pair<double, double> & pt = uniqueAbscissaPoints[iPt];
        if (pt.first == dbNVehNext)
        {
            pt.second = ttNext;
            bAlreadyHereAbscissa = true;
            break;
        }
    }
    if (!bAlreadyHereAbscissa)
    {
        uniqueAbscissaPoints.push_back(std::make_pair(dbNVehNext, ttNext));
    }

    double dbITT;
    switch (uniqueAbscissaPoints.size())
    {
    case 3:
    {
              std::vector<double> oX, oY;
              for (size_t iPt = 0; iPt < uniqueAbscissaPoints.size(); iPt++)
              {
                  std::pair<double, double> & pt = uniqueAbscissaPoints[iPt];
                  oX.push_back(pt.first);
                  oY.push_back(pt.second);
              }
              std::vector<double> polyCoefs = polyfit(oX, oY, 2);
              if (polyCoefs.empty())
              {
                  // we don't know how to do it :
                  dbITT = 0;
              }
              else
              {
                  assert(!boost::math::isnan(polyCoefs[2]) && !boost::math::isnan(polyCoefs[1]));
                  dbITT = 2.0 * polyCoefs[2] * dbNVeh + polyCoefs[1];
              }
              break;
    }
    case 2:
    {
              dbITT = computeITTFromLinearRegresion(uniqueAbscissaPoints.front().second, uniqueAbscissaPoints.front().first, uniqueAbscissaPoints.back().second, uniqueAbscissaPoints.back().first, bInfMarginal);
              break;
    }
    case 1:
    {
              dbITT = 0;
              break;
    }
    default:
    {
               assert(false); // impossible
    }
    }

    return dbITT;
}

void SpatialMarginalsHelper::ComputeMarginal(
    const std::map<MacroType*, std::vector<std::pair<SymuCore::CostFunction, SubPopulation*> > > & listMacroType,
    const std::map<MacroType*, bool> & forbiddenMacroTypes,
    const ListTimeFrame<std::map<SubPopulation*, Cost> > & temporalCosts,
    double dbMaxMarginalsValue, double dbPenalisationRatio)
{
    size_t nbPeriods = temporalCosts.size();

    // For each Macro-type
    for (std::map<MacroType*, std::vector<std::pair<SymuCore::CostFunction, SubPopulation*> > >::const_iterator iter = listMacroType.begin(); iter != listMacroType.end(); ++iter)
    {
        MacroType * pMacroType = iter->first;
        bool bIsForbidden = forbiddenMacroTypes.at(pMacroType);

        if (nbPeriods == 1)
        {
            // If only one period, marginals is the travel time so it's OK. We only apply the max limit of marginals (but we keep +INF if forbidden) :
            if (!bIsForbidden)
            {
                std::map<SubPopulation *, Cost> * pCostVariation = temporalCosts.getTimeFrame(0).getData();

                for (size_t iSubPop = 0; iSubPop < iter->second.size(); iSubPop++)
                {
                    const std::pair<SymuCore::CostFunction, SubPopulation*> & subPopInfo = iter->second[iSubPop];
                    Cost & cost = pCostVariation->at(subPopInfo.second);
                    if (subPopInfo.first == CF_Marginals)
                    {
                        if (cost.getCostValue() > dbMaxMarginalsValue)
                        {
                            cost.setUsedCostValue(dbMaxMarginalsValue);
                        }
                    }
                    else
                    {
                        if (cost.getOtherCostValue(CF_Marginals) > dbMaxMarginalsValue)
                        {
                            cost.setOtherCostValue(CF_Marginals, dbMaxMarginalsValue);
                        }
                    }
                }
            }
        }
        else
        {
            for (size_t iPeriod = 0; iPeriod < nbPeriods; iPeriod++)
            {
                // If forbidden element, cost should already be +INF
                if (!bIsForbidden)
                {
                    double tt = *m_mapTravelTimesForAllMacroTypes.getTimeFrame(iPeriod).getData();
                    double dbNVeh = m_mapTemporalNbVehicles.getTimeFrame(iPeriod).getData()->at(NULL);

                    // Calcul of ITT
                    double dbITT;
                    bool bInfMarginal = false;
                    if (iPeriod == 0)
                    {
                        // Case of first period : linear regression with two first points
                        double ttNext = *m_mapTravelTimesForAllMacroTypes.getTimeFrame(iPeriod + 1).getData();
                        double dbNVehNext = m_mapTemporalNbVehicles.getTimeFrame(iPeriod + 1).getData()->at(NULL);
                        dbITT = computeITTFromLinearRegresion(tt, dbNVeh, ttNext, dbNVehNext, bInfMarginal);
                    }
                    else if (iPeriod == nbPeriods - 1)
                    {
                        // Case of last period : linear regression with two last points
                        double ttPrev = *m_mapTravelTimesForAllMacroTypes.getTimeFrame(iPeriod - 1).getData();
                        double dbNVehPrev = m_mapTemporalNbVehicles.getTimeFrame(iPeriod - 1).getData()->at(NULL);
                        dbITT = computeITTFromLinearRegresion(ttPrev, dbNVehPrev, tt, dbNVeh, bInfMarginal);
                    }
                    else
                    {
                        double ttNext = *m_mapTravelTimesForAllMacroTypes.getTimeFrame(iPeriod + 1).getData();
                        double dbNVehNext = m_mapTemporalNbVehicles.getTimeFrame(iPeriod + 1).getData()->at(NULL);
                        double ttPrev = *m_mapTravelTimesForAllMacroTypes.getTimeFrame(iPeriod - 1).getData();
                        double dbNVehPrev = m_mapTemporalNbVehicles.getTimeFrame(iPeriod - 1).getData()->at(NULL);
                        dbITT = computeITTFromQuadraticRegresion(ttPrev, dbNVehPrev, tt, dbNVeh, ttNext, dbNVehNext, bInfMarginal);
                    }

                    double dbMarginal;
                    if (bInfMarginal)
                    {
                        dbMarginal = std::numeric_limits<double>::infinity();
                    }
                    else
                    {
                        // Marginal is the travel time (stocked in the cost for now) + ITT * number of vehicles,
                        // Be careful, we take the number of vehicles of macro-type here, in the rest, we work independantly from macro-type
                        double nbVehForMacroType = m_mapTemporalNbVehicles.getTimeFrame(iPeriod).getData()->at(pMacroType);

                        dbMarginal = tt;
                        if (nbVehForMacroType != 0) // rmq. : to avoid a 0 * +INF that lead to IND ...
                        {
                            dbMarginal += dbITT * nbVehForMacroType;
                        }

                        // All case of Nan should be traeted before...
                        assert(!boost::math::isnan(dbMarginal));
                    }

                    // Application of minimum marginals (the element is not forbiddent at this level, cf. if() cover it, so no risk to unforbidden by the minimization of the cost)
                    dbMarginal = std::min<double>(dbMarginal, dbMaxMarginalsValue);
                    dbMarginal = std::max<double>(0, dbMarginal); //No negative marginals

                    // Application of the penalization, as for any type of cost.
                    dbMarginal *= dbPenalisationRatio;

                    const TimeFrame<std::map<SubPopulation *, Cost>> & costVariation = temporalCosts.getTimeFrame(iPeriod);

                    for (size_t iSubPop = 0; iSubPop < iter->second.size(); iSubPop++)
                    {
                        const std::pair<CostFunction, SubPopulation*> & subpopInfo = iter->second.at(iSubPop);
                        Cost & cost = costVariation.getData()->operator[](subpopInfo.second);
                        if (subpopInfo.first == CF_Marginals)
                        {
                            cost.setUsedCostValue(dbMarginal);
                        }
                        else
                        {
                            cost.setOtherCostValue(CF_Marginals, dbMarginal);
                        }
                    }
                }
            }
        }
    }
}
