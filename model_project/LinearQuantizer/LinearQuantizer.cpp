#include "LinearQuantizer.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(LinearQuantizer)
{
	SET_MODEL_DESCRIPTION("Uniform Linear Quantizer with Step Number Output");
	SET_MODEL_SYMBOL("SYM_LinearQuantizer");
	SET_MODEL_CATEGORY("Signal Processing");

	ADD_MODEL_INPUT(input);   
	ADD_MODEL_OUTPUT(step);   
	ADD_MODEL_OUTPUT(amp);    

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Levels);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("128");  
		p.SetDescription("Number of quantization levels"); 
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(Low);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("-3");   
		p.SetDescription("Lowest quantization level"); 
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAM(High);
		p.SetUnit(SystemVueModelBuilder::Units::NONE);
		p.SetDefaultValue("3");    
		p.SetDescription("Highest quantization level"); 
	}

	return true;
}
#endif

LinearQuantizer::LinearQuantizer()
	: Levels(128), Low(-3.0), High(3.0)
{}

bool LinearQuantizer::Setup()
{

	if (Levels < 2) {
		POST_ERROR("Levels must be >= 2.");
		return false;
	}
	if (!(High > Low)) {
		POST_ERROR("High must be greater than Low.");
		return false;
	}

	// 速率：1 进 2 出（逐样本一进一出）；让调度器按 1:1 采样推进
	input.SetRate(1U);
	step.SetRate(1U);
	amp.SetRate(1U);
	return true;
}

bool LinearQuantizer::Run()
{
	// 速率调度：每次 Run 处理 1 个样本
	const int    L = Levels;
	const double low = Low;
	const double high = High;

	const double delta = (high - low) / static_cast<double>(L - 1);
	if (!(delta > 0.0) || !std::isfinite(delta)) {
		POST_ERROR("Invalid delta computed. Check Low/High/Levels.");
		return false;
	}


	const double x = input[0U];

	const double u = (x - low) / delta;
	double k = std::floor(u + 0.5);                 // 最近整数
	if (k < 0.0) k = 0.0;
	const double kmax = static_cast<double>(L - 1);
	if (k > kmax) k = kmax;

	const double qamp = low + k * delta;            // 码字幅值

	step[0U] = k;                                   // 黄口：索引（real）
	amp[0U] = qamp;                                // 蓝口：幅值（real）

	return true;
}

