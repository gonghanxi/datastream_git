#include "AddNDensity.h"
#include <random>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( AddNDensity )
{	
	SET_MODEL_DESCRIPTION("Add Noise Density to Input");
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

	//{
	//	SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(NDensityType, SelectedNDensityType);
	//	enumParam.SetDescription("Noise density type: Constant noise density, Noise density vs freq");
	//	enumParam.AddEnumeration("Constant_noise_density", Constant_noise_density);
	//	enumParam.AddEnumeration("Noise_density_vs_freq", Noise_density_vs_freq);
	//	enumParam.SetDefaultValue("0");
	//}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NDensity);
		param.SetDescription("Noise power spectral density in (Power Unit Selected)/Hz");
		param.SetUnit(SystemVueModelBuilder::Units::POWER);
		param.SetDefaultValue("4.00388587e-21");
		//param.SetHideCondition("NDensityType ~= 0");
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

AddNDensity::AddNDensity()
{

}

bool AddNDensity::Setup()
{
	bool bStatus = true;

	if (NDensity < 0)
	{
		POST_ERROR("NDensity must be >= 0");
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
bool AddNDensity::Run()
{
	double SampleRate = input.GetSampleRate();

	// 射频信号
	if (input.GetCharacterizationFrequency())
	{
		double BW = SampleRate / 2;
		double StdDev = std::sqrt(NDensity * BW * RefR);

		// 生成复高斯噪声
		std::random_device rd;	// 随机器
		std::mt19937 gen(rd()); // 梅森旋转生成种子
		std::normal_distribution<double>	dNRe(0, StdDev);
		std::normal_distribution<double>	dNIm(0, StdDev);

		std::complex<double> GaussianNoiseCx(dNRe(gen), dNIm(gen));

		output[0] = input[0].complex() + GaussianNoiseCx;
	}

	// 基带信号
	else
	{
		double BW = SampleRate;
		double StdDev = std::sqrt(NDensity * BW * RefR);

		// 生成高斯噪声
		std::random_device rd;	// 随机器
		std::mt19937 gen(rd()); // 梅森旋转生成种子
		std::normal_distribution<double>	dN(0, StdDev);

		output[0] = input[0].real() + dN(gen);
	}

	return true;
}
