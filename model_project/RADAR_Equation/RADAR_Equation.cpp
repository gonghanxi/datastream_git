#include "RADAR_Equation.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_Equation )
{	
	SET_MODEL_DESCRIPTION("Radar Range Equation");

	SET_MODEL_CATEGORY("Measurement");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("The output of radar equation:SNR or maximum range");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(EqType, SelectedEqType);
		enumParam.SetDescription("Radar Range Equation Type: Basic Equation for point target in internal noise, CW, Pulsed Doppler, Search, Track");
		enumParam.AddEnumeration("Basic Equation for point target in internal noise", Basic);	// 0
		enumParam.AddEnumeration("CW", CW);														// 1
		enumParam.AddEnumeration("Pulsed Doppler", PD);											// 2
		enumParam.AddEnumeration("Search", Search);												// 3
		enumParam.AddEnumeration("Track", Track);												// 4
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(OutputType, SelectedOutputType);
		enumParam.SetDescription("The output type of Radar Range Equation: SNR, Maximum Range");
		enumParam.AddEnumeration("SNR", SNROut);
		enumParam.AddEnumeration("Maximum Range", RangeOut);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Pt);
		param.SetDescription("Radar Transmit Peak Power");
		param.SetUnit(SystemVueModelBuilder::Units::POWER);
		param.SetDefaultValue("1e6");
		param.SetHideCondition("EqType ~= 0 && EqType ~= 3 && EqType ~= 4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Pavg);
		param.SetDescription("Radar Transmit Average Power");
		param.SetUnit(SystemVueModelBuilder::Units::POWER);
		param.SetDefaultValue("1e6");
		param.SetHideCondition("EqType ~= 1 && EqType ~= 2");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(DwellTime);
		param.SetDescription("The dwell or look time");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("100e-6");
		param.SetHideCondition("EqType ~= 1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRF);
		param.SetDescription("Pulse Repetition Frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("100e3");
		param.SetHideCondition("EqType ~= 2 && EqType ~= 3 && EqType ~= 4");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(AntennaType, SelectedAntennaType);
		enumParam.SetDescription("The type of antenna: Single, Monostatic Separate");
		enumParam.AddEnumeration("Single", Single);
		enumParam.AddEnumeration("Monostatic Separate", MonostaticSeparate);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Gain);
		param.SetDescription("Antenna Gain(Units: dB)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("30");
		param.SetHideCondition("AntennaType ~= 0"); // 单基地收发一体
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(GainTx);
		param.SetDescription("Antenna Gain(Units: dB)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("30");
		param.SetHideCondition("AntennaType ~= 1"); // 单基地收发分离
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(GainRx);
		param.SetDescription("Antenna Gain(Units: dB)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("30");
		param.SetHideCondition("AntennaType ~= 1"); // 单基地收发分离
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(RCS);
		param.SetDescription("Target RCS(Units: square meter)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(NoiseFigure);
		param.SetDescription("noise figure(Units: dB)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("2");
	}

	// 若使用SystemVue的TEMPERATURE作为参数单位，则默认单位为℃
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SystemNoiseTemperature);
                param.SetDescription("System Noise Temperature in Celsius");
		param.SetUnit(SystemVueModelBuilder::Units::TEMPERATURE);
		param.SetDefaultValue("16.85");
	}

	// 若不使用TEMPERATURE作为参数单位（无量纲参数），则视作温度的单位为K
	//{
	//	SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SystemNoiseTemperature);
	//	param.SetDescription("System Noise Temperature in ℃");
	//	param.SetUnit(SystemVueModelBuilder::Units::NONE);
	//	param.SetDefaultValue("290");
	//}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Freq);
		param.SetDescription("Carrier Frequency");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e9");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Pulsewidth);
		param.SetDescription("Signal Pulsewidth");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-6");
		param.SetHideCondition("EqType ~= 0 && EqType ~= 3 && EqType ~= 4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Bandwidth);
		param.SetDescription("Signal Bandwidth");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("5e6");
		param.SetHideCondition("EqType ~= 0 && EqType ~= 3 && EqType ~= 4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SystemLoss);
		param.SetDescription("System Loss(Units: dB)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PropagationLoss);
		param.SetDescription("Propagation Loss(Units: dB)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(GroundPlaneLoss);
		param.SetDescription("Propagation Loss(Units: dB)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Range);
		param.SetDescription("Range, Unit is meters");
		param.SetUnit(SystemVueModelBuilder::Units::LENGTH);
		param.SetDefaultValue("100e3");
		param.SetHideCondition("OutputType ~= 0"); // 已知距离指标（求信噪比指标）
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SNR);
		param.SetDescription("SNR in dB");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("10");
		param.SetHideCondition("OutputType ~= 1"); // 已知信噪比指标（求距离指标）
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(IntegrationType, SelectedIntegrationType);
		enumParam.SetDescription("With/Without Signal Integration: Single hit, Integration");
		enumParam.AddEnumeration("Single hit", Singlehit);
		enumParam.AddEnumeration("Integration", Integration);
		enumParam.SetDefaultValue("0");
		enumParam.SetHideCondition("EqType ~= 0"); // 仅在基础雷达显示这个设置
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PulseNumber);
		param.SetDescription("The number of pulses illuminate the target");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1");
		param.SetHideCondition("(EqType ~= 0 || IntegrationType ~= 1) && EqType ~= 2 ");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(IntegrationLoss);
		param.SetDescription("Integration loss (Units: dB)");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0");
		param.SetHideCondition("EqType == 0 && IntegrationType == 0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Theta3dB);
		param.SetDescription("The 3dB beamwidth of the antenna, Unit is degree");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("3");
		param.SetHideCondition("EqType ~= 3"); // 只在搜索雷达用到这个参数
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(ScanRate);
		param.SetDescription("The antenna scan rate per second, Unit is degree/s");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("40");
		param.SetHideCondition("EqType ~= 3"); // 只在搜索雷达用到这个参数
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(ServoBandwidth);
		param.SetDescription("The tracking servo bandwidth, which is equal to the multiplicative inverse of the data gathering or integration time");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("5");
		param.SetHideCondition("EqType ~= 4"); // 只在跟踪雷达用到这个参数
	}
	return true;
}
#endif

RADAR_Equation::RADAR_Equation()
{
	
}

// 倍数传分贝
double RADAR_Equation::PowerRatioTodB(double PowerRatio)
{
	return log10(PowerRatio) * 10.0;
}
// 分贝转倍数
double RADAR_Equation::dBToPowerRatio(double dB)
{
	return pow(10.0, dB / 10.0);
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_Equation::Run()
{
	const double PI = acos(-1.0);
	//const double c = 299792458;
	const double c = 3e8;				// 光速
	//---------------------------------------------------------------------
	// 计算雷达方程中各参数的值，单位为dB的参数转为倍数，较短的参数名保留，较长的参数名用缩写替代

	double lambda = c / Freq;			// 波长λ
	// double Pt;						// 雷达峰值功率
	// double Pavg;						// 雷达平均功率
	double Td = DwellTime;				// 驻留时间
	// double PRF;						// 脉冲重复频率
	double SNRm = dBToPowerRatio(SNR);			// 信噪比倍数形式
	double Ga;							// 总天线增益
	if (AntennaType == Single)
	{
		Ga = dBToPowerRatio(Gain) * dBToPowerRatio(Gain);
	}
	if (AntennaType == MonostaticSeparate)
	{
		Ga = dBToPowerRatio(GainTx) * dBToPowerRatio(GainRx);
	}

	// double RCS;						// 雷达散射截面积
	// double PulseNumber;				// 脉冲数量
	double tau = Pulsewidth;			// 脉宽
	double B = Bandwidth;				// 带宽
	double Li = dBToPowerRatio(IntegrationLoss); // 处理损耗
	double Gp = tau * B;				// 处理增益（一般由相参积累或脉冲压缩产生）,SystemVue默认需要经过脉冲压缩
	if (IntegrationType == Integration && EqType == 0)
	{
		Gp *= PulseNumber / Li;			// 额外进行相参积累
	}

	double T0 = SystemNoiseTemperature + 273.15;	// 系统噪声温度，摄氏度转为开尔文温度
	//double T0 = SystemNoiseTemperature; // 如果参数不设单位，则参数值不需要进行转换

	double F = dBToPowerRatio(NoiseFigure);		// 系统噪声系数
	double Ls = dBToPowerRatio(SystemLoss);		// 系统损耗
	double La = dBToPowerRatio(PropagationLoss); // 传播损耗
	double Lg = dBToPowerRatio(GroundPlaneLoss); // 地面损耗
	const double K = 1.38e-23;			// 玻尔兹曼常数
	
	// double Theta3dB;					// 3dB波束宽度
	// double ScanRate;					// 扫描速率
	// double ServoBandwidth;			// 伺服频宽

	//---------------------------------------------------------------------
	// 此处计算雷达方程

	// 已知距离指标求信噪比指标
	if (OutputType == SNROut)
	{
		switch (EqType)
		{
		case RADAR_Equation::Basic:
		{
			output[0] = Pt * Ga * lambda * lambda * RCS * Gp / (pow((4.0 * PI), 3)*pow(Range, 4)*K*T0*B*F*Ls*La*Lg);
			output[0] = PowerRatioTodB(output[0]); // 转为分贝输出
			break;
		}
		case RADAR_Equation::CW:
		{
			output[0] = Pavg * Td * Ga * lambda * lambda * RCS / (pow((4.0 * PI), 3)*pow(Range, 4)*K*T0*F*Ls*La*Lg*Li);
			output[0] = PowerRatioTodB(output[0]);
			break;
		}
		case RADAR_Equation::PD:
		{
			output[0] = Pavg * PulseNumber * Ga * lambda * lambda * RCS / (pow((4.0 * PI), 3)*pow(Range, 4)*K*T0*F*Ls*La*Lg*Li*PRF);
			output[0] = PowerRatioTodB(output[0]);
			break;
		}
		case RADAR_Equation::Search:
		{
			output[0] = Pt * Ga * lambda * lambda * RCS * Gp / (pow((4.0 * PI), 3)*pow(Range, 4)*K*T0*B*F*Ls*La*Lg) * Theta3dB*PRF / (ScanRate*Li);
			output[0] = PowerRatioTodB(output[0]);
			break;
		}
		case RADAR_Equation::Track:
		{
			output[0] = Pt * Ga * lambda * lambda * RCS * Gp / (pow((4.0 * PI), 3)*pow(Range, 4)*K*T0*B*F*Ls*La*Lg)*PRF / (ServoBandwidth*Li);
			output[0] = PowerRatioTodB(output[0]);
			break;
		}
		default:
			break;
		}
	}
	// 已知信噪比指标求距离指标
	if (OutputType == RangeOut)
	{
		switch (EqType)
		{
		case RADAR_Equation::Basic:
		{
			output[0] = pow(Pt * Ga * lambda * lambda * RCS * Gp / (pow((4.0 * PI), 3)*SNRm*K*T0*B*F*Ls*La*Lg), 0.25);
			break;
		}
		case RADAR_Equation::CW:
		{
			output[0] = pow(Pavg * Td * Ga * lambda * lambda * RCS / (pow((4.0 * PI), 3)*SNRm*K*T0*F*Ls*La*Lg*Li), 0.25);
			break;
		}
		case RADAR_Equation::PD:
		{
			output[0] = pow(Pavg * PulseNumber * Ga * lambda * lambda * RCS / (pow((4.0 * PI), 3)*SNRm*K*T0*F*Ls*La*Lg*Li*PRF), 0.25);
			break;
		}
		case RADAR_Equation::Search:
		{
			output[0] = pow(Pt * Ga * lambda * lambda * RCS * Gp / (pow((4.0 * PI), 3)*SNRm*K*T0*B*F*Ls*La*Lg) * Theta3dB*PRF / (ScanRate*Li), 0.25);
			break;
		}
		case RADAR_Equation::Track:
		{
			output[0] = pow(Pt * Ga * lambda * lambda * RCS * Gp / (pow((4.0 * PI), 3)*SNRm*K*T0*B*F*Ls*La*Lg)*PRF / (ServoBandwidth*Li), 0.25);
			break;
		}
		default:
			break;
		}
	}
	return true;
}
