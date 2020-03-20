#pragma once

#ifndef SYMUCORE_ROBUST_TRAVEL_TIMES_HELPER_H
#define SYMUCORE_ROBUST_TRAVEL_TIMES_HELPER_H

#include "SymuCoreExports.h"
#include "TravelTimeClass.h"
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

    class TravelTimeClass;

    class SYMUCORE_DLL_DEF RobustTravelTimesHelper
    {
    public:
		RobustTravelTimesHelper();
		RobustTravelTimesHelper(int nClassNumber, double dbMaxSpatialVehNumber, double dbMinT, double dbMaxT, double dbLength, int nMaxNbPoints=40, int nNbPointsForSigma=10);
		virtual ~RobustTravelTimesHelper();

		bool				AddTravelTimesData(double dbSpatialVehNumber, double dbTotalTravelTime, double dbTotalTravelDistance);
		TravelTimeClass*	GetClass(double dbSpatialVehNumber);
		double				GetRobustTravelTime(double dbSpatialVehNumber, double dbTTD);
		int					GetLastInactiveClassAtBeginning();
		int					GetFirstInactiveClassAtEnd();
		double				GetItt(double dbN);
		std::string			GetDataString();
		void				SetData(double dbMinSpatialVehNb, double dbMaxSpatialVehNb, double dbRegXLowerBound, double dbRegXUpperBound, 
									double dbMinTravelTime, double dbMaxTravelTime, int nRegType, double dbCoefa, double dbCoefb, double dbCoefc);

		void				SetPreComputed() { m_bPreComputed = true; }
		bool				IsPreComputed() { return m_bPreComputed; }

    private:

		int		m_nActiveClassNumber;			// Number of active classe
		double	m_dbMinSpatialVehNb;			// Min of spatial vehicle number (upper bound of the first active class)
		double	m_dbMaxSpatialVehNb;			// Max of spatial vehicle number (lower bound of the last active class)
		int		m_nMaxNbPoints;
		int		m_nNbPointsForSigma;
		double	m_dbClassWidth;					// Width of active class
		double	m_dbMinTravelTime;				// Min of travel time
		double	m_dbMaxTravelTime;				// Max of travel time

		double	m_dbLowerLimitTTD;				// Lower limit of the total travel distance

		double	m_dbLength;						// Length of the link

		std::vector<TravelTimeClass*> m_Classes; // Classes

		double	m_dbCoefa;
		double	m_dbCoefb;
		double	m_dbCoefc;

		double	m_dbRegXLowerBound;				// X-axis lower bound for the regression curve
		double	m_dbRegXUpperBound;				// Y-axis upper bound for the regression curve

		enum regression {REG_UNDEFINED, REG_LINEAR, REG_QUADRATIC};
		regression m_RegressionType;

		bool	m_bNeedToUpdate;				// Indicates whether the regression calculation should be 
		bool	m_bPreComputed;					// Indicates whether the robust travel times are pre-computed (and must be loaded)

    };

}

#pragma warning(pop)

#endif // SYMUCORE_ROBUST_TRAVEL_TIMES_HELPER_H


