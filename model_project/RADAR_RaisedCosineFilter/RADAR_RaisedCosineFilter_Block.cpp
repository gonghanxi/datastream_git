#include "RADAR_RaisedCosineFilter_Block.h"

#include <algorithm>
#include <cmath>
#include <string>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

// ============================================================================
// sinc function
// ============================================================================

static inline double sinc(double x)
{
	return (std::abs(x) < 1e-12) ? 1.0 : std::sin(M_PI * x) / (M_PI * x);
}

// ============================================================================
// raised cosine FIR filter coefficients
// ============================================================================

static std::vector<double> raisedCosine(double alpha, int numTaps)
{
	std::vector<double> taps(static_cast<size_t>(numTaps), 0.0);

	double T = static_cast<double>(numTaps - 1);

	for (int i = 0; i < numTaps; ++i) {
		double t = static_cast<double>(i - numTaps / 2);
		if (t == 0.0) {
			taps[static_cast<size_t>(i)] = 1.0 - alpha + 4.0 * alpha / M_PI;
		}
		else {
			double denom = 1.0 - std::pow(2.0 * alpha * t / T, 2);
			if (std::abs(denom) < 1e-12) {
				taps[static_cast<size_t>(i)] = alpha / 2.0 * std::sin(M_PI / (2.0 * alpha)) * sinc(1.0 / (2.0 * alpha));
			}
			else {
				taps[static_cast<size_t>(i)] = sinc(t / T) * std::cos(M_PI * alpha * t / T) / denom;
			}
		}
	}
	return taps;
}

// ============================================================================
// convolution: A (lenA) * B (lenB) -> result (lenA + lenB - 1)
// ============================================================================

static std::vector<double> convolve(const std::vector<double>& A, const std::vector<double>& B,
                                     int lenA, int lenB)
{
	std::vector<double> result(static_cast<size_t>(lenA + lenB - 1), 0.0);
	for (int i = 0; i < lenA; ++i) {
		for (int j = 0; j < lenB; ++j) {
			result[static_cast<size_t>(i + j)] += A[static_cast<size_t>(i)] * B[static_cast<size_t>(j)];
		}
	}
	return result;
}

// ============================================================================
// constructor
// ============================================================================

RADAR_RaisedCosineFilter_Block::RADAR_RaisedCosineFilter_Block(const std::string& name)
	: Block(name)
	, m_Alpha(0.5)
	, m_PRI(1e-4)
	, m_FilterLen(24)
	, m_SampleRate(10e6)
	, m_numPRI(1000)
	, m_inputCount(0)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_RaisedCosineFilter_Block::SetDefaultParameters()
{
	m_Alpha     = 0.5;
	m_PRI       = 1e-4;
	m_FilterLen = 24;
	m_SampleRate = 10e6;
}

// ============================================================================
// SetParameters
// ============================================================================

void RADAR_RaisedCosineFilter_Block::SetParameters()
{
	if (!m_algo) return;
	m_algo->Alpha      = m_Alpha;
	m_algo->PRI        = m_PRI;
	m_algo->FilterLen  = m_FilterLen;
	m_algo->SampleRate = m_SampleRate;
}

// ============================================================================
// validateAndPrepare
// ============================================================================

bool RADAR_RaisedCosineFilter_Block::validateAndPrepare()
{
	if (m_Alpha < 0.0 || m_Alpha > 1.0) {
		LOG_ERROR("Alpha must be >= 0 and <= 1");
		return false;
	}
	if (m_PRI * m_SampleRate < 1.0) {
		LOG_ERROR("PRI must be >= 1 / SampleRate");
		return false;
	}
	if (m_FilterLen <= 0) {
		LOG_ERROR("FilterLen must be > 0");
		return false;
	}
	if (m_SampleRate <= 0.0) {
		LOG_ERROR("SampleRate must be > 0");
		return false;
	}

	m_numPRI = static_cast<int>(m_PRI * m_SampleRate);
	if (m_numPRI < 1) m_numPRI = 1;

	return true;
}

// ============================================================================
// Setup — 每轮仿真开始前，清空累积缓冲区与输出队列
// ============================================================================

bool RADAR_RaisedCosineFilter_Block::Setup()
{
	Block::Setup();

	// 清空变步长模式的累积缓冲区
	m_inputBuffer.clear();
	while (!m_outputQueue.empty()) m_outputQueue.pop();
	m_inputCount = 0;

	return true;
}

// ============================================================================
// Run — 根据仿真模式分发：可变步长模式走 TimeDrivenRun，否则走 DataStreamRun
// ============================================================================

bool RADAR_RaisedCosineFilter_Block::Run()
{
	if (IsVariableStepMode()) return TimeDrivenRun();
	return DataStreamRun();
}

// ============================================================================
// Initialize — 创建算法实例、解析参数、校验并注册端口
// ============================================================================

bool RADAR_RaisedCosineFilter_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	// 创建算法实例
	m_algo = std::make_unique<RADAR_RaisedCosineFilter>();

	SetDefaultParameters();

	// 从引擎读取用户设置的参数，支持字符串到数值的转换
	try { m_Alpha     = std::stod(getParameter("Alpha").Value);      } catch (...) { LOG_WARN("Failed to parse parameter 'Alpha', using default value."); }
	try { m_PRI       = std::stod(getParameter("PRI").Value);        } catch (...) { LOG_WARN("Failed to parse parameter 'PRI', using default value."); }
	try { m_FilterLen = std::stoi(getParameter("FilterLen").Value);  } catch (...) { LOG_WARN("Failed to parse parameter 'FilterLen', using default value."); }
	try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }

	// 同步参数到算法实例
	SetParameters();

	// 参数校验并计算派生量：numPRI = PRI * SampleRate
	if (!validateAndPrepare()) {
		return false;
	}

    // 注册端口
    //   input  : 单路 envelope 输入，速率 = numPRI
    //   output : 滤波后 envelope 输出，速率 = numPRI
    //   coeff  : 升余弦滤波器系数（复数），速率 = FilterLen
    AddInputPort("input",  m_algo->input,  static_cast<unsigned>(m_numPRI),    Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_algo->output, static_cast<unsigned>(m_numPRI),    Block::DataType::ENVELOPE_SIGNAL);
	AddOutputPort("coeff",  m_algo->coeff,  static_cast<unsigned>(m_FilterLen), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

	return true;
}

// ============================================================================
// DataStreamRun
// ============================================================================

bool RADAR_RaisedCosineFilter_Block::DataStreamRun()
{
	std::string inputName = GetInputPortName(0);
	auto inputData = ReadInputData<EnvelopeSignal>(inputName);

	if (inputData.empty()) return true;

	const int N = m_numPRI;
	const int L = m_FilterLen;

	if (static_cast<int>(inputData.size()) != N) {
		LOG_ERROR("Unexpected input data size.");
		return false;
	}

	// 分离实部和虚部
	std::vector<double> realPart(static_cast<size_t>(N));
	std::vector<double> imagPart(static_cast<size_t>(N));
	for (int i = 0; i < N; ++i) {
		realPart[static_cast<size_t>(i)] = inputData[static_cast<size_t>(i)].real();
		imagPart[static_cast<size_t>(i)] = inputData[static_cast<size_t>(i)].imag();
	}

	// 生成升余弦滤波器系数
	std::vector<double> filterCoef = raisedCosine(m_Alpha, L);

	// 写入系数输出
	{
		std::vector<Cx> coeffOut(static_cast<size_t>(L));
		for (int i = 0; i < L; ++i) {
			coeffOut[static_cast<size_t>(i)] = Cx(filterCoef[static_cast<size_t>(i)],
			                                       filterCoef[static_cast<size_t>(i)]);
		}
		WriteOutputData(GetOutputPortName(1), coeffOut);
	}

	// 实部和虚部分别卷积
	std::vector<double> convReal = convolve(realPart, filterCoef, N, L);
	std::vector<double> convImag = convolve(imagPart, filterCoef, N, L);

	// 写入 envelope 输出（取卷积结果前 N 个样本）
	{
		std::vector<EnvelopeSignal> outputData(static_cast<size_t>(N));
		for (int i = 0; i < N; ++i) {
			outputData[static_cast<size_t>(i)] = EnvelopeSignal(
				Cx(convReal[static_cast<size_t>(i)], convImag[static_cast<size_t>(i)]));
		}
		WriteOutputData(GetOutputPortName(0), outputData);
	}

	return true;
}

// ============================================================================
// TimeDrivenRun
// ============================================================================

bool RADAR_RaisedCosineFilter_Block::TimeDrivenRun()
{
	// 累积输入
	{
		std::string inputName = GetInputPortName(0);
		auto data = ReadInputData<EnvelopeSignal>(inputName);
		for (auto& sig : data)
			m_inputBuffer.push_back(sig);
	}

	const int N = m_numPRI;
	const int L = m_FilterLen;

	// 累积足够数据后处理
	if (static_cast<int>(m_inputBuffer.size()) >= N)
	{
		// 分离实部和虚部
		std::vector<double> realPart(static_cast<size_t>(N));
		std::vector<double> imagPart(static_cast<size_t>(N));
		for (int i = 0; i < N; ++i) {
			realPart[static_cast<size_t>(i)] = m_inputBuffer[static_cast<size_t>(i)].real();
			imagPart[static_cast<size_t>(i)] = m_inputBuffer[static_cast<size_t>(i)].imag();
		}

		// 生成升余弦滤波器系数
		std::vector<double> filterCoef = raisedCosine(m_Alpha, L);

		// 写入系数输出
		{
			std::vector<Cx> coeffOut(static_cast<size_t>(L));
			for (int i = 0; i < L; ++i) {
				coeffOut[static_cast<size_t>(i)] = Cx(filterCoef[static_cast<size_t>(i)],
				                                       filterCoef[static_cast<size_t>(i)]);
			}
			WriteOutputData(GetOutputPortName(1), coeffOut);
		}

		// 卷积
		std::vector<double> convReal = convolve(realPart, filterCoef, N, L);
		std::vector<double> convImag = convolve(imagPart, filterCoef, N, L);

		// 滤波结果入队
		for (int i = 0; i < N; ++i) {
			m_outputQueue.push(EnvelopeSignal(
				Cx(convReal[static_cast<size_t>(i)], convImag[static_cast<size_t>(i)])));
		}

		// 清空已处理的输入
		m_inputBuffer.erase(m_inputBuffer.begin(), m_inputBuffer.begin() + N);
	}

	// 输出一个 envelope token
	if (!m_outputQueue.empty()) {
		EnvelopeSignal v = m_outputQueue.front(); m_outputQueue.pop();
		WriteOutputData(GetOutputPortName(0), std::vector<EnvelopeSignal>{v});
	}

	return true;
}
