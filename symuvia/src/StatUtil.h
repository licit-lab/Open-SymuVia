#pragma once

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/variate_generator.hpp>

class StatUtil
{
public:
	StatUtil(double moyenne, double ecart_type);
	~StatUtil(void);

private:
	double m_dbMoyenne;
	boost::mt19937 m_rng;
	boost::normal_distribution<double> *m_pGauss;
	boost::variate_generator<boost::mt19937&, boost::normal_distribution<> > *m_pRandom;

public:
	double NormalRandom();
};
