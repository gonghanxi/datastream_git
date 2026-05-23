#include "HPF_Butterworth.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( HPF_Butterworth )
{	
	SET_MODEL_DESCRIPTION("Highpass Butterworth Filter");
	SET_MODEL_SYMBOL("SYM_HPF");
	SET_MODEL_CATEGORY("IIR");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
		port.SetDescription("input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("output signal");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Loss);
		param.SetDescription("Magnitude loss in dB");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	//{
	//	SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FCenter);
	//	param.SetDescription("Center frequency");
	//	param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
	//	param.SetDefaultValue("150e3");
	//}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(PassFreq);
		param.SetDescription("Passband edge frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("150e3");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(PassAtten);
		param.SetDescription("Passband edge attenuation in dB");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("3");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(StopFreq);
		param.SetDescription("Stopband edge frequency (required only when OrderType = Auto)");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("100e3");
		param.SetHideCondition("OrderType ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(StopAtten);
		param.SetDescription("Stopband edge attenuation in dB (required only when OrderType = Auto)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("50");
		param.SetHideCondition("OrderType ~= 0");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(OrderType, SelectedOrderType);
		enumParam.SetDescription("Order specification: Auto, User Defined");
		enumParam.AddEnumeration("Auto", Auto);
		enumParam.AddEnumeration("User Defined", UserDefined);
		enumParam.SetDefaultValue("1");
		enumParam.SetSchematicDisplay(0);
		enumParam.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Order);
		param.SetDescription("User defined order for the lowpass prototype analog filter");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("5");
		param.SetHideCondition("OrderType ~= 1");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	//{
	//	SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Transform, SelectedTransform);
	//	enumParam.SetDescription("S to Z domain transformation method: Bilinear, Impulse Invariance");
	//	enumParam.AddEnumeration("Bilinear", Bilinear);
	//	enumParam.AddEnumeration("Impulse Invariance", ImpulseInvariance);
	//	enumParam.SetDefaultValue("0");
	//	enumParam.SetSchematicDisplay(0);
	//	enumParam.SetUseDefault(1);
	//}

	//{
	//	SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(UnderSampledModel, SelectedUnderSampledModel);
	//	enumParam.SetDescription("Default behavior when sampling rate is too small to represent the filter. This parameter is only used for simulation not for filter design tool: Model As Allpass, Error Out");
	//	enumParam.AddEnumeration("Model As Allpass", ModelAsAllpass);
	//	enumParam.AddEnumeration("Error Out", ErrorOut);
	//	enumParam.SetDefaultValue("0");
	//	enumParam.SetSchematicDisplay(0);
	//	enumParam.SetUseDefault(1);
	//}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetDescription("Sample rate");
		param.SetDefaultValue("Sample_Rate");
	}
	return true;
}
#endif

HPF_Butterworth::HPF_Butterworth()
{

}

ERESULT HPF_Butterworth::PropagateCharacterizationFrequency()
{
	bool bStatus = true;
	fc = input.GetCharacterizationFrequency();
	output.SetCharacterizationFrequency(fc);
	return bStatus;
}

bool HPF_Butterworth::Setup()
{
	bool bStatus = true;
	// 设置滤波器阶数
	if (OrderType == UserDefined)
	{
		FilterOrder = Order;
	}
	else
	{
		FilterOrder = 4; ///TODO 自动滤波器阶数
	}

	if (SampleRate > 2 * PassFreq)
	{
		// 滤波器初始化
		shelfFilterReal.setup(FilterOrder, SampleRate, PassFreq);
		shelfFilterImag.setup(FilterOrder, SampleRate, PassFreq);
	}
	else if (SampleRate <= 2 * PassFreq)
	{
		// 欠采样时报错
        char str[256];
        sprintf(str, "SampleRate (%f) should be larger than 2 * PassFreq (%f).", SampleRate, 2 * PassFreq);
		POST_ERROR(str);
        LOG_ERROR("SampleRate (",SampleRate,") should be larger than 2 * PassFreq (",2 * PassFreq,").");
		bStatus = false;
	}
	return bStatus;
}

const double PI = acos(-1.0);

// 计算 exp(j * 2 * PI * f_c * t)
std::complex<double> HPF_Butterworth::complexExponential(double f_c, double t) {
	// 计算角频率：ω = 2 * π * f_c
	double omega = 2 * PI * f_c;
	// 计算相位：θ = ω * t
	double phase = omega * t;
	// 返回复数指数：exp(j * θ)
	return std::exp(std::complex<double>(0, phase));
}

// 分贝转倍数
double HPF_Butterworth::dBToPowerRatio(double dB) {
	return std::pow(10.0, dB / 10.0);
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool HPF_Butterworth::Run()
{
	// 获取时间戳
	double t = input.GetTime(0, m_iFiringCount);

	// 复指数信号，用于表示搬移至中心频率后产生的频率偏移量
	std::complex<double> in = input[0].complex();
	std::complex<double> exp = complexExponential(fc, t);
	std::complex<double> inExp = in * exp;
	// 滤波
	double real = shelfFilterReal.filter(inExp.real());
	double imag = shelfFilterImag.filter(inExp.imag());
	std::complex<double> zz = std::complex<double>(real, imag);
	output[0] = zz * complexExponential(-fc, t);

	// 滤波损耗
	output[0] /= dBToPowerRatio(Loss / 2);
	return true;
}
