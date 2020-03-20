#pragma once

#ifndef SYMUCORE_TRAVEL_TIME_UTILS_H
#define SYMUCORE_TRAVEL_TIME_UTILS_H

#include "SymuCoreExports.h"

#include <map>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

    class MacroType;

class SYMUCORE_DLL_DEF TravelTimeUtils {

public:

    static void SelectVehicleForTravelTimes(const std::map<double, std::map<int, std::pair<bool, std::pair<double, double> > > > & mapUsefuleVehicles, double dbInstFinPeriode, double dbPeriodDuration, int nbPeriodsForTravelTimes,
        int nbVehiclesForTravelTimes, int nbMarginalsDeltaN, bool bUseMeanMethod, bool bUseMedianMethod,
        double & dbITT, std::vector<std::pair<std::pair<double, double>, double> > & exitInstantAndTravelTimesWithLengthsForMacroType,
        std::vector<std::pair<std::pair<double, double>, double> > & exitInstantAndTravelTimesWithLengthsForAllMacroTypes, std::vector<int> & vehicleIDsToClean);

    static double ComputeTravelTime(const std::vector<std::pair<std::pair<double, double>, double> > & exitInstantAndTravelTimesWithLengths, double dbMeanConcentration, double dbEmptyTravelTime, bool bIsForbidden, int nbVehiclesForTravelTimes, double dbConcentrationRatioForFreeFlowCriterion, double dbKx);
    static double ComputeSpatialTravelTime(double dbTravelledTime, double dbTravelledDistance, double dbLength, double dbMeanConcentration, double dbTpsVitReg, bool bIsForbidden, double dbConcentrationRatioForFreeFlowCriterion, double dbKx);
    static double ComputeSpatialTravelTime(double dbMeanSpeed, double dbEmptyMeanSpeed, double dbMeanLengthForPaths, double dbTotalMeanConcentration, bool bIsForbidden, double dbConcentrationRatioForFreeFlowCriterion, double dbKx);

    // for bus stops penalties :
    static void ComputeTravelTimeAndMarginalAtBusStation(double dbMeanDescentTime, double dbLineFreq, double dbLastGetOnRatio, double dbLastStopDuration, bool bComingFromAnotherLine, double & dbTravelTime, double & dbMarginal);

};
}

#pragma warning(pop)

#endif // SYMUCORE_TRAVEL_TIME_UTILS_H
