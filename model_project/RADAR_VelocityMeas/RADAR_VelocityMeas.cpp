#include "RADAR_VelocityMeas.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_VelocityMeas )
{	
	SET_MODEL_DESCRIPTION("Measure the velocity");

	SET_MODEL_CATEGORY("Measurement");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input of range-doppler matrix");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(Velocity);
		port.SetDescription("The measured velocity which maybe is ambiguous.");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(Index);
		port.SetDescription("The index value of maximum value in the range-doppler matrix.");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRI);
		param.SetDescription("Pulse Repetition Interval");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(CPI_Num);
		param.SetDescription("The number of pulses in one CPI");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("32");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SampleRate);
		param.SetDescription("Waveform Baseband Sampling Rate");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("10e6");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(fc);
		param.SetDescription("Carrier Frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e9");
	}
	return true;
}
#endif

RADAR_VelocityMeas::RADAR_VelocityMeas()
{

}

bool RADAR_VelocityMeas::Setup()
{
	bool bStatus = true;
	PRINum = PRI * SampleRate;
	portRate = PRINum * CPI_Num;
	if (portRate > 0)
	{
		input.SetRate(portRate);
	}
	else
	{
		POST_ERROR("input port rate PRI * SampleRate * CPI_Num must be greater than 0.");
		bStatus = false;
	}
	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_VelocityMeas::Run()
{
	// 快时间维求距离量
	double maxValue = 0.0;
	int maxIndex = 0;
	for (int i = 0; i < portRate; i++)
	{
		if (input[i] > maxValue)
		{
			maxValue = input[i];
			maxIndex = i;
		}
	}

	//maxIndex = fmod(maxIndex, PRI*SampleRate);

	// 慢时间维求速度量

	int fmaxIndex = std::floor(maxIndex / PRINum);
	double PRF = 1.0 / PRI;
	double fd = PRF * fmaxIndex / CPI_Num;

	const double c = 3e8;
	double lambda = c / fc;
	Velocity[0] = fd * lambda / 2;

	Index[0] = fmaxIndex;
	return true;
}
