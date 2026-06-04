#include "RADAR_Sea_Clutter.h"
#include <random>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_Sea_Clutter )
{	
	SET_MODEL_DESCRIPTION("Radar clutter simulation");

	SET_MODEL_CATEGORY("Environments");
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("The input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(BodyRoll);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(BodyPitch);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(BodyYaw);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(AntTilt);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(AntYaw);
		port.SetOptional();
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(ClutterSample);
		port.SetDescription("The clutter samples to evaluate the clutter performance");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("The clutter");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(SeaState, SelectedSeaState);
		enumParam.SetDescription("Douglas Sea State Levels: Sea State 1, Sea State 2, Sea State 3, Sea State 4, Sea State 5, Sea State 6, Sea State 7");
		enumParam.AddEnumeration("SeaState_1", SeaState_1);
		enumParam.AddEnumeration("SeaState_2", SeaState_2);
		enumParam.AddEnumeration("SeaState_3", SeaState_3);
		enumParam.AddEnumeration("SeaState_4", SeaState_4);
		enumParam.AddEnumeration("SeaState_5", SeaState_5);
		enumParam.AddEnumeration("SeaState_6", SeaState_6);
		enumParam.AddEnumeration("SeaState_7", SeaState_7);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(RF_Freq);
		param.SetDescription("RF carrier frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("1e9");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Antenna_Pattern, SelectedAntenna_Pattern);
		enumParam.SetDescription("Antenna Pattern: Gaussian");
		enumParam.AddEnumeration("Gaussian", Gaussian);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(BodyRollAngle);
		param.SetDescription("The roll angle from NED frame to body frame. The unit is degree");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(BodyPitchAngle);
		param.SetDescription("The pitch angle from NED frame to body frame. The unit is degree");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(BodyYawAngle);
		param.SetDescription("The yaw angle from NED frame to body frame. The unit is degree");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(AntTiltAngle);
		param.SetDescription("The tilt angle from body frame to antenna frame. The unit is degree");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(AntYawAngle);
		param.SetDescription("The yaw angle from body frame to antenna frame. The unit is degree");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRF);
		param.SetDescription("Pulse Repetion Frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("1e4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SampleRate);
		param.SetDescription("Sample Rate");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Antenna_Height);
		param.SetDescription("Antenna height above horizon");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("3000");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Platform_Velocity);
		param.SetDescription("Platform velocity");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("400");
	}
	return true;
}
#endif

RADAR_Sea_Clutter::RADAR_Sea_Clutter()
{
	
}

ERESULT RADAR_Sea_Clutter::PropagateCharacterizationFrequency()
{
	bool bStatus = true;

	if (RF_Freq > 0)
	{
		output.SetCharacterizationFrequency(RF_Freq);
		ClutterSample.SetCharacterizationFrequency(RF_Freq);
	}
	else
	{
		POST_ERROR("The characterizatuon frequency RF_Freq must be > 0.");
		bStatus = false;
	}

	return bStatus;
}

bool RADAR_Sea_Clutter::Setup()
{
	bool bStatus = true;

	if (PRF <= 0)
	{
		POST_ERROR("PRF must be > 0.");
		bStatus = false;
	}

	if (SampleRate <= 0)
	{
		POST_ERROR("SampleRate must be > 0");
		bStatus = false;
	}

	if (Antenna_Height <= 0)
	{
		POST_ERROR("Antenna_Height must be > 0");
		bStatus = false;
	}

	if (Platform_Velocity < 0)
	{
		POST_WARNING("Platform_Velocity < 0, assuming that the platform is moving away from target, velocity will be absolute value.");
		Platform_Velocity = std::abs(Platform_Velocity);
	}

	num_sample = SampleRate / PRF;

	input.SetRate(num_sample);
	output.SetRate(num_sample);
	ClutterSample.SetRate(num_sample);

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_Sea_Clutter::Run()
{
	// 角度信息
	if (BodyRoll.IsConnected())
	{
		BodyRollAngle = BodyRoll[0];
	}
	if (BodyPitch.IsConnected())
	{
		BodyPitchAngle = BodyPitch[0];
	}
	if (BodyYaw.IsConnected())
	{
		BodyYawAngle = BodyYaw[0];
	}
	if (AntTilt.IsConnected())
	{
		AntTiltAngle = AntTilt[0];
	}
	if (AntYaw.IsConnected())
	{
		AntYawAngle = AntYaw[0];
	}

	// 海况参数
	switch (SeaState)
	{
	case RADAR_Sea_Clutter::SeaState_1:
		SS = 1;
		break;
	case RADAR_Sea_Clutter::SeaState_2:
		SS = 2;
		break;
	case RADAR_Sea_Clutter::SeaState_3:
		SS = 3;
		break;
	case RADAR_Sea_Clutter::SeaState_4:
		SS = 4;
		break;
	case RADAR_Sea_Clutter::SeaState_5:
		SS = 5;
		break;
	case RADAR_Sea_Clutter::SeaState_6:
		SS = 6;
		break;
	case RADAR_Sea_Clutter::SeaState_7:
		SS = 7;
		break;
	default:
		break;
	}

	// 风速
	WindVelocity = 1 + 2 * SS + std::pow(SS / 5.3, 6);
	// 分布方差
	Sigma = std::pow(WindVelocity / 15.0, 2);

	const double PI = std::acos(-1);
	const std::complex<double> imag_I(0, 1);

	// 瑞利分布
	std::random_device rd; // random seed
	std::mt19937 gen(rd()); //mersenne twister
	std::uniform_real_distribution<>	d1(0, 1);
	std::uniform_real_distribution<>	d2(0, 1);

	SystemVueModelBuilder::Matrix<double>	xi(1, num_sample);
	SystemVueModelBuilder::Matrix<double>	xq(1, num_sample);
	xi.Zero();
	xq.Zero();

	for (int i = 0; i < num_sample; i++)
	{
		xi(i) = Sigma * std::sqrt(-2.0 * std::log(d1(gen))) * std::cos(2.0 * PI * d2(gen));
		xq(i) = Sigma * std::sqrt(-2.0 * std::log(d1(gen))) * std::sin(2.0 * PI * d2(gen));

		ClutterSample[i] = xi(i) + imag_I * xq(i);
		output[i] = input[i].complex() * ClutterSample[i].complex();
	}

	return true;
}
