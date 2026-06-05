#include "RADAR_LFMRef.h"

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_LFMRef )
{	
	SET_MODEL_DESCRIPTION(" Linear Frequency Modulation Waveform Reference Function");

	SET_MODEL_CATEGORY("Signal Processing");
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetDescription("The conjunction of spectrum of referenced signal");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Pulsewidth);
		param.SetDescription("Pulse Width");
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-5");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Bandwidth);
		param.SetDescription("Waveform Bandwidth");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("5e6");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(FM_Offset);
		param.SetDescription("Frequency Modulation Offset");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("0");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SampleRate);
		param.SetDescription("Waveform Baseband Sampling Rate");
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(FFTSize);
		param.SetDescription("The fft size which should be 2^N and greater than sample number of pulse");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1024");
	}
	return true;
}
#endif

RADAR_LFMRef::RADAR_LFMRef()
{

}

bool RADAR_LFMRef::Setup()
{
	bool bStatus = true;

	if (Pulsewidth <= 0)
	{
		POST_ERROR("Pulsewidth must be > 0");
		bStatus = false;
	}
	if (Bandwidth <= 0)
	{
		POST_ERROR("Bandwidth must be > 0");
		bStatus = false;
	}
	if (SampleRate <= 0)
	{
		POST_ERROR("SampleRate must be > 0");
		bStatus = false;
	}
	if (FFTSize <= 0)
	{
		POST_ERROR("FFTSize must be > 0");
		bStatus = false;
	}
	/// 目前只支持幕2的FFT，FFTSize不为2的幕次时可能会出问题///
	if ((FFTSize & (FFTSize - 1)) != 0)
	{
		POST_WARNING("Only 2^N FFTSize is supported now. For FFTSize not equels to 2^N, performance may be insufficient.");
	}

	output.SetRate(FFTSize);

	return bStatus;
}

// 递归法FFT
void RADAR_LFMRef::fft(SystemVueModelBuilder::Matrix<std::complex<double>>& a, int n, int invert)
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


//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_LFMRef::Run()
{
	SystemVueModelBuilder::Matrix< std::complex<double> >	LFMSequence(1, FFTSize);
	SystemVueModelBuilder::Matrix< std::complex<double> >	FFTSequence(1, FFTSize);
	
	const double PI = std::acos(-1);
	std::complex<double> imag_I(0, 1);

	// LFM信号序列
	for (int i = 0; i < FFTSize; i++)
	{
		if (i < Pulsewidth*SampleRate)
		{
			double t = i / SampleRate;
			LFMSequence(i) = std::exp(imag_I*PI*Bandwidth*std::pow((t - Pulsewidth / 2), 2) / Pulsewidth)*std::exp(imag_I*2.0*PI*FM_Offset*t);
		}
		// 脉宽外补零
		else
		{
			LFMSequence(i) = 0.0;
		}
	}

	// 共轭翻转
	for (int i = 0; i < FFTSize; i++)
	{
		FFTSequence(i) = std::conj(LFMSequence(FFTSize - i - 1));
	}

	// 此处进行 FFT
	fft(FFTSequence, FFTSize, 1);

	// 按 FFT 的点数进行加权
	FFTSequence *= FFTSize;

	// 输出序列
	for (int i = 0; i < FFTSize; i++)
	{
		output[i] = FFTSequence(i);
	}

	return true;
}
