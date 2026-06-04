#include "RADAR_MultiCH_Tx.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_MultiCH_Tx)
{
	SET_MODEL_DESCRIPTION("RADAR ideal multichannel transmitter");
	//SET_MODEL_SYMBOL("SYM_RADAR_MultiCH_Tx");
	SET_MODEL_CATEGORY("Array TR");

	// --------- 端口 ---------
	{
		auto p = ADD_MODEL_INPUT(input);
		p.SetDescription("input I/Q signal");
	}
	{
		auto p = ADD_MODEL_OUTPUT(output);
		p.SetDescription("output signal");
	}

	// --------- 参数 ---------
	{
		auto p = ADD_MODEL_PARAM(NumOfCH);
		p.SetDefaultValue("16");
		p.SetDescription("Number of transmitter channel");
	}

	{
		auto p = ADD_MODEL_PARAM(ImbalanceCoef);
		p.SetDefaultValue("[1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0,1+j*0]");
		p.SetDescription("The imbalance coefficient of channels");
	}

	{
		auto p = ADD_MODEL_PARAM(TStep);
		p.SetDefaultValue("1/10e6");
		p.SetUnit(SystemVueModelBuilder::Units::TIME);
		p.SetDescription("The time step of output");
	}

	{
		auto p = ADD_MODEL_PARAM(FCarrier);
		p.SetDefaultValue("1.0e9");
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDescription("The carrier frequency of output");
	}

	return true;
}
#endif // SV_CODE_GEN

RADAR_MultiCH_Tx::RADAR_MultiCH_Tx()
	: NumOfCH(16)
	, TStep(1.0 / 10e6)
	, FCarrier(1.0e9)
{
}

RADAR_MultiCH_Tx::Cx RADAR_MultiCH_Tx::getImbalance_(int k) const
{
	if (k < 0) return Cx(1.0, 0.0);
	if (k < static_cast<int>(imbCache_.size()))
		return imbCache_[k];
	return Cx(1.0, 0.0);
}

void RADAR_MultiCH_Tx::applyOutputTiming_(double startTime)
{
	const size_t busSize = output.GetSize();
	for (size_t k = 0; k < busSize; ++k)
	{
		output[k].SetStartTime(startTime);
		output[k].SetTimeStep(TStep);
	}
}

void RADAR_MultiCH_Tx::applyOutputFc_()
{
	const size_t busSize = output.GetSize();
	for (size_t k = 0; k < busSize; ++k)
		output[k].SetCharacterizationFrequency(FCarrier);
}

ERESULT RADAR_MultiCH_Tx::CalculateLatency()
{
	// 继承输入第0路的 StartTime
	double st = 0.0;
	const size_t inSize = input.GetSize();
	if (inSize > 0)
		st = input[0].GetStartTime();

	applyOutputTiming_(st);

	return (ERESULT)0;
}

ERESULT RADAR_MultiCH_Tx::PropagateCharacterizationFrequency()
{
	// 调用此函数传播 Fc
	applyOutputFc_();
	return (ERESULT)0;
}

bool RADAR_MultiCH_Tx::Setup()
{
	inBusSize_ = input.GetSize();
	outBusSize_ = output.GetSize();

	nChExpected_ = std::max(0, NumOfCH);

	// 缓存 ImbalanceCoef
	imbCache_.assign(static_cast<size_t>(nChExpected_), Cx(1.0, 0.0));
	const size_t nCoef = ImbalanceCoef.NumElements();
	const size_t nFill = std::min(nCoef, imbCache_.size());
	for (size_t k = 0; k < nFill; ++k)
		imbCache_[k] = ImbalanceCoef(k);

	double st = 0.0;
	if (inBusSize_ > 0)
		st = input[0].GetStartTime();

	applyOutputTiming_(st);
	applyOutputFc_();

	return true;
}

bool RADAR_MultiCH_Tx::Run()
{
	// 驱动时间轴
	if (inBusSize_ > 0)
		(void)input[0].GetTime(0, GetCount());

	// Run 再保险写入元数据
	double st = 0.0;
	if (inBusSize_ > 0)
		st = input[0].GetStartTime();
	applyOutputTiming_(st);
	applyOutputFc_();

	const size_t nWrite = std::min(outBusSize_, static_cast<size_t>(nChExpected_));

	for (size_t k = 0; k < nWrite; ++k)
	{
		Cx x(0.0, 0.0);
		if (k < inBusSize_)
			x = input[k][0];

		const Cx c = getImbalance_(static_cast<int>(k));
		const Cx y = x * c;

		//  强制 complex envelope 写入
		EnvSig env;
		env = static_cast<std::complex<double>>(y);
		output[k][0] = env;
	}

	for (size_t k = nWrite; k < outBusSize_; ++k)
	{
		EnvSig z;
		z = std::complex<double>(0.0, 0.0);
		output[k][0] = z;
	}

	return true;
}