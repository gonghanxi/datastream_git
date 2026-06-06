#include "RADAR_MTD_M.h"

#include <algorithm>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_MTD_M)
{
	SET_MODEL_DESCRIPTION("Moving Target Detection for Matrix signals");
	SET_MODEL_CATEGORY("Signal Processing");

	// ===== 端口 =====
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(input);
		p.SetName("input");
		p.SetDescription("The input signal");
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(output);
		p.SetName("output");
		p.SetDescription("The output signal after MTD processing");
	}

	// ===== NumOfPulse =====
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(NumOfPulse);
		param.SetDescription("Number of pulses");
		param.SetDefaultValue("8");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
	}

	// ===== Freq_Weight =====
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_ARRAY_PARAM(Freq_Weight, Freq_Weight_Size);
		param.SetDescription("The weights in frequency domain");
		param.SetDefaultValue("[1,1,1,1,1,1,1,1]");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
	}

	// ===== WindowType =====
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(WindowType, SelectedWindowType);
		enumParam.SetDescription("The type of window function: Rectangle, Bartlett, Hanning, Hamming, Blackman, SteepBlackman, Kaiser");
		enumParam.AddEnumeration("Rectangle", Rectangle);
		enumParam.AddEnumeration("Bartlett", Bartlett);
		enumParam.AddEnumeration("Hanning", Hanning);
		enumParam.AddEnumeration("Hamming", Hamming);
		enumParam.AddEnumeration("Blackman", Blackman);
		enumParam.AddEnumeration("SteepBlackman", SteepBlackman);
		enumParam.AddEnumeration("Kaiser", Kaiser);
		enumParam.SetDefaultValue("0");
	}

	// ===== WindowParameters =====
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_ARRAY_PARAM(WindowParameters, WindowParameters_Size);
		param.SetDescription("The array of values for the window");
		param.SetDefaultValue("0");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
	}

	return true;
}
#endif


RADAR_MTD_M::RADAR_MTD_M()
	: NumOfPulse(8)
	, Freq_Weight(nullptr)
	, Freq_Weight_Size(0)
	, WindowType(Rectangle)
	, WindowParameters(nullptr)
	, WindowParameters_Size(0)
{
}

RADAR_MTD_M::~RADAR_MTD_M()
{
}

bool RADAR_MTD_M::Setup()
{
	if (NumOfPulse <= 0) {
		NumOfPulse = 1;
	}

	generateWindow(NumOfPulse);

	return true;
}

//-----------------------------------------------------------------------------------
// Run
//
// 矩阵版 MTD：
// input  : complex matrix
// output : complex matrix
//
// 为了适配 Pack_M / DynamicPack_M 可能产生的不同矩阵方向，这里做两种兼容：
//
// 1. 若列数 == NumOfPulse：
//      认为每一行是一个距离门，每一行内部的列方向是慢时间脉冲序列。
//      即 input(range, pulse)
//
// 2. 若行数 == NumOfPulse：
//      认为每一列是一个距离门，每一列内部的行方向是慢时间脉冲序列。
//      即 input(pulse, range)
//
// 若两者都不满足，则优先尝试按列方向分块处理；仍不满足则直接复制输入到输出。
//-----------------------------------------------------------------------------------
bool RADAR_MTD_M::Run()
{
	const SystemVueModelBuilder::Matrix< std::complex<double> >& inMat = input[0];

	const size_t nRows = inMat.NumRows();
	const size_t nCols = inMat.NumColumns();

	SystemVueModelBuilder::Matrix< std::complex<double> > outMat;
	outMat.Resize(nRows, nCols);

	if (NumOfPulse <= 0 || nRows == 0 || nCols == 0) {
		output[0] = inMat;
		return true;
	}

	generateWindow(NumOfPulse);

	const int N = NumOfPulse;

	// ===== 情况 1：列方向为脉冲维度，行方向为距离门 =====
	if (static_cast<int>(nCols) == N)
	{
		for (size_t r = 0; r < nRows; ++r)
		{
			std::vector<std::complex<double>> x(N);

			for (int p = 0; p < N; ++p) {
				x[p] = inMat(r, static_cast<size_t>(p));
			}

			processOneSlowTimeVector(x);

			for (int k = 0; k < N; ++k) {
				outMat(r, static_cast<size_t>(k)) = x[k];
			}
		}

		output[0] = outMat;
		return true;
	}

	// ===== 情况 2：行方向为脉冲维度，列方向为距离门 =====
	if (static_cast<int>(nRows) == N)
	{
		for (size_t c = 0; c < nCols; ++c)
		{
			std::vector<std::complex<double>> x(N);

			for (int p = 0; p < N; ++p) {
				x[p] = inMat(static_cast<size_t>(p), c);
			}

			processOneSlowTimeVector(x);

			for (int k = 0; k < N; ++k) {
				outMat(static_cast<size_t>(k), c) = x[k];
			}
		}

		output[0] = outMat;
		return true;
	}

	// ===== 情况 3：列方向可按 NumOfPulse 分块 =====
	if (static_cast<int>(nCols) > N && static_cast<int>(nCols) % N == 0)
	{
		for (size_t r = 0; r < nRows; ++r)
		{
			for (size_t start = 0; start < nCols; start += static_cast<size_t>(N))
			{
				std::vector<std::complex<double>> x(N);

				for (int p = 0; p < N; ++p) {
					x[p] = inMat(r, start + static_cast<size_t>(p));
				}

				processOneSlowTimeVector(x);

				for (int k = 0; k < N; ++k) {
					outMat(r, start + static_cast<size_t>(k)) = x[k];
				}
			}
		}

		output[0] = outMat;
		return true;
	}

	// ===== 情况 4：行方向可按 NumOfPulse 分块 =====
	if (static_cast<int>(nRows) > N && static_cast<int>(nRows) % N == 0)
	{
		for (size_t c = 0; c < nCols; ++c)
		{
			for (size_t start = 0; start < nRows; start += static_cast<size_t>(N))
			{
				std::vector<std::complex<double>> x(N);

				for (int p = 0; p < N; ++p) {
					x[p] = inMat(start + static_cast<size_t>(p), c);
				}

				processOneSlowTimeVector(x);

				for (int k = 0; k < N; ++k) {
					outMat(start + static_cast<size_t>(k), c) = x[k];
				}
			}
		}

		output[0] = outMat;
		return true;
	}

	// ===== 无法判断矩阵方向时，保持输入不变，避免越界或错误处理 =====
	output[0] = inMat;
	return true;
}

//-----------------------------------------------------------------------------------
// processOneSlowTimeVector
//
// 单个距离门上的慢时间 MTD 处理：
// 1. 三脉冲对消
// 2. 时域加窗
// 3. FFT / DFT
// 4. 频域权重
//-----------------------------------------------------------------------------------
void RADAR_MTD_M::processOneSlowTimeVector(std::vector<std::complex<double>>& x)
{
	const int N = static_cast<int>(x.size());
	if (N <= 0) return;

	// ===== 1. 三脉冲对消 =====
	// 帮助文档结构为：I/Q data -> 3-Pulse Canceller -> Weight Function -> FFT
	//
	// 这里采用帧内边界清零方式：
	// y[0] = 0
	// y[1] = 0
	// y[p] = x[p] - 2*x[p-1] + x[p-2], p >= 2
	//
	// 如果后续黑盒测试发现内置前两个点使用历史状态，可再改成跨帧缓存。
	std::vector<std::complex<double>> y(N, std::complex<double>(0.0, 0.0));

	if (N == 1) {
		y[0] = x[0];
	}
	else if (N == 2) {
		y[0] = std::complex<double>(0.0, 0.0);
		y[1] = std::complex<double>(0.0, 0.0);
	}
	else {
		y[0] = std::complex<double>(0.0, 0.0);
		y[1] = std::complex<double>(0.0, 0.0);

		for (int p = 2; p < N; ++p) {
			y[p] = x[p] - 2.0 * x[p - 1] + x[p - 2];
		}
	}

	// ===== 2. 时域加窗 =====
	if (static_cast<int>(window.size()) != N) {
		generateWindow(N);
	}

	for (int p = 0; p < N; ++p) {
		y[p] *= window[p];
	}

	// ===== 3. FFT / DFT =====
	internalFFT(y);

	// ===== 4. 频域权重 =====
	for (int k = 0; k < N; ++k)
	{
		double w = 1.0;

		if (Freq_Weight != nullptr && Freq_Weight_Size > 0)
		{
			if (k < Freq_Weight_Size) {
				w = Freq_Weight[k];
			}
			else {
				// 若数组长度不足，超出的 Doppler bin 默认权重为 1
				w = 1.0;
			}
		}

		y[k] *= w;
	}

	x = y;
}

//-----------------------------------------------------------------------------------
// 纯 C++ FFT / DFT
//-----------------------------------------------------------------------------------
bool RADAR_MTD_M::isPowerOfTwo(int n) const
{
	return n > 0 && ((n & (n - 1)) == 0);
}

void RADAR_MTD_M::internalFFT(std::vector<std::complex<double>>& x)
{
	const int N = static_cast<int>(x.size());

	if (N <= 1) {
		return;
	}

	if (isPowerOfTwo(N)) {
		backupFFT(x);
	}
	else {
		directDFT(x);
	}
}

// 基 2 Cooley-Tukey FFT，不做 1/N 归一化
void RADAR_MTD_M::backupFFT(std::vector<std::complex<double>>& x)
{
	const size_t N = x.size();

	if (N <= 1) {
		return;
	}

	std::vector<std::complex<double>> even(N / 2);
	std::vector<std::complex<double>> odd(N / 2);

	for (size_t i = 0; i < N / 2; ++i)
	{
		even[i] = x[2 * i];
		odd[i] = x[2 * i + 1];
	}

	backupFFT(even);
	backupFFT(odd);

	for (size_t k = 0; k < N / 2; ++k)
	{
		const double angle = -2.0 * kPI * static_cast<double>(k) / static_cast<double>(N);
		const std::complex<double> twiddle(std::cos(angle), std::sin(angle));

		const std::complex<double> t = twiddle * odd[k];

		x[k] = even[k] + t;
		x[k + N / 2] = even[k] - t;
	}
}

// 任意点数 DFT，不做 1/N 归一化
void RADAR_MTD_M::directDFT(std::vector<std::complex<double>>& x)
{
	const int N = static_cast<int>(x.size());

	if (N <= 1) {
		return;
	}

	std::vector<std::complex<double>> y(N, std::complex<double>(0.0, 0.0));

	for (int k = 0; k < N; ++k)
	{
		std::complex<double> sum(0.0, 0.0);

		for (int n = 0; n < N; ++n)
		{
			const double angle =
				-2.0 * kPI * static_cast<double>(k) * static_cast<double>(n) / static_cast<double>(N);

			const std::complex<double> twiddle(std::cos(angle), std::sin(angle));

			sum += x[n] * twiddle;
		}

		y[k] = sum;
	}

	x = y;
}

//-----------------------------------------------------------------------------------
// 窗函数生成
//-----------------------------------------------------------------------------------
void RADAR_MTD_M::generateWindow(int size)
{
	if (size <= 0) {
		window.clear();
		return;
	}

	switch (WindowType)
	{
	case RADAR_MTD_M::Rectangle:
		window.assign(size, 1.0);
		break;

	case RADAR_MTD_M::Bartlett:
		window = generateBartlettWindow(size);
		break;

	case RADAR_MTD_M::Hanning:
		window = generateHanningWindow(size);
		break;

	case RADAR_MTD_M::Hamming:
		window = generateHammingWindow(size);
		break;

	case RADAR_MTD_M::Blackman:
		window = generateBlackmanWindow(size);
		break;

	case RADAR_MTD_M::SteepBlackman:
		window = generateSteepBlackmanWindow(size);
		break;

	case RADAR_MTD_M::Kaiser:
	{
		double beta = 0.0;

		if (WindowParameters != nullptr && WindowParameters_Size > 0) {
			beta = WindowParameters[0];
		}

		window = generateKaiserWindow(size, beta);
		break;
	}

	default:
		window.assign(size, 1.0);
		break;
	}
}

std::vector<double> RADAR_MTD_M::generateBartlettWindow(int size)
{
	std::vector<double> w(size, 1.0);

	if (size <= 1) {
		return w;
	}

	const double M = static_cast<double>(size - 1);

	for (int n = 0; n < size; ++n)
	{
		w[n] = 1.0 - std::abs((static_cast<double>(n) - M / 2.0) / (M / 2.0));
	}

	return w;
}

std::vector<double> RADAR_MTD_M::generateHanningWindow(int size)
{
	std::vector<double> w(size, 1.0);

	if (size <= 1) {
		return w;
	}

	const double M = static_cast<double>(size - 1);

	for (int n = 0; n < size; ++n)
	{
		w[n] = 0.5 - 0.5 * std::cos(2.0 * kPI * static_cast<double>(n) / M);
	}

	return w;
}

std::vector<double> RADAR_MTD_M::generateHammingWindow(int size)
{
	std::vector<double> w(size, 1.0);

	if (size <= 1) {
		return w;
	}

	const double M = static_cast<double>(size - 1);

	for (int n = 0; n < size; ++n)
	{
		w[n] = 0.54 - 0.46 * std::cos(2.0 * kPI * static_cast<double>(n) / M);
	}

	return w;
}

std::vector<double> RADAR_MTD_M::generateBlackmanWindow(int size)
{
	std::vector<double> w(size, 1.0);

	if (size <= 1) {
		return w;
	}

	const double M = static_cast<double>(size - 1);

	for (int n = 0; n < size; ++n)
	{
		const double a = 2.0 * kPI * static_cast<double>(n) / M;
		w[n] = 0.42 - 0.50 * std::cos(a) + 0.08 * std::cos(2.0 * a);
	}

	return w;
}

std::vector<double> RADAR_MTD_M::generateSteepBlackmanWindow(int size)
{
	std::vector<double> w(size, 1.0);

	if (size <= 1) {
		return w;
	}

	// 这里采用常见 4-term Blackman-Harris 形式作为 SteepBlackman 的近似实现。
	// 若后续要求与内置完全 Sub=0，需要单独黑盒确认 SystemVue 的精确系数。
	const double a0 = 0.35875;
	const double a1 = 0.48829;
	const double a2 = 0.14128;
	const double a3 = 0.01168;

	const double M = static_cast<double>(size - 1);

	for (int n = 0; n < size; ++n)
	{
		const double a = 2.0 * kPI * static_cast<double>(n) / M;

		w[n] =
			a0
			- a1 * std::cos(a)
			+ a2 * std::cos(2.0 * a)
			- a3 * std::cos(3.0 * a);
	}

	return w;
}

std::vector<double> RADAR_MTD_M::generateKaiserWindow(int size, double beta)
{
	std::vector<double> w(size, 1.0);

	if (size <= 1) {
		return w;
	}

	const double denom = modifiedBesselI0(beta);
	const double M = static_cast<double>(size - 1);

	if (denom == 0.0) {
		return w;
	}

	for (int n = 0; n < size; ++n)
	{
		const double ratio = (2.0 * static_cast<double>(n)) / M - 1.0;
		const double value = beta * std::sqrt(std::max(0.0, 1.0 - ratio * ratio));

		w[n] = modifiedBesselI0(value) / denom;
	}

	return w;
}

// 修正零阶贝塞尔函数 I0 的级数近似
double RADAR_MTD_M::modifiedBesselI0(double x)
{
	double sum = 1.0;
	double term = 1.0;

	const double halfX = x / 2.0;

	for (int k = 1; k <= 30; ++k)
	{
		term *= (halfX * halfX) / static_cast<double>(k * k);
		sum += term;

		if (std::abs(term) < 1e-15) {
			break;
		}
	}

	return sum;
}