#include "RADAR_PhaseShift_Block.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <cctype>

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

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

RADAR_PhaseShift_Block::RADAR_PhaseShift_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_PhaseShift_Block::Setup()
{
    Block::Setup();
    // 清空时间驱动模式的缓冲区
    m_inputBuffer.clear();
    while(!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }
    return true;
}

// ============================================================================
// Run — 运行模式分发
// ============================================================================

bool RADAR_PhaseShift_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_PhaseShift_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_ps = std::make_unique<RADAR_PhaseShift>();

    SetDefaultParameters();

    // 解析参数
    try { NumOfAntx = std::stoi(getParameter("NumOfAntx").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumOfAntx', using default value."); }
    try { NumOfAnty = std::stoi(getParameter("NumOfAnty").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumOfAnty', using default value."); }
    try { Type = ConvertStringToTypeEnum(getParameter("Type").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Type', using default value."); }
    try { Dx = std::stod(getParameter("Dx").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Dx', using default value."); }
    try { Dy = std::stod(getParameter("Dy").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Dy', using default value."); }
    try { Theta = std::stod(getParameter("Theta").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Theta', using default value."); }
    try { Phi = std::stod(getParameter("Phi").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Phi', using default value."); }
    try { DesiredPhaseShiftVec = ParseDoubleArray(getParameter("DesiredPhaseShift").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DesiredPhaseShift', using default value."); }

    SetParameters();

    if(!ModelSetup()) return false;

    // 注册端口

    AddInputPort("Input", m_ps->Input, 1, DataType::ENVELOPE_SIGNAL);
    AddInputPort("InTheta", m_ps->InTheta, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("InPhi", m_ps->InPhi, 1, DataType::CIRCULAR_BUFFER_DOUBLE);

    // 输出 bus：每个通道速率 = 1，框架自动遍历所有 bus 连接
    AddOutputPort("output", m_ps->output, 1, DataType::ENVELOPE_BUS);

    // AntPhase：普通 double 端口，writeSize = nAnt_
    AddOutputPort("AntPhase", m_ps->AntPhase, static_cast<size_t>(nAnt_), DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_PhaseShift_Block::SetDefaultParameters()
{
    NumOfAntx = 4;
    NumOfAnty = 4;
    Type = RADAR_PhaseShift::Calculate_by_theta_and_phi;
    Dx = 0.5;
    Dy = 0.5;
    Theta = 0.0;
    Phi = 0.0;
    DesiredPhaseShiftVec.clear();
}

// ============================================================================
// SetParameters — 将 Block 参数同步到算法实例（仅用于端口注册）
// ============================================================================

void RADAR_PhaseShift_Block::SetParameters()
{
    if(!m_ps) return;
    m_ps->NumOfAntx = NumOfAntx;
    m_ps->NumOfAnty = NumOfAnty;
    m_ps->Type = Type;
    m_ps->Dx = Dx;
    m_ps->Dy = Dy;
    m_ps->Theta = Theta;
    m_ps->Phi = Phi;

    // DesiredPhaseShift 数组
    if(!DesiredPhaseShiftVec.empty()) {
        m_ps->DesiredPhaseShift_Size = static_cast<int>(DesiredPhaseShiftVec.size());
        if(m_ps->DesiredPhaseShift == nullptr) {
            m_ps->DesiredPhaseShift = new double[m_ps->DesiredPhaseShift_Size];
        }
        std::copy(DesiredPhaseShiftVec.begin(), DesiredPhaseShiftVec.end(), m_ps->DesiredPhaseShift);
    }
}

// ============================================================================
// ModelSetup — Block 自行初始化，不调用 m_ps->Setup()
// ============================================================================

bool RADAR_PhaseShift_Block::ModelSetup()
{
    if(NumOfAntx < 1) NumOfAntx = 1;
    if(NumOfAnty < 1) NumOfAnty = 1;

    nAnt_ = NumOfAntx * NumOfAnty;
    if(nAnt_ < 1) nAnt_ = 1;

    phaseCacheRad_.assign(static_cast<size_t>(nAnt_), 0.0);
    outBusSize_ = nAnt_;
    inputFc_ = 0.0;

    // 预构建相位表
    buildPhaseTable_();

    return true;
}

// ============================================================================
// UpdateCharacterizationFrequency
// ============================================================================

void RADAR_PhaseShift_Block::UpdateCharacterizationFrequency()
{
    // 从输入端口获取 Fc 并传播到输出端口
    double fc = 0.0;
    if(GetInputPortCount() > 0) {
        BufferReader* inputReader = GetInputPort(GetInputPortName(0));
        if(inputReader) {
            fc = inputReader->getCharacterizationFrequency();
        }
    }
    inputFc_ = fc;

    // 传播到 output bus
    auto* outputBuffer = GetOutputPort(GetOutputPortName(0));
    if(outputBuffer) {
        outputBuffer->setCharacterizationFrequency(fc);
    }
}

// ============================================================================
// DataStreamRun — 固定步长相位偏移处理
// ============================================================================
//
// output bus 的 writeSize = 1（每个 bus 通道的速率），
//   框架自动遍历所有 bus 连接。
// AntPhase 的 writeSize = nAnt_，一次写出全部天线相位值。
// 每次 firing 处理全部天线。

bool RADAR_PhaseShift_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    // 读取 1 个 envelope 输入样本
    auto inputData = ReadInputData<EnvelopeSignal>(inputPortName);
    if(inputData.empty()) return true;

    const std::complex<double> xCx = inputData[0].complex();

    // 每次 firing 重建相位表（支持动态 theta/phi 输入）
    buildPhaseTable_();

    // 写入 AntPhase（全部 nAnt_ 个相位值一次性写出）
    {
        std::string antPhasePortName = GetOutputPortName(1);
        std::vector<double> phaseData(phaseCacheRad_.begin(), phaseCacheRad_.end());
        WriteOutputData(antPhasePortName, phaseData);
    }

    // 写入 output bus — 构建完整输出向量后一次性写出
    {
        const size_t busSize = static_cast<size_t>(outBusSize_);
        std::vector<EnvelopeSignal> outputData(busSize);

        for(size_t k = 0; k < busSize; ++k) {
            if(k < static_cast<size_t>(nAnt_)) {
                const double phase = phaseCacheRad_[k];
                const std::complex<double> yCx = xCx * phaseRotator_(phase);
                outputData[k] = EnvelopeSignal(yCx);
            } else {
                // 未映射的 lane 补零
                outputData[k] = EnvelopeSignal(std::complex<double>(0.0, 0.0));
            }
        }

        WriteOutputData(outputPortName, outputData);
    }

    // 传播载波频率
    UpdateCharacterizationFrequency();

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长相位偏移模式
// ============================================================================
//
// 时间流模式：输入累积到缓冲区，输出推入队列，逐通道写出
// 参考 TimeSynchronizer_Block 的实现模式

bool RADAR_PhaseShift_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    Buffer* outputBuffer = GetOutputPort(outputPortName);
    if(!outputBuffer) {
        return true;
    }

    // —— 步骤 1：读取输入并累积到缓冲区 ——
    auto inputData = ReadInputData<EnvelopeSignal>(inputPortName);
    for(const auto& sig : inputData) {
        m_inputBuffer.push_back(sig);
    }

    // —— 步骤 2：有输入时处理并推入输出队列 ——
    if(!m_inputBuffer.empty()) {
        // 每次 firing 重建相位表（支持动态 theta/phi 输入）
        buildPhaseTable_();

        // 写入 AntPhase（全部 nAnt_ 个相位值一次性写出）
        {
            std::string antPhasePortName = GetOutputPortName(1);
            std::vector<double> phaseData(phaseCacheRad_.begin(), phaseCacheRad_.end());
            WriteOutputData(antPhasePortName, phaseData);
        }

        // 处理每个输入样本，生成对应的输出向量
        while(!m_inputBuffer.empty()) {
            const std::complex<double> xCx = m_inputBuffer.front().complex();
            m_inputBuffer.pop_front();

            const size_t busSize = static_cast<size_t>(outBusSize_);
            std::vector<EnvelopeSignal> frameOutput(busSize);

            for(size_t k = 0; k < busSize; ++k) {
                if(k < static_cast<size_t>(nAnt_)) {
                    const double phase = phaseCacheRad_[k];
                    const std::complex<double> yCx = xCx * phaseRotator_(phase);
                    frameOutput[k] = EnvelopeSignal(yCx);
                } else {
                    // 未映射的 lane 补零
                    frameOutput[k] = EnvelopeSignal(std::complex<double>(0.0, 0.0));
                }
            }

            m_outputQueue.push(frameOutput);
        }

        // 传播载波频率
        UpdateCharacterizationFrequency();
    }

    // —— 步骤 3：从输出队列取一帧结果，逐通道写出 ——
    if(!m_outputQueue.empty()) {
        std::vector<EnvelopeSignal> outputFrame = m_outputQueue.front();
        m_outputQueue.pop();

        const size_t busSize = static_cast<size_t>(outBusSize_);
        for(size_t i = 0; i < busSize && i < outputFrame.size(); ++i) {
            outputBuffer->WriteDataToChannel(static_cast<int>(i), std::vector<EnvelopeSignal>{outputFrame[i]});
        }
    }

    return true;
}

// ============================================================================
// 相位表构建（从 RADAR_PhaseShift.cpp 复制）
// ============================================================================

void RADAR_PhaseShift_Block::buildPhaseTable_()
{
    if(nAnt_ <= 0) {
        nAnt_ = 1;
    }

    if(phaseCacheRad_.size() != static_cast<size_t>(nAnt_)) {
        phaseCacheRad_.assign(static_cast<size_t>(nAnt_), 0.0);
    }

    if(Type == RADAR_PhaseShift::DesiredPhaseShiftType)
    {
        for(int i = 0; i < nAnt_; ++i)
        {
            phaseCacheRad_[static_cast<size_t>(i)] = getDesiredPhaseRad_(i);
        }
        return;
    }

    const double thetaRad = getThetaRad_();
    const double phiRad = getPhiRad_();

    for(int ky = 0; ky < NumOfAnty; ++ky)
    {
        for(int kx = 0; kx < NumOfAntx; ++kx)
        {
            const int idx = ky * NumOfAntx + kx;
            if(idx >= 0 && idx < nAnt_)
            {
                phaseCacheRad_[static_cast<size_t>(idx)] = computePhaseRad_(kx, ky, thetaRad, phiRad);
            }
        }
    }
}

double RADAR_PhaseShift_Block::getThetaRad_()
{
    // 仅在端口已连接时从输入端口读取
    BufferReader* thetaReader = GetInputPort("InTheta");
    if(thetaReader && thetaReader->IsConnected()) {
        auto thetaData = ReadInputData<double>("InTheta");
        if(!thetaData.empty()) {
            return thetaData[0];
        }
    }

    // Theta 在界面上显示为 degrees，但 C++ 成员变量已经是 radians
    return Theta;
}

double RADAR_PhaseShift_Block::getPhiRad_()
{
    // 仅在端口已连接时从输入端口读取
    BufferReader* phiReader = GetInputPort("InPhi");
    if(phiReader && phiReader->IsConnected()) {
        auto phiData = ReadInputData<double>("InPhi");
        if(!phiData.empty()) {
            return phiData[0];
        }
    }

    // Phi 同 Theta，C++ 成员变量已经是 radians
    return Phi;
}

double RADAR_PhaseShift_Block::computePhaseRad_(int kx, int ky, double thetaRad, double phiRad) const
{
    // 相位公式：
    // theta(kx,ky) = (kx*dx/lambda)*sin(theta)*cos(phi)
    //              + (ky*dy/lambda)*sin(theta)*sin(phi)
    //
    // Dx / Dy 的单位已经是 wavelengths，AntPhase 输出单位为 radians
    // 因此需要乘以 2*pi
    const double sx = std::sin(thetaRad) * std::cos(phiRad);
    const double sy = std::sin(thetaRad) * std::sin(phiRad);

    const double cycles = static_cast<double>(kx) * Dx * sx
        + static_cast<double>(ky) * Dy * sy;

    return 2.0 * M_PI * cycles;
}

double RADAR_PhaseShift_Block::getDesiredPhaseRad_(int index) const
{
    if(index >= 0 && index < static_cast<int>(DesiredPhaseShiftVec.size()))
    {
        // DesiredPhaseShift 在界面上显示为 degrees
        // 但 C++ 内部值已经是 radians
        return DesiredPhaseShiftVec[static_cast<size_t>(index)];
    }

    return 0.0;
}

// ============================================================================
// 相位旋转
// ============================================================================

std::complex<double> RADAR_PhaseShift_Block::phaseRotator_(double phaseRad) const
{
    return std::complex<double>(std::cos(phaseRad), std::sin(phaseRad));
}

// ============================================================================
// 工具函数
// ============================================================================

double RADAR_PhaseShift_Block::deg2rad(double x)
{
    return x * M_PI / 180.0;
}

double RADAR_PhaseShift_Block::clampFinite(double x, double fallback)
{
    if(x != x) {
        return fallback;
    }
    if(!std::isfinite(x)) {
        return fallback;
    }
    return x;
}

// ============================================================================
// 枚举解析
// ============================================================================

RADAR_PhaseShift::PhaseShiftTypeEnum RADAR_PhaseShift_Block::ConvertStringToTypeEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "desiredphaseshift" || lower == "desiredphaseshifttype" || lower == "1") {
        return RADAR_PhaseShift::DesiredPhaseShiftType;
    }
    return RADAR_PhaseShift::Calculate_by_theta_and_phi;
}

// ============================================================================
// 数组参数解析
// ============================================================================

std::vector<double> RADAR_PhaseShift_Block::ParseDoubleArray(const std::string& str)
{
    std::vector<double> result;
    std::string s = TrimCopy(str);

    // 去除方括号
    if(!s.empty() && s.front() == '[') s = s.substr(1);
    if(!s.empty() && s.back() == ']') s.pop_back();

    std::stringstream ss(s);
    std::string token;
    while(std::getline(ss, token, ',')) {
        token = TrimCopy(token);
        if(token.empty()) continue;
        try {
            result.push_back(std::stod(token));
        } catch(...) {
            result.push_back(0.0);
        }
    }

    return result;
}
