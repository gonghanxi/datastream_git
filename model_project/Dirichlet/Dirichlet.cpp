#include "Dirichlet.h"
#define _USE_MATH_DEFINES
#include <cmath>   

#define USE_CUSTOM_SYMBOL 0
static const char* kSymbolString = "SYM_DIRICHLET@Math Functions";

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(Dirichlet)
{
	SET_MODEL_DESCRIPTION("Dirichlet (Aliased Sinc) Function");
	SET_MODEL_SYMBOL("SYM_Dirichlet");
	SET_MODEL_CATEGORY("Math Scalar");

	ADD_MODEL_INPUT(input);
	ADD_MODEL_OUTPUT(output);

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(N);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("10");
		param.SetDescription("Length of Dirichlet kernel");
	}
	return true;
}
#endif

Dirichlet::Dirichlet()
	: N(10)
{}

bool Dirichlet::Setup()
{
	if (N < 1) {
		POST_ERROR("N must be >= 1.");
        LOG_ERROR("N must be >= 1.");
		return false;
	}
	return true;
}


static inline double dirichlet_sample(double omega_rad, int Nval)
{
	if (Nval < 1) Nval = 1;

	constexpr double TWO_PI = 6.28318530717958647692;
	const long double x_cycles = static_cast<long double>(omega_rad) / static_cast<long double>(TWO_PI);

	const long long k_nearest = llround(x_cycles);

	long double xw = x_cycles - floor(x_cycles + 0.5L);  // (-0.5, 0.5]

	const long double pi = 3.1415926535897932384626433832795L;
	const long double den = sinl(pi * xw);

	const long double eps = 1e-12L;
	if (fabsl(den) < eps) {
		const long long e = static_cast<long long>(Nval - 1) * k_nearest;
		const long double sign = (e & 1LL) ? -1.0L : 1.0L;
		return static_cast<double>(sign);
	}

	const long double num = sinl(static_cast<long double>(Nval) * pi * xw);
	const long double y = (num / den) / static_cast<long double>(Nval);
	return static_cast<double>(y);
}

bool Dirichlet::Run()
{
	const double omega = input[0];
	const int    Nv = (N < 1) ? 1 : N;

	output[0] = dirichlet_sample(omega, Nv);
	return true;
}
