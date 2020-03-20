#include "TravelIndicatorClass.h"

#include <algorithm>
#include <iostream>

using namespace SymuCore;

TravelIndicatorClass::TravelIndicatorClass()
{
	m_dbLowerBound = 0;		
	m_dbUpperBound = 0;		

	m_bActive = false;			

	m_dbAverageTTT = 0;		
	m_dbAverageTTD = 0;		

	m_nNbPoints = 0;		
	m_nMaxNbPoints = 0;		

	m_dbSigmaTTT=0;		
	m_dbSigmaTTD=0;		
	m_NbPointsForSigma=0;	
}

TravelIndicatorClass::TravelIndicatorClass(double dbLowerBound, double dbUpperBound, bool bActive, int nMaxNbPoints, int nNbPointsForSigma)
{
	m_dbLowerBound = dbLowerBound;
	m_dbUpperBound = dbUpperBound;

	m_dbMiddle = (dbLowerBound + dbUpperBound) / 2.0;

	m_bActive = bActive;

	m_dbAverageTTT = 0;
	m_dbAverageTTD = 0;

	m_nNbPoints = 0;
	m_nMaxNbPoints = nMaxNbPoints;

	m_dbSigmaTTT = -1;
	m_dbSigmaTTD = -1;
	m_NbPointsForSigma = nNbPointsForSigma;
}

TravelIndicatorClass::~TravelIndicatorClass()
{}

// Add point to the class
bool TravelIndicatorClass::AddPoint(double dbLength, double ttt, double ttd)
{
	if (m_nNbPoints >= m_nMaxNbPoints)	// Enough points
		return false;

	// Check the local growth hypothesis
	if (m_nNbPoints > 0)
		if ((ttt > m_dbAverageTTT && ttd < m_dbAverageTTD) || (ttt < m_dbAverageTTT && ttd > m_dbAverageTTD))
			return false;
		
	// Check statistical relevance
	if (m_nNbPoints > m_NbPointsForSigma && m_dbSigmaTTT>0 && m_dbSigmaTTD>0)
	{
		if (ttt >= m_dbAverageTTT + m_dbSigmaTTT || ttt <= m_dbAverageTTT - m_dbSigmaTTT)
			return false;

		if (ttd >= m_dbAverageTTD + m_dbSigmaTTD || ttd <= m_dbAverageTTD - m_dbSigmaTTD)
			return false;
	}

	// Update averages
	m_dbAverageTTT = (ttt + m_nNbPoints*m_dbAverageTTT) / (m_nNbPoints + 1);
	m_dbAverageTTD = (ttd + m_nNbPoints*m_dbAverageTTD) / (m_nNbPoints + 1);
	m_nNbPoints++;

//	m_bActive = true;

	// Sigma calculations (TTT and TTD standard deviation)
	if (m_nNbPoints >= m_NbPointsForSigma && m_dbSigmaTTT == -1 && m_dbSigmaTTD == -1)
	{
		boost::accumulators::accumulator_set<double, boost::accumulators::stats<boost::accumulators::tag::mean, boost::accumulators::tag::variance>> accTTT;	
		accTTT = std::for_each(m_dataTTT.begin(), m_dataTTT.end(), accTTT);
		m_dbSigmaTTT = sqrt(boost::accumulators::variance(accTTT));

		boost::accumulators::accumulator_set<double, boost::accumulators::stats<boost::accumulators::tag::mean, boost::accumulators::tag::variance>> accTTD;
		accTTD = std::for_each(m_dataTTT.begin(), m_dataTTT.end(), accTTD);
		m_dbSigmaTTD = sqrt(boost::accumulators::variance(accTTD));
	}
	
	m_dbTravelSpeed = (m_dbAverageTTD / m_dbAverageTTT);
	m_dbTravelTime = dbLength / (m_dbAverageTTD / m_dbAverageTTT);

	m_dataTTT.push_back(ttt);
	m_dataTTD.push_back(ttd);

	//std::cout << " TT: " << m_dbTT << " Nb points: " << m_nNbPoints;
	//std::cout << ";" << m_dbTT << m_nNbPoints;

	return true;
}