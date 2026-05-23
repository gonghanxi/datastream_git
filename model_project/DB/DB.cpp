#include "DB.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(DB)
{
	SET_MODEL_DESCRIPTION("Decibel Functon");
	SET_MODEL_SYMBOL("SYM_DB");
	SET_MODEL_CATEGORY("Math Scalar");

	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("input");
	}
	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("output");
	}

	{
		auto p = ADD_MODEL_PARAM(Min);
		p.SetDefaultValue("-100");
		p.SetDescription("Minimum output value");
	}

	{
		auto p = ADD_MODEL_ENUM_PARAM(DbType, DbTypeEnum);
		p.AddEnumeration("Power", DB::POWER);
		p.AddEnumeration("Amplitude", DB::AMPLITUDE);
		p.SetDefaultValue("Amplitude");
		p.SetDescription("Types of dB values");
	}

	return true;
}
#endif 

DB::DB()
	: Min(-100.0)
	, DbType(AMPLITUDE) 
{
}

bool DB::Initialize()
{
	return true;
}
bool DB::UpdateDynamicParameters()
{
	return true;
}
bool DB::Finalize()
{
	return true;
}
bool DB::Run()
{
	const double x = input[0];
	double       y;

	if (x <= 0.0)
	{
		y = Min;
	}
	else
	{
		double v;
		if (DbType == POWER)
		{
			v = 10.0 * std::log10(x);
		}
		else
		{
			v = 20.0 * std::log10(x);
		}

		y = (v >= Min) ? v : Min;
	}

	output[0] = y;
	return true;
}
