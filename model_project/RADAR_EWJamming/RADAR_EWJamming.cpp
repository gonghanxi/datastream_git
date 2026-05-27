#include "RADAR_EWJamming.h"
#include <random>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_EWJamming )
{	
	SET_MODEL_DESCRIPTION("EW Jamming");

	SET_MODEL_CATEGORY("EW");

	ADD_MODEL_OUTPUT(jamming);

	//{
	//	SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(JammingType, SelectedJammingType);
	//	enumParam.AddEnumeration("BarrageJamming", BarrageJamming);
	//	enumParam.AddEnumeration("SpotJamming", SpotJamming);
	//	enumParam.AddEnumeration("MultiSpotJamming", MultiSpotJamming);
	//	enumParam.AddEnumeration("SweptSpotJamming", SweptSpotJamming);
	//	enumParam.SetDefaultValue("0");
	//}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleNum);
		param.SetDescription("The number of samples which this model generated each time when it is fired");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1000");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetDescription("The SampleRate of the Jammer.");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Mean);
		param.SetDescription("The mean value of gaussian variables");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Stdev);
		param.SetDescription("The stdev value of gaussian variables");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}
	//{
	//	SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Bandwidth);
	//	param.SetDescription("The normalized bandwidth of jamming");
	//	param.SetUnit(SystemVueModelBuilder::Units::NONE);
	//	param.SetDefaultValue("0.1");
	//	param.SetHideCondition("JammingType == 2");
	//}
	//{
	//	SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SweepFreqStep);
	//	param.SetDescription("The normalized step sweep frequency");
	//	param.SetUnit(SystemVueModelBuilder::Units::NONE);
	//	param.SetDefaultValue("0.001");
	//	param.SetHideCondition("JammingType ~= 3");
	//}
	//{
	//	SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(MultiSpotBand);
	//	param.SetDescription("The normalized start frequency and stop frequency of MultiSpotBand jamming");
	//	param.SetUnit(SystemVueModelBuilder::Units::NONE);
	//	param.SetDefaultValue("[0 0.05 0.1 0.15]");
	//	param.SetHideCondition("JammingType ~= 2");
	//}
	//{
	//	SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FilterTapsLength);
	//	param.SetDescription("The length of filter taps which is used to filter the gaussian white noise.");
	//	param.SetUnit(SystemVueModelBuilder::Units::NONE);
	//	param.SetDefaultValue("256");
	//}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(System_Loss);
		param.SetDescription("System and propogation loss in dB");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
	}
	//{
	//	SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Atmospheric_Loss_Factor);
	//	param.SetDescription("One way Atmospheric loss with dB/km");
	//	param.SetUnit(SystemVueModelBuilder::Units::NONE);
	//	param.SetDefaultValue("0");
	//}
	return true;
}
#endif

RADAR_EWJamming::RADAR_EWJamming()
{
	
}

bool RADAR_EWJamming::Setup()
{
	bool bStatus = true;

	if (SampleNum <= 0)
	{
		POST_ERROR("SampleNum must be > 0");
		bStatus = false;
	}
	if (SampleRate <= 0)
	{
		POST_ERROR("SampleRate must be > 0");
		bStatus = false;
	}
	if (Stdev < 0)
	{
		POST_ERROR("Stdev must be >= 0");
		bStatus = false;
	}
	//if (Bandwidth <= 0)
	//{
	//	POST_ERROR("Bandwidth must be > 0");
	//	bStatus = false;
	//}
	//if (SweepFreqStep <= 0)
	//{
	//	POST_ERROR("SweepFreqStep must be > 0");
	//	bStatus = false;
	//}
	//if (FilterTapsLength <= 0)
	//{
	//	POST_ERROR("FilterTapsLength must be > 0");
	//	bStatus = false;
	//}

	jamming.SetRate(SampleNum);
	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_EWJamming::Run()
{

	// 生成高斯噪声
	std::random_device rd;	// 随机器
	std::mt19937 gen(rd()); // 梅森旋转生成种子
	std::normal_distribution<double>	dNRe(Mean, Stdev);
	std::normal_distribution<double>	dNIm(Mean, Stdev);

	for (int i = 0; i < SampleNum; i++)
	{
		std::complex<double> GaussianNoiseCx(dNRe(gen), dNIm(gen));
		jamming[i] = GaussianNoiseCx * std::pow(10, -System_Loss/20);
	}

	return true;
}
