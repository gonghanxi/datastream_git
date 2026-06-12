#include "RADAR_ArrayCouple_Block.h"

#include <complex>

// ============================================================================
// 构造函数
// ============================================================================

RADAR_ArrayCouple_Block::RADAR_ArrayCouple_Block(const std::string& name)
    : Block(name)
    , m_ChannelNum(4)
{
    m_CoupleCoef.Resize(4, 4);
    m_CoupleCoef.Zero();
    for (int i = 0; i < 4; ++i)
        m_CoupleCoef(i, i) = std::complex<double>(1.0, 0.0);
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_ArrayCouple_Block::SetDefaultParameters()
{
    m_ChannelNum = 4;
    m_CoupleCoef.Resize(4, 4);
    m_CoupleCoef.Zero();
    for (int i = 0; i < 4; ++i)
        m_CoupleCoef(i, i) = std::complex<double>(1.0, 0.0);
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_ArrayCouple_Block::SetParameters()
{
    if (!m_algo) return;

    m_algo->ChannelNum = m_ChannelNum;
    m_algo->CoupleCoef = m_CoupleCoef;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_ArrayCouple_Block::Setup()
{
    Block::Setup();

    while (!m_outputQueue.empty()) m_outputQueue.pop();
    m_inputAccumulator.clear();

    // 从输入总线连接数推导通道数
    std::string inputPortName = GetInputPortName(0);
    BufferReader* inputReader = GetInputPort(inputPortName);
    if (inputReader)
        m_ChannelNum = static_cast<int>(inputReader->GetBusConnectionCount());
    else
        m_ChannelNum = 0;

    SetParameters();
    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_ArrayCouple_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 数据流模式：一次处理整帧
// ============================================================================

bool RADAR_ArrayCouple_Block::DataStreamRun()
{
    if (m_ChannelNum <= 0) return true;

    auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(0));
    if (inputData.empty()) return true;

    const int actualChannels = static_cast<int>(inputData.size());
    std::vector<EnvelopeSignal> outputData;
    outputData.reserve(actualChannels);

    // 移植自 RADAR_ArrayCouple::Run()：矩阵-向量乘
    for (int m = 0; m < actualChannels; ++m) {
        std::complex<double> sum(0.0, 0.0);
        for (int n = 0; n < actualChannels; ++n) {
            sum += inputData[n].complex() * m_CoupleCoef(m, n);
        }
        outputData.push_back(EnvelopeSignal(sum));
    }

    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// ============================================================================

bool RADAR_ArrayCouple_Block::TimeDrivenRun()
{
    if (m_ChannelNum <= 0) return true;

    // ① 累积输入总线数据
    auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(0));
    if (inputData.empty()) return true;

    for (const auto& sig : inputData)
        m_inputAccumulator.push_back(sig);

    // ② 收齐一帧（ChannelNum 个通道）后处理
    if (static_cast<int>(m_inputAccumulator.size()) >= m_ChannelNum) {
        std::vector<EnvelopeSignal> frame;
        frame.reserve(m_ChannelNum);

        for (int m = 0; m < m_ChannelNum; ++m) {
            std::complex<double> sum(0.0, 0.0);
            for (int n = 0; n < m_ChannelNum; ++n) {
                sum += m_inputAccumulator[n].complex() * m_CoupleCoef(m, n);
            }
            frame.push_back(EnvelopeSignal(sum));
        }

        m_outputQueue.push(frame);
        m_inputAccumulator.clear();
    }

    // ③ 出队写入
    if (!m_outputQueue.empty()) {
        WriteOutputData(GetOutputPortName(0), m_outputQueue.front());
        m_outputQueue.pop();
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_ArrayCouple_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_ArrayCouple>();

    SetDefaultParameters();

    try { m_ChannelNum = std::stoi(getParameter("ChannelNum").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ChannelNum', using default value."); }
    try { m_CoupleCoef = ParseStringToMatrix<std::complex<double>>(getParameter("CoupleCoef").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'CoupleCoef', using default value."); }

    // 参数校验（移植自 RADAR_ArrayCouple::Setup）
    if (m_CoupleCoef.NumColumns() != m_ChannelNum || m_CoupleCoef.NumRows() != m_ChannelNum) {
        LOG_ERROR("Columns and Rows of CoupleCoef must = ChannelNum");
        return false;
    }

    SetParameters();

    AddInputPort("input",  m_algo->input,  1, Block::DataType::ENVELOPE_BUS);
    AddOutputPort("output", m_algo->output, 1, Block::DataType::ENVELOPE_BUS);

    return true;
}
