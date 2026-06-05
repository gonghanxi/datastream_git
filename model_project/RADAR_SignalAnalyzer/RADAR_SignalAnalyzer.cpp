#include "RADAR_SignalAnalyzer.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_SignalAnalyzer )
{	
	SET_MODEL_DESCRIPTION("Analyze the signal");

	SET_MODEL_CATEGORY("Measurement");

	ADD_MODEL_INPUT( input );
	ADD_MODEL_OUTPUT( output );
	
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(AnalyzerType, SelectedAnalyzerType);
		enumParam.AddEnumeration("FFT", FFT);
		enumParam.AddEnumeration("IFFT", IFFT);
		enumParam.AddEnumeration("ACF", ACF);
		enumParam.SetDefaultValue("0");
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
		enumParam.SetHideCondition("AnalyzerType == 2");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(WindowParameter);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1.0");
		param.SetHideCondition("WindowType ~= 6"); // 仅为 Kaiser 窗提供这个参数
	}
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(CorrType, SelectedCorrType);
		enumParam.AddEnumeration("Normal", Normal);
		enumParam.AddEnumeration("Biased", Biased);
		enumParam.AddEnumeration("UnBiased", UnBiased);
		enumParam.SetDefaultValue("0");
		enumParam.SetHideCondition("AnalyzerType ~= 2");
	}
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(NormalizedType, SelectedNormalizedType);
		enumParam.AddEnumeration("Normalized", Normalized);
		enumParam.AddEnumeration("NonNormalized", NonNormalized);
		enumParam.SetDefaultValue("0");
	}
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(FFTShiftType, SelectedFFTShiftType);
		enumParam.AddEnumeration("Shifted", Shifted);
		enumParam.AddEnumeration("NonShift", NonShift);
		enumParam.SetDefaultValue("1");
		enumParam.SetHideCondition("AnalyzerType == 2");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleNum);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1024");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FFTSize);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1024");
		param.SetHideCondition("AnalyzerType == 2");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}
	return true;
}
#endif

RADAR_SignalAnalyzer::RADAR_SignalAnalyzer()
{
	
}

bool RADAR_SignalAnalyzer::Setup()
{
	bool bStatus = true;

	// 参数校验
	if (SampleNum <= 0)
	{
		POST_ERROR("SampleNum must be > 0");
		bStatus = false;
	}
	if (FFTSize < SampleNum)
	{
		POST_ERROR("FFTSize must be >= SampleNum");
		bStatus = false;
	}
	/// 目前只支持幕2的FFT，FFTSize不为2的幕次时可能会出问题///
	if ((FFTSize & (FFTSize - 1)) != 0)
	{
		POST_WARNING("Only 2^N FFTSize is supported now. For FFTSize not equels to 2^N, performance may be insufficient.");
	}
	if (SampleRate <= 0)
	{
		POST_ERROR("SampleRate must be > 0");
		bStatus = false;
	}

	input.SetRate(SampleNum);
	output.SetRate(SampleNum);

	return bStatus;
}

// 递归法FFT
void RADAR_SignalAnalyzer::fft(SystemVueModelBuilder::Matrix<std::complex<double>>& a, int n, int invert)
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
int RADAR_SignalAnalyzer::factorial(int n) {
	int result = 1;
	for (int i = 1; i <= n; ++i) {
		result *= i;
	}
	return result;
}

// 计算零阶第一类修正贝塞尔函数
double RADAR_SignalAnalyzer::I0(int n, double x) {
	double I0_x = 1.0;
	for (int i = 1; i <= n; ++i) {
		I0_x += pow((pow(x / 2, i) / factorial(i)), 2);
	}
	return I0_x;
}

// 自相关函数
SystemVueModelBuilder::Matrix<std::complex<double>> RADAR_SignalAnalyzer::autoCorr(SystemVueModelBuilder::Matrix<std::complex<double>>& A, int LenA)
{
	SystemVueModelBuilder::Matrix<std::complex<double>> result(1, 2 * LenA - 1);
	result.Zero();

	for (int i = 0; i < LenA; ++i)
	{
		for (int j = 0; j < LenA; ++j)
		{
			result(i + j) += A(i)*std::conj(A(LenA - j - 1));
		}
	}
	return result;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_SignalAnalyzer::Run()
{
	// 窗函数
	int WindowLen = FFTSize;
	int WindowN = WindowLen - 1;
	const double PI = acos(-1);
	SystemVueModelBuilder::Matrix< std::complex<double> >	WindowSequence(1, FFTSize);

	switch (WindowType)
	{
	case RADAR_SignalAnalyzer::Rectangle:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			WindowSequence(i) = 1.0; // SystemVue的矩形窗实际上是不加窗
		}
		break;
	}
	case RADAR_SignalAnalyzer::Bartlett:
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
	case RADAR_SignalAnalyzer::Hanning:
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
	case RADAR_SignalAnalyzer::Hamming:
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
	case RADAR_SignalAnalyzer::Blackman:
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
	case RADAR_SignalAnalyzer::SteepBlackman:
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
	case RADAR_SignalAnalyzer::Kaiser:
	{
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

	//——————————————————————————————————————————
	// 后处理
	switch (AnalyzerType)
	{
	case RADAR_SignalAnalyzer::FFT:
	{
		SystemVueModelBuilder::Matrix< std::complex<double> >	FullSequence(1, FFTSize);
		
		for (int i = 0; i < FFTSize; i++)
		{
			if (i < SampleNum)
			{
				FullSequence(i) = input[i];
			}
			// 输入长度小于 FFT 长度时需补零
			else
			{
				FullSequence(i) = 0.0;
			}
		}
		// 此处进行 FFT
		fft(FullSequence, FFTSize, 1);

		// 若选择非归一化则按 FFT 的点数进行加权
		if (NormalizedType == NonNormalized)
		{
			FullSequence *= FFTSize;
		}

		// 对频谱加窗
		for (int i = 0; i < FFTSize; i++)
		{
			FullSequence(i) *= WindowSequence(i);
		}

		// 不进行 Shift
		if (FFTShiftType == NonShift)
		{
			for (int i = 0; i < SampleNum; i++)
			{
				output[i] = std::abs(FullSequence(i));
			}
		}

		// 进行 Shift
		else if (FFTShiftType == Shifted)
		{
			// FFT Shift 是向右圆周位移 FFTSize/2（向下取整）位
			for (int i = 0; i < SampleNum; i++)
			{
				int n = i - FFTSize / 2;

				output[i] = std::abs(FullSequence(n >= 0 ? n : n + FFTSize));
			}
		}
		break;
	}
	//————————————————————————————————————————————
	case RADAR_SignalAnalyzer::IFFT:
	{
		SystemVueModelBuilder::Matrix< std::complex<double> >	FullSequence(1, FFTSize);
		SystemVueModelBuilder::Matrix< std::complex<double> >	ShiftSequence(1, FFTSize);

		for (int i = 0; i < FFTSize; i++)
		{
			if (i < SampleNum)
			{
				FullSequence(i) = input[i];
			}
			// 输入长度小于 FFT 长度时需补零
			else
			{
				FullSequence(i) = 0.0;
			}
		}

		// 不进行 Shift
		if (FFTShiftType == NonShift)
		{
			for (int i = 0; i < FFTSize; i++)
			{
				ShiftSequence(i) = FullSequence(i);
			}
		}

		// 进行 Shift
		else if (FFTShiftType == Shifted)
		{
			// IFFT Shift是向左圆周位移 FFTSize/2（向下取整）位

			for (int i = 0; i < FFTSize; i++)
			{
				int n = i + FFTSize / 2;

				ShiftSequence(i) = FullSequence(n < FFTSize ? n : n - FFTSize);
			}
		}

		// 对频谱加窗
		for (int i = 0; i < FFTSize; i++)
		{
			ShiftSequence(i) *= WindowSequence(i);
		}

		// 此处进行 IFFT
		fft(ShiftSequence, FFTSize, -1);

		// 若选择非归一化则按 FFT 的点数进行加权
		if (NormalizedType == NonNormalized)
		{
			ShiftSequence *= FFTSize;
		}

		// 输出时需将序列颠倒
		for (int i = 0; i < SampleNum; i++)
		{
			output[i] = std::abs(ShiftSequence(FFTSize - i < FFTSize ? FFTSize - i : 0));
		}
		break;
	}
	//————————————————————————————————————————————
	case RADAR_SignalAnalyzer::ACF:
	{
		SystemVueModelBuilder::Matrix< std::complex<double> >	FullSequence(1, SampleNum);
		SystemVueModelBuilder::Matrix< std::complex<double> >	CorrSequence(1, 2 * SampleNum - 1);

		for (int i = 0; i < SampleNum; i++)
		{
			FullSequence(i) = input[i];
		}
		// 此处进行自相关运算
		CorrSequence = autoCorr(FullSequence, SampleNum);

		// 输出后自相关结果后半部分
		for (int i = 0; i < SampleNum; i++)
		{
			output[i] = std::abs(CorrSequence(i + SampleNum - 1));
		}
		break;
	}
	default:
		break;
	}


	return true;
}
