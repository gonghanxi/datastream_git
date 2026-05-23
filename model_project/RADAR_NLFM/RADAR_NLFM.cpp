#include "RADAR_NLFM.h"
#include <cmath>

#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_NLFM)
{
	SET_MODEL_DESCRIPTION("Non-Linear Frequency Modulation Waveform Generator");
	SET_MODEL_SYMBOL("SYM_RADAR_NLFM@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Source");

	ADD_MODEL_OUTPUT(output);
	//ADD_MODEL_OUTPUT(testVAR);

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Pulsewidth);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-5");
		param.SetDescription("Pulse Width");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(PRI);
		param.SetUnit(SystemVueModelBuilder::Units::TIME);
		param.SetDefaultValue("1e-4");
		param.SetDescription("Pulse Repeat Interval");
	}
	
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Bandwidth);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("5e6");
		param.SetDescription("Waveform Bandwidth");
	}

	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(SampleRate);
		param.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
		param.SetDefaultValue("10e6");
		param.SetDescription("Waveform Baseband Sampling Rate");
	}

	{
		SystemVueModelBuilder::DFParam enumParam = ADD_MODEL_ENUM_PARAM(NLF_Type, NLF_Types);
		enumParam.SetUnit(SystemVueModelBuilder::Units::NONE);
		enumParam.AddEnumeration("Hamming", Hamming);
		enumParam.AddEnumeration("Cos4", Cos4);
		enumParam.AddEnumeration("Gauss", Gauss);
		enumParam.AddEnumeration("Polynomial", Polynomial);
		enumParam.SetDefaultValue("Hamming");
		enumParam.SetDescription("Nonlinear Function Type: Hamming, Cos4, Gauss, Polynomial");
	}
	
	{
		SystemVueModelBuilder::DFParam param = ADD_MODEL_PARAM(Polynomial_Coef);
		param.SetUnit(SystemVueModelBuilder::Units::NONE);
		param.SetDefaultValue("[0.426]");
		param.SetHideCondition("NLF_Type ~= 3");
		param.SetDescription("Polynomial Coeficient to generate nonlinear frequency modulation");
	}
	return true;
}
#endif

RADAR_NLFM::RADAR_NLFM()
{
	counter = 0;
}

//-----------------------------------------------------------------------------------
//	Setup
//		Declare the sample rate (if set) to SystemVue. If not set, the model will use the sample rate computed by SystemVue.
//-----------------------------------------------------------------------------------
bool RADAR_NLFM::Setup()
{
	bool bStatus = true;
	if (SampleRate > 0)
	{
		// Use TimedCircularBuffer::SetSampleRate method to set the output sample rate
		// output.SetSampleRate(SampleRate);
	}
	else
	{
		POST_ERROR("SampleRate must be greater than 0.");
		bStatus = false;
	}
	return bStatus;
}

bool RADAR_NLFM::Initialize()
{
	// 参数设置
	double T = Pulsewidth;      // 脉冲宽度：秒
	double B = Bandwidth;       // 带宽：Hz
	double Fs = SampleRate;     // 采样率：Hz
	int N_samples = static_cast<int>(T * Fs); // 脉内总采样点数
	const double PI = 3.14159265358979323846;

	// 生成时间轴 [-T/2, T/2]
	std::vector<double> t_axis = linspace(-T / 2, T / 2, N_samples);

	// 生成频率轴 [-B/2, B/2]
	std::vector<double> f_axis = linspace(-B / 2, B / 2, N_samples);

	// 生成 对应窗 作为功率谱
	std::vector<double> S_f = generateWindow(N_samples, NLF_Type);
	double sum_S_f = std::accumulate(S_f.begin(), S_f.end(), 0.0);
	for (double& val : S_f) {
		val /= sum_S_f; // 归一化功率
	}

	// 计算群时延函数 τ(f)
	double df = B / (N_samples - 1); // 频率间隔
	std::vector<double> cum_integral = cumsum(S_f);
	for (double& val : cum_integral) {
		val *= df; // 累积积分
	}
	double total_integral = cum_integral.back();
	std::vector<double> tau_f(N_samples);
	for (int i = 0; i < N_samples; ++i) {
		tau_f[i] = -T / 2 + (T / total_integral) * cum_integral[i];
	}

	// 通过插值获取调频函数 f(t)s
	std::vector<double> f_interp = interp1(tau_f, f_axis, t_axis, "pchip");

	// 计算相位函数 φ(t) = 2π∫f(t)dt
	std::vector<double> phi = cumtrapz(t_axis, f_interp);
	for (double& val : phi) {
		val *= 2.0 * PI;
	}

	//// 生成 NLFM 信号
	signal.resize(N_samples);  // 配置信号缓存的大小
	for (int i = 0; i < N_samples; ++i) {
		signal[i] = std::complex<double>(std::cos(phi[i]), -1.0 * std::sin(phi[i]));
	}
	
	return true;
}
// 频率轴生成函数
std::vector<double> RADAR_NLFM::linspace(double d1, double d2, int n)
{
	std::vector<double> y(n);

	int n1 = n - 1;

	// 处理特殊情况：d1 == -d2 且 n > 2
	if (d1 == -d2 && n > 2) {
		for (int i = 0; i < n; ++i) {
			y[i] = (-n1 + 2 * i) * (d2 / n1);
		}
		y[0] = d1;
		y[n - 1] = d2;
		if (n1 % 2 == 0) { // 奇数情况
			y[n1 / 2] = 0;
		}
		return y;
	}

	// 一般情况
	double step = (d2 - d1) / n1;
	for (int i = 0; i < n; ++i) {
		y[i] = d1 + i * step;
	}

	return y;
}

// 窗函数（Hamming, Cos4, Gauss, Polynomial）
std::vector<double> RADAR_NLFM::generateWindow(int N, NLF_Types windowType) {
	const double PI = 3.14159265358979323846;
	std::vector<double> w(N);
	if (N == 1) {
		w[0] = 1.0; // 当 N=1 时，窗函数值为 1
		return w;
	}

	switch (windowType) {
	case Hamming: {   // （调试OK）
		double factor = 2 * PI / (N - 1);
		for (int n = 0; n < N; ++n) {
			w[n] = 0.54 - 0.46 * std::cos(factor * n);
		}
		break;
	}

	case Cos4: {
		const double factor2 = 2.0 * PI / (N - 1); // 2π/(N-1)
		const double factor4 = 2.0 * factor2;        // 4π/(N-1)
		for (int n = 0; n < N; ++n) {
			double cos2 = std::cos(factor2 * n);
			double cos4 = std::cos(factor4 * n);
			w[n] = 3.0 / 8.0 - 0.5 * cos2 + 1.0 / 8.0 * cos4;
		}
		break;
	}

	case Gauss: {
		const double sigma = 0.4; // 归一化标准差（相对于半窗宽）
		const double center = (N - 1) * 0.5;
		const double inv_half_width = 1.0 / center; // = 2/(N-1)

		for (int n = 0; n < N; ++n) {
			double x_norm = (n - center) * inv_half_width; // 归一化到 [-1, 1]
			double arg = x_norm / sigma;
			w[n] = std::exp(-0.5 * arg * arg);
		}
		break;
	}

	case Polynomial: {
		size_t numCoeffs = Polynomial_Coef.NumElements();

		for (int n = 0; n < N; ++n) {
			double x = static_cast<double>(n) / (N - 1);

			// Horner's method for polynomial evaluation
			double result = Polynomial_Coef(numCoeffs - 1);
			for (size_t k = numCoeffs - 1; k > 0; --k) {
				result = result * x + Polynomial_Coef(k - 1);
			}
			w[n] = result;
		}
		break;
	}

	default:
		throw std::invalid_argument("Invalid window type.");
	}

	return w;
}

// 群时延函数中的累加函数
std::vector<double> RADAR_NLFM::cumsum(const std::vector<double>& input) {
	std::vector<double> result(input.size());
	double sum = 0.0;

	for (size_t i = 0; i < input.size(); ++i) {
		sum += input[i]; // 累积和
		result[i] = sum;
	}

	return result;
}

// 调频函数
std::vector<double> RADAR_NLFM::interp1(const std::vector<double>& tau_f, const std::vector<double>& f_axis, const std::vector<double>& t_axis, const std::string& method) {
	// 1. 方法检查：仅支持pchip
	if (method != "pchip") {
		throw std::invalid_argument("Only 'pchip' method is supported.");
	}

	// 2. 输入有效性检查
	const size_t n = tau_f.size();
	if (n != f_axis.size()) {
		throw std::invalid_argument("tau_f and f_axis must have the same size.");
	}
	if (n < 2 || t_axis.empty()) {
		throw std::invalid_argument("tau_f/f_axis must have at least 2 points, and t_axis must not be empty.");
	}

	// 3. 计算区间差商d_i（相邻点的差商）
	std::vector<double> d;
	d.reserve(n - 1);
	for (size_t i = 0; i < n - 1; ++i) {
	    double h = tau_f[i + 1] - tau_f[i];
		if (h == 0) {
			//throw std::invalid_argument("tau_f must be strictly increasing (no duplicate points).");
			h = tau_f[i] - tau_f[i - 1];
		}
		d.push_back((f_axis[i + 1] - f_axis[i]) / h);
	}

	// 4. 计算PCHIP导数值m_i（每个点的导数）
	std::vector<double> m(n);
	m[0] = d[0];                                  // 第一个点的导数：取右侧差商
	m[n - 1] = d.back();                           // 最后一个点的导数：取左侧差商
	for (size_t i = 1; i < n - 1; ++i) {           // 中间点的导数：保证单调性
		const double d_prev = d[i - 1];             // 左侧区间差商
		const double d_curr = d[i];                 // 右侧区间差商
		if (d_prev * d_curr > 0) {                  // 同号：调和平均
			m[i] = 2 * d_prev * d_curr / (d_prev + d_curr);
		}
		else {                                    // 异号或有一个为0：取0（避免震荡）
			m[i] = 0;
		}
	}

	// 5. 对每个t_axis点进行插值
	std::vector<double> yi(t_axis.size());
	for (size_t i = 0; i < t_axis.size(); ++i) {
		const double xq = t_axis[i];

		size_t left1, right1;
		if (xq < tau_f[0]) {
			// 左外推：用第一个区间[tau_f[0], tau_f[1]]的多项式
			left1 = 0;
			right1 = 1;
		}
		else if (xq > tau_f.back()) {
			// 右外推：用最后一个区间[tau_f[n-2], tau_f[n-1]]的多项式
			left1 = n - 2;
			right1 = n - 1;
		}
		else {
			// 内插：找到xq所在区间
			const auto it = std::lower_bound(tau_f.begin(), tau_f.end(), xq);
			right1 = std::distance(tau_f.begin(), it);
			left1 = right1 - 1;
		}

		// 5.2 找到xq所在的区间（[left, right]）
		const auto it = std::lower_bound(tau_f.begin(), tau_f.end(), xq);
		const size_t right = std::distance(tau_f.begin(), it);
		const size_t left = right - 1;

		// 5.3 区间参数
		const double x0 = tau_f[left];
		const double x1 = tau_f[right];
		const double y0 = f_axis[left];
		const double y1 = f_axis[right];
		const double m0 = m[left];                  // 左侧点导数（正确的m_i）
		const double m1 = m[right];                 // 右侧点导数（正确的m_i）
		const double h = x1 - x0;
		const double t = (xq - x0) / h;             // 归一化参数（t∈[0,1]）

		// 5.4 Hermite基函数
		const double h00 = 2 * t * t * t - 3 * t * t + 1;  // y0的基函数
		const double h10 = t * t * t - 2 * t * t + t;      // m0的基函数
		const double h01 = -2 * t * t * t + 3 * t * t;     // y1的基函数
		const double h11 = t * t * t - t * t;              // m1的基函数

		// 5.5 计算PCHIP插值值
		yi[i] = h00 * y0 + h01 * y1 + h10 * m0 * h + h11 * m1 * h;
	}

	return yi;
}

// 相位函数
std::vector<double> RADAR_NLFM::cumtrapz(const std::vector<double>& x, const std::vector<double>& y) {
	// 检查y非空
	if (y.empty()) return {};
	// 检查x是标量或长度与y一致
	if (x.size() != 1 && x.size() != y.size()) {
		throw std::invalid_argument("x must be scalar or have same length as y.");
	}

	std::vector<double> z(y.size(), 0.0);
	if (x.size() == 1) { // x是标量（步长）
		double step = x[0];
		for (size_t i = 1; i < y.size(); ++i) {
			z[i] = z[i - 1] + step * (y[i] + y[i - 1]) / 2.0;
		}
	}
	else { // x是向量
		for (size_t i = 1; i < y.size(); ++i) {
			double dx = x[i] - x[i - 1];
			z[i] = z[i - 1] + dx * (y[i] + y[i - 1]) / 2.0;
		}
	}
	return z;
}

//-----------------------------------------------------------------------------------
//	Run
//		Here we do the math
//-----------------------------------------------------------------------------------
bool RADAR_NLFM::Run()
{
	bool bStatus = true;
	
	int PRI_PointNum = static_cast<int>(PRI * SampleRate);
	int PW_PointNum = static_cast<int>(Pulsewidth * SampleRate);
	int currentPt = counter % PRI_PointNum;

	if (currentPt < PW_PointNum) {
		output[0].real(signal[currentPt].real());
		output[0].imag(signal[currentPt].imag());
	}
	else {
		output[0].real(0.0);
		output[0].imag(0.0);
	}

	counter++;

	return bStatus;
}