#include "PulseGen.h"
#include <cmath>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( PulseGen )
{	
	SET_MODEL_DESCRIPTION("Pulse Waveform Generator");
	//SET_MODEL_SYMBOL("SYM_PulseGen");
	SET_MODEL_CATEGORY("Sources");

	ADD_MODEL_OUTPUT( output );
	
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(LoLevel);
		param.SetName("LoLevel");
		param.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		param.SetDefaultValue("0");
		param.SetDescription("Low voltage level");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(HiLevel);
		param.SetName("HiLevel");
		param.SetUnit(SystemVueModelBuilder::Units::VOLTAGE);
		param.SetDefaultValue("1");
		param.SetDescription("High voltage level");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Period);
		param.SetName("Period");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("200e-6");
		param.SetDescription("Pulse waveform period");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Phase);
		param.SetName("Phase");
		param.SetUnit(SystemVueModelBuilder::Units::ANGLE);
		param.SetDefaultValue("0");
		param.SetDescription("Phase");
		param.SetSchematicDisplay(false);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PulseWidth);
		param.SetName("PulseWidth");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("100e-6");
		param.SetDescription("Pulse width at 50% waveform levels");
		param.SetSchematicDisplay(false);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(EdgeSymmetry, EdgeSymmetrys);
		enumParam.SetUnit(SystemVueModelBuilder::Units::NONE);
		enumParam.AddEnumeration("Symmetric", Symmetric);
		enumParam.AddEnumeration("Asymmetric", Asymmetric);
		enumParam.SetDefaultValue("Symmetric");
		enumParam.SetDescription("Rising and falling edge symmetry: Symmetric, Asymmetric");
		enumParam.SetSchematicDisplay(false);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(EdgeTime);
		param.SetName("EdgeTime");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("50e-6");
		param.SetDescription("Rising and falling edge times (0% to 100% values)");
		param.SetHideCondition("EdgeSymmetry ~= 0");
		param.SetSchematicDisplay(false);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(RisingEdgeTime);
		param.SetName("RisingEdgeTime");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("50e-6");
		param.SetDescription("Rising edge time (0% to 100% value)");
		param.SetHideCondition("EdgeSymmetry ~= 1");
		param.SetSchematicDisplay(false);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(FallingEdgeTime);
		param.SetName("FallingEdgeTime");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("50e-6");
		param.SetDescription("Falling edge time (0% to 100% value)");
		param.SetHideCondition("EdgeSymmetry ~= 1");
		param.SetSchematicDisplay(false);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(Polarity, Polaritys);
		enumParam.SetUnit(SystemVueModelBuilder::Units::NONE);
		enumParam.AddEnumeration("normal", normal);
		enumParam.AddEnumeration("inverted", inverted);
		enumParam.SetDefaultValue("normal");
		enumParam.SetDescription("Signal polarity: normal, inverted");
		enumParam.SetSchematicDisplay(false);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SampleRate);
		param.SetName("SampleRate");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
		param.SetDescription("Explicit sample rate");
		param.SetSchematicDisplay(false);
	}

	return true;
}
#endif

PulseGen::PulseGen()
{
	counter = 0;
}

//-----------------------------------------------------------------------------------
//	Setup
//		Declare the sample rate (if set) to SystemVue. If not set, the model will use the sample rate computed by SystemVue.
//-----------------------------------------------------------------------------------
bool PulseGen::Setup()
{
	bool bStatus = true;
	if (SampleRate > 0)
	{
		output.SetSampleRate(SampleRate);
	}
	else
	{
		POST_ERROR("SampleRate must be greater than 0.");
        LOG_ERROR("SampleRate must be greater than 0.");
		bStatus = false;
	}

	if (HiLevel <= LoLevel)
	{
		POST_ERROR("HiLevel must be greater than LoLevel.");
        LOG_ERROR("HiLevel must be greater than LoLevel.");
		bStatus = false;
	}

	if (Period < 2/SampleRate)
	{
		POST_ERROR("Period must be greater than 2/SampleRate.");
        LOG_ERROR("Period must be greater than 2/SampleRate.");
		bStatus = false;
	}

	if (PulseWidth < EdgeTime || PulseWidth > Period - EdgeTime)
	{
		POST_ERROR("PulseWidth must be >= EdgeTime and <= Period - EdgeTime.");
        LOG_ERROR("PulseWidth must be >= EdgeTime and <= Period - EdgeTime.");
		bStatus = false;
	}

	if ((EdgeTime < 1 / SampleRate || EdgeTime > Period / 2) && (EdgeSymmetry == Symmetric))
	{
		POST_ERROR("EdgeTime must be >= 1 / SampleRate and <= Period / 2.");
        LOG_ERROR("EdgeTime must be >= 1 / SampleRate and <= Period / 2.");
		bStatus = false;
	}

	if ((RisingEdgeTime < 1 / SampleRate || RisingEdgeTime > Period / 2) && (EdgeSymmetry == Asymmetric))
	{
		POST_ERROR("RisingEdgeTime must be >= 1 / SampleRate and <= Period / 2.");
        LOG_ERROR("RisingEdgeTime must be >= 1 / SampleRate and <= Period / 2.");
		bStatus = false;
	}

	if ((FallingEdgeTime < 1 / SampleRate || FallingEdgeTime > Period / 2) && (EdgeSymmetry == Asymmetric))
	{
		POST_ERROR("FallingEdgeTime must be >= 1 / SampleRate and <= Period / 2.");
        LOG_ERROR("FallingEdgeTime must be >= 1 / SampleRate and <= Period / 2.");
		bStatus = false;
	}

	return bStatus;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool PulseGen::Run()
{
	bool bStatus = true;
	//Use TimedCircularBuffer::GetTime method to get the time stamp of the output sample
	//In output.GetTime( 0, m_iFiringCount ), 0 means the 0th output sample of each firing (run), and TimedDFModel::GetCount returns the current firing count.

	const double PI = 3.14159265358979323846;

	double t = counter / SampleRate;

	// 1. 提取实际使用的上升/下降沿时间（根据对称配置）
	double tr, tf;
	if (EdgeSymmetry == Symmetric)
	{
		tr = EdgeTime;
		tf = EdgeTime;
	}
	else // Asymmetric
	{
		tr = RisingEdgeTime;
		tf = FallingEdgeTime;
	}

	// 2. 计算当前时间在脉冲周期内的归一化位置（叠加相位偏移）
	double t_cycle = std::fmod(t + Phase * Period / (2 * PI), Period);

	// 3. 确定高低电平区间和边沿位置（边沿中心在50%幅度位置）
	double rising_start = 0;
	double rising_end = tr;
	double falling_start = PulseWidth;
	double falling_end = PulseWidth + tf;

	// 4. 计算当前时间的电压幅值
	double out_val;
	if (t_cycle < rising_start || t_cycle > falling_end)
	{
		// 低电平区间
		out_val = LoLevel;
	}
	else if (t_cycle >= rising_start && t_cycle <= rising_end)
	{
		// 上升沿：线性渐变（也可替换为余弦平滑，这里保持简单线性）
		double alpha = (HiLevel - LoLevel) / tr;
		out_val = LoLevel + (t_cycle - rising_start) * alpha;
	}
	else if (t_cycle >= rising_end && t_cycle <= falling_start)
	{
		// 高电平区间
		out_val = HiLevel;
	}
	else // 下降沿区间
	{
		double alpha = (t_cycle - falling_start) / tf;
		out_val = HiLevel - (HiLevel - LoLevel) * alpha;
	}

	// 5. 处理极性反转
	if (Polarity == inverted)
	{
		// 高低电平互换
		out_val = LoLevel + HiLevel - out_val;
	}

	// 写入当前输出样本
	output[0] = out_val;

		counter++;
	
	return bStatus;
}
