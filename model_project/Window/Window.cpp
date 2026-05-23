#include "Window.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( Window )
{	
	SET_MODEL_DESCRIPTION("Window Generator");
	SET_MODEL_SYMBOL("SYM_Window");
	SET_MODEL_CATEGORY("Sources");

	{
		// 计时输出端口
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(WindowType, SelectedWindowType);
		enumParam.SetDescription("Window type: RECTANGLE, BARTLETT, HANNING, HAMMING, BLACKMAN, STEEPBLACKMAN, KAISER");
		enumParam.AddEnumeration("RECTANGLE", Rectangle);			// 0
		enumParam.AddEnumeration("BARTLETT", Bartlett);				// 1
		enumParam.AddEnumeration("HANNING", Hanning);				// 2
		enumParam.AddEnumeration("HAMMING", Hamming);				// 3
		enumParam.AddEnumeration("BLACKMAN", Blackman);				// 4
		enumParam.AddEnumeration("STEEPBLACKMAN", SteepBlackman);	// 5
		enumParam.AddEnumeration("KAISER", Kaiser);					// 6
		enumParam.SetDefaultValue("2");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Length);
		param.SetDescription("Window sample length");
		param.SetDefaultValue("256");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(ZeroPad);
		param.SetDescription("Number of zero values appended to length");
		param.SetDefaultValue("0");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(KaiserParameter);
		param.SetDescription("Beta parameter");
		param.SetDefaultValue("1");
		param.SetHideCondition("WindowType ~= 6"); // 仅为 Kaiser 窗提供这个参数
		param.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(ShowAdvancedParams, SelectedShowAdvancedParams);
		enumParam.SetDescription("Show advanced parameters: NO, YES");
		enumParam.AddEnumeration("NO", No);
		enumParam.AddEnumeration("YES", Yes);
		enumParam.SetDefaultValue("0");
		enumParam.SetSchematicDisplay(0);
		enumParam.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(SampleRateOption, SelectedSampleRateOption);
		enumParam.SetDescription("Sample rate option: UnTimed, Timed from SampleRate, Timed from Schematic");
		enumParam.AddEnumeration("UnTimed", UnTimed);
		enumParam.AddEnumeration("Timed from SampleRate", TimedFromSampleRate);
		enumParam.AddEnumeration("Timed from Schematic", TimedFromSchematic);
		enumParam.SetDefaultValue("2");
		enumParam.SetHideCondition("ShowAdvancedParams ~= 1");
		enumParam.SetSchematicDisplay(0);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetDescription("Explicit sample rate");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("Sample_Rate");
		param.SetHideCondition("SampleRateOption ~= 1 || ShowAdvancedParams ~= 1"); // 仅为直接设置采样率的选项提供这个参数
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(InitialDelay);
		param.SetDescription("Output sample delay");
		param.SetDefaultValue("0");
		param.SetHideCondition("ShowAdvancedParams ~= 1");
		param.SetSchematicDisplay(0);
		param.SetUseDefault(1);
	}

	return true;
}
#endif

Window::Window()
{

}

bool Window::Setup()
{
	bool bStatus = true;

	if (Length < 4)
	{
		POST_ERROR("The window length must be greater than 3.");
		bStatus = false;
	}

	if (ZeroPad < 0)
	{
		POST_ERROR("The padding length must not be negtive.");
		bStatus = false;
	}

	if (InitialDelay < 0)
	{
		POST_ERROR("The initial delay must not be negtive.");
		bStatus = false;
	}

	/// 在计时模型中如何设置非计时端口尚待研究
	if (SampleRateOption == UnTimed)
	{
		POST_WARNING("Untimed sample is not supported yet. Output index may still be time related.");
	}

	// 设置采样率
	if (SampleRateOption == TimedFromSampleRate)
	{
		if (SampleRate > 0)
		{
			// Use TimedCircularBuffer::SetSampleRate method to set the output sample rate
			output.SetSampleRate(SampleRate);
		}
		else
		{
			POST_ERROR("SampleRate must be greater than 0.");
			bStatus = false;
		}
	}

	return bStatus;
}

// 计算n阶乘
int Window::factorial(int n) {
	int result = 1;
	for (int i = 1; i <= n; ++i) {
		result *= i;
	}
	return result;
}

// 计算零阶第一类修正贝塞尔函数
double Window::I0(int n, double x) {
	double I0_x = 1.0;
	for (int i = 1; i <= n; ++i) {
		I0_x += pow((pow(x / 2, i) / factorial(i)), 2);
	}
	return I0_x;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool Window::Run()
{
	int i = GetCount();

	// 初始时延
	if (i < InitialDelay)
	{
		output[0] = 0.0;
	}

	if (i >= InitialDelay)
	{
		// 生成窗
		int WindowN = Length - 1;
		int SequenceLen = Length + ZeroPad;
		int wi = (i - InitialDelay) % SequenceLen; // 当前索引对应窗内索引
		const double PI = acos(-1);

		switch (WindowType)
		{
		case Rectangle:
		{
			if (wi < Length)
			{
				output[0] = 1.0; // SystemVue的矩形窗实际上是不加窗
			}
			break;
		}
		case Bartlett:
		{
			if (wi < Length)
			{
				if (wi < Length / 2)
				{
					output[0] = 2.0 * wi / WindowN;
				}
				else
				{
					output[0] = 2.0 - 2.0 * wi / WindowN;
				}
			}
			break;
		}
		case Hanning:
		{
			if (wi < Length)
			{
				output[0] = 0.5 * (1.0 - cos(2.0 * PI*wi / WindowN));
			}
			break;
		}
		case Hamming:
		{
			if (wi < Length)
			{
				output[0] = 0.54 - 0.46 * cos(2.0 * PI*wi / WindowN);
			}
			break;
		}
		case Blackman:
		{
			if (wi < Length)
			{
				if (wi < Length / 2)
				{
					output[0] = 0.42 - 0.5*cos(2.0 * PI*wi / WindowN) + 0.08*cos(4.0 * PI*wi / WindowN);
				}
				else
				{
					output[0] = 0.42 - 0.5*cos(2.0 * PI*(Length - wi - 1) / WindowN) + 0.08*cos(4.0 * PI*(Length - wi - 1) / WindowN);
				}
			}
			break;
		}
		// SystemVue中的Blackman-Harris窗系数（下）和Matlab（上）中的不同，这里提供了两种系数的写法
		// 但SystemVue中的Steep Blackman似乎并非Blackman-Harris窗，仍需进一步研究。
		case SteepBlackman:
		{
			if (wi < Length)
			{
				if (wi < Length / 2)
				{
					output[0] = 0.35875 - 0.48829*cos(2.0 * PI*wi / WindowN) + 0.14128*cos(4.0 * PI*wi / WindowN) - 0.01168*cos(6.0 * PI*wi / WindowN);
					//output[0] = 0.355768 - 0.487396*cos(2.0 * PI*wi / WindowN) + 0.144232*cos(4.0 * PI*wi / WindowN) - 0.012604*cos(6.0 * PI*wi / WindowN);
				}
				else
				{
					output[0] = 0.35875 - 0.48829*cos(2.0 * PI*(Length - wi - 1) / WindowN) + 0.14128*cos(4.0 * PI*(Length - wi - 1) / WindowN) - 0.01168*cos(6.0 * PI*(Length - wi - 1) / WindowN);
					//output[0] = 0.355768 - 0.487396*cos(2.0 * PI*(Length - wi - 1) / WindowN) + 0.144232*cos(4.0 * PI*(Length - wi - 1) / WindowN) - 0.012604*cos(6.0 * PI*(Length - wi - 1) / WindowN);
				}
			}
			break;
		}
		case Kaiser:
		{
			if (wi < Length)
			{
				output[0] = I0(20, KaiserParameter*sqrt(1.0 - pow(2.0*wi / (WindowN)-1.0, 2))) / I0(20, KaiserParameter);
			}
			break;
		}
		default:
			break;
		}

		// 补零
		if (wi >= Length)
		{
			output[0] = 0.0;
		}
	}
	return true;
}
