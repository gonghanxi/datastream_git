#include "RADAR_MatchedFilter.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_MatchedFilter )
{	
	SET_MODEL_DESCRIPTION("Matched Filter");

	SET_MODEL_CATEGORY("Signal Processing");
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(signal);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(reference);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PulseWidth);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-5");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRI);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SampleRate);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}
	return true;
}
#endif

RADAR_MatchedFilter::RADAR_MatchedFilter()
{

}

bool RADAR_MatchedFilter::Setup()
{
	bool bStatus = true;

	int	NumPRI = PRI * SampleRate;
	int NumPulse = PulseWidth * SampleRate;
	if (NumPRI > 0 && NumPulse > 0)
	{
		signal.SetRate(NumPRI);
		reference.SetRate(NumPulse);
		output.SetRate(NumPRI);
	}

	else
	{
		POST_ERROR("Port rate must be greater than 0. Check to make sure rate of reference: PulseWidth * SampleRate > 0 and rate of signal: PRI * SampleRate > 0.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_MatchedFilter::Run()
{
	int NumPRI = PRI * SampleRate;
	int NumPulse = PulseWidth * SampleRate;

	// 将上一个输出切片的输出缓存清 0
	for (int n = 0; n < NumPRI; n++)
	{
		output[n] = 0.0;
	}

	// 翻转共轭生成匹配滤波器
	SystemVueModelBuilder::Matrix< std::complex<double> > FilterSequence(1, NumPulse);
	for (int i = 0; i < NumPulse; i++)
	{
		FilterSequence(NumPulse - i - 1) = std::conj(reference[i]);
	}

	// 卷积进行匹配滤波
	// 此处采用的是圆周卷积
	for (int n = 0; n < NumPRI; n++)
	{
		for (int k = 0; k < NumPulse; k++)
		{
			int n_k = n - k;
			n_k = n - k < 0 ? n - k + NumPRI : n - k;
			output[n_k] += signal[n] * FilterSequence(k);
		}
	}
	return true;
}
