#include "RADAR_MTD.h"
#include <cmath>
#include <fftw3.h>  // FFTW库头文件，用于快速傅里叶变换

// 如果未定义M_PI，则定义圆周率常量
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 如果不是在SystemVue代码生成模式下，定义模型接口
#ifndef SV_CODE_GEN
DEFINE_MODEL_INTERFACE(RADAR_MTD)
{
	SET_MODEL_DESCRIPTION("Moving Target Detection");
	SET_MODEL_SYMBOL("SYM_RADAR_MTD@RADAR Symbols");
	SET_MODEL_CATEGORY("Signal Processing");

	// 添加输入端口
	ADD_MODEL_INPUT(input);          // 添加模型输入
	ADD_MODEL_OUTPUT(output);        // 添加模型输出

	// 添加脉冲重复间隔参数，设置单位为时间，默认值为0.1ms
	SystemVueModelBuilder::DFParam P2 = ADD_MODEL_PARAMETER(PRI);
	P2.SetUnit(SystemVueModelBuilder::Units::TIME);
	P2.SetDefaultValue("1e-4");

	// 添加脉冲数量参数，默认值为8个脉冲
	SystemVueModelBuilder::DFParam P3 = ADD_MODEL_PARAMETER(NumOfPulse);
	P3.SetDefaultValue("8");

	// 添加频率权重数组参数，数组长度等于脉冲数量，默认值为全1数组
	SystemVueModelBuilder::DFParam P1 = ADD_MODEL_ARRAY_PARAM(Freq_Weight, NumOfPulse);
	P1.SetDefaultValue("[1,1,1,1,1,1,1,1]");

	// 添加窗函数类型枚举参数，支持多种窗函数类型
	SystemVueModelBuilder::DFParam P4 = ADD_MODEL_ENUM_PARAMETER(WindowType, SelectedWindowType);
	P4.AddEnumeration("Rectangle", Rectangle);        // 矩形窗
	P4.AddEnumeration("Bartlett", Bartlett);          // 三角窗
	P4.AddEnumeration("Hanning", Hanning);            // 汉宁窗
	P4.AddEnumeration("Hamming", Hamming);            // 汉明窗
	P4.AddEnumeration("Blackman", Blackman);          // 布莱克曼窗
	P4.AddEnumeration("SteepBlackman", SteepBlackman);// 陡峭布莱克曼窗
	P4.AddEnumeration("Kaiser", Kaiser);              // 凯泽窗
	P4.SetDefaultValue(0);                  // 默认使用矩形窗

	// 添加窗函数参数数组，用于传递窗函数特定参数（如凯泽窗的beta参数）
	SystemVueModelBuilder::DFParam P5 = ADD_MODEL_ARRAY_PARAM(WindowParameters, WindowParameters_Size);
	P5.SetDefaultValue("0");

	// 添加采样率参数，设置单位为频率，默认值为10MHz
	SystemVueModelBuilder::DFParam P6 = ADD_MODEL_PARAMETER(SampleRate);
	P6.SetUnit(SystemVueModelBuilder::Units::FREQUENCY);
	P6.SetDefaultValue("10e6");

	return true;
}
#endif

// 构造函数：初始化所有成员变量
RADAR_MTD::RADAR_MTD()
	: samplesPerPulse(0), totalSamples(0),
	Freq_Weight(nullptr), Freq_Weight_Size(0),
	WindowParameters(nullptr), WindowParameters_Size(0),
	fft_plan(nullptr), ifft_plan(nullptr),
	fftw_input(nullptr), fftw_output(nullptr),
	fftw_initialized(false)
{
}

// 析构函数：清理FFTW资源，防止内存泄漏
RADAR_MTD::~RADAR_MTD()
{
	// 清理FFTW资源
	if (fft_plan != nullptr) {
		fftw_destroy_plan(fft_plan);  // 销毁FFT计划
		fft_plan = nullptr;
	}
	if (ifft_plan != nullptr) {
		fftw_destroy_plan(ifft_plan); // 销毁IFFT计划
		ifft_plan = nullptr;
	}
	if (fftw_input != nullptr) {
		fftw_free(fftw_input);        // 释放FFTW输入数组内存
		fftw_input = nullptr;
	}
	if (fftw_output != nullptr) {
		fftw_free(fftw_output);       // 释放FFTW输出数组内存
		fftw_output = nullptr;
	}
}

// Setup函数：根据参数初始化模型，计算采样点数，设置采样率，分配缓冲区
bool RADAR_MTD::Setup()
{
	// 计算每个脉冲的采样点数和总采样点数
	// 采样点数 = 脉冲重复间隔 × 采样率（四舍五入取整）
	samplesPerPulse = static_cast<int>(std::round(PRI * SampleRate));
	totalSamples = NumOfPulse * samplesPerPulse;  // 总采样点数 = 脉冲数 × 每脉冲采样点数

	// 设置输入输出采样率，确保与模型采样率一致
	input.SetRate(totalSamples);
	output.SetRate(totalSamples);

	// 调整缓冲区大小以容纳所有采样点
	inputBuffer.resize(totalSamples);        // 输入缓冲区
	outputBuffer.resize(totalSamples);       // 输出缓冲区
	pulseMatrix.resize(samplesPerPulse * NumOfPulse);  // 脉冲矩阵，用于存储脉冲数据
	cancelledData.resize(samplesPerPulse * NumOfPulse); // MTI滤波后数据

	// 初始化FFTW，准备进行FFT变换
	fftw_initialized = InitializeFFTW();

	return true;  // 初始化成功
}

// InitializeFFTW函数：初始化FFTW库，分配内存并创建FFT计划
bool RADAR_MTD::InitializeFFTW()
{
	// 清理现有的FFT计划，防止重复创建
	if (fft_plan != nullptr) {
		fftw_destroy_plan(fft_plan);
		fft_plan = nullptr;
	}
	if (ifft_plan != nullptr) {
		fftw_destroy_plan(ifft_plan);
		ifft_plan = nullptr;
	}

	// 释放现有的FFTW数组
	if (fftw_input != nullptr) {
		fftw_free(fftw_input);
		fftw_input = nullptr;
	}
	if (fftw_output != nullptr) {
		fftw_free(fftw_output);
		fftw_output = nullptr;
	}

	// 分配FFTW输入输出数组，使用复数格式
	int fft_size = NumOfPulse;  // FFT大小为脉冲数量
	fftw_input = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fft_size);
	fftw_output = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fft_size);

	// 检查内存分配是否成功
	if (fftw_input == nullptr || fftw_output == nullptr) {
		// 内存分配失败
		return false;
	}

	// 创建FFT和IFFT计划
	// FFTW_FORWARD：正向FFT，FFTW_BACKWARD：反向FFT
	// FFTW_ESTIMATE：让FFTW自动选择最佳算法
	fft_plan = fftw_plan_dft_1d(fft_size, fftw_input, fftw_output, FFTW_FORWARD, FFTW_ESTIMATE);
	ifft_plan = fftw_plan_dft_1d(fft_size, fftw_output, fftw_input, FFTW_BACKWARD, FFTW_ESTIMATE);

	// 检查计划创建是否成功
	return (fft_plan != nullptr && ifft_plan != nullptr);
}

// Run函数：主处理函数，执行MTD算法
bool RADAR_MTD::Run()
{
	// 步骤1：读取输入数据到输入缓冲区
	for (int i = 0; i < totalSamples; ++i) {
		inputBuffer[i] = input[i];  // 从SystemVue输入端口读取数据
	}

	// 步骤2：重塑数据为脉冲矩阵格式
	// 输入数据是一维数组，需要转换为二维矩阵格式：[距离门数 × 脉冲数]
	// 每列代表一个脉冲的所有距离门数据（MATLAB列优先顺序）
	for (int pulse = 0; pulse < NumOfPulse; ++pulse) {
		for (int sample = 0; sample < samplesPerPulse; ++sample) {
			int input_idx = pulse * samplesPerPulse + sample;        // 输入数据索引（行优先）
			int matrix_idx = sample * NumOfPulse + pulse;            // 矩阵索引（列优先，兼容MATLAB）
			pulseMatrix[matrix_idx] = inputBuffer[input_idx];       // 存储到脉冲矩阵
		}
	}

	// 步骤3：三脉冲对消器（MTI滤波器），用于抑制静态杂波
	if (NumOfPulse >= 3) {
		// 初始化前两个脉冲的处理结果为0，因为三脉冲对消器需要前两个脉冲作为参考
		for (int sample = 0; sample < samplesPerPulse; ++sample) {
			// 第一个脉冲：设置为零
			int idx1 = sample * NumOfPulse + 0;
			cancelledData[idx1] = std::complex<double>(0, 0);

			// 第二个脉冲：设置为零
			int idx2 = sample * NumOfPulse + 1;
			cancelledData[idx2] = std::complex<double>(0, 0);
		}

		// 应用三脉冲对消器公式：y[n] = x[n] - 2*x[n-1] + x[n-2] (n >= 3)
		// 该滤波器是一个二阶FIR滤波器，用于抑制静态目标
		for (int pulse = 2; pulse < NumOfPulse; ++pulse) {
			for (int sample = 0; sample < samplesPerPulse; ++sample) {
				int idx_n = sample * NumOfPulse + pulse;      // 当前脉冲索引
				int idx_n1 = sample * NumOfPulse + (pulse - 1); // 前一个脉冲索引
				int idx_n2 = sample * NumOfPulse + (pulse - 2); // 前两个脉冲索引

				// 应用三脉冲对消公式
				cancelledData[idx_n] = pulseMatrix[idx_n]
					- 2.0 * pulseMatrix[idx_n1]
					+ pulseMatrix[idx_n2];
			}
		}
	}
	else {
		// 脉冲数量不足3个，无法应用三脉冲对消，直接复制原始数据
		cancelledData = pulseMatrix;
	}

	// 步骤4：对每个距离门进行FFT处理（多普勒处理）
	// 在雷达MTD中，对同一距离门的多个脉冲进行FFT，以提取多普勒频率
	for (int range_gate = 0; range_gate < samplesPerPulse; ++range_gate) {
		// 提取该距离门在所有脉冲上的数据（一个距离门对应一个多普勒谱）
		std::vector<std::complex<double>> range_gate_data(NumOfPulse);
		for (int pulse = 0; pulse < NumOfPulse; ++pulse) {
			int idx = range_gate * NumOfPulse + pulse;
			range_gate_data[pulse] = cancelledData[idx];  // 获取该距离门的所有脉冲数据
		}

		// 尝试使用外部FFT库（FFTW）进行变换，如果失败则使用备用FFT
		if (fftw_initialized && !ExternalFFT(range_gate_data)) {
			// 如果外部FFT失败，回退到内置FFT（递归FFT实现）
			BackupFFT(range_gate_data);
		}
		else if (!fftw_initialized) {
			// 如果FFTW未初始化，使用备用FFT
			BackupFFT(range_gate_data);
		}

		// 将FFT结果存储到输出缓冲区，恢复为行优先格式
		for (int pulse = 0; pulse < NumOfPulse; ++pulse) {
			int output_idx = pulse * samplesPerPulse + range_gate;
			outputBuffer[output_idx] = range_gate_data[pulse];
		}
	}

	// 步骤5：将处理结果写入输出端口
	for (int i = 0; i < totalSamples; ++i) {
		output[i] = outputBuffer[i];  // 写入SystemVue输出端口
	}

	return true;  // 处理成功
}

// ExternalFFT函数：使用FFTW库执行FFT变换
bool RADAR_MTD::ExternalFFT(std::vector<std::complex<double>>& x)
{
	// 检查FFTW资源是否已正确初始化
	if (fft_plan == nullptr || fftw_input == nullptr || fftw_output == nullptr) {
		return false;
	}

	const int N = x.size();  // 获取输入数据长度
	if (N <= 0) return false;  // 空数据检查

	// 将数据复制到FFTW输入数组（FFTW使用分离的实部和虚部数组）
	for (int i = 0; i < N; ++i) {
		fftw_input[i][0] = x[i].real();  // 实部
		fftw_input[i][1] = x[i].imag();  // 虚部
	}

	// 执行FFT变换（快速傅里叶变换）
	fftw_execute(fft_plan);

	// 将结果复制回输出向量，将FFTW格式转换为复数格式
	for (int i = 0; i < N; ++i) {
		x[i] = std::complex<double>(fftw_output[i][0], fftw_output[i][1]);
	}

	return true;  // FFT执行成功
}

// ExternalIFFT函数：使用FFTW库执行逆FFT变换
bool RADAR_MTD::ExternalIFFT(std::vector<std::complex<double>>& x)
{
	// 检查IFFT资源是否已正确初始化
	if (ifft_plan == nullptr || fftw_input == nullptr || fftw_output == nullptr) {
		return false;
	}

	const int N = x.size();  // 获取输入数据长度
	if (N <= 0) return false;  // 空数据检查

	// 将数据复制到FFTW输出数组（作为IFFT的输入）
	for (int i = 0; i < N; ++i) {
		fftw_output[i][0] = x[i].real();  // 实部
		fftw_output[i][1] = x[i].imag();  // 虚部
	}

	// 执行IFFT变换（逆快速傅里叶变换）
	fftw_execute(ifft_plan);

	// 将结果复制回输出向量并缩放（IFFT需要除以N进行归一化）
	for (int i = 0; i < N; ++i) {
		x[i] = std::complex<double>(fftw_input[i][0] / N, fftw_input[i][1] / N);
	}

	return true;  // IFFT执行成功
}

// BackupFFT函数：备用FFT实现（递归Cooley-Tukey算法）
// 当FFTW库不可用时，使用此函数进行FFT计算
void RADAR_MTD::BackupFFT(std::vector<std::complex<double>>& x)
{
	const size_t N = x.size();  // 获取输入数据长度
	if (N <= 1) return;  // 递归终止条件：单个点不需要FFT

	// 分割为偶数和奇数部分（基2-FFT算法）
	std::vector<std::complex<double>> even(N / 2);  // 偶数索引元素
	std::vector<std::complex<double>> odd(N / 2);   // 奇数索引元素

	// 分离偶数和奇数索引的元素
	for (size_t i = 0; i < N / 2; ++i) {
		even[i] = x[i * 2];      // 偶数索引：x[0], x[2], x[4]...
		odd[i] = x[i * 2 + 1];   // 奇数索引：x[1], x[3], x[5]...
	}

	// 递归FFT：分别计算偶数和奇数部分的FFT
	BackupFFT(even);
	BackupFFT(odd);

	// 合并结果（蝶形运算）
	for (size_t k = 0; k < N / 2; ++k) {
		// 计算旋转因子（twiddle factor）：W_N^k = e^(-j*2πk/N)
		std::complex<double> t = std::polar(1.0, -2.0 * M_PI * k / N) * odd[k];

		// 蝶形运算：前一半和后一半
		x[k] = even[k] + t;            // 前一半
		x[k + N / 2] = even[k] - t;    // 后一半
	}
}

// BackupIFFT函数：备用逆FFT实现
// 通过共轭FFT实现逆变换
void RADAR_MTD::BackupIFFT(std::vector<std::complex<double>>& x)
{
	// 对输入取共轭：IFFT可以通过对输入取共轭，执行FFT，再取共轭并缩放得到
	for (auto& val : x) {
		val = std::conj(val);  // 取复数共轭
	}

	// 正向FFT
	BackupFFT(x);

	// 取共轭并缩放（除以N进行归一化）
	const size_t N = x.size();
	for (auto& val : x) {
		val = std::conj(val) / static_cast<double>(N);
	}
}

// generateWindow函数：根据选择的窗函数类型生成窗函数系数
void RADAR_MTD::generateWindow()
{
	// 从WindowParameters数组中提取窗函数参数（如凯泽窗的beta参数）
	double beta = 0.0;
	if (WindowParameters != nullptr && WindowParameters_Size > 0) {
		beta = WindowParameters[0];
	}

	// 根据窗函数类型调用相应的生成函数
	switch (WindowType) {
	case Bartlett:
		window = generateBartlettWindow(NumOfPulse);  // 三角窗
		break;
	case Hanning:
		window = generateHanningWindow(NumOfPulse);   // 汉宁窗
		break;
	case Hamming:
		window = generateHammingWindow(NumOfPulse);   // 汉明窗
		break;
	case Blackman:
		window = generateBlackmanWindow(NumOfPulse);  // 布莱克曼窗
		break;
	case SteepBlackman:
		window = generateSteepBlackmanWindow(NumOfPulse);  // 陡峭布莱克曼窗
		break;
	case Kaiser:
		window = generateKaiserWindow(NumOfPulse, beta);   // 凯泽窗
		break;
	default: // 矩形窗
		window = std::vector<double>(NumOfPulse, 1.0);     // 全1数组
		break;
	}
}

// generateBartlettWindow函数：生成三角窗（Bartlett窗）
std::vector<double> RADAR_MTD::generateBartlettWindow(int size)
{
	std::vector<double> window(size);  // 创建窗函数数组
	for (int n = 0; n < size; ++n) {
		// Bartlett窗公式：w(n) = 1 - |(2n/(N-1)) - 1|
		window[n] = 1.0 - std::abs(2.0 * n / (size - 1.0) - 1.0);
	}
	return window;
}

// generateHanningWindow函数：生成汉宁窗（Hann窗）
std::vector<double> RADAR_MTD::generateHanningWindow(int size)
{
	std::vector<double> window(size);
	for (int n = 0; n < size; ++n) {
		// Hanning窗公式：w(n) = 0.5 * (1 - cos(2πn/(N-1)))
		window[n] = 0.5 * (1.0 - std::cos(2.0 * M_PI * n / (size - 1.0)));
	}
	return window;
}

// generateHammingWindow函数：生成汉明窗（Hamming窗）
std::vector<double> RADAR_MTD::generateHammingWindow(int size)
{
	std::vector<double> window(size);
	for (int n = 0; n < size; ++n) {
		// Hamming窗公式：w(n) = 0.54 - 0.46 * cos(2πn/(N-1))
		window[n] = 0.54 - 0.46 * std::cos(2.0 * M_PI * n / (size - 1.0));
	}
	return window;
}

// generateBlackmanWindow函数：生成布莱克曼窗
std::vector<double> RADAR_MTD::generateBlackmanWindow(int size)
{
	std::vector<double> window(size);
	for (int n = 0; n < size; ++n) {
		// Blackman窗公式：w(n) = 0.42 - 0.5*cos(2πn/(N-1)) + 0.08*cos(4πn/(N-1))
		window[n] = 0.42 - 0.5 * std::cos(2.0 * M_PI * n / (size - 1.0)) +
			0.08 * std::cos(4.0 * M_PI * n / (size - 1.0));
	}
	return window;
}

// generateSteepBlackmanWindow函数：生成陡峭布莱克曼窗
std::vector<double> RADAR_MTD::generateSteepBlackmanWindow(int size)
{
	std::vector<double> window(size);
	for (int n = 0; n < size; ++n) {
		// 陡峭Blackman窗公式：四阶余弦窗
		window[n] = 0.35875 - 0.48829 * std::cos(2.0 * M_PI * n / (size - 1.0)) +
			0.14128 * std::cos(4.0 * M_PI * n / (size - 1.0)) -
			0.01168 * std::cos(6.0 * M_PI * n / (size - 1.0));
	}
	return window;
}

// generateKaiserWindow函数：生成凯泽窗（Kaiser窗）
std::vector<double> RADAR_MTD::generateKaiserWindow(int size, double beta)
{
	std::vector<double> window(size);
	if (beta <= 0) beta = 5.0;  // 默认beta值

	double alpha = beta;  // Kaiser窗参数
	double I0_alpha = modifiedBesselI0(alpha);  // 计算零阶修正贝塞尔函数在alpha处的值

	// 生成Kaiser窗系数
	for (int n = 0; n < size; ++n) {
		double x = 2.0 * n / (size - 1.0) - 1.0;  // 归一化到[-1, 1]
		double arg = alpha * std::sqrt(1.0 - x * x);  // 计算贝塞尔函数参数
		window[n] = modifiedBesselI0(arg) / I0_alpha;  // Kaiser窗公式
	}
	return window;
}

// modifiedBesselI0函数：计算零阶修正贝塞尔函数
// 使用级数展开法计算I0(x)，用于Kaiser窗
double RADAR_MTD::modifiedBesselI0(double x)
{
	double result = 1.0;   // 级数第一项
	double term = 1.0;     // 当前项
	double x_sq = x * x;   // x的平方，用于级数计算

	// 使用级数展开：I0(x) = Σ (x^2/4)^k / (k!)^2, k从0到∞
	for (int k = 1; k <= 20; ++k) {
		term *= x_sq / (4.0 * k * k);  // 计算下一项
		result += term;                 // 累加到结果
		if (std::abs(term) < 1e-12) break;  // 如果项足够小，提前终止
	}
	return result;
}
