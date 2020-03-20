#pragma once

#ifndef SYMUCORE_EMISSION_UTILS_H
#define SYMUCORE_EMISSION_UTILS_H

#include "SymuCoreExports.h"
#include "SymuCoreConstants.h"
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

	class SYMUCORE_DLL_DEF EmissionUtils {

	public:
		static const std::vector<double> getCOPERTvector(Pollutant p);

		static double a1(Pollutant p, double a, double b, double c);
		static double a2(Pollutant p, double a, double b, double c);
		static double a3(Pollutant p, double a, double b, double c);
		static double a4(Pollutant p, double a, double b, double c);
		static double a5(Pollutant p, double a, double b, double c);
		static double a6(Pollutant p, double a, double b, double c);
		static double a7(Pollutant p, double a, double b, double c);
		static double a8(Pollutant p, double a, double b, double c);
		static double a9(Pollutant p, double a, double b, double c);

		static double b0(Pollutant p, double a, double b, double c);
		static double b1(Pollutant p, double a, double b, double c);
		static double b2(Pollutant p, double a, double b, double c);
		static double b3(Pollutant p, double a, double b, double c);
		static double b4(Pollutant p, double a, double b, double c);
		static double b5(Pollutant p, double a, double b, double c);
		static double b6(Pollutant p, double a, double b, double c);
		static double b7(Pollutant p, double a, double b, double c);
		static double b8(Pollutant p, double a, double b, double c);

		static double emission_v(Pollutant p, double v);
		static double emission(Pollutant p, double a, double b, double c, double n, double nmin, double nmax, double vmax, double vmin, double T, double length);
		static double emission(Pollutant p, double n, double v, double T);
		static double derivative_emission(Pollutant p, double a, double b, double c, double n, double T);

		static double emission_factor(Pollutant p, double v, double dbLength);
		static double link_marginal_emission(Pollutant p, double a, double b, double c, double n, double nmin, double nmax, double T, double length, double nc, double mlec);
		static void link_marginal_emission_ex(Pollutant p, double a, double b, double c, double nmin, double nmax, double length, double &nc, double &lmec);
		
	};
}

#pragma warning(pop)

#endif // SYMUCORE_EMISSION_UTILS_H
