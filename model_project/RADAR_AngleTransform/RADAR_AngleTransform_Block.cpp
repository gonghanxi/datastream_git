#include "RADAR_AngleTransform_Block.h"

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
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(),
        [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}

} // anonymous namespace

// ============================================================================
// 构造函数
// ============================================================================

RADAR_AngleTransform_Block::RADAR_AngleTransform_Block(const std::string& name)
    : Block(name)
    , m_TransformType(RADAR_AngleTransform::From_Antenna_Coordinates_to_Radar_Coordinates)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_AngleTransform_Block::SetDefaultParameters()
{
    m_TransformType = RADAR_AngleTransform::From_Antenna_Coordinates_to_Radar_Coordinates;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_AngleTransform_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->TransformType = m_TransformType;
}

// ============================================================================
// ConvertStringToTransformTypeEnum
// ============================================================================

RADAR_AngleTransform_Block::TransformTypeEnum
RADAR_AngleTransform_Block::ConvertStringToTransformTypeEnum(const std::string& value)
{
    const std::string v = ToLowerCopy(TrimCopy(value));
    if (v == "from antenna coordinates to radar coordinates"
        || v == "from_antenna_coordinates_to_radar_coordinates" || v == "0")
        return RADAR_AngleTransform::From_Antenna_Coordinates_to_Radar_Coordinates;
    if (v == "from radar coordinates to antenna coordinates"
        || v == "from_radar_coordinates_to_antenna_coordinates" || v == "1")
        return RADAR_AngleTransform::From_Radar_Coordinates_to_Antenna_Coordinates;
    return RADAR_AngleTransform::From_Antenna_Coordinates_to_Radar_Coordinates;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_AngleTransform_Block::Setup()
{
    Block::Setup();

    m_inElBuffer.clear();
    m_inAzBuffer.clear();
    while (!m_outElQueue.empty()) m_outElQueue.pop();
    while (!m_outAzQueue.empty()) m_outAzQueue.pop();

    return true;
}

// ============================================================================
// Run — 双模式分发
// ============================================================================

bool RADAR_AngleTransform_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_AngleTransform_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_AngleTransform>();

    SetDefaultParameters();

    try { m_TransformType = ConvertStringToTransformTypeEnum(getParameter("TransformType").Value); } catch (...) {}

    SetParameters();

    AddInputPort("inEl_inTheta", m_algo->inEl_inTheta, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("inAz_inPhi",   m_algo->inAz_inPhi,   1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("outEl_outTheta", m_algo->outEl_outTheta, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("outAz_outPhi",   m_algo->outAz_outPhi,   1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// DataStreamRun — 数据流模式：逐对读取、变换、输出
// ============================================================================

bool RADAR_AngleTransform_Block::DataStreamRun()
{
    auto inElData = ReadInputData<double>(GetInputPortName(0));
    auto inAzData = ReadInputData<double>(GetInputPortName(1));

    if (inElData.empty() || inAzData.empty()) return true;

    const size_t count = std::min(inElData.size(), inAzData.size());
    std::vector<double> outElData(count);
    std::vector<double> outAzData(count);

    for (size_t i = 0; i < count; ++i) {
        const double a1 = sanitize(inElData[i], 0.0);
        const double a2 = sanitize(inAzData[i], 0.0);
        double out1 = 0.0;
        double out2 = 0.0;

        if (m_TransformType == RADAR_AngleTransform::From_Radar_Coordinates_to_Antenna_Coordinates) {
            radarToAntenna(a1, a2, out1, out2);
        } else {
            antennaToRadar(a1, a2, out1, out2);
        }

        outElData[i] = out1;
        outAzData[i] = out2;
    }

    WriteOutputData(GetOutputPortName(0), outElData);
    WriteOutputData(GetOutputPortName(1), outAzData);
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式：逐对累积、变换、出队
// ============================================================================

bool RADAR_AngleTransform_Block::TimeDrivenRun()
{
    // ① 累积输入
    {
        auto inElData = ReadInputData<double>(GetInputPortName(0));
        for (auto& v : inElData) m_inElBuffer.push_back(v);
        auto inAzData = ReadInputData<double>(GetInputPortName(1));
        for (auto& v : inAzData) m_inAzBuffer.push_back(v);
    }

    // ② 当两个输入缓冲区都有数据时，处理所有可用对
    if (!m_inElBuffer.empty() && !m_inAzBuffer.empty()) {
        const size_t count = std::min(m_inElBuffer.size(), m_inAzBuffer.size());
        for (size_t i = 0; i < count; ++i) {
            const double a1 = sanitize(m_inElBuffer[i], 0.0);
            const double a2 = sanitize(m_inAzBuffer[i], 0.0);
            double out1 = 0.0;
            double out2 = 0.0;

            if (m_TransformType == RADAR_AngleTransform::From_Radar_Coordinates_to_Antenna_Coordinates) {
                radarToAntenna(a1, a2, out1, out2);
            } else {
                antennaToRadar(a1, a2, out1, out2);
            }

            m_outElQueue.push(out1);
            m_outAzQueue.push(out2);
        }
        m_inElBuffer.clear();
        m_inAzBuffer.clear();
    }

    // ③ 出队写入
    if (!m_outElQueue.empty()) {
        double v = m_outElQueue.front(); m_outElQueue.pop();
        WriteOutputData(GetOutputPortName(0), std::vector<double>{v});
    }
    if (!m_outAzQueue.empty()) {
        double v = m_outAzQueue.front(); m_outAzQueue.pop();
        WriteOutputData(GetOutputPortName(1), std::vector<double>{v});
    }

    return true;
}

// ============================================================================
// 算法核心 — 内联自 RADAR_AngleTransform 的 private 方法
// ============================================================================

void RADAR_AngleTransform_Block::antennaToRadar(double theta,
    double phi,
    double& elevation,
    double& azimuth)
{
    const double st = std::sin(theta);
    const double x = st * std::cos(phi);
    const double y = st * std::sin(phi);
    const double z = std::cos(theta);

    elevation = std::asin(clampUnit(y));
    azimuth   = std::atan2(x, z);
}

void RADAR_AngleTransform_Block::radarToAntenna(double elevation,
    double azimuth,
    double& theta,
    double& phi)
{
    const double ce = std::cos(elevation);
    const double x = std::sin(azimuth) * ce;
    const double y = std::sin(elevation);
    const double z = std::cos(azimuth) * ce;

    theta = std::acos(clampUnit(z));
    phi   = std::atan2(y, x);
}

double RADAR_AngleTransform_Block::clampUnit(double x)
{
    if (x > 1.0)  return 1.0;
    if (x < -1.0) return -1.0;
    return x;
}

double RADAR_AngleTransform_Block::sanitize(double x, double fallback)
{
    if (x != x) {
        return fallback;
    }
    if (!std::isfinite(x)) {
        return fallback;
    }
    return x;
}
