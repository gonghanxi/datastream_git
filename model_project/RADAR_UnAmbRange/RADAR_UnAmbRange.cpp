#include "RADAR_UnAmbRange.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_UnAmbRange )
{	
	SET_MODEL_DESCRIPTION("Measure the un-ambiguous range");

	SET_MODEL_CATEGORY("Measurement");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(Index);
		port.SetDescription("The index of maximum value in the range-doppler matrix");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(Range);
		port.SetDescription("The un-ambiguous range.");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRI);
		param.SetDescription("Pulse Repetition Interval");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("[1e-4]");
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
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}
	return true;
}
#endif

RADAR_UnAmbRange::RADAR_UnAmbRange()
{
	
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_UnAmbRange::Run()
{
	if (Index.GetSize() != PRI.NumElements())
	{
		POST_ERROR("Num of PRIs must match num of inputs");
		return false;
	}

	const double c = 3e8;

	double t1 = Index[0][0] / SampleRate;
	double t2 = Index[1][0] / SampleRate;

	// 余数定理求解
	while (std::abs(t1 - t2) > 1 / SampleRate)
	{
		if (t1 < t2)
		{
			t1 += PRI(0);
		}
		else if (t1 > t2)
		{
			t2 += PRI(1);
		}
		
		if (t1 > PRI(0)*CPI_Num/4 || t2 > PRI(1)*CPI_Num/4)
		{
			POST_INFO("The maximum range is over the limitation.");
			return false;
		}
	}

	Range[0] = c * t1 / 2;

	std::string const& OutputRangeInfo = "Range = " + std::to_string(Range[0]);
	POST_INFO(OutputRangeInfo.c_str());
	return true;
}
