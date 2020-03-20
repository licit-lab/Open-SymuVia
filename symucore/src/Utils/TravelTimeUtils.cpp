#include "TravelTimeUtils.h"

#include <algorithm>
#include <cassert>

using namespace SymuCore;

void TravelTimeUtils::SelectVehicleForTravelTimes(const std::map<double, std::map<int, std::pair<bool, std::pair<double, double> > > > & mapUsefuleVehicles, double dbInstFinPeriode, double dbPeriodDuration, int nbPeriodsForTravelTimes,
    int nbVehiclesForTravelTimes, int nbMarginalsDeltaN, bool bUseMeanMethod, bool bUseMedianMethod,
    double & dbITT, std::vector<std::pair<std::pair<double, double>, double> > & exitInstantAndTravelTimesWithLengthsForMacroType,
    std::vector<std::pair<std::pair<double, double>, double> > & exitInstantAndTravelTimesWithLengthsForAllMacroTypes, std::vector<int> & vehicleIDsToClean)
{
    // remarque : le ITT est le ITT global (pas filtré par macro-type)
    dbITT = 0;
    int nbVeh = 0;
    double dbOldestAllowedExitInstant = dbInstFinPeriode - dbPeriodDuration * nbPeriodsForTravelTimes;

    // for mean method
    std::vector<double> travelTimesForMeanMethod;

    // Si on demande un calcul sur 0 véhicules, c'est déjà fini.
    bool bFinished = nbVehiclesForTravelTimes <= 0;

    for (std::map<double, std::map<int, std::pair<bool, std::pair<double, double> > > >::const_reverse_iterator rit = mapUsefuleVehicles.rbegin(); rit != mapUsefuleVehicles.rend(); ++rit)
    {
        for (std::map<int, std::pair<bool, std::pair<double, double> > >::const_iterator iterVeh = rit->second.begin(); iterVeh != rit->second.end(); ++iterVeh)
        {
            bool bIsTargetmacroType = iterVeh->second.first;
            if (!bFinished)
            {
                // On note le temps du véhicule
                exitInstantAndTravelTimesWithLengthsForAllMacroTypes.push_back(std::make_pair(std::make_pair(rit->first, iterVeh->second.second.first), iterVeh->second.second.second));
                if (bIsTargetmacroType)	// Véhicule du bon macrotype
                {
                    exitInstantAndTravelTimesWithLengthsForMacroType.push_back(std::make_pair(std::make_pair(rit->first, iterVeh->second.second.first), iterVeh->second.second.second));
                }

                if (bUseMeanMethod || bUseMedianMethod)
                {
                    // Application de la règle de trois lorsque les véhicules comparés n'ont pas emprunté le même itinéraire (utilisé uniquement pour
                    // le calcul de marginals en zone. Les coefficients valent 1.0 dans les autres cas)
                    travelTimesForMeanMethod.push_back(iterVeh->second.second.first * iterVeh->second.second.second);
                    nbVeh++;
                    bFinished = nbVeh == nbVehiclesForTravelTimes;
                }
                else
                {
                    // si on a assez de delta dans la liste des temps de parcours, on a un nouveau delta à moyenner :
                    if ((int)exitInstantAndTravelTimesWithLengthsForAllMacroTypes.size() > nbMarginalsDeltaN)
                    {
                        const std::pair<std::pair<double, double>, double> & exitInstantTravelTimeAndLengthRatioForPreviousVehicle = exitInstantAndTravelTimesWithLengthsForAllMacroTypes[exitInstantAndTravelTimesWithLengthsForAllMacroTypes.size() - 1 - nbMarginalsDeltaN];
                        if (exitInstantTravelTimeAndLengthRatioForPreviousVehicle.first.first >= dbOldestAllowedExitInstant)
                        {
                            if (nbMarginalsDeltaN > 0)
                            {
                                // Application de la règle de trois lorsque les véhicules comparés n'ont pas emprunté le même itinéraire (utilisé uniquement pour
                                // le calcul de marginals en zone. Les coefficients valent 1.0 dans les autres cas)
                                dbITT += (exitInstantTravelTimeAndLengthRatioForPreviousVehicle.first.second * exitInstantTravelTimeAndLengthRatioForPreviousVehicle.second - iterVeh->second.second.first * iterVeh->second.second.second) / nbMarginalsDeltaN;
                            }
                            nbVeh++;
                            bFinished = nbVeh == nbVehiclesForTravelTimes;
                        }
                        else
                        {
                            // On n'atteindra pas le nombre de véhicules demandés pour le calcul du marginal et on arrête là.
                            bFinished = true;
                        }
                    }
                }
            }
            else
            {
                // on supprime le véhicule de la map initiale, uniquement pour les véhicules du macro type courant (ce véhicule pouvant encore être utile à un calcul pour son macro-type)
                if (bIsTargetmacroType)
                {
                    vehicleIDsToClean.push_back(iterVeh->first);
                }
            }
        }
    }

    if (bUseMeanMethod || bUseMedianMethod)
    {
        if (travelTimesForMeanMethod.size() >= 2)
        {
            if (bUseMeanMethod)
            {
                // Moyenne des temps de parcours :
                double dbSumTT = 0;
                double dbMaxTT = 0;
                double dbTT;
                for (size_t iTT = 0; iTT < travelTimesForMeanMethod.size(); iTT++)
                {
                    dbTT = travelTimesForMeanMethod.at(iTT);
                    dbSumTT += dbTT;
                    if (dbTT > dbMaxTT) {
                        dbMaxTT = dbTT;
                    }
                }

                dbITT = (dbSumTT / travelTimesForMeanMethod.size()) - (dbSumTT - dbMaxTT) / (travelTimesForMeanMethod.size() - 1);
            }
            else
            {
                // median method
                assert(bUseMedianMethod);

                std::sort(travelTimesForMeanMethod.begin(), travelTimesForMeanMethod.end());

                
                size_t iMedianIndex = travelTimesForMeanMethod.size() / 2;
                if (travelTimesForMeanMethod.size() % 2 == 0)
                {
                    dbITT = ((travelTimesForMeanMethod[iMedianIndex - 1] + travelTimesForMeanMethod[iMedianIndex]) / 2.0) - travelTimesForMeanMethod[iMedianIndex - 1];
                }
                else
                {
                    dbITT = travelTimesForMeanMethod[iMedianIndex] - ((travelTimesForMeanMethod[iMedianIndex] + travelTimesForMeanMethod[iMedianIndex - 1]) / 2.0);
                }
            }
        }
        else
        {
            dbITT = 0;
        }
    }
    else
    {
        if (nbVeh > 0)
        {
            dbITT = dbITT / nbVeh;
        }
        else
        {
            dbITT = 0;
        }
    }
}

double TravelTimeUtils::ComputeTravelTime(const std::vector<std::pair<std::pair<double, double>, double> > & exitInstantAndTravelTimesWithLengths, double dbMeanConcentration, double dbEmptyTravelTime, bool bIsForbidden, int nbVehiclesForTravelTimes, double dbConcentrationRatioForFreeFlowCriterion, double dbKx)
{
    double dbResult;

    if (bIsForbidden)
    {
        dbResult = std::numeric_limits<double>::infinity();
    }
    else
    {
        if (!exitInstantAndTravelTimesWithLengths.empty())
        {
            assert(nbVehiclesForTravelTimes > 0);

            double dbTps = 0;
            int nbVeh = std::min<int>(nbVehiclesForTravelTimes, (int)exitInstantAndTravelTimesWithLengths.size());
            for (int iVeh = 0; iVeh < nbVeh; iVeh++)
            {
                // rmq : on se ramène au temps que le véhicule aurait mis pour faire la longueur moyenne des itinéraires en zone via le ratio caculé précédemment
                dbTps += exitInstantAndTravelTimesWithLengths[iVeh].first.second * exitInstantAndTravelTimesWithLengths[iVeh].second;
            }
            dbResult = dbTps / nbVeh;
        }
        else
        {
            if (dbMeanConcentration < dbConcentrationRatioForFreeFlowCriterion * dbKx)
            {
                // tuyau relativement vide
                dbResult = dbEmptyTravelTime;
            }
            else
            {
                // tuyau relativement plein
                dbResult = std::numeric_limits<double>::infinity();
            }
        }
    }

    return dbResult;
}

double TravelTimeUtils::ComputeSpatialTravelTime(double dbTotalTravelledTime, double dbTotalTravelledDistance, double dbLength, double dbMeanConcentration, double dbTpsVitReg, bool bIsForbidden, double dbConcentrationRatioForFreeFlowCriterion, double dbKx)
{
    double dbResult;
    if (bIsForbidden)
    {
        dbResult = std::numeric_limits<double>::infinity();
    }
    else
    {
        if (dbTotalTravelledTime > 0)
        {
            if (dbTotalTravelledDistance > 0)
            {
                double dbSpatialSpeed = dbTotalTravelledDistance / dbTotalTravelledTime;
                dbResult = dbLength / dbSpatialSpeed;
            }
            else
            {
                if (dbMeanConcentration < dbConcentrationRatioForFreeFlowCriterion * dbKx)
                {
                    // tuyau relativement vide
                    dbResult = dbTpsVitReg;
                }
                else
                {
                    // tuyau relativement plein
                    dbResult = std::numeric_limits<double>::infinity();
                }
            }
        }
        else
        {
            // Pas de temps passé : on n'a vu passer personne
            dbResult = dbTpsVitReg;
        }
    }
    return dbResult;
}

double TravelTimeUtils::ComputeSpatialTravelTime(double dbMeanSpeed, double dbEmptyMeanSpeed, double dbMeanLengthForPaths, double dbTotalMeanConcentration, bool bIsForbidden, double dbConcentrationRatioForFreeFlowCriterion, double dbKx)
{
    double dbResult;
    if (bIsForbidden)
    {
        dbResult = std::numeric_limits<double>::infinity();
    }
    else
    {
        if (dbMeanSpeed <= 0)
        {
            if (dbTotalMeanConcentration < dbConcentrationRatioForFreeFlowCriterion * dbKx)
            {
                if (dbEmptyMeanSpeed >= 0)
                {
                    dbResult = dbMeanLengthForPaths / dbEmptyMeanSpeed;
                }
                else
                {
                    dbResult = std::numeric_limits<double>::infinity();
                }
            }
            else
            {
                // Zone complètement congestionnée
                dbResult = std::numeric_limits<double>::infinity();
            }
        }
        else
        {
            dbResult = dbMeanLengthForPaths / dbMeanSpeed;
        }
    }
    return dbResult;
}

void TravelTimeUtils::ComputeTravelTimeAndMarginalAtBusStation(double dbMeanDescentTime, double dbLineFreq, double dbLastGetOnRatio, double dbLastStopDuration, bool bComingFromAnotherLine, double & dbTravelTime, double & dbMarginal)
{
    // penalty time = descent time + wait time + stop duration on the downstream line
    double dbDescentTime = bComingFromAnotherLine ? dbMeanDescentTime : 0;

    // mean wai time = frequency / 2
    double dbMeanWaitTime = dbLineFreq / 2;

    // take into account the users unable to get on the bus : they have a higher wait time
    // if freq is +INF (no bus), skip this part to avoid 0 * INF => IND and keep a INF mean wait time
    if (dbLineFreq < std::numeric_limits<double>::infinity())
    {
        dbMeanWaitTime += (1.0 - dbLastGetOnRatio) * dbLineFreq;
    }

    dbTravelTime = dbDescentTime + dbMeanWaitTime + dbLastStopDuration;

    // marginal :
    dbMarginal = dbTravelTime;
    if (dbLastGetOnRatio < 1) // if some people couldn't get on
    {
        if (dbLastGetOnRatio > 0.5)
        {
            dbMarginal += dbLineFreq;
        }
        else
        {
            dbMarginal += std::max<double>(dbLineFreq, (dbLineFreq / 2) / dbLastGetOnRatio);
        }
    }
}
