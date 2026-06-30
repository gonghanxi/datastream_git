#include "RADAR_PhasedArrayRx_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

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

RADAR_PhasedArrayRx_Block::RADAR_PhasedArrayRx_Block(const std::string& name)
    : Block(name)
    , m_NumChannels(0)
{
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_PhasedArrayRx_Block::SetDefaultParameters()
{
    m_Configuration      = RADAR_PhasedArrayRx::UniformLinearArray;
    m_AxisType           = RADAR_PhasedArrayRx::X;
    m_Array2DShapeType   = RADAR_PhasedArrayRx::Full;
    m_NumOfAnt1D         = 8;
    m_NumOfAnt2D_H       = 8;
    m_NumOfAnt2D_V       = 8;
    m_ElementFactor      = 1.0;
    m_SpaceType          = RADAR_PhasedArrayRx::Uniform;
    m_GridType           = RADAR_PhasedArrayRx::Rectangular;
    m_D                  = 0.5;
    m_D_H                = 0.5;
    m_D_V                = 0.5;
    m_D_array.Resize(1u, 1u); m_D_array(0) = 0.0;
    m_D_H_array.Resize(1u, 1u); m_D_H_array(0) = 0.0;
    m_D_V_array.Resize(1u, 1u); m_D_V_array(0) = 0.0;
    m_mask_array.Resize(1u, 1u); m_mask_array(0) = 0;
    m_ReliabilityType    = RADAR_PhasedArrayRx::NoFailures;
    m_FailureProbability = 0.1;
    m_TargetTheta        = 0.0;
    m_TargetPhi          = 0.0;
    m_WindowType         = RADAR_PhasedArrayRx::Rectangle;
    m_KaiserWindowParameter = 1.0;
    m_Sidelobe_Levels    = -20.0;
    m_nBar               = 2;
    m_IsPhaseShift       = RADAR_PhasedArrayRx::Yes;
    m_BeamTheta          = 0.0;
    m_BeamPhi            = 0.0;
    m_QuantizationType   = RADAR_PhasedArrayRx::No;
    m_PhaseShifterBitwidth = 5;
    m_PhaseShiftType     = RADAR_PhasedArrayRx::CalculateByThetaAndPhi;
    m_DesiredPhaseShiftAngle.Resize(1u, 1u); m_DesiredPhaseShiftAngle(0) = 0.0;
}

// ============================================================================
// SetParameters — 将解析后的参数写入算法对象
// ============================================================================

void RADAR_PhasedArrayRx_Block::SetParameters()
{
    if (!m_algo) return;

    m_algo->Configuration       = m_Configuration;
    m_algo->AxisType            = m_AxisType;
    m_algo->Array2DShapeType    = m_Array2DShapeType;
    m_algo->NumOfAnt1D          = m_NumOfAnt1D;
    m_algo->NumOfAnt2D_H        = m_NumOfAnt2D_H;
    m_algo->NumOfAnt2D_V        = m_NumOfAnt2D_V;
    m_algo->ElementFactor       = m_ElementFactor;
    m_algo->SpaceType           = m_SpaceType;
    m_algo->GridType            = m_GridType;
    m_algo->D                   = m_D;
    m_algo->D_H                 = m_D_H;
    m_algo->D_V                 = m_D_V;
    m_algo->ReliabilityType     = m_ReliabilityType;
    m_algo->FailureProbability  = m_FailureProbability;
    m_algo->TargetTheta         = m_TargetTheta;
    m_algo->TargetPhi           = m_TargetPhi;
    m_algo->WindowType          = m_WindowType;
    m_algo->KaiserWindowParameter = m_KaiserWindowParameter;
    m_algo->Sidelobe_Levels     = m_Sidelobe_Levels;
    m_algo->nBar                = m_nBar;
    m_algo->IsPhaseShift        = m_IsPhaseShift;
    m_algo->BeamTheta           = m_BeamTheta;
    m_algo->BeamPhi             = m_BeamPhi;
    m_algo->QuantizationType    = m_QuantizationType;
    m_algo->PhaseShifterBitwidth = m_PhaseShifterBitwidth;
    m_algo->PhaseShiftType      = m_PhaseShiftType;

    // Matrix parameters — direct assignment
    if (m_D_array.NumElements() > 0)
        m_algo->D_array = m_D_array;
    if (m_D_H_array.NumElements() > 0)
        m_algo->D_H_array = m_D_H_array;
    if (m_D_V_array.NumElements() > 0)
        m_algo->D_V_array = m_D_V_array;
    if (m_mask_array.NumElements() > 0)
        m_algo->mask_array = m_mask_array;
    if (m_DesiredPhaseShiftAngle.NumElements() > 0)
        m_algo->DesiredPhaseShiftAngle = m_DesiredPhaseShiftAngle;
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_PhasedArrayRx_Block::Setup()
{
    Block::Setup();

    while (!m_outputQueue.empty()) m_outputQueue.pop();
    m_inputAccumulator.clear();

    // 从输入总线连接数推导通道数
    std::string arrayInputPortName = GetInputPortName(0);
    BufferReader* arrayInputReader = GetInputPort(arrayInputPortName);
    if (arrayInputReader)
        m_NumChannels = static_cast<int>(arrayInputReader->GetBusConnectionCount());
    else
        m_NumChannels = 0;

    SetParameters();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool RADAR_PhasedArrayRx_Block::Run()
{
    if (IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 数据流模式
// ============================================================================

bool RADAR_PhasedArrayRx_Block::DataStreamRun()
{
    if (m_NumChannels <= 0) return true;

    // ---- 读取可选角度端口 ----
    {
        auto data = ReadInputData<double>(GetInputPortName(1)); // TargetThetaIn
        if (!data.empty()) m_TargetTheta = data[0];
    }
    {
        auto data = ReadInputData<double>(GetInputPortName(2)); // TargetPhiIn
        if (!data.empty()) m_TargetPhi = data[0];
    }
    {
        auto data = ReadInputData<double>(GetInputPortName(3)); // BeamThetaIn
        if (!data.empty()) m_BeamTheta = data[0];
    }
    {
        auto data = ReadInputData<double>(GetInputPortName(4)); // BeamPhiIn
        if (!data.empty()) m_BeamPhi = data[0];
    }

    // ---- 读取 ArrayInput 总线数据 ----
    auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(0));
    if (inputData.empty()) return true;

    const int actualChannels = static_cast<int>(inputData.size());
    std::vector<EnvelopeSignal> outputData;
    outputData.reserve(actualChannels);

    const double AntennaGain = 1.0;

    for (int i = 0; i < actualChannels; ++i)
    {
        outputData.push_back(EnvelopeSignal(inputData[i].complex() * AntennaGain));
    }

    // ---- 写入 ArrayOutput 总线 ----
    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长模式
// ============================================================================

bool RADAR_PhasedArrayRx_Block::TimeDrivenRun()
{
    if (m_NumChannels <= 0) return true;

    // ---- 读取可选角度端口 ----
    {
        auto data = ReadInputData<double>(GetInputPortName(1));
        if (!data.empty()) m_TargetTheta = data[0];
    }
    {
        auto data = ReadInputData<double>(GetInputPortName(2));
        if (!data.empty()) m_TargetPhi = data[0];
    }
    {
        auto data = ReadInputData<double>(GetInputPortName(3));
        if (!data.empty()) m_BeamTheta = data[0];
    }
    {
        auto data = ReadInputData<double>(GetInputPortName(4));
        if (!data.empty()) m_BeamPhi = data[0];
    }

    // ---- ① 累积 ArrayInput 总线数据 ----
    auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(0));
    if (inputData.empty()) return true;

    for (const auto& sig : inputData)
        m_inputAccumulator.push_back(sig);

    // ---- ② 判断阈值：收齐一帧数据 ----
    if (static_cast<int>(m_inputAccumulator.size()) >= m_NumChannels)
    {
        std::vector<EnvelopeSignal> frame;
        frame.reserve(m_NumChannels);
        const double AntennaGain = 1.0;

        for (int i = 0; i < m_NumChannels; ++i)
        {
            frame.push_back(EnvelopeSignal(m_inputAccumulator[i].complex() * AntennaGain));
        }

        m_outputQueue.push(frame);
        m_inputAccumulator.clear();
    }

    // ---- ③ 出队写入 ----
    if (!m_outputQueue.empty())
    {
        WriteOutputData(GetOutputPortName(0), m_outputQueue.front());
        m_outputQueue.pop();
    }

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_PhasedArrayRx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_PhasedArrayRx>();

    simulator_param = getSimu();
    SetDefaultParameters();

    // 解析参数
    try { m_Configuration       = ConvertStringToConfiguration(getParameter("Configuration").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Configuration', using default value."); }
    try { m_AxisType            = ConvertStringToAxisType(getParameter("AxisType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'AxisType', using default value."); }
    try { m_Array2DShapeType    = ConvertStringToArray2DShapeType(getParameter("Array2DShapeType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Array2DShapeType', using default value."); }
    try { m_NumOfAnt1D          = std::stoi(getParameter("NumOfAnt1D").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumOfAnt1D', using default value."); }
    try { m_NumOfAnt2D_H        = std::stoi(getParameter("NumOfAnt2D_H").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumOfAnt2D_H', using default value."); }
    try { m_NumOfAnt2D_V        = std::stoi(getParameter("NumOfAnt2D_V").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumOfAnt2D_V', using default value."); }
    try { m_ElementFactor       = std::stod(getParameter("ElementFactor").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ElementFactor', using default value."); }
    try { m_SpaceType           = ConvertStringToSpaceType(getParameter("SpaceType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SpaceType', using default value."); }
    try { m_GridType            = ConvertStringToGridType(getParameter("GridType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'GridType', using default value."); }
    try { m_D                   = std::stod(getParameter("D").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'D', using default value."); }
    try { m_D_H                 = std::stod(getParameter("D_H").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'D_H', using default value."); }
    try { m_D_V                 = std::stod(getParameter("D_V").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'D_V', using default value."); }
    try { m_D_array             = ParseStringToMatrix<double>(getParameter("D_array").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'D_array', using default value."); }
    try { m_D_H_array           = ParseStringToMatrix<double>(getParameter("D_H_array").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'D_H_array', using default value."); }
    try { m_D_V_array           = ParseStringToMatrix<double>(getParameter("D_V_array").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'D_V_array', using default value."); }
    try { m_mask_array          = ParseStringToMatrix<int>(getParameter("mask_array").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'mask_array', using default value."); }
    try { m_ReliabilityType     = ConvertStringToReliabilityType(getParameter("ReliabilityType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ReliabilityType', using default value."); }
    try { m_FailureProbability  = std::stod(getParameter("FailureProbability").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FailureProbability', using default value."); }
    try { m_TargetTheta         = std::stod(getParameter("TargetTheta").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'TargetTheta', using default value."); }
    try { m_TargetPhi           = std::stod(getParameter("TargetPhi").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'TargetPhi', using default value."); }
    try { m_WindowType          = ConvertStringToWindowType(getParameter("WindowType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'WindowType', using default value."); }
    try { m_KaiserWindowParameter = std::stod(getParameter("KaiserWindowParameter").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'KaiserWindowParameter', using default value."); }
    try { m_Sidelobe_Levels     = std::stod(getParameter("Sidelobe_Levels").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Sidelobe_Levels', using default value."); }
    try { m_nBar                = std::stoi(getParameter("nBar").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'nBar', using default value."); }
    try { m_IsPhaseShift        = ConvertStringToYesorNo(getParameter("IsPhaseShift").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'IsPhaseShift', using default value."); }
    try { m_BeamTheta           = std::stod(getParameter("BeamTheta").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BeamTheta', using default value."); }
    try { m_BeamPhi             = std::stod(getParameter("BeamPhi").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BeamPhi', using default value."); }
    try { m_QuantizationType    = ConvertStringToYesorNo(getParameter("QuantizationType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'QuantizationType', using default value."); }
    try { m_PhaseShifterBitwidth = std::stoi(getParameter("PhaseShifterBitwidth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PhaseShifterBitwidth', using default value."); }
    try { m_PhaseShiftType      = ConvertStringToPhaseShiftType(getParameter("PhaseShiftType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PhaseShiftType', using default value."); }
    try { m_DesiredPhaseShiftAngle = ParseStringToMatrix<double>(getParameter("DesiredPhaseShiftAngle").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DesiredPhaseShiftAngle', using default value."); }

    SetParameters();

    // ---- 参数校验 (from RADAR_PhasedArrayRx::Setup) ----
    bool bStatus = true;
    if (m_NumOfAnt1D <= 0 && m_Configuration == RADAR_PhasedArrayRx::UniformLinearArray)
    {
        LOG_ERROR("NumOfAnt1D must be > 0");
        bStatus = false;
    }
    if (m_NumOfAnt2D_H <= 0 && m_Configuration == RADAR_PhasedArrayRx::UniformRectangularArray)
    {
        LOG_ERROR("NumOfAnt2D_H must be > 0");
        bStatus = false;
    }
    if (m_NumOfAnt2D_V <= 0 && m_Configuration == RADAR_PhasedArrayRx::UniformRectangularArray)
    {
        LOG_ERROR("NumOfAnt2D_V must be > 0");
        bStatus = false;
    }
    if (m_D <= 0 && m_Configuration == RADAR_PhasedArrayRx::UniformLinearArray && m_SpaceType == RADAR_PhasedArrayRx::Uniform)
    {
        LOG_ERROR("D must be > 0");
        bStatus = false;
    }
    if (m_D_H <= 0 && m_Configuration == RADAR_PhasedArrayRx::UniformRectangularArray && m_SpaceType == RADAR_PhasedArrayRx::Uniform)
    {
        LOG_ERROR("D_H must be > 0");
        bStatus = false;
    }
    if (m_D_V <= 0 && m_Configuration == RADAR_PhasedArrayRx::UniformRectangularArray && m_SpaceType == RADAR_PhasedArrayRx::Uniform)
    {
        LOG_ERROR("D_V must be > 0");
        bStatus = false;
    }
    if ((m_FailureProbability < 0 || m_FailureProbability > 1) && m_ReliabilityType == RADAR_PhasedArrayRx::RandomElement)
    {
        LOG_ERROR("FailureProbability must be >= 0 and <= 1");
        bStatus = false;
    }
    if (!bStatus) return false;

    m_NumChannels = 0;

    // 端口注册（端口名必须与 JSON 定义一致）
    AddInputPort("ArrayInput",     m_algo->ArrayInput,      1, Block::DataType::ENVELOPE_BUS);
    AddInputPort("TargetThetaIn",  m_algo->TargetThetaIn,   1, Block::DataType::DOUBLE);
    AddInputPort("TargetPhiIn",    m_algo->TargetPhiIn,     1, Block::DataType::DOUBLE);
    AddInputPort("BeamThetaIn",    m_algo->BeamThetaIn,     1, Block::DataType::DOUBLE);
    AddInputPort("BeamPhiIn",      m_algo->BeamPhiIn,       1, Block::DataType::DOUBLE);
    AddOutputPort("ArrayOutput",   m_algo->ArrayOutput,     1, Block::DataType::ENVELOPE_BUS);

    return true;
}

// ============================================================================
// ConvertStringToConfiguration
// ============================================================================

RADAR_PhasedArrayRx::SelectedConfiguration RADAR_PhasedArrayRx_Block::ConvertStringToConfiguration(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "uniformlineararray"       || lower == "0") return RADAR_PhasedArrayRx::UniformLinearArray;
    if (lower == "uniformrectangulararray"  || lower == "1") return RADAR_PhasedArrayRx::UniformRectangularArray;
    return RADAR_PhasedArrayRx::UniformLinearArray;
}

// ============================================================================
// ConvertStringToAxisType
// ============================================================================

RADAR_PhasedArrayRx::SelectedAxisType RADAR_PhasedArrayRx_Block::ConvertStringToAxisType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "x" || lower == "0") return RADAR_PhasedArrayRx::X;
    if (lower == "y" || lower == "1") return RADAR_PhasedArrayRx::Y;
    if (lower == "z" || lower == "2") return RADAR_PhasedArrayRx::Z;
    return RADAR_PhasedArrayRx::X;
}

// ============================================================================
// ConvertStringToArray2DShapeType
// ============================================================================

RADAR_PhasedArrayRx::SelectedArray2DShapeType RADAR_PhasedArrayRx_Block::ConvertStringToArray2DShapeType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "full"       || lower == "0") return RADAR_PhasedArrayRx::Full;
    if (lower == "customized" || lower == "1") return RADAR_PhasedArrayRx::Customized;
    return RADAR_PhasedArrayRx::Full;
}

// ============================================================================
// ConvertStringToSpaceType
// ============================================================================

RADAR_PhasedArrayRx::SelectedSpaceType RADAR_PhasedArrayRx_Block::ConvertStringToSpaceType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "uniform"    || lower == "0") return RADAR_PhasedArrayRx::Uniform;
    if (lower == "nonuniform" || lower == "1") return RADAR_PhasedArrayRx::NonUniform;
    return RADAR_PhasedArrayRx::Uniform;
}

// ============================================================================
// ConvertStringToGridType
// ============================================================================

RADAR_PhasedArrayRx::SelectedGridType RADAR_PhasedArrayRx_Block::ConvertStringToGridType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "rectangular" || lower == "0") return RADAR_PhasedArrayRx::Rectangular;
    if (lower == "triangular"  || lower == "1") return RADAR_PhasedArrayRx::Triangular;
    return RADAR_PhasedArrayRx::Rectangular;
}

// ============================================================================
// ConvertStringToReliabilityType
// ============================================================================

RADAR_PhasedArrayRx::SelectedReliabilityType RADAR_PhasedArrayRx_Block::ConvertStringToReliabilityType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "nofailures"     || lower == "0") return RADAR_PhasedArrayRx::NoFailures;
    if (lower == "randomelement"  || lower == "1") return RADAR_PhasedArrayRx::RandomElement;
    return RADAR_PhasedArrayRx::NoFailures;
}

// ============================================================================
// ConvertStringToWindowType
// ============================================================================

RADAR_PhasedArrayRx::SelectedWindowType RADAR_PhasedArrayRx_Block::ConvertStringToWindowType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "rectangle"     || lower == "0") return RADAR_PhasedArrayRx::Rectangle;
    if (lower == "bartlett"      || lower == "1") return RADAR_PhasedArrayRx::Bartlett;
    if (lower == "hanning"       || lower == "2") return RADAR_PhasedArrayRx::Hanning;
    if (lower == "hamming"       || lower == "3") return RADAR_PhasedArrayRx::Hamming;
    if (lower == "blackman"      || lower == "4") return RADAR_PhasedArrayRx::Blackman;
    if (lower == "steepblackman" || lower == "5") return RADAR_PhasedArrayRx::SteepBlackman;
    if (lower == "kaiser"        || lower == "6") return RADAR_PhasedArrayRx::Kaiser;
    if (lower == "taylor"        || lower == "7") return RADAR_PhasedArrayRx::Taylor;
    return RADAR_PhasedArrayRx::Rectangle;
}

// ============================================================================
// ConvertStringToYesorNo
// ============================================================================

RADAR_PhasedArrayRx::SelectedYesorNo RADAR_PhasedArrayRx_Block::ConvertStringToYesorNo(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "yes" || lower == "0") return RADAR_PhasedArrayRx::Yes;
    if (lower == "no"  || lower == "1") return RADAR_PhasedArrayRx::No;
    return RADAR_PhasedArrayRx::Yes;
}

// ============================================================================
// ConvertStringToPhaseShiftType
// ============================================================================

RADAR_PhasedArrayRx::SelectedPhaseShiftType RADAR_PhasedArrayRx_Block::ConvertStringToPhaseShiftType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "calculatebythetaandphi" || lower == "0") return RADAR_PhasedArrayRx::CalculateByThetaAndPhi;
    if (lower == "desiredphaseshift"      || lower == "1") return RADAR_PhasedArrayRx::DesiredPhaseShift;
    return RADAR_PhasedArrayRx::CalculateByThetaAndPhi;
}

