#include "AdaptLinQuant.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(AdaptLinQuant)
{
	using SystemVueModelBuilder::DFParam;

	SET_MODEL_DESCRIPTION("Adaptive Linear Quantizer");
	SET_MODEL_SYMBOL("SYM_AdaptLinQuant");
	SET_MODEL_CATEGORY("Signal Processing");

	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetDescription("input");
	}

	{
		auto p = ADD_MODEL_INPUT(inStep);
		p.SetDescription("input quantization step");
	}

	{
		auto p = ADD_MODEL_OUTPUT(amplitude);
		p.SetDescription("quantized output amplitude");
	}

	{
		auto p = ADD_MODEL_OUTPUT(outStep);
		p.SetDescription("output quantization step");
	}

	{
		auto p = ADD_MODEL_OUTPUT(stepLevel);
		p.SetDescription("quantization index");
	}

	{
		DFParam p = ADD_MODEL_PARAM(Bits);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("8");
		p.SetDescription("Number of bits");
		p.SetDynamicUpdate(true);
	}

	return true;
}
#endif 


AdaptLinQuant::AdaptLinQuant()
	: Bits(8)
{
}

bool AdaptLinQuant::Setup()
{
	if (Bits < 1 || Bits > 31)
	{
		POST_ERROR("AdaptLinQuant: Bits must be between 1 and 31.");
		return false;
	}

	input.SetRate(1U);
	inStep.SetRate(1U);
	amplitude.SetRate(1U);
	outStep.SetRate(1U);
	stepLevel.SetRate(1U);

	return true;
}

bool AdaptLinQuant::Run()
{
	const int bits = Bits;
	if (bits < 1 || bits > 31)
	{
		POST_ERROR("AdaptLinQuant: Bits must be between 1 and 31.");
		return false;
	}

	const double step = inStep[0U];

	if (!(step > 0.0) || !std::isfinite(step))
	{
		POST_ERROR("AdaptLinQuant: inStep must be finite and > 0.");
		return false;
	}

	const unsigned int L_u = (1u << bits);          
	const double       L = static_cast<double>(L_u);

	const double halfSpanMinusHalf = 0.5 * L - 0.5;

	const double x = input[0U];

	double kReal = x / step + halfSpanMinusHalf;

	double kRound = std::floor(kReal + 0.5);

	if (kRound < 0.0)
		kRound = 0.0;

	const double kMax = static_cast<double>(L_u - 1u);
	if (kRound > kMax)
		kRound = kMax;

	const int k = static_cast<int>(kRound);

	const double q = (static_cast<double>(k) - halfSpanMinusHalf) * step;

	amplitude[0U] = q;      
	outStep[0U] = step;  
	stepLevel[0U] = k;      

	return true;
}
