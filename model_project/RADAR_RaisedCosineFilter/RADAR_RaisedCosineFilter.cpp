#include "RADAR_RaisedCosineFilter.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE ( RADAR_RaisedCosineFilter )
{	
	SET_MODEL_DESCRIPTION("Raised Cosine Filter");
	
	SET_MODEL_CATEGORY("Receiver");
	SET_MODEL_CATEGORY("Transmitters");

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(input);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(coeff);
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(Alpha);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("0.5");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(PRI);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-4");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(FilterLen);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("24");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(SampleRate);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
	}
	return true;
}
#endif

RADAR_RaisedCosineFilter::RADAR_RaisedCosineFilter()
{

}

bool RADAR_RaisedCosineFilter::Setup()
{
	bool bStatus = true;

	// 参数校验
	if (Alpha < 0 || Alpha>1)
	{
		POST_ERROR("Alpha must be >= 0 and <= 1");
		bStatus = false;
	}
	if (PRI < 1 / SampleRate)
	{
		POST_ERROR("PRI must be >= 1 / SampleRate");
		bStatus = false;
	}
	if (FilterLen <= 0)
	{
		POST_ERROR("FilterLen must be > 0");
		bStatus = false;
	}
	if (SampleRate <= 0)
	{
		POST_ERROR("SampleRate must be > 0");
		bStatus = false;
	}

	if (bStatus)
	{
		// 端口速率为一个PRI内的点数
		numPRI = PRI * SampleRate;
		input.SetRate(numPRI);
		output.SetRate(numPRI);
		// 滤波器系数端口速率为滤波器长度
		coeff.SetRate(FilterLen);
	}

	return bStatus;
}

// sinc函数
static inline double sinc(double x) {
	return (std::abs(x) < 1e-12) ? 1.0 : std::sin(M_PI * x) / (M_PI * x);
}

// 卷积函数
SystemVueModelBuilder::Matrix<double> RADAR_RaisedCosineFilter::convolve(SystemVueModelBuilder::Matrix<double>& A, SystemVueModelBuilder::Matrix<double>& B, int LenA, int LenB)
{
	SystemVueModelBuilder::Matrix<double> result(1, LenA + LenB - 1);
	result.Zero();

	for (int i = 0; i < LenA; ++i)
	{
		for (int j = 0; j < LenB; ++j)
		{
			result(i + j) += A(i)*B(j);
		}
	}
	return result;
}

// 升余弦滤波器FIR系数
SystemVueModelBuilder::Matrix<double> RADAR_RaisedCosineFilter::raisedCosine(double alpha, int numTaps)
{
	SystemVueModelBuilder::Matrix<double> taps(1, numTaps);
	taps.Zero();

	double T = (double)(numTaps - 1);

	for (int i = 0; i < numTaps; ++i) {
		double t = (double)(i - numTaps / 2);
		if (t == 0.0) {
			taps(i) = 1.0 - alpha + 4.0 * alpha / M_PI;
		}
		else {
			double denom = 1.0 - std::pow(2.0 * alpha * t / T, 2);
			if (std::abs(denom) < 1e-12) {
				taps(i) = alpha / 2.0 * std::sin(M_PI / (2.0 * alpha)) * sinc(1.0 / (2.0 * alpha));
			}
			else {
				taps(i) = sinc(t / T) * std::cos(M_PI * alpha * t / T) / denom;
			}
		}

	}
	return taps;
}


//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_RaisedCosineFilter::Run()
{
	const std::complex<double> Imag_I{ 0,1 };

	// 输入序列
	SystemVueModelBuilder::Matrix<double> inputArrayReal(1,numPRI);
	SystemVueModelBuilder::Matrix<double> inputArrayImag(1,numPRI);
	for (int i = 0; i < numPRI; i++)
	{
		// 输入序列实部
		inputArrayReal(i) = input[i].real();
		inputArrayImag(i) = input[i].imag();
	}

	// 滤波器系数
	SystemVueModelBuilder::Matrix<double> RCFilter(1, FilterLen);
	RCFilter = raisedCosine(Alpha, FilterLen);
	for (int i = 0; i < FilterLen; i++)
	{
		coeff[i] = RCFilter(i) + Imag_I * RCFilter(i);
	}

	// 卷积
	SystemVueModelBuilder::Matrix<double> outputArrayReal(1, numPRI + FilterLen - 1);
	SystemVueModelBuilder::Matrix<double> outputArrayImag(1, numPRI + FilterLen - 1);
	outputArrayReal = convolve(inputArrayReal, RCFilter, numPRI, FilterLen);
	outputArrayImag = convolve(inputArrayImag, RCFilter, numPRI, FilterLen);

	// 输出序列
	for (int i = 0; i < numPRI; i++)
	{
		output[i] = outputArrayReal(i) + Imag_I * outputArrayImag(i);
	}

	return true;
}


