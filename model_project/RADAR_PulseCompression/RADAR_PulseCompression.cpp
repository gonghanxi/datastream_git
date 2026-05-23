#include "RADAR_PulseCompression.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_PulseCompression )
{	
	SET_MODEL_DESCRIPTION("Pulse Compression");

	SET_MODEL_CATEGORY("Signal Processing");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(signal);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(reference);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	// test port
	//{
	//	SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(test);
	//}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Samplenum);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1000");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FFTSize);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1024");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Bandwidth);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("5e6");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(WindowType, SelectedWindowType);
		enumParam.AddEnumeration("Rectangle", Rectangle);
		enumParam.AddEnumeration("Bartlett", Bartlett);
		enumParam.AddEnumeration("Hanning", Hanning);
		enumParam.AddEnumeration("Hamming", Hamming);
		enumParam.AddEnumeration("Blackman", Blackman);
		enumParam.AddEnumeration("SteepBlackman", SteepBlackman);
		enumParam.AddEnumeration("Kaiser", Kaiser);
		enumParam.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(WindowParameter);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1.0");
		param.SetHideCondition("WindowType ~= 6"); // 仅为 Kaiser 窗提供这个参数
	}
	return true;
}
#endif

RADAR_PulseCompression::RADAR_PulseCompression()
{
	
}

//-----------------------------------------------------------------------------------
//	Setup
//		Port rate should be set here
//-----------------------------------------------------------------------------------
bool RADAR_PulseCompression::Setup()
{
	bool bStatus = true;

	if (Samplenum >= 1 && FFTSize >= Samplenum)
	{
		signal.SetRate(Samplenum);
		reference.SetRate(FFTSize);
		output.SetRate(Samplenum);

		//test port
		//test.SetRate(FFTSize);
	}
	else
	{
		POST_ERROR("FFTSize and Size should meet this condition: FFTSize >= Size >= 1");
		bStatus = false;
	}

	/// 目前只支持幕2的FFT，FFTSize不为2的幕次时可能会出问题///
	if ((FFTSize & (FFTSize - 1)) != 0)
	{
		POST_WARNING("Only 2^N FFTSize is supported now. For FFTSize not equels to 2^N, performance may be insufficient.");
	}

	return bStatus;
}

// 递归法FFT
void RADAR_PulseCompression::fft(SystemVueModelBuilder::Matrix<std::complex<double>>& a, int n, int invert)
{
	const double PI = acos(-1);

	if (n == 1) return;

	int half = n / 2;
	SystemVueModelBuilder::Matrix< std::complex<double> > even(1, half), odd(1, half);

	for (int i = 0; i < half; i++) {
		even(i) = a(i * 2);
		odd(i) = a(i * 2 + 1);
	}

	fft(even, half, invert);
	fft(odd, half, invert);

	double angle = 2 * PI / n * (invert ? -1 : 1);
	std::complex<double> w(1), wn(cos(angle), sin(angle));

	for (int i = 0; i < half; i++) {
		a(i) = even(i) + w * odd(i);
		a(i + half) = even(i) - w * odd(i);
		if (invert) {
			a(i) /= 2;
			a(i + half) /= 2;
		}
		w *= wn;
	}
}

// 计算n阶乘
int RADAR_PulseCompression::factorial(int n) {
	int result = 1;
	for (int i = 1; i <= n; ++i) {
		result *= i;
	}
	return result;
}

// 计算零阶第一类修正贝塞尔函数
double RADAR_PulseCompression::I0(int n, double x) {
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
bool RADAR_PulseCompression::Run()
{
	SystemVueModelBuilder::Matrix< std::complex<double> >	FullSequence(1, FFTSize);
	//-----------------------------------------------------------------------------------
	for (int i = 0; i < FFTSize; i++)
	{
		if (i < Samplenum)
		{
			FullSequence(i) = signal[i];
		}
		// 输入长度小于 FFT 长度时需补零
		else
		{
			FullSequence(i) = 0.0;
		}
	}

	// 此处进行 FFT
	fft(FullSequence, FFTSize, 1);

	// 按 FFT 的点数进行加权
	FullSequence *= FFTSize;
	//-----------------------------------------------------------------------------------
	// 与参考信号频域相乘
	for (int i = 0; i < FFTSize; i++)
	{
		FullSequence(i) *= reference[i];
	}
	//-----------------------------------------------------------------------------------
	// 加窗
	double	freq_resolution = SampleRate / FFTSize;
	int WindowLen = Bandwidth / freq_resolution;
	int WindowN = WindowLen - 1;
	const double PI = acos(-1);

	SystemVueModelBuilder::Matrix< std::complex<double> >	WindowSequence(1, FFTSize); // 窗函数

	switch (WindowType)
	{
	case RADAR_PulseCompression::Rectangle:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			//if (i < WindowLen)
			//{
			//	WindowSequence(i) = 1.0;
			//}
			//else
			//{
			//	WindowSequence(i) = 0.0;	// 补零至FFT长度
			//}
			WindowSequence(i) = 1.0; // SystemVue的矩形窗实际上是不加窗
		}
		break;
	}
	case RADAR_PulseCompression::Bartlett:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			if (i < WindowLen / 2)
			{
				WindowSequence(i) = 2.0 * i / WindowN;
			}
			else if (i >= WindowLen / 2 && i < WindowLen)
			{
				WindowSequence(i) = 2.0 - 2.0 * i / WindowN;
			}
			else
			{
				WindowSequence(i) = 0.0;
			}
		}
		break;
	}
	case RADAR_PulseCompression::Hanning:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			if (i < WindowLen)
			{
				WindowSequence(i) = 0.5 * (1.0 - cos(2.0 * PI*i / WindowN));
			}
			else
			{
				WindowSequence(i) = 0.0;
			}
		}
		break;
	}
	case RADAR_PulseCompression::Hamming:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			if (i < WindowLen)
			{
				WindowSequence(i) = 0.54 - 0.46 * cos(2.0 * PI*i / WindowN);
			}
			else
			{
				WindowSequence(i) = 0.0;
			}
		}
		break;
	}
	case RADAR_PulseCompression::Blackman:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			if (i < WindowLen / 2)
			{
				WindowSequence(i) = 0.42 - 0.5*cos(2.0 * PI*i / WindowN) + 0.08*cos(4.0 * PI*i / WindowN);
			}
			else if (i >= WindowLen / 2 && i < WindowLen)
			{
				WindowSequence(i) = 0.42 - 0.5*cos(2.0 * PI*(WindowLen - i) / WindowN) + 0.08*cos(4.0 * PI*(WindowLen - i) / WindowN);
			}
			else
			{
				WindowSequence(i) = 0.0;
			}
		}
		break;
	}
	// SystemVue中的Blackman-Harris窗系数（下）和Matlab（上）中的不同，这里提供了两种系数的写法
	// 但SystemVue中的Steep Blackman似乎并非Blackman-Harris窗，仍需进一步研究。
	case RADAR_PulseCompression::SteepBlackman:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			if (i < WindowLen / 2)
			{
				WindowSequence(i) = 0.35875 - 0.48829*cos(2.0 * PI*i / WindowN) + 0.14128*cos(4.0 * PI*i / WindowN) - 0.01168*cos(6.0 * PI*i / WindowN);
				//WindowSequence(i) = 0.355768 - 0.487396*cos(2.0 * PI*i / WindowN) + 0.144232*cos(4.0 * PI*i / WindowN) - 0.012604*cos(6.0 * PI*i / WindowN);
			}
			else if (i >= WindowLen / 2 && i < WindowLen)
			{
				WindowSequence(i) = 0.35875 - 0.48829*cos(2.0 * PI*(WindowLen - i) / WindowN) + 0.14128*cos(4.0 * PI*(WindowLen - i) / WindowN) - 0.01168*cos(6.0 * PI*(WindowLen - i) / WindowN);
				//WindowSequence(i) = 0.355768 - 0.487396*cos(2.0 * PI*(WindowLen - i) / WindowN) + 0.144232*cos(4.0 * PI*(WindowLen - i) / WindowN) - 0.012604*cos(6.0 * PI*(WindowLen - i) / WindowN);
			}
			else
			{
				WindowSequence(i) = 0.0;
			}
		}
		break;
	}
	case RADAR_PulseCompression::Kaiser:
	{
		//// 由衰减系数α求Kaiser窗的参数β
		//double beta = 0;
		//if (WindowParameter < 21.0)
		//{
		//	double beta = 0.0;
		//}
		//else if (WindowParameter >=21 && WindowParameter < 50.0)
		//{
		//	double beta = 0.5842*pow((WindowParameter - 21.0), 0.4) + 0.07886*(WindowParameter - 21.0);
		//}
		//else
		//{
		//	double beta = 0.1102*(WindowParameter - 8.7);
		//}

		for (int i = 0; i < FFTSize; i++)
		{
			if (i < WindowLen)
			{
				WindowSequence(i) = I0(20, WindowParameter*sqrt(1.0 - pow(2.0*i / (WindowN)-1.0, 2))) / I0(20, WindowParameter);
			}
			else
			{
				WindowSequence(i) = 0.0;
			}
		}
		break;
	}
	default:
		break;
	}

	// 对窗移位以对应频谱

	SystemVueModelBuilder::Matrix< std::complex<double> >	ShiftSequence(1, FFTSize);	// 移位后窗函数

	for (int i = 0; i < FFTSize; i++)
	{
		int n = i - WindowLen / 2;
		ShiftSequence(i) = WindowSequence(n >= 0 ? n : n + FFTSize);
	}

//	// 对频谱加窗
//	for (int i = 0; i < FFTSize; i++)
//	{
//		//test[i] = ShiftSequence(i);
//		FullSequence(i) *= ShiftSequence(i);
//	}
//	//-----------------------------------------------------------------------------------
//	// 此处进行 IFFT
//	fft(FullSequence, FFTSize, -1);

//	// 输出
//	for (int i = 0; i < Samplenum; i++)
//	{
//		output[i] = FullSequence(i);
//	}
//	return true;
    // 对频谱加窗
    for (int i = 0; i < FFTSize; i++)
    {
        FullSequence(i) *= WindowSequence(i + WindowLen / 2 < FFTSize ? i + WindowLen / 2 : i + WindowLen / 2 - FFTSize);
    }
    //-----------------------------------------------------------------------------------
    // 此处进行 IFFT
    fft(FullSequence, FFTSize, -1);

    // 输出时需将序列颠倒
    for (int i = 0; i < Samplenum; i++)
    {
        output[i] = FullSequence(FFTSize - i < FFTSize ? FFTSize - i : 0);
    }
    return true;
}
