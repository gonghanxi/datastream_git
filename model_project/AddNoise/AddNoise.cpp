#include "AddNoise.h"
#include <random>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( AddNoise )
{	
	SET_MODEL_DESCRIPTION("Add Noise to Input");
	SET_MODEL_SYMBOL("SYM_AddNDensity");
	SET_MODEL_CATEGORY("Communications");
	SET_MODEL_CATEGORY("Analog/RF");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("output signal");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Bandwidth);
		param.SetDescription("Noise bandwidth");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("5e6");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NoiseFigure);
		param.SetDescription("noise figure(Units: dB)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("2");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SystemNoiseTemperature);
		param.SetDescription("System Noise Temperature in ℃");
		param.SetUnit(SystemVueModelBuilder::Units::TEMPERATURE);
		param.SetDefaultValue("16.85");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(RefR);
		param.SetDescription("Reference resistance");
		param.SetUnit(SystemVueModelBuilder::Units::RESISTANCE);
		param.SetDefaultValue("50");
		param.SetSchematicDisplay(0);
	}
	return true;
}
#endif

AddNoise::AddNoise()
{
	
}

bool AddNoise::Setup()
{
	bool bStatus = true;

	if (Bandwidth < 0)
	{
		POST_ERROR("Bandwidth must be >= 0");
		bStatus = false;
	}
	if (SystemNoiseTemperature < -273.15)
	{
		POST_ERROR("SystemNoiseTemperature must be >= -273.15");
		bStatus = false;
	}
	if (RefR <= 0)
	{
		POST_ERROR("RefR must be > 0");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool AddNoise::Run()
{
	// 玻尔兹曼常数
	const double k = 1.3806504e-23;
	double NDensity = k * (SystemNoiseTemperature + 273.15) * std::pow(10.0, NoiseFigure / 10.0);

	double StdDev = std::sqrt(NDensity * Bandwidth * RefR);

	// 生成复高斯噪声
	std::random_device rd;	// 随机器
	std::mt19937 gen(rd()); // 梅森旋转生成种子
	std::normal_distribution<double>	dNRe(0, StdDev);
	std::normal_distribution<double>	dNIm(0, StdDev);

	std::complex<double> GaussianNoiseCx(dNRe(gen), dNIm(gen));

	output[0] = input[0].complex() + GaussianNoiseCx;

	return true;
}
