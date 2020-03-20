#pragma once

#ifndef SYMUCORE_ROBUST_TRAVEL_TIMES_HELPER_H
#define SYMUCORE_ROBUST_TRAVEL_TIMES_HELPER_H

#include "SymuCoreExports.h"
#include "TravelIndicatorClass.h"
#include "Graph/Model/ListTimeFrame.h"
#include "EmissionUtils.h"
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

#define DBL_LIMIT      1.00e-010 // to check if 2 numbers are equal

namespace SymuCore {

    class TravelIndicatorClass;

    class SYMUCORE_DLL_DEF RobustTravelIndicatorsHelper
    {
    public:
		enum travelindicator { time, speed };

		RobustTravelIndicatorsHelper();
		RobustTravelIndicatorsHelper(travelindicator ind, int nClassNumber, double dbMaxSpatialVehNumber, double dbMinT, double dbMaxT, double dbLength, int nMaxNbPoints=40, int nNbPointsForSigma=10);
		virtual ~RobustTravelIndicatorsHelper();

		bool				AddTravelIndicatorsData(double dbSpatialVehNumber, double dbTotalTravelTime, double dbTotalTravelDistance);
		TravelIndicatorClass*	GetClass(double dbSpatialVehNumber);
		double				GetRobustTravelIndicator(double dbSpatialVehNumber, double dbTTD, bool bEmission);
		int					GetLastInactiveClassAtBeginning();
		int					GetFirstInactiveClassAtEnd();
		double				GetDerivative(double dbN, bool bEmission);
		double				GetLinkMarginalEmission(double dbN);
		std::string			GetDataString();
		void				SetData(travelindicator	indicator, double dbLength, double dbMinSpatialVehNb, double dbMaxSpatialVehNb, double dbRegXLowerBound, double dbRegXUpperBound,
									double dbMinTravelTime, double dbMaxTravelTime, int nRegType, double dbCoefa, double dbCoefb, double dbCoefc);

		void				SetPreComputed() { m_bPreComputed = true; }
		bool				IsPreComputed() { return m_bPreComputed; }

    private:
		travelindicator	m_indicator;

		int		m_nActiveClassNumber;			// Number of active classe
		double	m_dbMinSpatialVehNb;			// Min of spatial vehicle number (upper bound of the first active class)
		double	m_dbMaxSpatialVehNb;			// Max of spatial vehicle number (lower bound of the last active class)
		int		m_nMaxNbPoints;
		int		m_nNbPointsForSigma;
		double	m_dbClassWidth;					// Width of active class
		double	m_dbLBTravelIndicator;				// Min of travel indicator
		double	m_dbUBTravelIndicator;				// Max of travel indicator

		double	m_dbLowerLimitTTD;				// Lower limit of the total travel distance

		double	m_dbLength;						// Length of the link

		double  m_nc;							// Critical n for marginal link emission
		double	m_mlec;							// Critical marginal link emission

		std::vector<TravelIndicatorClass*> m_Classes; // Classes

		double	m_dbCoefa;
		double	m_dbCoefb;
		double	m_dbCoefc;

		double	m_dbRegXLowerBound;				// X-axis lower bound for the regression curve
		double	m_dbRegXUpperBound;				// Y-axis upper bound for the regression curve

		int		m_nRollingAverageNbOfData;		// Number of data of the rolling average

		std::vector<double> m_lstRollingSpatialVehNumber; // Vector for storing the spatial vehicles number (to calculate the rolling average)
		std::vector<double> m_lstRollingTTT;				// Vector for storing the total travel time (to calculate the rolling average)
		std::vector<double> m_lstRollingTTD;				// Vector for storing the total travel distance (to calculate the rolling average)

		enum regression {REG_UNDEFINED, REG_LINEAR, REG_QUADRATIC};
		regression m_RegressionType;

		bool	m_bNeedToUpdate;				// Indicates whether the regression calculation should be 
		bool	m_bPreComputed;					// Indicates whether the robust travel times are pre-computed (and must be loaded)

		// Stockage du nombre de véhicules spatialisé pour tous les macro-types
		ListTimeFrame<double>                                   m_mapNbVehForAllMacroTypes;
    };

}

#pragma warning(pop)

#endif // SYMUCORE_ROBUST_TRAVEL_TIMES_HELPER_H
