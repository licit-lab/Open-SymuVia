#include "RobustTravelTimesHelper.h"

#include <boost/format.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/math/special_functions.hpp>

//#include <boost/make_shared.hpp>

#include <math.h>

using namespace SymuCore;

static std::vector<double> polyfit2(const std::vector<double>& oX,
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

SymuCore::RobustTravelTimesHelper::RobustTravelTimesHelper()
{
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
}

SymuCore::RobustTravelTimesHelper::RobustTravelTimesHelper(int nClassNumber, double dbMaxSpatialVehNumber, double dbMinT, double dbMaxT, double dbLength, int nMaxNbPoints, int nNbPointsForSigma)
{
	m_dbCoefa = 0;
	m_dbCoefb = 0;
	m_dbCoefc = 0;

	m_bNeedToUpdate = true;

	m_nActiveClassNumber = nClassNumber;
	m_dbMaxSpatialVehNb = dbMaxSpatialVehNumber;
	m_nMaxNbPoints = nMaxNbPoints;
	m_nNbPointsForSigma = nNbPointsForSigma;

	m_dbClassWidth = 0;
	m_dbMinTravelTime = dbMinT;
	m_dbMaxTravelTime = dbMaxT;
	m_dbLength = dbLength;

	m_dbLowerLimitTTD = 0.1;

	m_bPreComputed = false;
}

SymuCore::RobustTravelTimesHelper::~RobustTravelTimesHelper()
{
}

// Add travel time data to the classes
bool SymuCore::RobustTravelTimesHelper::AddTravelTimesData(double dbSpatialVehNumber, double dbTotalTravelTime, double dbTotalTravelDistance)
{
	//std::cout << ";" << dbSpatialVehNumber << ";" << dbTotalTravelTime << ";" << dbTotalTravelDistance;

	// Classes initialization
	if (m_Classes.empty())
	{
		double dbLowerBound = 0;
		m_dbClassWidth = m_dbMaxSpatialVehNb / (m_nActiveClassNumber + 1);

		for (int i = 0; i <= m_nActiveClassNumber; i++)
		{
			TravelTimeClass *TTC = new TravelTimeClass(dbLowerBound, dbLowerBound+ m_dbClassWidth, i>0, m_nMaxNbPoints, m_nNbPointsForSigma);
			dbLowerBound += m_dbClassWidth;
			m_Classes.push_back(TTC);
		}
		TravelTimeClass *TTC = new TravelTimeClass(dbLowerBound, m_dbMaxSpatialVehNb*2, false, m_nMaxNbPoints, m_nNbPointsForSigma);
		m_Classes.push_back(TTC);

		m_dbMinSpatialVehNb = m_dbClassWidth;
	}

	if (dbTotalTravelDistance < m_dbLowerLimitTTD )
	{
		//std::cout << ";0";
		return false;
	}
		
	// Search for the corresponding classe
	TravelTimeClass *TTC = GetClass(dbSpatialVehNumber);
	if (!TTC)			
	{
		//std::cout << ";0";
		return false;
	}

	if(!TTC->IsActive())	// Belongs to the inactive classes, no processing
	{
		//std::cout << ";4";
		/*int ntot = 0;
		int nbclasse = 0;
		for (int i = 0; i < m_Classes.size(); i++)
		{
			ntot += m_Classes[i]->GetNbPoints();
			if (m_Classes[i]->GetNbPoints() > 0)
				nbclasse += 1;
		}*/
		//std::cout << ";" << TTC->GetTT() << ";" << ntot << ";" << nbclasse;
		return true;
	}

	double dbTT;

	if(dbTotalTravelDistance>m_dbLowerLimitTTD && dbTotalTravelTime>0)
	{
		dbTT = m_dbLength / (dbTotalTravelDistance / dbTotalTravelTime );
		//std::cout << " TT " << dbTT;
	
	/*	if ( dbTT < m_dbMinTravelTime )
		{
			//m_dbMinTravelTime = dbTT;
			int nClassToDeactivate = GetLastInactiveClassAtBeginning() + 1;
			if (nClassToDeactivate < m_Classes.size())
			{ 
				m_Classes[nClassToDeactivate]->SetActive(false);
				m_dbMinSpatialVehNb = m_Classes[nClassToDeactivate]->GetUpperBound();
			}
			//std::cout << ";2";
			int ntot = 0;
			int nbclasse = 0;
			for (int i = 0; i < m_Classes.size(); i++)
			{
				ntot += m_Classes[i]->GetNbPoints();
				if (m_Classes[i]->GetNbPoints() > 0)
					nbclasse += 1;
			}
			//std::cout << ";" << dbTT << ";" << ntot << ";" << nbclasse;
			return true;
		}*/

		if ( dbTT > m_dbMaxTravelTime )
		{
			//m_dbMaxTravelTime = dbTT;
			/*int nClassToDeactivate = this->GetFirstInactiveClassAtEnd() - 1;
			if (nClassToDeactivate > 0)
			{
				m_Classes[nClassToDeactivate]->SetActive(false);
				m_dbMaxSpatialVehNb = m_Classes[nClassToDeactivate]->GetLowerBound();
				m_bNeedToUpdate = true;
			}
			//std::cout << ";3";
			int ntot = 0;
			int nbclasse = 0;
			for (int i = 0; i < m_Classes.size(); i++)
			{
				ntot += m_Classes[i]->GetNbPoints();
				if (m_Classes[i]->GetNbPoints() > 0)
					nbclasse += 1;
			}*/
			std::cout << "dbTT > m_dbMaxTravelTime: " << dbTT << ";" << dbTotalTravelDistance << ";" << dbTotalTravelTime << ";" << m_dbLength << std::endl;
			return true;
		}
	
		m_bNeedToUpdate = TTC->AddPoint(m_dbLength, dbTotalTravelTime, dbTotalTravelDistance);
	}
	return true;
}

TravelTimeClass * SymuCore::RobustTravelTimesHelper::GetClass(double dbSpatialVehNumber)
{
	if (m_dbClassWidth <= 0)
		return nullptr;

	int nClass = (int)(dbSpatialVehNumber / m_dbClassWidth);

	if (nClass >= m_Classes.size())	
		return nullptr;

	return m_Classes[nClass];
}

// Returns the index of the last inactive class at the beginning
int SymuCore::RobustTravelTimesHelper::GetLastInactiveClassAtBeginning()
{
	for (int i = 0; i < m_Classes.size(); i++)
	{
		if (m_Classes[i]->IsActive())
			return i - 1;
	}

	return (int)m_Classes.size();
}

// Returns the index of the first inactive class at the end
int SymuCore::RobustTravelTimesHelper::GetFirstInactiveClassAtEnd()
{
	for (int i = (int)m_Classes.size()-1; i >= 0 ; i--)
	{
		if (m_Classes[i]->IsActive())
			return i + 1;
	}

	return 0;
}

double SymuCore::RobustTravelTimesHelper::GetRobustTravelTime(double dbSpatialVehNumber, double dbTTD)
{
	if(m_bNeedToUpdate)
	{ 
		// Update robust travel time function
		// 1 - Update active/inactive classes

		// At the beginning
	/*	int nIndex = GetLastInactiveClassAtBeginning();
		if(nIndex<m_Classes.size()-1)
		{
			TravelTimeClass *TTC = m_Classes[nIndex + 1];
			if (TTC->GetNbPoints() > 0)
				if (TTC->GetTT() < m_dbMinTravelTime)
				{
					TTC->SetActive(false);
					m_dbMinSpatialVehNb = TTC->GetUpperBound();
				}
					
		}
		
		// At the end
		nIndex = GetFirstInactiveClassAtEnd();
		if (nIndex>2)
		{
			TravelTimeClass *TTC = m_Classes[nIndex - 1];
			if (TTC->GetNbPoints() > 0)
				if (TTC->GetTT() > m_dbMaxTravelTime)
				{
					TTC->SetActive(false);
					m_dbMaxSpatialVehNb = TTC->GetLowerBound();
				}
					
		}*/

		// 2 - Classes to take into account (active and with data)
		std::vector<TravelTimeClass*> vctClasses;
		std::vector<TravelTimeClass*>::iterator itC;
		for (itC = m_Classes.begin(); itC != m_Classes.end(); itC++)
		{
			if ((*itC)->IsActive() && (*itC)->GetNbPoints() > 0 
				/*&& (*itC)->GetTT() >= m_dbMinTravelTime && (*itC)->GetTT() <= m_dbMaxTravelTime*/)
					vctClasses.push_back(*itC);
		}
		
		// 3 - Regression calculation
		double X1, X2, Y1, Y2;
		// Linear regression
		if((int)vctClasses.size()<3)
		{ 
			switch ((int)vctClasses.size())
			{
				case 0:
					X1 = m_dbMinSpatialVehNb;
					Y1 = m_dbMinTravelTime;
					X2 = m_dbMaxSpatialVehNb;
					Y2 = m_dbMaxTravelTime;
					break;
				case 1:
					X1 = m_dbMinSpatialVehNb;
					Y1 = m_dbMinTravelTime;
					X2 = vctClasses[0]->GetMiddle();
					Y2 = std::max(m_dbMinTravelTime,vctClasses[0]->GetTT());
					break;
				case 2:
					X2 = vctClasses[1]->GetMiddle();
					Y2 = std::max(m_dbMinTravelTime,vctClasses[1]->GetTT());
					// Increasing function
					if(std::max(m_dbMinTravelTime, vctClasses[1]->GetTT()) > std::max(m_dbMinTravelTime, vctClasses[0]->GetTT())) // Increasing function (a>0)
					{
						X1 = vctClasses[0]->GetMiddle();
						Y1 = std::max(m_dbMinTravelTime, vctClasses[0]->GetTT());
					}
					else
					{ 
						X1 = m_dbMinSpatialVehNb;
						Y1 = m_dbMinTravelTime;
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
			for (size_t iClass = 0; iClass < vctClasses.size(); iClass++)
			{
				oX.push_back(vctClasses[iClass]->GetMiddle());
				oY.push_back(std::max(m_dbMinTravelTime, vctClasses[iClass]->GetTT()));
			}
			std::vector<double> polyCoefs = polyfit2(oX, oY, 2);
			if (polyCoefs.empty())
			{
				// we don't know how to do it :
				assert(false);
			}
			else
			{
				assert(!boost::math::isnan(polyCoefs[2]) && !boost::math::isnan(polyCoefs[1]));
				if (2.0 * polyCoefs[2] * m_dbMinSpatialVehNb + polyCoefs[1] > 0 && 2.0 * polyCoefs[2] * m_dbMaxSpatialVehNb + polyCoefs[1] > 0)
				{
					m_RegressionType = REG_QUADRATIC;
					m_dbCoefa = polyCoefs[2];
					m_dbCoefb = polyCoefs[1];
					m_dbCoefc = polyCoefs[0];
				}
				else
				{
					m_RegressionType = REG_LINEAR;

					polyCoefs.empty();
					polyCoefs = polyfit2(oX, oY, 1);
					if (polyCoefs.empty())
					{
						// we don't know how to do it :
						assert(false);
					}
					else
						if (polyCoefs[1] > 0)   // a>0
						{
							m_dbCoefa = polyCoefs[1];
							m_dbCoefb = polyCoefs[0];
						}
						else
						{
							// search the first inconsistent total travel time
							size_t iClass;
							for (iClass = 0; iClass < vctClasses.size()-1; iClass++)
							{
								if (std::max(m_dbMinTravelTime, vctClasses[iClass]->GetTT()) > std::max(m_dbMinTravelTime, vctClasses[iClass + 1]->GetTT()))
									break;
							}
							oX.empty();
							oY.empty();
							for (size_t i = 0; i <= iClass; i++)
							{
								oX.push_back(vctClasses[i]->GetMiddle());
								oY.push_back(std::max(m_dbMinTravelTime, vctClasses[i]->GetTT()));
							}
							if (oX.size() > 1)
							{
								polyCoefs.empty();
								polyCoefs = polyfit2(oX, oY, 1);
								if (polyCoefs.empty())
								{
									// we don't know how to do it :
									assert(false);
								}
								else
								{
									if (polyCoefs[1] > 0)
									{
										m_dbCoefa = polyCoefs[1];
										m_dbCoefb = polyCoefs[0];
									}
									else
									{
										X1 = m_dbMinSpatialVehNb;
										Y1 = m_dbMinTravelTime;
										X2 = GetClass(dbSpatialVehNumber)->GetMiddle();
										Y2 = std::max(m_dbMinTravelTime, GetClass(dbSpatialVehNumber)->GetTT());

										m_dbCoefa = (Y2 - Y1) / (X2 - X1);
										m_dbCoefb = Y1 - m_dbCoefa * X1;
									}
								}
							}
							else
							{
								X1 = m_dbMinSpatialVehNb;
								Y1 = m_dbMinTravelTime;
								X2 = oX[0];
								Y2 = oY[0];

								m_dbCoefa = (Y2 - Y1) / (X2 - X1);
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
			if (m_dbCoefa * m_dbMinSpatialVehNb + m_dbCoefb < m_dbMinTravelTime)
				m_dbRegXLowerBound = (m_dbMinTravelTime- m_dbCoefb) / m_dbCoefa;

			if (m_dbCoefa * m_dbMaxSpatialVehNb + m_dbCoefb > m_dbMaxTravelTime)
				m_dbRegXUpperBound = (m_dbMaxTravelTime - m_dbCoefb) / m_dbCoefa;
		}
		else // Quadratic case
		{
			if (m_dbCoefa * std::pow(m_dbMinSpatialVehNb, 2) + m_dbCoefb * m_dbMinSpatialVehNb + m_dbCoefc < m_dbMinTravelTime)
				m_dbRegXLowerBound = (- m_dbCoefb + std::sqrt( - 4 * m_dbCoefa*(m_dbCoefc - m_dbMinTravelTime) + std::pow(m_dbCoefb, 2))) / (2 * m_dbCoefa);

			if (m_dbCoefa * std::pow(m_dbMaxSpatialVehNb, 2) + m_dbCoefb * m_dbMaxSpatialVehNb + m_dbCoefc > m_dbMaxTravelTime)
				m_dbRegXUpperBound = (-m_dbCoefb + std::sqrt(-4 * m_dbCoefa*(m_dbCoefc - m_dbMaxTravelTime) + std::pow(m_dbCoefb, 2))) / (2 * m_dbCoefa);
		}
		
		//m_dbMinSpatialVehNb=m_dbRegXLowerBound;
		//m_dbMaxSpatialVehNb= m_dbRegXUpperBound;
		m_bNeedToUpdate = false;
	}
	
	//std::cout << ";" << m_RegressionType << ";" << m_dbCoefa << ";" << m_dbCoefb << ";" << m_dbCoefc <<
		//";" << m_dbMinSpatialVehNb << ";" << m_dbMaxSpatialVehNb << ";" << m_dbMinTravelTime << ";" << m_dbMaxTravelTime;

	if(dbTTD < m_dbLowerLimitTTD)
		if (dbSpatialVehNumber>= m_dbMaxSpatialVehNb)
			return m_dbMaxTravelTime;
		else
			return m_dbMinTravelTime;

	if (dbSpatialVehNumber <= m_dbRegXLowerBound)
		return m_dbMinTravelTime;

	if (dbSpatialVehNumber >= m_dbRegXUpperBound)
		return m_dbMaxTravelTime;

	if (m_RegressionType == REG_LINEAR)
		return (m_dbCoefa * dbSpatialVehNumber) + m_dbCoefb;
	
	if (m_RegressionType == REG_QUADRATIC)
		return (m_dbCoefa * std::pow(dbSpatialVehNumber,2)) + (m_dbCoefb * dbSpatialVehNumber) + m_dbCoefc;

	return -1;
}

double RobustTravelTimesHelper::GetItt(double dbN)
{
	if (dbN < m_dbRegXLowerBound)
		return 0.0;

	if (dbN > m_dbRegXUpperBound)
		return std::numeric_limits<double>::infinity();

	if (m_RegressionType == REG_QUADRATIC)
	{
		return 2 * m_dbCoefa * dbN + m_dbCoefb;
	}

	if (m_RegressionType == REG_LINEAR)
		return m_dbCoefa;

	return -1;
}
 
std::string RobustTravelTimesHelper::GetDataString()
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
	sstream << boost::format("%.4f;") % m_dbMinTravelTime;
	sstream << boost::format("%.4f;") % m_dbMaxTravelTime;
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
				sstream << boost::format("%.2f;") % m_Classes[i]->GetTT();
		}	
	}
	for (int i = 0; i < m_Classes.size(); i++)
	{
		sstream << boost::format("%d;") % m_Classes[i]->GetNbPoints();
	}

	return sstream.str();
}

void RobustTravelTimesHelper::SetData(double dbMinSpatialVehNb, double dbMaxSpatialVehNb, double dbRegXLowerBound, double dbRegXUpperBound,
	double dbMinTravelTime, double dbMaxTravelTime, int nRegType, double dbCoefa, double dbCoefb, double dbCoefc)
{
	m_dbMinSpatialVehNb = dbMinSpatialVehNb;
	m_dbMaxSpatialVehNb = dbMaxSpatialVehNb;
	m_dbRegXLowerBound = dbRegXLowerBound;
	m_dbRegXUpperBound = dbRegXUpperBound;
	m_dbMinTravelTime = dbMinTravelTime;
	m_dbMaxTravelTime = dbMaxTravelTime;

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
