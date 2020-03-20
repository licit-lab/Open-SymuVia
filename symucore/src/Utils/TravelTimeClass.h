#pragma once

#ifndef SYMUCORE_TRAVEL_TIME_CLASS_H
#define SYMUCORE_TRAVEL_TIME_CLASS_H

#include "SymuCoreExports.h"
#include <vector>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

	class SYMUCORE_DLL_DEF TravelTimeClass
	{
		public:

			TravelTimeClass();
			TravelTimeClass(double dbLowerBound, double dbUpperBound, bool bActive, int nMaxNbPoints, int nNbPointsForSigma);
			virtual ~TravelTimeClass();

			bool	AddPoint(double dbLength, double ttt, double ttd);	// Add point to the class

			void	SetActive(bool bActive) { m_bActive = bActive; };
			bool	IsActive() { return m_bActive; }

			double	GetTT() { return m_dbTT; };
			//double	GetAverageTTT() { return m_dbAverageTTT; };
			int		GetNbPoints() { return m_nNbPoints; };

			double	GetLowerBound() { return m_dbLowerBound; };
			double	GetUpperBound() { return m_dbUpperBound; };
			double	GetMiddle() { return m_dbMiddle; };

		private:
			std::vector< double> m_dataTTT;	// TTT points of the class
			std::vector< double> m_dataTTD; // TTD points of the class
			std::vector< double> m_dataT;	// Travel time of the class;
			
			double	m_dbLowerBound;		// Lower bound of the class
			double	m_dbUpperBound;		// Upper bound of the classs
			double	m_dbMiddle;			// Middle of the class

			bool	m_bActive;			// Indicates whether the class is active

			double	m_dbAverageTTT;		// Average of total travel time for the class
			double	m_dbAverageTTD;		// Average of total travel distance for the class
			double	m_dbTT;				// Travel time for the class

			int		m_nNbPoints;		// Number of points used to build averages
			int		m_nMaxNbPoints;		// Maximum number of points to be used

			double	m_dbSigmaTTT;		// Standard deviation of total travel time average (if a point is outside, it is not considered)
			double	m_dbSigmaTTD;		// Standard deviation of total travel distance average (if a point is outside, it is not considered)
			int		m_NbPointsForSigma;	// Number of points to be considered to calculate sigmaTTT et sigmaTTD
			
	};
}

#pragma warning(pop)

#endif // SYMUCORE_TRAVEL_TIME_CLASS_H


