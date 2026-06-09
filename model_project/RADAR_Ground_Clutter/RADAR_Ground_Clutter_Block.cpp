#include "RADAR_Ground_Clutter_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>

// ============================================================================
// 字符串处理
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
}

// ============================================================================
// 构造函数
// ============================================================================

RADAR_Ground_Clutter_Block::RADAR_Ground_Clutter_Block(const std::string& name)
    : Block(name)
    , m_cachedNumSample(-1)
    , m_rng(std::random_device{}())
{
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_Ground_Clutter_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty())  m_outputQueue.pop();
    while (!m_clutterQueue.empty()) m_clutterQueue.pop();
    m_inputBuffer.clear();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool RADAR_Ground_Clutter_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool RADAR_Ground_Clutter_Block::DataStreamRun()
{
    using Cx = std::complex<double>;

    std::string inputPort         = GetInputPortName(0);
    std::string bodyRollPort      = GetInputPortName(1);
    std::string bodyPitchPort     = GetInputPortName(2);
    std::string bodyYawPort       = GetInputPortName(3);
    std::string antTiltPort       = GetInputPortName(4);
    std::string antYawPort        = GetInputPortName(5);
    std::string outputPort        = GetOutputPortName(0);
    std::string clutterSamplePort = GetOutputPortName(1);

    // ---- 读可选角度端口 ----
    {
        auto data = ReadInputData<double>(bodyRollPort);
        if (!data.empty()) m_BodyRollAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(bodyPitchPort);
        if (!data.empty()) m_BodyPitchAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(bodyYawPort);
        if (!data.empty()) m_BodyYawAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(antTiltPort);
        if (!data.empty()) m_AntTiltAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(antYawPort);
        if (!data.empty()) m_AntYawAngle = data[0];
    }

    // ---- 读取全部输入 ----
    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);
    if (inputData.empty()) return true;

    const int numSample = static_cast<int>(inputData.size());

    // ---- 计算 Sigma 并生成杂波 ----
    computeSigma();
    if (m_cachedNumSample != numSample)
        generateClutter(numSample);

    // ---- 输出 = 输入 × 杂波 ----
    std::vector<EnvelopeSignal> outputData;
    std::vector<EnvelopeSignal> clutterData;
    outputData.reserve(numSample);
    clutterData.reserve(numSample);

    for (int i = 0; i < numSample; ++i)
    {
        const Cx x = inputData[i].complex();
        const Cx y = x * m_clutter[i];

        outputData.push_back(EnvelopeSignal(y));
        clutterData.push_back(EnvelopeSignal(m_clutter[i]));
    }

    WriteOutputData(outputPort, outputData);
    WriteOutputData(clutterSamplePort, clutterData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 逐点累积模式
// ============================================================================

bool RADAR_Ground_Clutter_Block::TimeDrivenRun()
{
    using Cx = std::complex<double>;

    std::string inputPort         = GetInputPortName(0);
    std::string bodyRollPort      = GetInputPortName(1);
    std::string bodyPitchPort     = GetInputPortName(2);
    std::string bodyYawPort       = GetInputPortName(3);
    std::string antTiltPort       = GetInputPortName(4);
    std::string antYawPort        = GetInputPortName(5);
    std::string outputPort        = GetOutputPortName(0);
    std::string clutterSamplePort = GetOutputPortName(1);

    // ---- 读可选角度端口 ----
    {
        auto data = ReadInputData<double>(bodyRollPort);
        if (!data.empty()) m_BodyRollAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(bodyPitchPort);
        if (!data.empty()) m_BodyPitchAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(bodyYawPort);
        if (!data.empty()) m_BodyYawAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(antTiltPort);
        if (!data.empty()) m_AntTiltAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(antYawPort);
        if (!data.empty()) m_AntYawAngle = data[0];
    }

    auto inputData = ReadInputData<EnvelopeSignal>(inputPort);

    for (size_t i = 0; i < inputData.size(); ++i)
        m_inputBuffer.push_back(inputData[i]);

    // ---- 全部输入收齐后生成杂波 ----
    const int numSample = static_cast<int>(m_SampleRate / m_PRF);
    if (static_cast<int>(m_inputBuffer.size()) >= numSample && m_cachedNumSample != numSample)
    {
        computeSigma();
        generateClutter(numSample);

        for (int i = 0; i < numSample; ++i)
        {
            const Cx x = m_inputBuffer[i].complex();
            const Cx y = x * m_clutter[i];
            m_outputQueue.push(EnvelopeSignal(y));
            m_clutterQueue.push(EnvelopeSignal(m_clutter[i]));
        }

        m_inputBuffer.clear();
    }

    if (!m_outputQueue.empty())
    {
        std::vector<EnvelopeSignal> outData;
        outData.push_back(m_outputQueue.front());
        m_outputQueue.pop();
        WriteOutputData(outputPort, outData);
    }

    if (!m_clutterQueue.empty())
    {
        std::vector<EnvelopeSignal> clutterData;
        clutterData.push_back(m_clutterQueue.front());
        m_clutterQueue.pop();
        WriteOutputData(clutterSamplePort, clutterData);
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_Ground_Clutter_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_Ground_Clutter>();

    simulator_param = getSimu();

    // 解析参数
    SetDefaultParameters();
    try { m_GroundType      = ConvertStringToGroundType(getParameter("GroundType").Value); } catch (...) {}
    try { m_Scatter0        = std::stod(getParameter("Scatter0").Value); } catch (...) {}
    try { m_RF_Freq         = std::stod(getParameter("RF_Freq").Value); } catch (...) {}
    try { m_AntennaPattern  = ConvertStringToAntennaPattern(getParameter("Antenna_Pattern").Value); } catch (...) {}
    try { m_GrazingAngle    = std::stod(getParameter("GrazingAngle").Value); } catch (...) {}
    try { m_BodyRollAngle   = std::stod(getParameter("BodyRollAngle").Value); } catch (...) {}
    try { m_BodyPitchAngle  = std::stod(getParameter("BodyPitchAngle").Value); } catch (...) {}
    try { m_BodyYawAngle    = std::stod(getParameter("BodyYawAngle").Value); } catch (...) {}
    try { m_AntTiltAngle    = std::stod(getParameter("AntTiltAngle").Value); } catch (...) {}
    try { m_AntYawAngle     = std::stod(getParameter("AntYawAngle").Value); } catch (...) {}
    try { m_PRF             = std::stod(getParameter("PRF").Value); } catch (...) {}
    try { m_SampleRate      = std::stod(getParameter("SampleRate").Value); } catch (...) {}
    try { m_AntennaHeight   = std::stod(getParameter("Antenna_Height").Value); } catch (...) {}
    try { m_PlatformVelocity = std::stod(getParameter("Platform_Velocity").Value); } catch (...) {}

    if (m_PRF <= 0)
    {
        LOG_ERROR("RADAR_Ground_Clutter: PRF must be > 0.");
        return false;
    }
    if (m_SampleRate <= 0)
    {
        LOG_ERROR("RADAR_Ground_Clutter: SampleRate must be > 0.");
        return false;
    }
    if (m_AntennaHeight <= 0)
    {
        LOG_ERROR("RADAR_Ground_Clutter: Antenna_Height must be > 0.");
        return false;
    }
    if (m_PlatformVelocity < 0)
        m_PlatformVelocity = std::abs(m_PlatformVelocity);

    SetParameters();

    const int num_sample = static_cast<int>(m_SampleRate / m_PRF);

    m_cachedNumSample = -1;
    m_rng = std::mt19937(std::random_device{}());

    AddInputPort("input",      m_algo->input,     num_sample, Block::DataType::ENVELOPE_SIGNAL);
    AddInputPort("BodyRoll",   m_algo->BodyRoll,  1,          Block::DataType::DOUBLE);
    AddInputPort("BodyPitch",  m_algo->BodyPitch, 1,          Block::DataType::DOUBLE);
    AddInputPort("BodyYaw",    m_algo->BodyYaw,   1,          Block::DataType::DOUBLE);
    AddInputPort("AntTilt",    m_algo->AntTilt,   1,          Block::DataType::DOUBLE);
    AddInputPort("AntYaw",     m_algo->AntYaw,    1,          Block::DataType::DOUBLE);
    AddOutputPort("output",        m_algo->output,        num_sample, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("ClutterSample", m_algo->ClutterSample, num_sample, Block::DataType::ENVELOPE_SIGNAL);

    return true;
}

// ============================================================================
// SetDefaultParameters — 设置参数默认值
// ============================================================================

void RADAR_Ground_Clutter_Block::SetDefaultParameters()
{
    m_GroundType      = RADAR_Ground_Clutter::Farmland;
    m_Scatter0        = 10e-3;
    m_RF_Freq         = 1e9;
    m_AntennaPattern  = RADAR_Ground_Clutter::Gaussian;
    m_GrazingAngle    = 30.0;
    m_BodyRollAngle   = 0.0;
    m_BodyPitchAngle  = 0.0;
    m_BodyYawAngle    = 0.0;
    m_AntTiltAngle    = 0.0;
    m_AntYawAngle     = 0.0;
    m_PRF             = 1e4;
    m_SampleRate      = 10e6;
    m_AntennaHeight   = 3000.0;
    m_PlatformVelocity = 400.0;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_Ground_Clutter_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->GroundType        = m_GroundType;
    m_algo->Scatter0          = m_Scatter0;
    m_algo->RF_Freq           = m_RF_Freq;
    m_algo->Antenna_Pattern   = m_AntennaPattern;
    m_algo->GrazingAngle      = m_GrazingAngle;
    m_algo->BodyRollAngle     = m_BodyRollAngle;
    m_algo->BodyPitchAngle    = m_BodyPitchAngle;
    m_algo->BodyYawAngle      = m_BodyYawAngle;
    m_algo->AntTiltAngle      = m_AntTiltAngle;
    m_algo->AntYawAngle       = m_AntYawAngle;
    m_algo->PRF               = m_PRF;
    m_algo->SampleRate        = m_SampleRate;
    m_algo->Antenna_Height    = m_AntennaHeight;
    m_algo->Platform_Velocity = m_PlatformVelocity;
}

// ============================================================================
// ConvertStringToGroundType
// ============================================================================

RADAR_Ground_Clutter::SelectedGroundType RADAR_Ground_Clutter_Block::ConvertStringToGroundType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "farmland"  || lower == "0") return RADAR_Ground_Clutter::Farmland;
    if (lower == "desert"    || lower == "1") return RADAR_Ground_Clutter::Desert;
    if (lower == "hill"      || lower == "2") return RADAR_Ground_Clutter::Hill;
    if (lower == "mountain"  || lower == "3") return RADAR_Ground_Clutter::Mountain;
    if (lower == "userdefine"|| lower == "4") return RADAR_Ground_Clutter::UserDefine;
    return RADAR_Ground_Clutter::Farmland;
}

// ============================================================================
// ConvertStringToAntennaPattern
// ============================================================================

RADAR_Ground_Clutter::SelectedAntenna_Pattern RADAR_Ground_Clutter_Block::ConvertStringToAntennaPattern(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "gaussian" || lower == "0") return RADAR_Ground_Clutter::Gaussian;
    return RADAR_Ground_Clutter::Gaussian;
}

// ============================================================================
// computeSigma — 根据 GroundType 计算 Morchin 杂波模型 Sigma
// ============================================================================

static constexpr double kPi = 3.14159265358979323846;

void RADAR_Ground_Clutter_Block::computeSigma()
{
    const double c = 3e8;
    const double phi = m_GrazingAngle;
    const double miu = std::sqrt(m_RF_Freq / 1e9) / 4.7;
    const double lambda = c / m_RF_Freq;

    double A = 0.0, B = 0.0, Beta0 = 0.0, Sigmac = 0.0;

    switch (m_GroundType)
    {
    case RADAR_Ground_Clutter::Farmland:
        A = 0.004;
        B = kPi / 2.0;
        Beta0 = 0.2;
        Sigmac = 1.0;
        break;
    case RADAR_Ground_Clutter::Desert:
        A = 0.00126;
        B = kPi / 2.0;
        Beta0 = 0.14;
        Sigmac = 1.0;
        break;
    case RADAR_Ground_Clutter::Hill:
        A = 0.0126;
        B = kPi / 2.0;
        Beta0 = 0.4;
        Sigmac = 1.0;
        break;
    case RADAR_Ground_Clutter::Mountain:
        A = 0.04;
        B = 1.24;
        Beta0 = 0.5;
        Sigmac = 1.0;
        break;
    case RADAR_Ground_Clutter::UserDefine:
        m_Sigma = m_Scatter0;
        return;
    default:
        break;
    }

    // Morchin 杂波模型
    const double tanBeta0 = std::tan(Beta0);
    m_Sigma = A * Sigmac * std::sin(phi) / lambda
        + miu / (tanBeta0 * tanBeta0)
        * std::exp(-std::tan(B - phi) * std::tan(B - phi) / (tanBeta0 * tanBeta0));
}

// ============================================================================
// generateClutter — 生成杂波序列
// ============================================================================

void RADAR_Ground_Clutter_Block::generateClutter(int numSample)
{
    using Cx = std::complex<double>;

    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    m_clutter.assign(numSample, Cx(0.0, 0.0));

    for (int i = 0; i < numSample; ++i)
    {
        const double u1 = dist01(m_rng);
        const double u2 = dist01(m_rng);
        const double ri = m_Sigma * std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * kPi * u2);
        const double rq = m_Sigma * std::sqrt(-2.0 * std::log(u1)) * std::sin(2.0 * kPi * u2);
        m_clutter[i] = Cx(ri, rq);
    }

    m_cachedNumSample = numSample;
}
