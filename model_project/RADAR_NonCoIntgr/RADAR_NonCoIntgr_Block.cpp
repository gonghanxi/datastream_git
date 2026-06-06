#include "RADAR_NonCoIntgr_Block.h"

#include <cmath>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_NonCoIntgr_Block::RADAR_NonCoIntgr_Block(const std::string& name)
    : Block(name)
    , m_PRI_Or_WaveGate(10e-3)
    , m_Number(5)
    , m_SampleRate(10e6)
    , m_samplesPerPulse(0)
    , m_inputRate(0)
    , m_outputRate(0)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_NonCoIntgr_Block::SetDefaultParameters()
{
    m_PRI_Or_WaveGate = 10e-3;
    m_Number          = 5;
    m_SampleRate      = 10e6;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_NonCoIntgr_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->PRI_Or_WaveGate = m_PRI_Or_WaveGate;
    m_algo->Number          = m_Number;
    m_algo->SampleRate      = m_SampleRate;
}

// ============================================================================
// ComputeRates — 派生速率（移植自 RADAR_NonCoIntgr::Setup）
// ============================================================================

void RADAR_NonCoIntgr_Block::ComputeRates()
{
    if (m_PRI_Or_WaveGate <= 0.0 || m_SampleRate <= 0.0 || m_Number <= 0) {
        m_samplesPerPulse = 0;
        m_inputRate       = 0;
        m_outputRate      = 0;
        return;
    }

    m_samplesPerPulse = static_cast<int>(std::round(m_PRI_Or_WaveGate * m_SampleRate));
    if (m_samplesPerPulse <= 0) {
        m_inputRate  = 0;
        m_outputRate = 0;
        return;
    }

    m_outputRate = m_samplesPerPulse;
    m_inputRate  = m_samplesPerPulse * m_Number;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_NonCoIntgr_Block::Setup()
{
    Block::Setup();

    if (m_inputRate <= 0 || m_outputRate <= 0) return false;

    m_inputBuffer.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    SetParameters();
    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_NonCoIntgr_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 数据流模式
// ============================================================================

bool RADAR_NonCoIntgr_Block::DataStreamRun()
{
    auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));

    if (inputData.empty()) return true;

    const int inSize = static_cast<int>(inputData.size());

    if (inSize < m_inputRate) return false;

    std::vector<double> outputData(m_outputRate);

    for (int sample = 0; sample < m_samplesPerPulse; ++sample) {
        double sumAbs = 0.0;
        for (int pulse = 0; pulse < m_Number; ++pulse) {
            sumAbs += std::abs(inputData[pulse * m_samplesPerPulse + sample]);
        }
        outputData[sample] = sumAbs;
    }

    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// ============================================================================

bool RADAR_NonCoIntgr_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
        if (inputData.empty()) return true;
        for (auto& s : inputData)
            m_inputBuffer.push_back(s);
    }

    // ② 累积够一帧输入后处理
    while (static_cast<int>(m_inputBuffer.size()) >= m_inputRate && m_inputRate > 0) {
        std::vector<double> outputData(m_outputRate);

        for (int sample = 0; sample < m_samplesPerPulse; ++sample) {
            double sumAbs = 0.0;
            for (int pulse = 0; pulse < m_Number; ++pulse) {
                sumAbs += std::abs(m_inputBuffer[pulse * m_samplesPerPulse + sample]);
            }
            outputData[sample] = sumAbs;
        }

        for (auto& s : outputData)
            m_outputQueue.push(s);

        m_inputBuffer.erase(m_inputBuffer.begin(), m_inputBuffer.begin() + m_inputRate);
    }

    // ③ 出队写入（输出速率 = m_outputRate 个点）
    while (static_cast<int>(m_outputQueue.size()) >= m_outputRate && m_outputRate > 0) {
        std::vector<double> outVec(m_outputRate);
        for (int i = 0; i < m_outputRate; ++i) {
            outVec[i] = m_outputQueue.front();
            m_outputQueue.pop();
        }
        WriteOutputData(GetOutputPortName(0), outVec);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_NonCoIntgr_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_NonCoIntgr>();

    SetDefaultParameters();

    try { m_PRI_Or_WaveGate = std::stod(getParameter("PRI_Or_WaveGate").Value); } catch (...) {}
    try { m_Number          = std::stoi(getParameter("Number").Value);          } catch (...) {}
    try { m_SampleRate      = std::stod(getParameter("SampleRate").Value);      } catch (...) {}

    ComputeRates();

    SetParameters();

    AddInputPort("input",  m_algo->input,  m_inputRate,  Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    double* tempOutput = nullptr;
    AddOutputPort<CircularBuffer<double>, double>("output", m_algo->output,
        m_outputRate, Block::DataType::CIRCULAR_BUFFER_DOUBLE, tempOutput, m_outputRate);

    return true;
}
