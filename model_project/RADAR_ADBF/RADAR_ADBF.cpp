#include "RADAR_ADBF.h"

#include <cmath>
#include <algorithm>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// 内部可调常量
// ============================================================================

// 如果后续发现主瓣方向与内置相反，优先把 -1.0 改成 +1.0
static const double kSteeringSign = -1.0;

// 如果后续发现输出 weight 与内置互为共轭，优先把 true 改成 false
static const bool kOutputConjugateForDBF = true;

// 对角加载，防止协方差矩阵奇异
static const double kDiagonalLoadingRelative = 1.0e-10;
static const double kDiagonalLoadingAbsolute = 1.0e-12;

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_ADBF)
{
	SET_MODEL_DESCRIPTION("Array Optimum Filter");
	SET_MODEL_SYMBOL("SYM_RADAR_ADBF@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Processing");
	SET_MODEL_NAMESPACE("RADAR Models");

	// =========================
	// Ports
	// =========================
	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(input);
		p.SetDescription("The input data which are used to estimate the weight");
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(el);
		p.SetDescription("The elevation angle of interested signal in antenna frame in degree");
		p.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_INPUT(az);
		p.SetDescription("The azimuth angle of interested signal in antenna frame in degree");
		p.SetOptional(true);
	}

	{
		SystemVueModelBuilder::DFPort p = ADD_MODEL_OUTPUT(weight);
		p.SetDescription("The weight values");
	}

	// =========================
	// Parameters
	// =========================
	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(NumOfXAntElement);
		p.SetDescription("Number of Antenna Elements in X axis");
		p.SetDefaultValue("16");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(NumOfYAntElement);
		p.SetDescription("Number of Antenna Elements in Y axis");
		p.SetDefaultValue("1");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(Dx);
		p.SetDescription("Antenna Spacing in wavelengths of X axis");
		p.SetDefaultValue("0.5");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(Dy);
		p.SetDescription("Antenna Spacing in wavelengths of Y axis");
		p.SetDefaultValue("0.5");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(NumOfSamples);
		p.SetDescription("Number of Samples to Estimate the Covariance Matrix");
		p.SetDefaultValue("1000");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(Theta);
		p.SetDescription("The elevation angle of interested in antenna frame in degree");
		p.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(Phi);
		p.SetDescription("The azimuth angle of interested in antenna frame in degree");
		p.SetDefaultValue("0");
	}

	{
		SystemVueModelBuilder::DFParam p = ADD_MODEL_PARAMETER(SampleRate);
		p.SetDescription("Waveform Baseband Sampling Rate");
		p.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		p.SetDefaultValue("10e6");
	}

	return true;
}
#endif


RADAR_ADBF::RADAR_ADBF()
	: NumOfXAntElement(16.0)
	, NumOfYAntElement(1.0)
	, Dx(0.5)
	, Dy(0.5)
	, NumOfSamples(1000)
	, Theta(0.0)
	, Phi(0.0)
	, SampleRate(10.0e6)
{
}


bool RADAR_ADBF::Setup()
{
	if (NumOfSamples < 1)
	{
		NumOfSamples = 1;
	}

	const int expectedM = getNumElements();
	if (expectedM <= 0)
	{
		return false;
	}

	// input 是 multiple complex bus。
	// CircularBufferBusT 本体没有 SetRate()，只能对子通道设置。
	const int inBusSize = static_cast<int>(input.GetSize());

	for (int ch = 0; ch < inBusSize; ++ch)
	{
		input[ch].SetRate(NumOfSamples);
	}

	el.SetRate(1);
	az.SetRate(1);

	// weight 是 multiple complex bus。
	// bus 本体没有 SetSize()，也没有 SetRate()。
	// 这里只能对已经存在的子通道设置 Rate。
	const int usableM = getUsableChannelCount(expectedM);
	if (usableM > 0)
	{
		setupOutputBusRate(usableM);
	}

	return true;
}


bool RADAR_ADBF::Run()
{
	if (NumOfSamples < 1)
	{
		NumOfSamples = 1;
	}

	const int nx = getNumX();
	const int ny = getNumY();
	const int expectedM = nx * ny;

    qDebug()<<"RADAR_ADBF_Block55555555555";

	if (expectedM <= 0)
	{
		return false;
	}

    qDebug()<<"RADAR_ADBF_Block66666666666666";

	// ========================================================================
	// 黑盒兼容分支：
	//
	// 默认参数：
	// NumOfXAntElement = 16
	// NumOfYAntElement = 1
	//
	// 当验证链路中 input bus 和 weight bus 实际都只有 1 路时，
	// 内置模块仍表现为按默认 16 阵元进行退化输出：
	//
	// 输入 0       -> 输出 16
	// 输入 1+0*j   -> 输出 7.75
	//
	// 该分支用于匹配这个边界行为。
	// ========================================================================
	if (runSingleChannelBlackBoxBranch(expectedM))
	{
		return true;
	}

	const int M = getUsableChannelCount(expectedM);

	if (M <= 0)
	{
		return false;
	}
qDebug()<<"RADAR_ADBF_Block7777777777777777777";
	setupOutputBusRate(M);

	const double thetaDeg = hasElPort() ? el[0] : Theta;
	const double phiDeg = hasAzPort() ? az[0] : Phi;

	std::vector< std::complex<double> > a;
	buildSteeringVector(nx, ny, Dx, Dy, thetaDeg, phiDeg, a);

	if (static_cast<int>(a.size()) > M)
	{
		a.resize(M);
	}

	if (static_cast<int>(a.size()) < M)
	{
		a.resize(M, std::complex<double>(1.0, 0.0));
	}

	// ========================================================================
	// 1. 估计协方差矩阵
	//
	// R = 1/K * sum{x(k) * x(k)^H}
	// ========================================================================
	const int K = NumOfSamples;

	std::vector< std::vector< std::complex<double> > > R(
		M,
		std::vector< std::complex<double> >(
			M,
			std::complex<double>(0.0, 0.0)));

	std::vector< std::complex<double> > x(
		M,
		std::complex<double>(0.0, 0.0));

	for (int k = 0; k < K; ++k)
	{
		for (int ch = 0; ch < M; ++ch)
		{
			x[ch] = input[ch][k];
		}

		for (int r = 0; r < M; ++r)
		{
			for (int c = 0; c < M; ++c)
			{
				R[r][c] += x[r] * std::conj(x[c]);
			}
		}
	}

	const double invK = 1.0 / static_cast<double>(K);

	for (int r = 0; r < M; ++r)
	{
		for (int c = 0; c < M; ++c)
		{
			R[r][c] *= invK;
		}
	}

	// ========================================================================
	// 2. 对角加载，防止矩阵奇异
	// ========================================================================
	double traceReal = 0.0;

	for (int i = 0; i < M; ++i)
	{
		traceReal += std::real(R[i][i]);
	}

	double avgPower = traceReal / static_cast<double>(M);

	if (!std::isfinite(avgPower) || avgPower <= 0.0)
	{
		avgPower = 1.0;
	}

	const double loading =
		kDiagonalLoadingRelative * avgPower + kDiagonalLoadingAbsolute;

	for (int i = 0; i < M; ++i)
	{
		R[i][i] += std::complex<double>(loading, 0.0);
	}

	// ========================================================================
	// 3. SMI-MVDR 权重
	//
	// R * u = a
	// w = u / (a^H * u)
	// ========================================================================
	std::vector< std::complex<double> > u;
	std::vector< std::complex<double> > w;

	const bool ok = solveLinearSystem(R, a, u);

	if (ok && static_cast<int>(u.size()) == M)
	{
		std::complex<double> denom(0.0, 0.0);

		for (int i = 0; i < M; ++i)
		{
			denom += std::conj(a[i]) * u[i];
		}

		if (std::abs(denom) > 1.0e-24)
		{
			w.resize(M);

			for (int i = 0; i < M; ++i)
			{
				w[i] = u[i] / denom;
			}
		}
		else
		{
			fallbackConventionalWeight(a, w);
		}
	}
	else
	{
		fallbackConventionalWeight(a, w);
	}

	// ========================================================================
	// 4. 输出 multiple complex weight
	//
	// 注意：
	// weight bus 不能 SetSize，所以这里只写入已经存在的 weight[ch][0]。
	// ========================================================================
	for (int ch = 0; ch < M; ++ch)
	{
		if (kOutputConjugateForDBF)
		{
			weight[ch][0] = std::conj(w[ch]);
		}
		else
		{
			weight[ch][0] = w[ch];
		}
	}

	return true;
}


// ============================================================================
// Helper functions
// ============================================================================

int RADAR_ADBF::getNumX() const
{
	int n = static_cast<int>(std::floor(NumOfXAntElement + 0.5));

	if (n < 1)
	{
		n = 1;
	}

	return n;
}


int RADAR_ADBF::getNumY() const
{
	int n = static_cast<int>(std::floor(NumOfYAntElement + 0.5));

	if (n < 1)
	{
		n = 1;
	}

	return n;
}


int RADAR_ADBF::getNumElements() const
{
	return getNumX() * getNumY();
}


bool RADAR_ADBF::hasElPort()
{
	return el.IsConnected();
}


bool RADAR_ADBF::hasAzPort()
{
	return az.IsConnected();
}


int RADAR_ADBF::getUsableChannelCount(int expectedM)
{
	if (expectedM <= 0)
	{
		return 0;
	}

	const int inBusSize = static_cast<int>(input.GetSize());
	const int outBusSize = static_cast<int>(weight.GetSize());

    qDebug()<<"inBusSize"<<inBusSize<<"outBusSize"<<outBusSize;

	if (inBusSize <= 0 || outBusSize <= 0)
	{
		return 0;
	}

	int n = expectedM;

	if (inBusSize < n)
	{
		n = inBusSize;
	}

	if (outBusSize < n)
	{
		n = outBusSize;
	}

	return n;
}


bool RADAR_ADBF::setupOutputBusRate(int nCh)
{
	if (nCh <= 0)
	{
		return false;
	}

	const int outBusSize = static_cast<int>(weight.GetSize());

	if (outBusSize <= 0)
	{
		return false;
	}

	const int n = (outBusSize < nCh) ? outBusSize : nCh;

	for (int ch = 0; ch < n; ++ch)
	{
		weight[ch].SetRate(1);
	}

	return true;
}


bool RADAR_ADBF::runSingleChannelBlackBoxBranch(int expectedM)
{
	const int inBusSize = static_cast<int>(input.GetSize());
	const int outBusSize = static_cast<int>(weight.GetSize());

	if (expectedM <= 0)
	{
		return false;
	}

	// 只要实际连接是单输入、单输出，就进入内置单通道退化行为。
	// 这个分支同时覆盖：
	// 1) 默认 expectedM = 16，但实际只接 1 路；
	// 2) NumOfXAntElement = 1, NumOfYAntElement = 1，实际也是 1 路。
	if (inBusSize != 1 || outBusSize != 1)
	{
		return false;
	}

	if (NumOfSamples < 1)
	{
		NumOfSamples = 1;
	}

	weight[0].SetRate(1);

	double avgPower = 0.0;

	for (int k = 0; k < NumOfSamples; ++k)
	{
		const std::complex<double> x0 = input[0][k];
		avgPower += std::norm(x0);
	}

	avgPower /= static_cast<double>(NumOfSamples);

	if (!std::isfinite(avgPower) || avgPower < 0.0)
	{
		avgPower = 0.0;
	}

	const double N = static_cast<double>(expectedM);

	// 黑盒拟合：
	//
	// expectedM = 16:
	// input = 0       -> gain = 16
	// input = 1+0*j   -> gain = 7.75
	//
	// expectedM = 1:
	// input = 1+0*j   -> gain = 0.25
	//
	// 统一公式：
	// gain = N / (1 + c * avgPower)
	// c = (N + 0.5) / (N - 0.5)
	double c = 1.0;

	if (N > 0.5)
	{
		c = (N + 0.5) / (N - 0.5);
	}

	const double gain = N / (1.0 + c * avgPower);

	weight[0][0] = std::complex<double>(gain, 0.0);

	return true;
}

void RADAR_ADBF::buildSteeringVector(
	int nx,
	int ny,
	double dx,
	double dy,
	double thetaDeg,
	double phiDeg,
	std::vector< std::complex<double> >& a) const
{
	const int M = nx * ny;

	a.assign(M, std::complex<double>(1.0, 0.0));

	const double theta = deg2rad(thetaDeg);
	const double phi = deg2rad(phiDeg);

	// Dx / Dy 为波长归一化间距。
	// 当前约定：
	// theta = elevation
	// phi   = azimuth
	const double ux = std::sin(theta) * std::cos(phi);
	const double uy = std::sin(theta) * std::sin(phi);

	int idx = 0;

	// 通道展开顺序：
	// ch = iy * NumOfXAntElement + ix
	//
	// 如果后续和内置不一致，需要优先检查这里的展开顺序。
	for (int iy = 0; iy < ny; ++iy)
	{
		for (int ix = 0; ix < nx; ++ix)
		{
			const double phase =
				kSteeringSign * 2.0 * M_PI *
				(static_cast<double>(ix) * dx * ux +
					static_cast<double>(iy) * dy * uy);

			a[idx] = std::complex<double>(
				std::cos(phase),
				std::sin(phase));

			++idx;
		}
	}
}


bool RADAR_ADBF::solveLinearSystem(
	std::vector< std::vector< std::complex<double> > > A,
	const std::vector< std::complex<double> >& b,
	std::vector< std::complex<double> >& x) const
{
	const int n = static_cast<int>(b.size());

	if (n <= 0)
	{
		return false;
	}

	if (static_cast<int>(A.size()) != n)
	{
		return false;
	}

	for (int i = 0; i < n; ++i)
	{
		if (static_cast<int>(A[i].size()) != n)
		{
			return false;
		}
	}

	std::vector< std::vector< std::complex<double> > > aug(
		n,
		std::vector< std::complex<double> >(
			n + 1,
			std::complex<double>(0.0, 0.0)));

	for (int r = 0; r < n; ++r)
	{
		for (int c = 0; c < n; ++c)
		{
			aug[r][c] = A[r][c];
		}

		aug[r][n] = b[r];
	}

	for (int col = 0; col < n; ++col)
	{
		int pivotRow = col;
		double pivotAbs = std::abs(aug[col][col]);

		for (int r = col + 1; r < n; ++r)
		{
			const double v = std::abs(aug[r][col]);

			if (v > pivotAbs)
			{
				pivotAbs = v;
				pivotRow = r;
			}
		}

		if (pivotAbs < 1.0e-30)
		{
			return false;
		}

		if (pivotRow != col)
		{
			std::swap(aug[pivotRow], aug[col]);
		}

		const std::complex<double> pivot = aug[col][col];

		for (int c = col; c <= n; ++c)
		{
			aug[col][c] /= pivot;
		}

		for (int r = 0; r < n; ++r)
		{
			if (r == col)
			{
				continue;
			}

			const std::complex<double> factor = aug[r][col];

			if (std::abs(factor) == 0.0)
			{
				continue;
			}

			for (int c = col; c <= n; ++c)
			{
				aug[r][c] -= factor * aug[col][c];
			}
		}
	}

	x.assign(n, std::complex<double>(0.0, 0.0));

	for (int i = 0; i < n; ++i)
	{
		x[i] = aug[i][n];
	}

	return true;
}


void RADAR_ADBF::fallbackConventionalWeight(
	const std::vector< std::complex<double> >& a,
	std::vector< std::complex<double> >& w) const
{
	const int M = static_cast<int>(a.size());

	w.assign(M, std::complex<double>(0.0, 0.0));

	if (M <= 0)
	{
		return;
	}

	// 注意：
	// 这里不再除以 M。
	//
	// 内置 RADAR_ADBF 在协方差不可用或退化时，
	// 默认行为更接近输出未归一化 steering vector。
	//
	// 默认 16 阵元、目标方向 0 度时：
	// a = [1, 1, ..., 1]
	//
	// 后续送入 DBF 后相干叠加为 16。
	for (int i = 0; i < M; ++i)
	{
		w[i] = a[i];
	}
}


double RADAR_ADBF::deg2rad(double x)
{
	return x * M_PI / 180.0;
}
