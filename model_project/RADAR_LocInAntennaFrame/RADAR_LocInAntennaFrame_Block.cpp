#include "RADAR_LocInAntennaFrame_Block.h"

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

RADAR_LocInAntennaFrame_Block::RADAR_LocInAntennaFrame_Block(const std::string& name)
    : Block(name)
    , m_TargetNum(0)
{
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_LocInAntennaFrame_Block::Setup()
{
    Block::Setup();

    while (!m_azimuthQueue.empty())   m_azimuthQueue.pop();
    while (!m_elevationQueue.empty()) m_elevationQueue.pop();
    m_radarLocBuffer.clear();
    m_targetLocBuffer.clear();

    // 从输入总线连接数推导目标数量
    BufferReader* targetLocReader = GetInputPort("TxPlatformLoc");
    if (targetLocReader)
        m_TargetNum = targetLocReader->GetBusConnectionCount();
    else
        m_TargetNum = 0;

    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool RADAR_LocInAntennaFrame_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 数据流模式
// ============================================================================

bool RADAR_LocInAntennaFrame_Block::DataStreamRun()
{
    std::string bodyYawPort   = GetInputPortName(2);
    std::string bodyPitchPort = GetInputPortName(3);
    std::string bodyRollPort  = GetInputPortName(4);
    std::string antYawPort    = GetInputPortName(5);
    std::string antPitchPort  = GetInputPortName(6);
    std::string antRollPort   = GetInputPortName(7);
    std::string elevationPort = GetOutputPortName(0);
    std::string azimuthPort   = GetOutputPortName(1);

    // ---- 读可选角度端口 ----
    {
        auto data = ReadInputData<double>(bodyYawPort);
        if (!data.empty()) m_BodyYawAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(bodyPitchPort);
        if (!data.empty()) m_BodyPitchAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(bodyRollPort);
        if (!data.empty()) m_BodyRollAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(antYawPort);
        if (!data.empty()) m_AntYawAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(antPitchPort);
        if (!data.empty()) m_AntPitchAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(antRollPort);
        if (!data.empty()) m_AntRollAngle = data[0];
    }
    // ---- 读取雷达位置（ReadInputData） ----
    auto radarLocData = ReadInputData<SystemVueModelBuilder::Matrix<double>>("RadarLoc");
    if (radarLocData.empty()) return true;
    if (radarLocData[0].NumRows() != 1 || radarLocData[0].NumColumns() != 3) {
        LOG_ERROR("RadarLoc matrix must be 1x3, got ", radarLocData[0].NumRows(), "x", radarLocData[0].NumColumns());
        return false;
    }
    double rx = radarLocData[0](0);
    double ry = radarLocData[0](1);
    double rz = radarLocData[0](2);

    if (m_TargetNum <= 0) return true;

    // ---- 逐目标读取位置（GetBusConnections + ReadData） ----
    BufferReader* targetLocReader = GetInputPort("TxPlatformLoc");
    std::vector<SystemVueModelBuilder::BusConnection> busConns = targetLocReader->GetBusConnections();

    std::vector<double> azimuthData;
    std::vector<double> elevationData;
    azimuthData.reserve(m_TargetNum);
    elevationData.reserve(m_TargetNum);

    for (int i = 0; i < m_TargetNum; ++i)
    {
        std::vector<SystemVueModelBuilder::Matrix<double>> targetBuf(1);
        busConns[i].bridgeReader->ReadData(targetBuf);
        if (targetBuf[0].NumRows() != 1 || targetBuf[0].NumColumns() != 3) {
            LOG_ERROR("TxPlatformLoc matrix must be 1x3, got ", targetBuf[0].NumRows(), "x", targetBuf[0].NumColumns());
            continue;
        }
        double tx = targetBuf[0](0);
        double ty = targetBuf[0](1);
        double tz = targetBuf[0](2);

        double dx = tx - rx;
        double dy = ty - ry;
        double dz = tz - rz;

        double R = std::sqrt(dx * dx + dy * dy + dz * dz);

        double azimuth = 0.0, elevation = 0.0;

        if (m_AntennaPlaneType == RADAR_LocInAntennaFrame::XYPlane)
        {
            elevation = std::asin(dz / R);
            azimuth   = std::atan2(dx, dz);
        }
        else if (m_AntennaPlaneType == RADAR_LocInAntennaFrame::YZPlane)
        {
            elevation = std::asin(dz / R);
            azimuth   = std::atan2(dy, dx);
        }

        azimuthData.push_back(azimuth);
        elevationData.push_back(elevation);
    }

    // ---- 写入输出端口 ----
    WriteOutputData(elevationPort, elevationData);
    WriteOutputData(azimuthPort,   azimuthData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式（逐点累积 + 双输出队列）
// ============================================================================

bool RADAR_LocInAntennaFrame_Block::TimeDrivenRun()
{
    std::string bodyYawPort   = GetInputPortName(2);
    std::string bodyPitchPort = GetInputPortName(3);
    std::string bodyRollPort  = GetInputPortName(4);
    std::string antYawPort    = GetInputPortName(5);
    std::string antPitchPort  = GetInputPortName(6);
    std::string antRollPort   = GetInputPortName(7);
    std::string elevationPort = GetOutputPortName(0);
    std::string azimuthPort   = GetOutputPortName(1);

    // ---- 读可选角度端口 ----
    {
        auto data = ReadInputData<double>(bodyYawPort);
        if (!data.empty()) m_BodyYawAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(bodyPitchPort);
        if (!data.empty()) m_BodyPitchAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(bodyRollPort);
        if (!data.empty()) m_BodyRollAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(antYawPort);
        if (!data.empty()) m_AntYawAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(antPitchPort);
        if (!data.empty()) m_AntPitchAngle = data[0];
    }
    {
        auto data = ReadInputData<double>(antRollPort);
        if (!data.empty()) m_AntRollAngle = data[0];
    }

    // ---- ① 累积雷达位置输入（ReadInputData） ----
    auto radarLocData = ReadInputData<SystemVueModelBuilder::Matrix<double>>("RadarLoc");
    if (radarLocData.empty()) return true;
    if (radarLocData[0].NumRows() != 1 || radarLocData[0].NumColumns() != 3) {
        LOG_ERROR("RadarLoc matrix must be 1x3, got ", radarLocData[0].NumRows(), "x", radarLocData[0].NumColumns());
        return false;
    }
    m_radarLocBuffer.push_back(radarLocData[0](0));
    m_radarLocBuffer.push_back(radarLocData[0](1));
    m_radarLocBuffer.push_back(radarLocData[0](2));

    // ---- ① 累积目标位置输入（GetBusConnections + ReadData） ----
    BufferReader* targetLocReader = GetInputPort("TxPlatformLoc");
    std::vector<SystemVueModelBuilder::BusConnection> busConns = targetLocReader->GetBusConnections();
    for (int i = 0; i < m_TargetNum; ++i)
    {
        std::vector<SystemVueModelBuilder::Matrix<double>> targetBuf(1);
        busConns[i].bridgeReader->ReadData(targetBuf);
        if (targetBuf[0].NumRows() != 1 || targetBuf[0].NumColumns() != 3) {
            LOG_ERROR("TxPlatformLoc matrix must be 1x3, got ", targetBuf[0].NumRows(), "x", targetBuf[0].NumColumns());
            continue;
        }
        m_targetLocBuffer.push_back(targetBuf[0](0));
        m_targetLocBuffer.push_back(targetBuf[0](1));
        m_targetLocBuffer.push_back(targetBuf[0](2));
    }

    // ---- ② 判断阈值：收齐一帧雷达数据（3个 double） ----
    const int radarFrameSize = 3;
    const int targetNum = static_cast<int>(m_targetLocBuffer.size()) / 3;
    if (static_cast<int>(m_radarLocBuffer.size()) >= radarFrameSize && targetNum > 0)
    {
        double rx = m_radarLocBuffer[0];
        double ry = m_radarLocBuffer[1];
        double rz = m_radarLocBuffer[2];

        // ---- ③ 处理并分别入队两个输出队列 ----
        std::vector<double> azimuths(targetNum);
        std::vector<double> elevations(targetNum);

        for (int i = 0; i < targetNum; ++i)
        {
            const int base = i * 3;
            double tx = (base + 0 < static_cast<int>(m_targetLocBuffer.size())) ? m_targetLocBuffer[base + 0] : 0.0;
            double ty = (base + 1 < static_cast<int>(m_targetLocBuffer.size())) ? m_targetLocBuffer[base + 1] : 0.0;
            double tz = (base + 2 < static_cast<int>(m_targetLocBuffer.size())) ? m_targetLocBuffer[base + 2] : 0.0;

            double dx = tx - rx;
            double dy = ty - ry;
            double dz = tz - rz;

            double R = std::sqrt(dx * dx + dy * dy + dz * dz);

            double azimuth = 0.0, elevation = 0.0;

            if (m_AntennaPlaneType == RADAR_LocInAntennaFrame::XYPlane)
            {
                elevation = std::asin(dz / R);
                azimuth   = std::atan2(dx, dz);
            }
            else if (m_AntennaPlaneType == RADAR_LocInAntennaFrame::YZPlane)
            {
                elevation = std::asin(dz / R);
                azimuth   = std::atan2(dy, dx);
            }

            azimuths[i]   = azimuth;
            elevations[i] = elevation;
        }

        m_azimuthQueue.push(azimuths);
        m_elevationQueue.push(elevations);

        m_radarLocBuffer.clear();
        m_targetLocBuffer.clear();
    }

    // ---- ④ 出队写入两个输出端口（每次各出一个） ----
    if (!m_azimuthQueue.empty())
    {
        WriteOutputData(elevationPort, m_elevationQueue.front());
        WriteOutputData(azimuthPort,   m_azimuthQueue.front());

        m_elevationQueue.pop();
        m_azimuthQueue.pop();
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_LocInAntennaFrame_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_LocInAntennaFrame>();

    simulator_param = getSimu();

    // 解析参数
    SetDefaultParameters();
    try { m_TimeStep         = std::stod(getParameter("TimeStep").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'TimeStep', using default value."); }
    try { m_XYZFrameType     = ConvertStringToXYZFrameType(getParameter("XYZFrameType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'XYZFrameType', using default value."); }
    try { m_AntennaPlaneType = ConvertStringToAntennaPlaneType(getParameter("AntennaPlaneType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'AntennaPlaneType', using default value."); }
    try { m_CoordinateType   = ConvertStringToCoordinateType(getParameter("CoordinateType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'CoordinateType', using default value."); }

    SetParameters();

    m_TargetNum = 0;

    // 端口注册（rate=1：无 SetRate 的数据流模型）
    AddInputPort("RadarLoc",   m_algo->RadarLoc,   1, Block::DataType::MATRIX_DOUBLE);
    AddInputPort("TxPlatformLoc",  m_algo->TargetLoc,  1, Block::DataType::MATRIX_DOUBLE_BUS);
    AddInputPort("BodyYaw",    m_algo->BodyYaw,    1, Block::DataType::DOUBLE);
    AddInputPort("BodyPitch",  m_algo->BodyPitch,  1, Block::DataType::DOUBLE);
    AddInputPort("BodyRoll",   m_algo->BodyRoll,   1, Block::DataType::DOUBLE);
    AddInputPort("AntYaw",     m_algo->AntYaw,     1, Block::DataType::DOUBLE);
    AddInputPort("AntPitch",   m_algo->AntPitch,   1, Block::DataType::DOUBLE);
    AddInputPort("AntRoll",    m_algo->AntRoll,    1, Block::DataType::DOUBLE);
    // 输出顺序对齐原算法宏定义：Elevation 先，Azimuth 后
    AddOutputPort("Elevation", m_algo->Elevation,  1, Block::DataType::DOUBLE_BUS);
    AddOutputPort("Azimuth",   m_algo->Azimuth,    1, Block::DataType::DOUBLE_BUS);

    return true;
}

// ============================================================================
// SetDefaultParameters — 设置参数默认值
// ============================================================================

void RADAR_LocInAntennaFrame_Block::SetDefaultParameters()
{
    m_TimeStep         = 1e-9;
    m_XYZFrameType     = RADAR_LocInAntennaFrame::ECIFrame;
    m_AntennaPlaneType = RADAR_LocInAntennaFrame::XYPlane;
    m_CoordinateType   = RADAR_LocInAntennaFrame::RADARCoordinate;

    m_BodyYawAngle   = 0.0;
    m_BodyPitchAngle = 0.0;
    m_BodyRollAngle  = 0.0;
    m_AntYawAngle    = 0.0;
    m_AntPitchAngle  = 0.0;
    m_AntRollAngle   = 0.0;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_LocInAntennaFrame_Block::SetParameters()
{
    if (!m_algo) return;
    m_algo->TimeStep         = m_TimeStep;
    m_algo->XYZFrameType     = m_XYZFrameType;
    m_algo->AntennaPlaneType = m_AntennaPlaneType;
    m_algo->CoordinateType   = m_CoordinateType;
}

// ============================================================================
// ConvertStringToXYZFrameType
// ============================================================================

RADAR_LocInAntennaFrame::SelectedXYZFrameTypes RADAR_LocInAntennaFrame_Block::ConvertStringToXYZFrameType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "eci frame"        || lower == "eciframe"       || lower == "0") return RADAR_LocInAntennaFrame::ECIFrame;
    if (lower == "simple xyz frame" || lower == "xyzframe"       || lower == "1") return RADAR_LocInAntennaFrame::XYZFrame;
    return RADAR_LocInAntennaFrame::ECIFrame;
}

// ============================================================================
// ConvertStringToAntennaPlaneType
// ============================================================================

RADAR_LocInAntennaFrame::SelectedAntennaPlaneTypes RADAR_LocInAntennaFrame_Block::ConvertStringToAntennaPlaneType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "xy plane" || lower == "xyplane" || lower == "0") return RADAR_LocInAntennaFrame::XYPlane;
    if (lower == "yz plane" || lower == "yzplane" || lower == "1") return RADAR_LocInAntennaFrame::YZPlane;
    return RADAR_LocInAntennaFrame::XYPlane;
}

// ============================================================================
// ConvertStringToCoordinateType
// ============================================================================

RADAR_LocInAntennaFrame::SelectedCoordinateTypes RADAR_LocInAntennaFrame_Block::ConvertStringToCoordinateType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "radar coordinate"   || lower == "radarcoordinate"   || lower == "0") return RADAR_LocInAntennaFrame::RADARCoordinate;
    if (lower == "antenna coordinate" || lower == "antennacoordinate" || lower == "1") return RADAR_LocInAntennaFrame::AntennaCoordinate;
    return RADAR_LocInAntennaFrame::RADARCoordinate;
}
