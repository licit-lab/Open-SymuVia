#include "StdAfx.h"
#include "StatUtil.h"


StatUtil::StatUtil(double moyenne, double ecart_type)
{
	m_dbMoyenne = moyenne;
	if (ecart_type <= 0)
	{
		m_pGauss = NULL;
		m_pRandom = NULL;
	}
	else
	{
		m_pGauss = new boost::normal_distribution<double>(moyenne, ecart_type);
		m_pRandom = new boost::variate_generator<boost::mt19937&, boost::normal_distribution<double>>(m_rng, *m_pGauss);
	}
}

StatUtil::~StatUtil(void)
{
	if (m_pGauss != NULL)
	{
		delete m_pGauss;
	}
	if (m_pRandom != NULL)
	{
		delete m_pRandom;
	}
}

double StatUtil::NormalRandom()
{
	if (m_pRandom == NULL)
	{
		return m_dbMoyenne;
	}
	else
	{
		return (*m_pRandom)();
	}
}
