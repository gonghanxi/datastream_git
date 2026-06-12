#include "RADAR_PulseCompression_M.h"

#include <cmath>
#include <algorithm>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_PulseCompression_M)
{
	SET_MODEL_DESCRIPTION("Pulse Compression for Matrix signals");
	SET_MODEL_CATEGORY("Signal Processing");

	// ============================================================
	// 端口注册
	// ============================================================
	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(reference);
		port.SetName("reference");
		port.SetDescription("The reference for pulse compression");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_INPUT(signal);
		port.SetName("signal");
		port.SetDescription("The input signal");
	}

	{
		SystemVueModelBuilder::DFPort port = ADD_MODEL_OUTPUT(output);
		port.SetName("output");
		port.SetDescription("The pulse compression result");
	}

	// ============================================================
	// 参数注册
	// ============================================================
	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(WindowType, SelectedWindowType);
		enumParam.SetName("WindowType");
		enumParam.AddEnumeration("Rectangle", Rectangle);
		enumParam.AddEnumeration("Bartlett", Bartlett);
		enumParam.AddEnumeration("Hanning", Hanning);
		enumParam.AddEnumeration("Hamming", Hamming);
		enumParam.AddEnumeration("Blackman", Blackman);
		enumParam.AddEnumeration("SteepBlackman", SteepBlackman);
		enumParam.AddEnumeration("Kaiser", Kaiser);
		enumParam.SetDefaultValue("Rectangle");
		enumParam.SetDescription("The type of window function: Rectangle, Bartlett, Hanning, Hamming, Blackman, SteepBlackman, Kaiser");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAMETER(WindowParameter);
		param.SetName("WindowParameter");
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("1.0");
		param.SetDescription("The alpha value for Kaiser window function");
		param.SetHideCondition("WindowType ~= 6");
	}

	return true;
}
#endif

// ============================================================
// 构造函数
// ============================================================
RADAR_PulseCompression_M::RADAR_PulseCompression_M()
	: reference()
	, signal()
	, output()
	, WindowType(Rectangle)
	, WindowParameter(1.0)
{
}

// ============================================================
// Setup
// 矩阵版每次 firing 消耗 / 产生 1 个 Matrix。
// 样本数和 FFTSize 由 Run 阶段读取矩阵尺寸动态确定。
// ============================================================
bool RADAR_PulseCompression_M::Setup()
{
	reference.SetRate(1u);
	signal.SetRate(1u);
	output.SetRate(1u);

	return true;
}

// ============================================================
// 递归 FFT
// 该函数保持普通版 RADAR_PulseCompression 的核心逻辑：
// invert 为非 0 时使用负号角度，并在递归合并时除以 2，最终实现 1/N 缩放。
// ============================================================
void RADAR_PulseCompression_M::fft(SystemVueModelBuilder::Matrix<std::complex<double>>& a,
	int n,
	int invert)
{
	const double PI = acos(-1.0);

	if (n == 1) return;

	int half = n / 2;
	SystemVueModelBuilder::Matrix<std::complex<double> > even(1, half), odd(1, half);

	for (int i = 0; i < half; i++)
	{
		even(i) = a(i * 2);
		odd(i) = a(i * 2 + 1);
	}

	fft(even, half, invert);
	fft(odd, half, invert);

	double angle = 2.0 * PI / n * (invert ? -1.0 : 1.0);
	std::complex<double> w(1.0, 0.0), wn(cos(angle), sin(angle));

	for (int i = 0; i < half; i++)
	{
		a(i) = even(i) + w * odd(i);
		a(i + half) = even(i) - w * odd(i);

		if (invert)
		{
			a(i) /= 2.0;
			a(i + half) /= 2.0;
		}

		w *= wn;
	}
}

// ============================================================
// 阶乘：Kaiser 窗 I0 近似计算使用
// ============================================================
int RADAR_PulseCompression_M::factorial(int n)
{
	int result = 1;
	for (int i = 1; i <= n; ++i)
	{
		result *= i;
	}
	return result;
}

// ============================================================
// 第一类零阶修正贝塞尔函数 I0 的有限项近似，沿用普通版实现。
// ============================================================
double RADAR_PulseCompression_M::I0(int n, double x)
{
	double I0_x = 1.0;
	for (int i = 1; i <= n; ++i)
	{
		I0_x += pow((pow(x / 2.0, i) / factorial(i)), 2.0);
	}
	return I0_x;
}

// ============================================================
// 从 reference 矩阵尺寸推断 FFTSize
// ============================================================
int RADAR_PulseCompression_M::getReferenceFFTSize_(const CxMatrix& ref) const
{
	const int rows = static_cast<int>(ref.NumRows());
	const int cols = static_cast<int>(ref.NumColumns());

	// 常规矩阵打包方式：1×FFTSize 或 N×FFTSize。
	if (cols > 1) return cols;

	// 兼容列向量 reference：FFTSize×1。
	if (rows > 1) return rows;

	return cols;
}

// ============================================================
// 读取 reference 频谱值
// ============================================================
RADAR_PulseCompression_M::Cx
RADAR_PulseCompression_M::getReferenceValue_(const CxMatrix& ref, int row, int k) const
{
	const int rows = static_cast<int>(ref.NumRows());
	const int cols = static_cast<int>(ref.NumColumns());

	if (rows <= 0 || cols <= 0) return Cx(0.0, 0.0);

	// reference 为行向量或多行频谱矩阵。
	if (cols > 1)
	{
		int rr = 0;
		if (rows > 1)
		{
			rr = row;
			if (rr < 0) rr = 0;
			if (rr >= rows) rr = rows - 1;
		}

		if (k < 0 || k >= cols) return Cx(0.0, 0.0);
		return ref(rr, k);
	}

	// reference 为列向量时的兼容处理。
	if (k < 0 || k >= rows) return Cx(0.0, 0.0);
	return ref(k, 0);
}

// ============================================================
// 生成频域窗函数序列
// 矩阵版帮助文档没有 SampleRate / Bandwidth 参数，无法像普通版那样计算 WindowLen。
// 这里取 WindowLen = FFTSize，保持所有频点均参与加窗。
// 矩形窗情况下输出全 1，与普通版 Rectangle 逻辑一致。
// ============================================================
void RADAR_PulseCompression_M::buildWindowSequence_(
	SystemVueModelBuilder::Matrix<std::complex<double> >& WindowSequence,
	int fftSize)
{
	const double PI = acos(-1.0);

	const int FFTSize = std::max(1, fftSize);
	const int WindowLen = FFTSize;
	const int WindowN = std::max(1, WindowLen - 1);

	WindowSequence.Resize(1, FFTSize);

	switch (WindowType)
	{
	case RADAR_PulseCompression_M::Rectangle:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			WindowSequence(i) = 1.0;
		}
		break;
	}
	case RADAR_PulseCompression_M::Bartlett:
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
	case RADAR_PulseCompression_M::Hanning:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			if (i < WindowLen)
			{
				WindowSequence(i) = 0.5 * (1.0 - cos(2.0 * PI * i / WindowN));
			}
			else
			{
				WindowSequence(i) = 0.0;
			}
		}
		break;
	}
	case RADAR_PulseCompression_M::Hamming:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			if (i < WindowLen)
			{
				WindowSequence(i) = 0.54 - 0.46 * cos(2.0 * PI * i / WindowN);
			}
			else
			{
				WindowSequence(i) = 0.0;
			}
		}
		break;
	}
	case RADAR_PulseCompression_M::Blackman:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			if (i < WindowLen / 2)
			{
				WindowSequence(i) = 0.42
					- 0.5 * cos(2.0 * PI * i / WindowN)
					+ 0.08 * cos(4.0 * PI * i / WindowN);
			}
			else if (i >= WindowLen / 2 && i < WindowLen)
			{
				WindowSequence(i) = 0.42
					- 0.5 * cos(2.0 * PI * (WindowLen - i) / WindowN)
					+ 0.08 * cos(4.0 * PI * (WindowLen - i) / WindowN);
			}
			else
			{
				WindowSequence(i) = 0.0;
			}
		}
		break;
	}
	case RADAR_PulseCompression_M::SteepBlackman:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			if (i < WindowLen / 2)
			{
				WindowSequence(i) = 0.35875
					- 0.48829 * cos(2.0 * PI * i / WindowN)
					+ 0.14128 * cos(4.0 * PI * i / WindowN)
					- 0.01168 * cos(6.0 * PI * i / WindowN);
			}
			else if (i >= WindowLen / 2 && i < WindowLen)
			{
				WindowSequence(i) = 0.35875
					- 0.48829 * cos(2.0 * PI * (WindowLen - i) / WindowN)
					+ 0.14128 * cos(4.0 * PI * (WindowLen - i) / WindowN)
					- 0.01168 * cos(6.0 * PI * (WindowLen - i) / WindowN);
			}
			else
			{
				WindowSequence(i) = 0.0;
			}
		}
		break;
	}
	case RADAR_PulseCompression_M::Kaiser:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			if (i < WindowLen)
			{
				const double t = 2.0 * i / WindowN - 1.0;
				const double v = std::max(0.0, 1.0 - t * t);
				WindowSequence(i) = I0(20, WindowParameter * sqrt(v)) / I0(20, WindowParameter);
			}
			else
			{
				WindowSequence(i) = 0.0;
			}
		}
		break;
	}
	default:
	{
		for (int i = 0; i < FFTSize; i++)
		{
			WindowSequence(i) = 1.0;
		}
		break;
	}
	}
}

// ============================================================
// Run
// 普通版核心逻辑的矩阵化版本：
// 1. 从 signal 矩阵每一行取一组待压缩数据；
// 2. 补零到 reference 矩阵给出的 FFTSize；
// 3. 执行普通版相同的 FFT、乘 reference 频谱、加窗、IFFT；
// 4. 按普通版 FullSequence(FFTSize - i) 的索引方式写输出；
// 5. 输出矩阵尺寸与 signal 矩阵保持一致。
// ============================================================
bool RADAR_PulseCompression_M::Run()
{
	CxMatrix refMat = reference[0];
	CxMatrix sigMat = signal[0];

	const int sigRows = static_cast<int>(sigMat.NumRows());
	const int sigCols = static_cast<int>(sigMat.NumColumns());

	if (sigRows <= 0 || sigCols <= 0)
	{
		// 输入信号矩阵为空时无法确定输出矩阵尺寸，直接报错返回。
		// 这里不再构造 CxMatrix(0, 0)，避免 SystemVue 2020 下 Matrix 构造函数重载歧义。
		POST_ERROR("RADAR_PulseCompression_M: signal matrix must not be empty.");
		return false;
	}

	const int Samplenum = sigCols;
	const int FFTSize = getReferenceFFTSize_(refMat);

	if (Samplenum < 1 || FFTSize < Samplenum)
	{
		POST_ERROR("RADAR_PulseCompression_M: reference FFTSize and signal size should meet this condition: FFTSize >= Size >= 1");
		return false;
	}

	if ((FFTSize & (FFTSize - 1)) != 0)
	{
		POST_WARNING("RADAR_PulseCompression_M: Only 2^N FFTSize is supported now. For FFTSize not equals to 2^N, performance may be insufficient.");
	}

	SystemVueModelBuilder::Matrix<std::complex<double> > WindowSequence(1, FFTSize);
	buildWindowSequence_(WindowSequence, FFTSize);

	const int WindowLen = FFTSize;
	const int winShift = WindowLen / 2;

	CxMatrix outMat(sigRows, Samplenum);

	for (int row = 0; row < sigRows; ++row)
	{
		SystemVueModelBuilder::Matrix<std::complex<double> > FullSequence(1, FFTSize);

		// signal 矩阵当前行补零到 FFTSize。
		for (int i = 0; i < FFTSize; i++)
		{
			if (i < Samplenum)
			{
				FullSequence(i) = sigMat(row, i);
			}
			else
			{
				FullSequence(i) = 0.0;
			}
		}

		// 与普通版一致：先做 FFT，然后乘 FFTSize 抵消递归 FFT 中的 1/N 缩放。
		fft(FullSequence, FFTSize, 1);
		FullSequence *= FFTSize;

		// 乘 reference 端口输入的匹配滤波器频谱 H[k]。
		for (int i = 0; i < FFTSize; i++)
		{
			FullSequence(i) *= getReferenceValue_(refMat, row, i);
		}

		// 频域加窗，循环移位规则与普通版保持一致。
		for (int i = 0; i < FFTSize; i++)
		{
			const int wi = (i + winShift < FFTSize) ? (i + winShift) : (i + winShift - FFTSize);
			FullSequence(i) *= WindowSequence(wi);
		}

		// 与普通版一致：执行 IFFT。
		fft(FullSequence, FFTSize, -1);

		// 与普通版一致：按 FFTSize - i 的方式从 IFFT 结果中取回 SampleNum 点。
		for (int i = 0; i < Samplenum; i++)
		{
			const int src = (FFTSize - i < FFTSize) ? (FFTSize - i) : 0;
			outMat(row, i) = FullSequence(src);
		}
	}

	output[0] = outMat;
	return true;
}
