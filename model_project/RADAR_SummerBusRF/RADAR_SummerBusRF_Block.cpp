#include "RADAR_SummerBusRF_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

// ============================================================================
// 匿名命名空间 — 纯静态工具函数
// ============================================================================

namespace {

std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}

} // anonymous namespace

// ============================================================================
// 构造函数
// ============================================================================

RADAR_SummerBusRF_Block::RADAR_SummerBusRF_Block(const std::string& name)
    : Block(name)
    , m_FcOut(2)  // default: max
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_SummerBusRF_Block::SetDefaultParameters()
{
    m_FcOut = 2;  // max
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
//   SelectedFcOut 是 private enum，无法从外部赋值，仅 m_algo 用于端口注册和计数器
// ============================================================================

void RADAR_SummerBusRF_Block::SetParameters()
{
    if (!m_algo) return;
}

// ============================================================================
// ConvertStringToFcOut
// ============================================================================

int RADAR_SummerBusRF_Block::ConvertStringToFcOut(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "min" || v == "0")    return 0;
    if (v == "center" || v == "1") return 1;
    if (v == "max" || v == "2")    return 2;
    return 2;  // default: max
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_SummerBusRF_Block::Setup()
{
    Block::Setup();

    m_input1Accumulator.clear();
    m_input2Accumulator.clear();
    while (!m_outputQueue.empty()) m_outputQueue.pop();

    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_SummerBusRF_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 数据流模式
// 移植自 RADAR_SummerBusRF::PropagateCharacterizationFrequency + Run
// fcOut 根据 FcOut 枚举逐通道计算，然后做载频转换后求和
// ============================================================================

bool RADAR_SummerBusRF_Block::DataStreamRun()
{
    std::string input1Name = GetInputPortName(0);
    std::string input2Name = GetInputPortName(1);

    auto input1Data = ReadInputData<EnvelopeSignal>(input1Name);
    auto input2Data = ReadInputData<EnvelopeSignal>(input2Name);

    if (input1Data.empty() || input2Data.empty()) return true;

    const int nChannels = static_cast<int>(input1Data.size());
    if (nChannels != static_cast<int>(input2Data.size()))
    {
        LOG_ERROR("The width of input1 and input2 should be the same.");
        return false;
    }

    double dTime = simulator_param.startTime
                 + static_cast<double>(m_algo->GetCount()) / simulator_param.samplingRate;

    std::vector<EnvelopeSignal> outputData;
    outputData.reserve(nChannels);

    auto& conn1 = GetInputPort(input1Name)->GetBusConnections();
    auto& conn2 = GetInputPort(input2Name)->GetBusConnections();

    for (int i = 0; i < nChannels; ++i)
    {
        double fc1 = conn1.at(i).bridgeReader->getCharacterizationFrequency();
        double fc2 = conn2.at(i).bridgeReader->getCharacterizationFrequency();

        double fcOut;
        switch (m_FcOut)
        {
        case 0:  fcOut = std::min(fc1, fc2);          break;  // min
        case 1:  fcOut = (fc1 + fc2) / 2.0;            break;  // center
        case 2:
        default: fcOut = std::max(fc1, fc2);          break;  // max
        }

        // 将 input1 转换到 fcOut
        std::complex<double> sum;
        if (fc1 != fcOut)
        {
            double phase = 2.0 * M_PI * (fc1 - fcOut) * dTime;
            sum = input1Data[i].complex()
                * std::complex<double>(std::cos(phase), std::sin(phase));
        }
        else
        {
            sum = input1Data[i].complex();
        }

        // 将 input2 转换到 fcOut 并累加
        if (fc2 != fcOut)
        {
            double phase = 2.0 * M_PI * (fc2 - fcOut) * dTime;
            sum += input2Data[i].complex()
                 * std::complex<double>(std::cos(phase), std::sin(phase));
        }
        else
        {
            sum += input2Data[i].complex();
        }

        outputData.push_back(EnvelopeSignal(sum));
    }

    WriteOutputData(GetOutputPortName(0), outputData);

    m_algo->Advance();

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// 逐通道累积两个输入端口的包络信号，收齐一帧后处理
// ============================================================================

bool RADAR_SummerBusRF_Block::TimeDrivenRun()
{
    std::string input1Name = GetInputPortName(0);
    std::string input2Name = GetInputPortName(1);

    int nChannels = static_cast<int>(GetInputPort(input1Name)->GetBusConnectionCount());
    if (nChannels <= 0) return true;

    // ① 累积 input1
    {
        auto data = ReadInputData<EnvelopeSignal>(input1Name);
        for (auto& sig : data)
            m_input1Accumulator.push_back(sig);
    }

    // ② 累积 input2
    {
        auto data = ReadInputData<EnvelopeSignal>(input2Name);
        for (auto& sig : data)
            m_input2Accumulator.push_back(sig);
    }

    // ③ 两个输入端都收齐一帧后处理
    while (static_cast<int>(m_input1Accumulator.size()) >= nChannels
        && static_cast<int>(m_input2Accumulator.size()) >= nChannels)
    {
        double dTime = simulator_param.startTime
                     + static_cast<double>(m_algo->GetCount()) / simulator_param.samplingRate;

        std::vector<EnvelopeSignal> outputData;
        outputData.reserve(nChannels);

        for (int i = 0; i < nChannels; ++i)
        {
            double fc1 = GetInputPort(input1Name)->GetBusConnections().at(i).bridgeReader->getCharacterizationFrequency();
            double fc2 = GetInputPort(input2Name)->GetBusConnections().at(i).bridgeReader->getCharacterizationFrequency();

            double fcOut;
            switch (m_FcOut)
            {
            case 0:  fcOut = std::min(fc1, fc2);          break;
            case 1:  fcOut = (fc1 + fc2) / 2.0;            break;
            case 2:
            default: fcOut = std::max(fc1, fc2);          break;
            }

            std::complex<double> sum;
            if (fc1 != fcOut)
            {
                double phase = 2.0 * M_PI * (fc1 - fcOut) * dTime;
                sum = m_input1Accumulator[i].complex()
                    * std::complex<double>(std::cos(phase), std::sin(phase));
            }
            else
            {
                sum = m_input1Accumulator[i].complex();
            }

            if (fc2 != fcOut)
            {
                double phase = 2.0 * M_PI * (fc2 - fcOut) * dTime;
                sum += m_input2Accumulator[i].complex()
                     * std::complex<double>(std::cos(phase), std::sin(phase));
            }
            else
            {
                sum += m_input2Accumulator[i].complex();
            }

            outputData.push_back(EnvelopeSignal(sum));
        }

        m_outputQueue.push(outputData);
        m_input1Accumulator.erase(m_input1Accumulator.begin(), m_input1Accumulator.begin() + nChannels);
        m_input2Accumulator.erase(m_input2Accumulator.begin(), m_input2Accumulator.begin() + nChannels);

        m_algo->Advance();
    }

    // ④ 出队写入
    while (!m_outputQueue.empty())
    {
        WriteOutputData(GetOutputPortName(0), m_outputQueue.front());
        m_outputQueue.pop();
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_SummerBusRF_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_SummerBusRF>();

    SetDefaultParameters();

    simulator_param = getSimu();

    try { m_FcOut = ConvertStringToFcOut(getParameter("FcOut").Value); } catch (...) {}

    SetParameters();

    AddInputPort("input1", m_algo->input1, 1, Block::DataType::ENVELOPE_BUS);
    AddInputPort("input2", m_algo->input2, 1, Block::DataType::ENVELOPE_BUS);
    AddOutputPort("output", m_algo->output, 1, Block::DataType::ENVELOPE_BUS);

    return true;
}
