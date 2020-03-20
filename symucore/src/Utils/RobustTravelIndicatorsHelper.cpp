#include "RobustTravelIndicatorsHelper.h"
#include "SymuCoreConstants.h"
#include <boost/format.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/math/special_functions.hpp>

//#include <boost/make_shared.hpp>

#include <math.h>
#include <numeric>

using namespace SymuCore;

static std::vector<double> polyfit2(const std::vector<double>& oX,
	const std::vector<double>& oY, int nDegree)
{
	using namespace boost::numeric::ublas;

	std::cout << std::endl << " oX.size() == oY.size() " << oX.size()  << " " << oY.size() << std::endl;
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

SymuCore::RobustTravelIndicatorsHelper::RobustTravelIndicatorsHelper()
{
	m_indicator = time;

	m_dbCoefa = 0;
	m_dbCoefb = 0;
	m_dbCoefc = 0;

	m_bNeedToUpdate = true;

	m_nActiveClassNumber = 0;
	m_dbMaxSpatialVehNb = 0;
	m_nMaxNbPoints = 0;
	m_nNbPointsForSigma = 0;

	m_RegressionType = REG_UNDEFINED;

	m_dbLowerLimitTTD = 0.1;

	m_bPreComputed = false;

	m_nc = -1;
	m_mlec = -1;

	m_nRollingAverageNbOfData = 3;
}

SymuCore::RobustTravelIndicatorsHelper::RobustTravelIndicatorsHelper(travelindicator ind, int nClassNumber, double dbMaxSpatialVehNumber, double dbMinT, double dbMaxT, double dbLength, int nMaxNbPoints, int nNbPointsForSigma)
{
	m_indicator = ind;

	m_RegressionType = REG_UNDEFINED;

	m_dbCoefa = 0;
	m_dbCoefb = 0;
	m_dbCoefc = 0;

	m_bNeedToUpdate = true;

	m_nActiveClassNumber = nClassNumber;
	m_dbMaxSpatialVehNb = dbMaxSpatialVehNumber;
	m_nMaxNbPoints = nMaxNbPoints;
	m_nNbPointsForSigma = nNbPointsForSigma;

	m_dbClassWidth = 0;
	m_dbLBTravelIndicator = dbMinT;
	m_dbUBTravelIndicator = dbMaxT;
	m_dbLength = dbLength;

	m_dbLowerLimitTTD = 0.1;

	m_bPreComputed = false;

	m_nc = -1;
	m_mlec = -1;

	m_nRollingAverageNbOfData = 3;
}

SymuCore::RobustTravelIndicatorsHelper::~RobustTravelIndicatorsHelper()
{
}

// Add travel indicator data to the classes
bool SymuCore::RobustTravelIndicatorsHelper::AddTravelIndicatorsData(double dbSpatialVehNumber, double dbTotalTravelTime, double dbTotalTravelDistance)
{
	// Classes initialization
	if (m_Classes.empty())
	{
		double dbLowerBound = 0;
		m_dbClassWidth = m_dbMaxSpatialVehNb / (m_nActiveClassNumber + 1);

		for (int i = 0; i <= m_nActiveClassNumber; i++)
		{
			TravelIndicatorClass *TTC = new TravelIndicatorClass(dbLowerBound, dbLowerBound+ m_dbClassWidth, i>0, m_nMaxNbPoints, m_nNbPointsForSigma);
			dbLowerBound += m_dbClassWidth;
			m_Classes.push_back(TTC);
		}
		TravelIndicatorClass *TTC = new TravelIndicatorClass(dbLowerBound, m_dbMaxSpatialVehNb*2, false, m_nMaxNbPoints, m_nNbPointsForSigma);
		m_Classes.push_back(TTC);

		m_dbMinSpatialVehNb = m_dbClassWidth;
	}


	// Computation of rolling averages
	if (dbSpatialVehNumber <= DBL_LIMIT)
		return false;

	std::cout << "Data          " << dbSpatialVehNumber << " " << dbTotalTravelTime << " " << dbTotalTravelDistance << " " << m_lstRollingSpatialVehNumber.size() << std::endl;


	m_lstRollingSpatialVehNumber.push_back(dbSpatialVehNumber);
	m_lstRollingTTT.push_back(dbTotalTravelTime);
	m_lstRollingTTD.push_back(dbTotalTravelDistance);

	if (m_lstRollingSpatialVehNumber.size() >= m_nRollingAverageNbOfData)
	{
		dbSpatialVehNumber = std::accumulate(m_lstRollingSpatialVehNumber.begin(), m_lstRollingSpatialVehNumber.end(), 0.0) / m_nRollingAverageNbOfData;
		dbTotalTravelTime = std::accumulate(m_lstRollingTTT.begin(), m_lstRollingTTT.end(), 0.0) / m_nRollingAverageNbOfData;
		dbTotalTravelDistance = std::accumulate(m_lstRollingTTD.begin(), m_lstRollingTTD.end(), 0.0) / m_nRollingAverageNbOfData;

		m_lstRollingSpatialVehNumber.erase(m_lstRollingSpatialVehNumber.begin());
		m_lstRollingTTT.erase(m_lstRollingTTT.begin());
		m_lstRollingTTD.erase(m_lstRollingTTD.begin());
	}
	else
		return false;

	std::cout << "Rolling average " << dbSpatialVehNumber << " " << dbTotalTravelTime << " " << dbTotalTravelDistance << std::endl;


	if (dbTotalTravelDistance < m_dbLowerLimitTTD )
	{
		//std::cout << ";0";
		return false;
	}
		
	// Search for the corresponding classe
	TravelIndicatorClass *TTC = GetClass(dbSpatialVehNumber);
	if (!TTC)			
	{
		//std::cout << ";0";
		return false;
	}

	if(!TTC->IsActive())	// Belongs to the inactive classes, no processing
	{
		//std::cout << ";" << TTC->GetTravelTime() << ";" << ntot << ";" << nbclasse;
		return true;
	}

	if(dbTotalTravelDistance>m_dbLowerLimitTTD && dbTotalTravelTime>0)
	{
		if ( m_indicator == time && m_dbLength / (dbTotalTravelDistance / dbTotalTravelTime) > m_dbUBTravelIndicator )
		{
			return true;
		}

		if (m_indicator == speed && (dbTotalTravelDistance / dbTotalTravelTime) < m_dbUBTravelIndicator)
		{
			return true;
		}
	
		m_bNeedToUpdate = TTC->AddPoint(m_dbLength, dbTotalTravelTime, dbTotalTravelDistance);
	}
	return true;
}

TravelIndicatorClass * SymuCore::RobustTravelIndicatorsHelper::GetClass(double dbSpatialVehNumber)
{
	if (m_dbClassWidth <= 0)
		return nullptr;

	int nClass = (int)(dbSpatialVehNumber / m_dbClassWidth);

	if (nClass >= m_Classes.size())	
		return nullptr;

	return m_Classes[nClass];
}

// Returns the index of the last inactive class at the beginning
int SymuCore::RobustTravelIndicatorsHelper::GetLastInactiveClassAtBeginning()
{
	for (int i = 0; i < m_Classes.size(); i++)
	{
		if (m_Classes[i]->IsActive())
			return i - 1;
	}

	return (int)m_Classes.size();
}

// Returns the index of the first inactive class at the end
int SymuCore::RobustTravelIndicatorsHelper::GetFirstInactiveClassAtEnd()
{
	for (int i = (int)m_Classes.size()-1; i >= 0 ; i--)
	{
		if (m_Classes[i]->IsActive())
			return i + 1;
	}

	return 0;
}

double SymuCore::RobustTravelIndicatorsHelper::GetRobustTravelIndicator(double dbSpatialVehNumber, double dbTTD, bool bEmission)
{
	if(m_bNeedToUpdate)
	{ 
		// 1 - Classes to take into account (active and with data)
		std::vector<TravelIndicatorClass*> vctClasses;
		std::vector<TravelIndicatorClass*>::iterator itC;
		for (itC = m_Classes.begin(); itC != m_Classes.end(); itC++)
		{
			if ((*itC)->IsActive() && (*itC)->GetNbPoints() > 0 )
					vctClasses.push_back(*itC);
		}
		
		// 2 - Regression calculation
		double X1, X2, Y1, Y2;
		// Linear regression
		if((int)vctClasses.size()<3)
		{ 
			switch ((int)vctClasses.size())
			{
				case 0:
					X1 = m_dbMinSpatialVehNb;
					Y1 = m_dbLBTravelIndicator;
					X2 = m_dbMaxSpatialVehNb;
					Y2 = m_dbUBTravelIndicator;
					break;

				case 1:
					X1 = m_dbMinSpatialVehNb;
					Y1 = m_dbLBTravelIndicator;
					X2 = vctClasses[0]->GetMiddle();

					if(m_indicator==time)
						Y2 = std::max(m_dbLBTravelIndicator,vctClasses[0]->GetTravelTime());
					else
						Y2 = std::min(m_dbLBTravelIndicator, vctClasses[0]->GetTravelSpeed());
					break;

				case 2:
					X2 = vctClasses[1]->GetMiddle();

					if (m_indicator == time)
						Y2 = std::max(m_dbLBTravelIndicator,vctClasses[1]->GetTravelTime());
					else
						Y2 = std::min(m_dbLBTravelIndicator, vctClasses[1]->GetTravelSpeed());

					if (m_indicator == time)
					{
						// Increasing function
						if(std::max(m_dbLBTravelIndicator, vctClasses[1]->GetTravelTime()) > std::max(m_dbLBTravelIndicator, vctClasses[0]->GetTravelTime())) // Increasing function (a>0)
						{
							X1 = vctClasses[0]->GetMiddle();
							Y1 = std::max(m_dbLBTravelIndicator, vctClasses[0]->GetTravelTime());
						}
						else
						{ 
							X1 = m_dbMinSpatialVehNb;
							Y1 = m_dbLBTravelIndicator;
						}
					}
					else
					{
						// Decreasing function
						if (std::min(m_dbLBTravelIndicator, vctClasses[1]->GetTravelSpeed()) < std::min(m_dbLBTravelIndicator, vctClasses[0]->GetTravelSpeed())) // Increasing function (a<0)
						{
							X1 = vctClasses[0]->GetMiddle();
							Y1 = std::min(m_dbLBTravelIndicator, vctClasses[0]->GetTravelSpeed());
						}
						else
						{
							X1 = m_dbMinSpatialVehNb;
							Y1 = m_dbLBTravelIndicator;
						}
					}
			}
			m_RegressionType = REG_LINEAR;
			m_dbCoefa = (Y2 - Y1) / (X2 - X1);
			m_dbCoefb = Y1 - m_dbCoefa * X1;
		}
		else
		{
			// Quadratic regression
			std::vector<double> oX, oY;
			double dbPrevY, dbTmpY;

			if (m_indicator == time)
				dbPrevY = m_dbLBTravelIndicator;
			else
				dbPrevY = m_dbLBTravelIndicator;

			//oX.push_back(m_dbMinSpatialVehNb);
			//oY.push_back(m_dbLBTravelIndicator);

			for (size_t iClass = 0; iClass < vctClasses.size(); iClass++)
			{
				oX.push_back(vctClasses[iClass]->GetMiddle());

				if (m_indicator == time)
				{
					dbTmpY = std::max(dbPrevY, vctClasses[iClass]->GetTravelTime());
					oY.push_back(std::max(m_dbLBTravelIndicator, dbTmpY));
				}
				else
				{
					dbTmpY = std::min(dbPrevY, vctClasses[iClass]->GetTravelSpeed());
					oY.push_back(std::max(m_dbUBTravelIndicator, dbTmpY));
				}
				dbPrevY = oY.back();
			}
			if (m_indicator == speed)
			{
				oX.push_back(m_dbMaxSpatialVehNb);
				oY.push_back(m_dbUBTravelIndicator);
			}

			std::vector<double> polyCoefs = polyfit2(oX, oY, 2);
			//std::vector<double> polyCoefs = polyfit2(oX, oY, 1);
			if (polyCoefs.empty())
			{
				// we don't know how to do it :
				std::cout << "polyCoefs.empty() " << oX.size() << " " << oY.size() << std::endl;
				for(int i=0;i<oX.size();i++)
					std::cout << oX[i] << " " ;
				for (int i = 0; i < oY.size(); i++)
					std::cout << oY[i] << " ";
				std::cout << std::endl;
				assert(false);
			}
			else
			{
				if ((!boost::math::isnan(polyCoefs[2]) && !boost::math::isnan(polyCoefs[1])) == false)
				{
					std::cout << "!boost::math::isnan(polyCoefs[2]) && !boost::math::isnan(polyCoefs[1]) " << oX.size() << " " << oY.size() << std::endl;
					for (int i = 0; i < oX.size(); i++)
						std::cout << oX[i] << " ";
					for (int i = 0; i < oY.size(); i++)
						std::cout << oY[i] << " ";
				}
				std::cout << std::endl << polyCoefs[2] << " " << polyCoefs[1] << polyCoefs[0] << std::endl;
				
				//assert(!boost::math::isnan(polyCoefs[2]) && !boost::math::isnan(polyCoefs[1]));

				if ( (m_indicator == time && 2.0 * polyCoefs[2] * m_dbMinSpatialVehNb + polyCoefs[1] >= -DBL_LIMIT && 2.0 * polyCoefs[2] * m_dbMaxSpatialVehNb + polyCoefs[1] >= -DBL_LIMIT)
				 || (m_indicator == speed && 2.0 * polyCoefs[2] * m_dbMinSpatialVehNb + polyCoefs[1] <= DBL_LIMIT && 2.0 * polyCoefs[2] * m_dbMaxSpatialVehNb + polyCoefs[1] <= DBL_LIMIT))
				{
					std::cout << "REG_QUADRATIC" << std::endl;
					m_RegressionType = REG_QUADRATIC;
					m_dbCoefa = polyCoefs[2];
					m_dbCoefb = polyCoefs[1];
					m_dbCoefc = polyCoefs[0];
				}
				else
				{
					m_RegressionType = REG_LINEAR;

					std::cout << "REG_LINEAR" << std::endl;

					polyCoefs.empty();
					polyCoefs = polyfit2(oX, oY, 1);

					std::cout << std::endl << polyCoefs[2] << " " << polyCoefs[1] << " " << polyCoefs[0] << std::endl;
					if (polyCoefs.empty())
					{
						// we don't know how to do it :
						std::cout << "polyCoefs.empty() REG_LINEAR" << oX.size() << " " << oY.size() << std::endl;
						for (int i = 0; i < oX.size(); i++)
							std::cout << oX[i] << " ";
						for (int i = 0; i < oY.size(); i++)
							std::cout << oY[i] << " ";
						std::cout << std::endl;
						assert(false);
					}
					else
						if ( (m_indicator == time && polyCoefs[1] > -DBL_LIMIT)  || (m_indicator == speed && polyCoefs[1] < DBL_LIMIT)) // a>0 if time, a<0 if speed
						{
							m_dbCoefa = polyCoefs[1];
							m_dbCoefb = polyCoefs[0];
						}
						else
						{
							// search the first inconsistent total travel indicator
							size_t iClass;
							for (iClass = 0; iClass < vctClasses.size()-1; iClass++)
							{
								if (m_indicator == time && std::max(m_dbLBTravelIndicator, vctClasses[iClass]->GetTravelTime()) > std::max(m_dbLBTravelIndicator, vctClasses[iClass + 1]->GetTravelTime()))
									break;

								if (m_indicator == speed && std::min(m_dbLBTravelIndicator, vctClasses[iClass]->GetTravelSpeed()) < std::min(m_dbLBTravelIndicator, vctClasses[iClass + 1]->GetTravelSpeed()))
									break;
							}
							oX.empty();
							oY.empty();
							for (size_t i = 0; i <= iClass; i++)
							{
								oX.push_back(vctClasses[i]->GetMiddle());

								if(m_indicator==time)
									oY.push_back(std::max(m_dbLBTravelIndicator, vctClasses[i]->GetTravelTime()));
								else
									oY.push_back(std::max(m_dbLBTravelIndicator, vctClasses[i]->GetTravelSpeed()));
							}
							if (oX.size() > 1)
							{
								polyCoefs.empty();
								polyCoefs = polyfit2(oX, oY, 1);
								std::cout << "l 457 " << polyCoefs[2] << " " << polyCoefs[1] << " " << polyCoefs[0] << std::endl;
								if (polyCoefs.empty())
								{
									// we don't know how to do it :
									std::cout << "polyCoefs.empty() REG_LINEAR 2" << oX.size() << " " << oY.size() << std::endl;
									for (int i = 0; i < oX.size(); i++)
										std::cout << oX[i] << " ";
									for (int i = 0; i < oY.size(); i++)
										std::cout << oY[i] << " ";
									std::cout << std::endl;
									assert(false);
								}
								else
								{
									if ( (m_indicator == time && polyCoefs[1] > -DBL_LIMIT) || (m_indicator == speed && polyCoefs[1] < DBL_LIMIT) )
									{
										m_dbCoefa = polyCoefs[1];
										m_dbCoefb = polyCoefs[0];
									}
									else
									{
										X1 = m_dbMinSpatialVehNb;
										Y1 = m_dbLBTravelIndicator;
										X2 = GetClass(dbSpatialVehNumber)->GetMiddle();

										if (m_indicator == time)
											Y2 = std::max(m_dbLBTravelIndicator, GetClass(dbSpatialVehNumber)->GetTravelTime());
										else
											Y2 = std::min(m_dbLBTravelIndicator, GetClass(dbSpatialVehNumber)->GetTravelSpeed());

										if (fabs(X2 - X1) > DBL_LIMIT)
											m_dbCoefa = (Y2 - Y1) / (X2 - X1);
										else
											m_dbCoefa = 0.0;
										m_dbCoefb = Y1 - m_dbCoefa * X1;
									}
								}
							}
							else
							{
								X1 = m_dbMinSpatialVehNb;
								Y1 = m_dbLBTravelIndicator;
								X2 = oX[0];
								Y2 = oY[0];

								if (fabs(X2 - X1) > DBL_LIMIT)
									m_dbCoefa = (Y2 - Y1) / (X2 - X1);
								else
									m_dbCoefa = 0.0;
							
								m_dbCoefb = Y1 - m_dbCoefa * X1;
							}
						}
				}
			}
		}

		// Upper and lower bound calculation
		m_dbRegXLowerBound = m_dbMinSpatialVehNb;
		m_dbRegXUpperBound = m_dbMaxSpatialVehNb;

		if (m_RegressionType == REG_LINEAR)
		{
			if(m_indicator == time)
			{
				if (m_dbCoefa * m_dbMinSpatialVehNb + m_dbCoefb < m_dbLBTravelIndicator)
					m_dbRegXLowerBound = (m_dbLBTravelIndicator- m_dbCoefb) / m_dbCoefa;

				if (m_dbCoefa * m_dbMaxSpatialVehNb + m_dbCoefb > m_dbUBTravelIndicator)
					m_dbRegXUpperBound = (m_dbUBTravelIndicator - m_dbCoefb) / m_dbCoefa;
			}
			else
			{
				if (m_dbCoefa * m_dbMinSpatialVehNb + m_dbCoefb > m_dbLBTravelIndicator)
					m_dbRegXLowerBound = (m_dbLBTravelIndicator - m_dbCoefb) / m_dbCoefa;

				if (m_dbCoefa * m_dbMaxSpatialVehNb + m_dbCoefb < m_dbUBTravelIndicator)
					m_dbRegXUpperBound = (m_dbUBTravelIndicator - m_dbCoefb) / m_dbCoefa;
			}
		}
		else // Quadratic case
		{
			if (m_indicator == time)
			{
				if (m_dbCoefa * std::pow(m_dbMinSpatialVehNb, 2) + m_dbCoefb * m_dbMinSpatialVehNb + m_dbCoefc < m_dbLBTravelIndicator)
					m_dbRegXLowerBound = (- m_dbCoefb + std::sqrt( - 4 * m_dbCoefa*(m_dbCoefc - m_dbLBTravelIndicator) + std::pow(m_dbCoefb, 2))) / (2 * m_dbCoefa);

				if (m_dbCoefa * std::pow(m_dbMaxSpatialVehNb, 2) + m_dbCoefb * m_dbMaxSpatialVehNb + m_dbCoefc > m_dbUBTravelIndicator)
					m_dbRegXUpperBound = (-m_dbCoefb + std::sqrt(-4 * m_dbCoefa*(m_dbCoefc - m_dbUBTravelIndicator) + std::pow(m_dbCoefb, 2))) / (2 * m_dbCoefa);
			}
			else
			{
				if (m_dbCoefa * std::pow(m_dbMinSpatialVehNb, 2) + m_dbCoefb * m_dbMinSpatialVehNb + m_dbCoefc > m_dbLBTravelIndicator)
					m_dbRegXLowerBound = (-m_dbCoefb - std::sqrt(-4 * m_dbCoefa*(m_dbCoefc - m_dbLBTravelIndicator) + std::pow(m_dbCoefb, 2))) / (2 * m_dbCoefa);

				if (m_dbCoefa * std::pow(m_dbMaxSpatialVehNb, 2) + m_dbCoefb * m_dbMaxSpatialVehNb + m_dbCoefc < m_dbUBTravelIndicator)
					m_dbRegXUpperBound = (-m_dbCoefb - std::sqrt(-4 * m_dbCoefa*(m_dbCoefc - m_dbUBTravelIndicator) + std::pow(m_dbCoefb, 2))) / (2 * m_dbCoefa);
			}
		}
		
		m_bNeedToUpdate = false;
	}
	
	//std::cout << ";" << m_RegressionType << ";" << m_dbCoefa << ";" << m_dbCoefb << ";" << m_dbCoefc <<
		//";" << m_dbMinSpatialVehNb << ";" << m_dbMaxSpatialVehNb << ";" << m_dbLBTravelIndicator << ";" << m_dbUBTravelIndicator;

	if(!bEmission)
	{
		// 31/08/19: travel time calculé à partir des fonctions vitesse
		if(m_indicator==speed)
		{
			if(dbTTD < m_dbLowerLimitTTD)
			{
				if (dbSpatialVehNumber>= m_dbMaxSpatialVehNb)
				{
					return m_dbLength/m_dbUBTravelIndicator;
				}
				else
				{
					return m_dbLength / m_dbLBTravelIndicator;
				}
			}
			if (dbSpatialVehNumber <= m_dbRegXLowerBound)
				return m_dbLength / m_dbLBTravelIndicator;

			if (dbSpatialVehNumber >= m_dbRegXUpperBound)
				return m_dbLength / m_dbUBTravelIndicator;

			if (m_RegressionType == REG_LINEAR)
				return m_dbLength /  ((m_dbCoefa * dbSpatialVehNumber) + m_dbCoefb);
	
			if (m_RegressionType == REG_QUADRATIC)
				return (m_dbCoefa * std::pow(dbSpatialVehNumber,2)) + (m_dbCoefb * dbSpatialVehNumber) + m_dbCoefc;

			return -1;
		}
		else if (m_indicator == time)
		{
			if (dbTTD < m_dbLowerLimitTTD)
			{
				if (dbSpatialVehNumber >= m_dbMaxSpatialVehNb)
				{
					return m_dbUBTravelIndicator;
				}
				else
				{
					return m_dbLBTravelIndicator;
				}
			}
			if (dbSpatialVehNumber <= m_dbRegXLowerBound)
				return m_dbLBTravelIndicator;

			if (dbSpatialVehNumber >= m_dbRegXUpperBound)
				return m_dbUBTravelIndicator;

			if (m_RegressionType == REG_LINEAR)
				return m_dbCoefa * dbSpatialVehNumber + m_dbCoefb;

			if (m_RegressionType == REG_QUADRATIC)
				return (m_dbCoefa * std::pow(dbSpatialVehNumber, 2)) + (m_dbCoefb * dbSpatialVehNumber) + m_dbCoefc;

			return -1;
		}
		return -1;
	}
	else
	{
		if (dbSpatialVehNumber <= 0)
			return 0.00;

		if (dbTTD < m_dbLowerLimitTTD)
		{
			if (dbSpatialVehNumber >= m_dbMaxSpatialVehNb)
				dbSpatialVehNumber = m_dbMaxSpatialVehNb;
			else
			{
				dbSpatialVehNumber = m_dbMinSpatialVehNb;
			}
		}
		if (m_RegressionType == REG_LINEAR)
			return EmissionUtils::emission(SymuCore::P_CO, 0, m_dbCoefa, m_dbCoefb, dbSpatialVehNumber, m_dbRegXLowerBound, m_dbRegXUpperBound, m_dbLBTravelIndicator, m_dbUBTravelIndicator, 60, dbTTD);

		if (m_RegressionType == REG_QUADRATIC)
			return EmissionUtils::emission(SymuCore::P_CO, m_dbCoefa, m_dbCoefb, m_dbCoefc, dbSpatialVehNumber, m_dbRegXLowerBound, m_dbRegXUpperBound, m_dbLBTravelIndicator, m_dbUBTravelIndicator, 60, dbTTD);

		return -1;
	}
}

double RobustTravelIndicatorsHelper::GetDerivative(double dbN, bool bEmission)
{
	if (dbN < m_dbRegXLowerBound)
		return 0.0;

	if (dbN > m_dbRegXUpperBound)
		return std::numeric_limits<double>::infinity();

	if(!bEmission)
	{
		if (m_RegressionType == REG_QUADRATIC)
		{
			return 2 * m_dbCoefa * dbN + m_dbCoefb;
		}

		if (m_RegressionType == REG_LINEAR)
			return m_dbCoefa;
	}
	else
	{
		if (m_RegressionType == REG_LINEAR)
			return EmissionUtils::derivative_emission(SymuCore::P_CO, 0, m_dbCoefa, m_dbCoefb, dbN, 60);

		if (m_RegressionType == REG_QUADRATIC)
			return EmissionUtils::derivative_emission(SymuCore::P_CO, m_dbCoefa, m_dbCoefb, m_dbCoefc, dbN, 60);

		return -1;
	}

	return -1;
}

double RobustTravelIndicatorsHelper::GetLinkMarginalEmission(double dbN) 
{
	/*if (m_nc == -1)
	{
		if (m_RegressionType == REG_LINEAR)
			EmissionUtils::link_marginal_emission_ex(SymuCore::P_CO, 0, m_dbCoefa, m_dbCoefb, m_dbRegXLowerBound, m_dbRegXUpperBound, m_dbLength, m_nc, m_mlec);

		if (m_RegressionType == REG_QUADRATIC)
			EmissionUtils::link_marginal_emission_ex(SymuCore::P_CO, m_dbCoefa, m_dbCoefb, m_dbCoefc, m_dbRegXLowerBound, m_dbRegXUpperBound, m_dbLength, m_nc, m_mlec);
	}*/
	
	if (m_RegressionType == REG_LINEAR)
		return EmissionUtils::link_marginal_emission(SymuCore::P_CO, 0, m_dbCoefa, m_dbCoefb, dbN, m_dbRegXLowerBound, m_dbRegXUpperBound, 60, m_dbLength, m_nc, m_mlec);

	if (m_RegressionType == REG_QUADRATIC)
		return EmissionUtils::link_marginal_emission(SymuCore::P_CO, m_dbCoefa, m_dbCoefb, m_dbCoefc, dbN, m_dbRegXLowerBound, m_dbRegXUpperBound, 60, m_dbLength, m_nc, m_mlec);

	return 0;
}
 
std::string RobustTravelIndicatorsHelper::GetDataString()
{
	int ntot = 0;
	int nbActiveClass = 0;
	int nbActiveClassWithPoints = 0;
	for (int i = 0; i < m_Classes.size(); i++)
	{
		if (m_Classes[i]->IsActive())
		{
			nbActiveClass += 1;
		}
		ntot += m_Classes[i]->GetNbPoints();

		if (m_Classes[i]->GetNbPoints() > 0)
			nbActiveClassWithPoints += 1;
	}

	//m_MinSpatialVehNb; MaxSpatialVehNb; MinTT; MaxTT; RegressionType; a; b; c; NbOfActiveClasses; NbOfActiveClassWithPoints; NbOfUsefulPoints;
	std::ostringstream sstream;
	sstream << boost::format("%.4f;") % m_dbLength;
	sstream << boost::format("%.4f;") % m_dbClassWidth;
	sstream << boost::format("%.4f;") % m_dbMinSpatialVehNb;
	sstream << boost::format("%.4f;") % m_dbMaxSpatialVehNb;
	sstream << boost::format("%.4f;") % m_dbRegXLowerBound;
	sstream << boost::format("%.4f;") % m_dbRegXUpperBound;
	sstream << boost::format("%.4f;") % m_dbLBTravelIndicator;
	sstream << boost::format("%.4f;") % m_dbUBTravelIndicator;
	sstream << boost::format("%d;") % m_RegressionType;
	sstream << boost::format("%.4f;") % m_dbCoefa;
	sstream << boost::format("%.4f;") % m_dbCoefb;
	sstream << boost::format("%.4f;") % m_dbCoefc;
	sstream << boost::format("%d;") % nbActiveClass;
	sstream << boost::format("%d;") % nbActiveClassWithPoints;
	sstream << boost::format("%d;") % ntot;

	for (int i = 0; i < m_Classes.size(); i++)
	{
		//if (!m_Classes[i]->IsActive())
		//	sstream << "-2;";
		//else
		{
			if (m_Classes[i]->GetNbPoints() == 0)
				sstream << "-1;";
			else
				if(m_indicator==speed)
					sstream << boost::format("%.2f;") % m_Classes[i]->GetTravelSpeed();
				else
					sstream << boost::format("%.2f;") % m_Classes[i]->GetTravelTime();
		}	
	}
	for (int i = 0; i < m_Classes.size(); i++)
	{
		sstream << boost::format("%d;") % m_Classes[i]->GetNbPoints();
	}

	return sstream.str();
}

void RobustTravelIndicatorsHelper::SetData(travelindicator	indicator, double dbLength, double dbMinSpatialVehNb, double dbMaxSpatialVehNb, double dbRegXLowerBound, double dbRegXUpperBound,
	double dbMinTravelTime, double dbMaxTravelTime, int nRegType, double dbCoefa, double dbCoefb, double dbCoefc)
{
	m_indicator = indicator;

	m_dbLength = dbLength;
	m_dbMinSpatialVehNb = dbMinSpatialVehNb;
	m_dbMaxSpatialVehNb = dbMaxSpatialVehNb;
	m_dbRegXLowerBound = dbRegXLowerBound;
	m_dbRegXUpperBound = dbRegXUpperBound;
	m_dbLBTravelIndicator = dbMinTravelTime;
	m_dbUBTravelIndicator = dbMaxTravelTime;

	if (nRegType == 1)
		m_RegressionType = REG_LINEAR;
	else
		m_RegressionType = REG_QUADRATIC;

	nRegType=nRegType;
	m_dbCoefa=dbCoefa;
	m_dbCoefb=dbCoefb;
	m_dbCoefc=dbCoefc;

	m_bNeedToUpdate = false;
}
