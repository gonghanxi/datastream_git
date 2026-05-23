#include "RADAR_UnAmbVelocity.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_UnAmbVelocity )
{	
	SET_MODEL_DESCRIPTION("Measure the un-ambiguous velocity");

	SET_MODEL_CATEGORY("Measurement");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(Index);
		port.SetDescription("The index of maximum value in the range-doppler matrix");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(Velocity);
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
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(fc);
		param.SetDescription("The carrier frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e9");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SampleRate);
		param.SetDescription("Waveform Baseband Sampling Rate");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Direction, SelectedDirection);
		enumParam.AddEnumeration("Approaching Radar", ApproachingRadar);
		enumParam.AddEnumeration("Leaving Radar", LeavingRadar);
		enumParam.SetDefaultValue(0);
	}

	return true;
}
#endif

RADAR_UnAmbVelocity::RADAR_UnAmbVelocity()
{

}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_UnAmbVelocity::Run()
{
	if (Index.GetSize() != PRI.NumElements())
	{
		POST_ERROR("Num of PRIs must match num of inputs");
		return false;
	}

	const double c = 3e8;
	double PRF1 = 1.0 / PRI(0);
	double PRF2 = 1.0 / PRI(1);


	double fd1 = PRF1 * Index[0][0] / CPI_Num;
	double fd2 = PRF2 * Index[1][0] / CPI_Num;

	// 余数定理求解
	switch (Direction)
	{
	case RADAR_UnAmbVelocity::ApproachingRadar:
		while (std::abs(fd1 - fd2) > PRF1 / CPI_Num)
		{
			if (fd1 < fd2)
			{
				fd1 += PRF1;
			}
			else if (fd1 > fd2)
			{
				fd2 += PRF2;
			}

			if (fd1 > PRF1*CPI_Num/4 || fd2 > PRF2*CPI_Num/4)
			{
				POST_INFO("The maximum velocity is over the limitation.");
				return false;
			}
		}
		break;
	case RADAR_UnAmbVelocity::LeavingRadar:
		while (std::abs(fd1 - fd2) > PRF1 / CPI_Num)
		{
			// 远离雷达具有负的多普勒频移
			if (fd1 > fd2)
			{
				fd1 -= PRF1;
			}
			else if (fd1 < fd2)
			{
				fd2 -= PRF2;
			}

			if (fd1 < -PRF1*CPI_Num/4 || fd2 < -PRF2*CPI_Num/4)
			{
				POST_INFO("The maximum velocity is over the limitation.");
				return false;
			}
		}
		break;
	default:
		break;
	}

	double lambda = c / fc;
	Velocity[0] = fd1 * lambda / 2;

	std::string const& OutputVelocityInfo = "Velocity = " + std::to_string(Velocity[0]);
	POST_INFO(OutputVelocityInfo.c_str());

	return true;
}
