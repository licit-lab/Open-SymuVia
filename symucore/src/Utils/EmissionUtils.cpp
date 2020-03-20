#include "EmissionUtils.h"
#include "SymuCoreConstants.h"
#include <math.h>

using namespace SymuCore;

const std::vector<double> SymuCore::EmissionUtils::getCOPERTvector(Pollutant p)
{
	std::vector<double> p_CO = { -0.00000078455,0.00018777,-0.0140,0.4799 };
	std::vector<double> p_FC = { -0.00021068, 0.0437,-2.9795,111.2141 };
	std::vector<double> p_NOx = { -0.00000044286,0.00019039,-0.0196,0.9783 };
	std::vector<double> p_PM = { -0.00000000010531,0.0000024216,-0.00031248,0.0221 };

	switch (p)
	{
	case Pollutant::P_CO:
		return p_CO;
	case Pollutant::P_FC:
		return p_FC;
	case Pollutant::P_NOx:
		return p_NOx;
	case Pollutant::P_PM:
		return p_PM;
	default:
		return std::vector<double>();
	}
}

double EmissionUtils::a1(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	return  pow(c, 4) * pow(MPS2KPH_MF, 3) * p3 +
			pow(c, 3) * pow(MPS2KPH_MF, 2) * p2 +
			pow(c, 2) * MPS2KPH_MF * p1 +
			c * p0;
}

double EmissionUtils::a2(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	return  4 * b * pow(c, 3) * pow(MPS2KPH_MF, 3) * p3 +
			3 * b * pow(c, 2) * pow(MPS2KPH_MF, 2) * p2 +
			2 * b * c * MPS2KPH_MF * p1 +
				b * p0;
}

double EmissionUtils::a3(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	return  4 * a * pow(c, 3) * pow(MPS2KPH_MF, 3) * p3 +
			3 * a * pow(c, 2) * pow(MPS2KPH_MF, 2) * p2 +
			2 * a * c * MPS2KPH_MF * p1 +
				a * p0 + 
			6 * pow(b,2) * pow(c, 2) * pow(MPS2KPH_MF, 3) * p3 +
			3 * pow(b,2) * c * pow(MPS2KPH_MF,2) * p2 +
				pow(b, 2) * MPS2KPH_MF * p1;
}

double EmissionUtils::a4(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	return  12 * a * b * pow(c, 2) * pow(MPS2KPH_MF, 3) * p3 +
		6 * a * b * c * pow(MPS2KPH_MF, 2) * p2 +
		2 * a * b * MPS2KPH_MF * p1 +
		4 * pow(b, 3) * c * pow(MPS2KPH_MF, 3) * p3 +
		pow(b, 3) * pow(MPS2KPH_MF, 2) * p2;
}

double EmissionUtils::a5(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	return	6 * pow(a,2) * pow(c,2) * pow(MPS2KPH_MF,3) * p3 +
			3 * pow(a,2) * c * pow(MPS2KPH_MF,2) * p2 +
			pow(a,2) * MPS2KPH_MF * p1 +
			12 * a * pow(b,2) * c * pow(MPS2KPH_MF,3) * p3 +
			3 * a * pow(b,2) * pow(MPS2KPH_MF,2) * p2 +
			pow(b,4) * pow(MPS2KPH_MF,3) * p3;
}

double EmissionUtils::a6(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	return  12 * pow(a, 2) * b * c * pow(MPS2KPH_MF, 3) * p3 +
		3 * pow(a, 2) * b * pow(MPS2KPH_MF, 2) * p2 +
		4 * a * pow(b, 3) * pow(MPS2KPH_MF, 3) * p3;
}

double EmissionUtils::a7(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	return  4 * pow(a, 3) * c * pow(MPS2KPH_MF, 3) * p3 +
		pow(a, 3) * pow(MPS2KPH_MF, 2) * p2 +
		6 * pow(a, 2) * pow(b, 2) * pow(MPS2KPH_MF, 3) * p3;
}

double EmissionUtils::a8(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	return  4 * pow(a, 3) * b * pow(MPS2KPH_MF, 3) * p3;
}

double EmissionUtils::a9(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	return  pow(a, 4) * pow(MPS2KPH_MF, 3) * p3;
}

double EmissionUtils::b0(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

/*	return  pow(c, 4) * pow(MPS2KPH_MF, 3) * p3 +
		pow(c, 3) * pow(MPS2KPH_MF, 2) * p2 +
		pow(c, 2) * MPS2KPH_MF * p1 +
		c * p0;*/

	return  pow(c, 3) * pow(MPS2KPH_MF, 3) * p3 +
		pow(c, 2) * pow(MPS2KPH_MF, 2) * p2 +
		pow(c, 1) * MPS2KPH_MF * p1 +
		p0;
}

double EmissionUtils::b1(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	/*return  8 * b * pow(c, 3) * pow(MPS2KPH_MF, 3) * p3 +
		6 * b * pow(c, 2) * pow(MPS2KPH_MF, 2) * p2 +
		4 * b * c * MPS2KPH_MF * p1 +
		2 * b * p0;*/

	return  6 * b * pow(c, 2) * pow(MPS2KPH_MF, 3) * p3 +
		4 * b * c * pow(MPS2KPH_MF, 2) * p2 +
		2 * b * MPS2KPH_MF * p1;
}

double EmissionUtils::b2(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	/*return  12 * a * pow(c, 3) * pow(MPS2KPH_MF, 3) * p3 +
		9 * a * pow(c, 2) * pow(MPS2KPH_MF, 2) * p2 +
		6 * a *c * MPS2KPH_MF * p1 +
		3 * a * p0 +
		18 * pow(b, 2) * pow(c, 2) * pow(MPS2KPH_MF, 3) * p3 +
		9 * pow(b, 2) * c *pow(MPS2KPH_MF, 2) * p2 +
		3 * pow(b, 2) * MPS2KPH_MF * p1;*/

	return  9 * a * pow(c, 2) * pow(MPS2KPH_MF, 3) * p3 +
		6 * a * c * pow(MPS2KPH_MF, 2) * p2 +
		3 * a * MPS2KPH_MF * p1 +
		9 * pow(b, 2) * c * pow(MPS2KPH_MF, 3) * p3 +
		3 * pow(b, 2) * pow(MPS2KPH_MF, 2) * p2 ;

}

double EmissionUtils::b3(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	/*return 48 * a * b * pow(c,2) * pow(MPS2KPH_MF,3) * p3 +
		24 * a * b * c * pow(MPS2KPH_MF,2) * p2 +
		8 * a * b * MPS2KPH_MF * p1 +
		16 * pow(b,3) * c *pow(MPS2KPH_MF,3) * p3 +
		4 * pow(b,3) * pow(MPS2KPH_MF,2) * p2;*/

	return 24 * a * b * c * pow(MPS2KPH_MF, 3) * p3 +
		8 * a * b * pow(MPS2KPH_MF, 2) * p2 +
		4 * pow(b, 3) * pow(MPS2KPH_MF, 3) * p3;
}

double EmissionUtils::b4(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	/*return 30 * pow(a,2) * pow(c,2) * pow(MPS2KPH_MF,3) * p3 +
		15 * pow(a,2) * c * pow(MPS2KPH_MF,2) * p2 +
		5 * pow(a,2) * MPS2KPH_MF * p1 +
		60 * a * pow(b,2) * c * pow(MPS2KPH_MF,3) * p3 +
		15 * a * pow(b,2) * pow(MPS2KPH_MF,2) * p2 +
		5 * pow(b,4) * pow(MPS2KPH_MF,3) * p3;*/

	return 15 * pow(a, 2) * c * pow(MPS2KPH_MF, 3) * p3 +
		5 * pow(a, 2) * pow(MPS2KPH_MF, 2) * p2 +
		15 * a * pow(b, 2) * pow(MPS2KPH_MF, 3) * p3;
}

double EmissionUtils::b5(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	/*return 72 * pow(a,2) * b * c * pow(MPS2KPH_MF,3) * p3 +
		18 * pow(a,2) * b * pow(MPS2KPH_MF,2) * p2 +
		24 * a * pow(b,3) * pow(MPS2KPH_MF,3) * p3;*/

	return 18 * pow(a, 2) * b * pow(MPS2KPH_MF, 3) * p3;
}

double EmissionUtils::b6(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	/*return 28 * pow(a, 3) * c * pow(MPS2KPH_MF, 3) * p3 +
		7 * pow(a, 3) * pow(MPS2KPH_MF, 2) * p2 +
		42 * pow(a, 2) * pow(b, 2) * pow(MPS2KPH_MF, 3) * p3;*/

	return 7 * pow(a, 3) * pow(MPS2KPH_MF, 3) * p3 ;
}

double EmissionUtils::b7(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	return 32 * pow(a, 3) * b * pow(MPS2KPH_MF, 3) * p3;
}

double EmissionUtils::b8(Pollutant p, double a, double b, double c)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	return 9 * pow(a,4) * pow(MPS2KPH_MF, 3) * p3;
}

double SymuCore::EmissionUtils::emission_v(Pollutant p, double v)
{
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	return p3 * pow(v, 3) + p2 * pow(v, 2) + p1 * v + p0;
}

double SymuCore::EmissionUtils::emission(Pollutant p, double n, double v, double T)
{
	double Ev = emission_v(p, v*MPS2KPH_MF);
	return T * n * v * Ev;
}

double SymuCore::EmissionUtils::emission(Pollutant p, double a, double b, double c, double n, double nmin, double nmax, double tmin, double tmax, double T, double TTD)
{
	double TmpSpeed, FE, Emission;

	if (n <= nmin || n >= nmax)
	{
		if (n <= nmin)
			//TmpSpeed = dbLength / tmin;
			TmpSpeed = tmin;
		else
			//TmpSpeed = dbLength / tmax;
			TmpSpeed = tmax;
	}
	else
	{
		//TmpSpeed = dbLength / ((a * pow(n, 2)) + (b * n) + c);
		TmpSpeed = ((a * pow(n, 2)) + (b * n) + c);
	}

	FE = emission_factor(p, TmpSpeed, TTD);
	//Emission = T * n * TmpSpeed * FE;

	Emission = TTD * FE;

	/*double etmp, vtmp;
	vtmp = a * pow(n, 2) + b * n + c;
	double Evtmp = emission_v(p, vtmp*MPS2KPH_MF);
	etmp = T * n * vtmp * Evtmp;

	double restmp;
	restmp = 
		 T * (
		a9(p, a, b, c) * pow(n, 9) +
		a8(p, a, b, c) * pow(n, 8) +
		a7(p, a, b, c) * pow(n, 7) +
		a6(p, a, b, c) * pow(n, 6) +
		a5(p, a, b, c) * pow(n, 5) +
		a4(p, a, b, c) * pow(n, 4) +
		a3(p, a, b, c) * pow(n, 3) +
		a2(p, a, b, c) * pow(n, 2) +
		a1(p, a, b, c) * pow(n, 1));*/

	return Emission;
}

double SymuCore::EmissionUtils::derivative_emission(Pollutant p, double a, double b, double c, double n, double T)
{
	/*return 0.001 * T * (
		b8(p, a, b, c) * pow(n, 8) +
		b7(p, a, b, c) * pow(n, 7) +
		b6(p, a, b, c) * pow(n, 6) +
		b5(p, a, b, c) * pow(n, 5) +
		b4(p, a, b, c) * pow(n, 4) +
		b3(p, a, b, c) * pow(n, 3) +
		b2(p, a, b, c) * pow(n, 2) +
		b1(p, a, b, c) * pow(n, 1) +
		b0(p, a, b, c) ) ;*/

	return T * (
		b6(p, a, b, c) * pow(n, 6) +
		b5(p, a, b, c) * pow(n, 5) +
		b4(p, a, b, c) * pow(n, 4) +
		b3(p, a, b, c) * pow(n, 3) +
		b2(p, a, b, c) * pow(n, 2) +
		b1(p, a, b, c) * n +
		b0(p, a, b, c));
}

double SymuCore::EmissionUtils::emission_factor(Pollutant p, double speed, double length)
{
	double eV;

	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty())
		return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	/*eV = p3 * pow(MPS2KPH_MF, 3) * pow(speed, 4) +
		p2 * pow(MPS2KPH_MF, 2) * pow(speed, 3) +
		p1 * pow(MPS2KPH_MF, 1) * pow(speed, 2) +
		p0 * speed;*/

	eV = p3 * pow(MPS2KPH_MF, 3) * pow(speed, 3) +
		p2 * pow(MPS2KPH_MF, 2) * pow(speed, 2) +
		p1 * pow(MPS2KPH_MF, 1) * pow(speed, 1) +
		p0;

		return eV;
}

double SymuCore::EmissionUtils::link_marginal_emission(Pollutant p, double a, double b, double c, double n, double nmin, double nmax, double T, double length, double nc, double mle)
{
	double eV, eV1, deVdV, dVdn, speed, speed1, LME;
    double n1;
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty()) return 0.0;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	/*if (nc!=-1 && n > nc && n <nmax )
		return mle;*/

	if (n <= nmin || n >= nmax)
	{
		if (n <= nmin)
			n = 0;
		else
			return std::numeric_limits<double>::infinity();
	}
    
	n1 = n + ( (1/50.0) * length);
    
    //speed = length / ((a * pow(n, 2)) + (b * n) + c);
    //speed1 = length / ((a * pow(n1, 2)) + (b * n1) + c);

	speed = a * pow(n, 2) + (b * n) + c;
	speed1 = a * pow(n1, 2) + (b * n1) + c;
    
    eV = emission_factor(p, speed, length);
    eV1 = emission_factor(p, speed1, length);
	
    LME = length * (n1*eV1 - n*eV) / (n1-n);

	//LME = length * ( (n1*eV1*speed1) - (n*eV*speed)) / ( (n1*speed1) - (n*speed));

	/*deVdV = 3 * p3 * pow(MPS2KPH_MF, 3) * pow(speed, 2) +
		2 * p2 * pow(MPS2KPH_MF, 2) * pow(speed, 1) +
		p1 * pow(MPS2KPH_MF, 1);

	dVdn = length * ( (-2 * a * n) - b) / pow( (a * pow(n, 2) + b * n + c), 2);

	LME = length * (eV + (n * deVdV * dVdn));*/
    
    

	return LME;
}

void SymuCore::EmissionUtils::link_marginal_emission_ex(Pollutant p, double a, double b, double c, double nmin, double nmax, double length, double &nc, double &lmec)
{
	double eV, eV1, deVdV, dVdn, speed, LME;
    double n, n1;
	std::vector<double> p_pol = getCOPERTvector(p);

	if (p_pol.empty()) return;

	const double p3 = p_pol[0];
	const double p2 = p_pol[1];
	const double p1 = p_pol[2];
	const double p0 = p_pol[3];

	double LMEPrev = 0;
	double step = 0.01;
    
    double speed1;

	for (n = nmin+step; n <= nmax; n=n+ step)
	{
        n1 = n+1;
        
		//speed = length / ((a * pow(n, 2)) + (b * n) + c);
        //speed1 = length / ((a * pow(n1, 2)) + (b * n1) + c);

		speed = a * pow(n, 2) + (b * n) + c;
		speed1 = a * pow(n1, 2) + (b * n1) + c;
        
        eV = emission_factor(p, speed, length);
        eV1 = emission_factor(p, speed1, length);

		/*eV = emission_factor(p, speed, length);

		deVdV = 3 * p3 * pow(MPS2KPH_MF, 3) * pow(speed, 2) +
			2 * p2 * pow(MPS2KPH_MF, 2) * pow(speed, 1) +
			p1 * pow(MPS2KPH_MF, 1);

		dVdn = length * ((-2 * a * n) - b) / pow((a * pow(n, 2) + b * n + c), 2);*/

		//LME = length * (eV + (n * deVdV * dVdn));
        
        LME = length * (n1*eV1 - n*eV);

		if (LMEPrev > LME)
		{ 
			nc = n - step;
			lmec = LMEPrev;
			return;
		}
		LMEPrev = LME;
	}

	nc = nmax;
	lmec = LME;

	return;
}
