#include "RADAR_Tx_Synthesis_Block.h"
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

RADAR_Tx_Synthesis_Block::RADAR_Tx_Synthesis_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_Tx_Synthesis_Block::Setup()
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

bool RADAR_Tx_Synthesis_Block::Run()
{
    if(!CanProcess()) {
        return false;
    }

    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

// ============================================================================
// Initialize
// ============================================================================
//
// 创建算法实例并注册端口：
//  - 输入端口：
//      input   : ENVELOPE_BUS 类型，多路 envelope 输入，bus 宽度 N=NumOfAntx*NumOfAnty
//      InTheta : CIRCULAR_BUFFER_DOUBLE，可选，单位 radians
//      InPhi   : CIRCULAR_BUFFER_DOUBLE，可选，单位 radians
//  - 输出端口：
//      output  : ENVELOPE_SIGNAL 类型，单路 envelope 输出
//      AntPhase: CIRCULAR_BUFFER_DOUBLE，输出 N 个相位值

bool RADAR_Tx_Synthesis_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_algo = std::make_unique<RADAR_Tx_Synthesis>();

    SetDefaultParameters();

    // ========== 解析参数 ==========
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

    // ========== 注册端口 ==========

    // 输入 bus：每个通道速率 = 1，框架自动遍历所有 bus 连接
    AddInputPort("input", m_algo->input, 1, DataType::ENVELOPE_BUS);
    AddInputPort("InTheta", m_algo->InTheta, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("InPhi", m_algo->InPhi, 1, DataType::CIRCULAR_BUFFER_DOUBLE);

    // 输出：单路 envelope
    AddOutputPort("output", m_algo->output, 1, DataType::ENVELOPE_SIGNAL);

    // AntPhase：N 个 double 输出
    AddOutputPort("AntPhase", m_algo->AntPhase, static_cast<size_t>(nAnt_), DataType::CIRCULAR_BUFFER_DOUBLE);

    // 设置输出端口写入大小
    GetOutputPort(GetOutputPortName(0))->SetWriteSize(1);

    return true;
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_Tx_Synthesis_Block::SetDefaultParameters()
{
    NumOfAntx = 4;
    NumOfAnty = 4;
    Type = RADAR_Tx_Synthesis::Calculate_by_theta_and_phi;
    Dx = 0.5;
    Dy = 0.5;
    Theta = 0.0;
    Phi = 0.0;
    DesiredPhaseShiftVec.clear();
}

// ============================================================================
// SetParameters — 将 Block 参数同步到算法实例（仅用于端口注册）
// ============================================================================

void RADAR_Tx_Synthesis_Block::SetParameters()
{
    if(!m_algo) return;
    m_algo->NumOfAntx = NumOfAntx;
    m_algo->NumOfAnty = NumOfAnty;
    m_algo->Type = Type;
    m_algo->Dx = Dx;
    m_algo->Dy = Dy;
    m_algo->Theta = Theta;
    m_algo->Phi = Phi;

    // DesiredPhaseShift 数组
    if(!DesiredPhaseShiftVec.empty()) {
        m_algo->DesiredPhaseShift_Size = static_cast<int>(DesiredPhaseShiftVec.size());
        if(m_algo->DesiredPhaseShift == nullptr) {
            m_algo->DesiredPhaseShift = new double[m_algo->DesiredPhaseShift_Size];
        }
        std::copy(DesiredPhaseShiftVec.begin(), DesiredPhaseShiftVec.end(), m_algo->DesiredPhaseShift);
    }
}

// ============================================================================
// ModelSetup — Block 自行初始化，不调用 m_algo->Setup()
// ============================================================================

bool RADAR_Tx_Synthesis_Block::ModelSetup()
{
    if(NumOfAntx < 1) NumOfAntx = 1;
    if(NumOfAnty < 1) NumOfAnty = 1;

    nAnt_ = NumOfAntx * NumOfAnty;
    if(nAnt_ < 1) nAnt_ = 1;

    phaseCacheRad_.assign(static_cast<size_t>(nAnt_), 0.0);
    inputBusSize_ = 0;
    inputFc_ = 0.0;

    // 预构建相位表
    buildPhaseTable_();

    return true;
}

// ============================================================================
// UpdateCharacterizationFrequency
// ============================================================================

void RADAR_Tx_Synthesis_Block::UpdateCharacterizationFrequency()
{
    // 从输入 bus 端口获取 Fc 并传播到输出端口
    double fc = 0.0;
    if(GetInputPortCount() > 0) {
        BufferReader* inputReader = GetInputPort(GetInputPortName(0));
        if(inputReader) {
            fc = inputReader->getCharacterizationFrequency();
        }
    }
    inputFc_ = fc;

    // 传播到 output 端口
    auto* outputBuffer = GetOutputPort(GetOutputPortName(0));
    if(outputBuffer) {
        outputBuffer->setCharacterizationFrequency(fc);
    }
}

// ============================================================================
// DataStreamRun — 固定步长天线阵列合成处理
// ============================================================================
//
// 从 input bus 读取 N 路 envelope 输入，
// 对每路信号施加对应相位旋转后进行相干求和，
// 输出 1 路 envelope 和 N 个 AntPhase 值。
//
// 算法公式：y(t) = sum_k x_k(t) * exp(j * A_k)
// 不做归一化（帮助文档未给出归一化参数）。

bool RADAR_Tx_Synthesis_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    // 读取 input bus 的所有通道 envelope 数据
    // 框架自动遍历所有 bus 连接，返回展平向量
    auto inputData = ReadInputData<EnvelopeSignal>(inputPortName);
    if(inputData.empty()) return true;

    inputBusSize_ = static_cast<int>(inputData.size());

    // 每次 firing 重建相位表（支持动态 theta/phi 输入）
    buildPhaseTable_();

    // 相干求和：y = sum_k x_k * exp(j * phase_k)
    std::complex<double> acc(0.0, 0.0);

    const int nRead = std::min(inputBusSize_, nAnt_);
    for(int k = 0; k < nRead; ++k) {
        const std::complex<double> xCx = inputData[static_cast<size_t>(k)].complex();
        acc += xCx * phaseRotator_(phaseCacheRad_[static_cast<size_t>(k)]);
    }

    // 写出 output（单路 envelope）
    EnvelopeSignal y(acc);
    WriteOutputData(outputPortName, std::vector<EnvelopeSignal>{y});

    // 写出 AntPhase（全部 nAnt_ 个相位值一次性写出）
    {
        std::string antPhasePortName = GetOutputPortName(1);
        std::vector<double> phaseData(phaseCacheRad_.begin(), phaseCacheRad_.end());
        WriteOutputData(antPhasePortName, phaseData);
    }

    // 传播载波频率
    UpdateCharacterizationFrequency();

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长天线阵列合成模式
// ============================================================================
//
// 时间流模式：输入累积到缓冲区，输出推入队列，逐点写出。
// 参考 RADAR_PhaseShift_Block 的 TimeDrivenRun 实现模式。

bool RADAR_Tx_Synthesis_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    Buffer* outputBuffer = GetOutputPort(outputPortName);
    if(!outputBuffer) {
        return true;
    }

    // —— 步骤 1：读取输入 bus 数据并累积到缓冲区 ——
    auto inputData = ReadInputData<EnvelopeSignal>(inputPortName);
    if(!inputData.empty()) {
        inputBusSize_ = static_cast<int>(inputData.size());

        // 将一帧 bus 数据作为整体存入输入缓冲区
        m_inputBuffer.push_back(inputData);
    }

    // —— 步骤 2：有输入时处理并推入输出队列 ——
    if(!m_inputBuffer.empty()) {
        // 每次 firing 重建相位表（支持动态 theta/phi 输入）
        buildPhaseTable_();

        // 写出 AntPhase（全部 nAnt_ 个相位值一次性写出）
        {
            std::string antPhasePortName = GetOutputPortName(1);
            std::vector<double> phaseData(phaseCacheRad_.begin(), phaseCacheRad_.end());
            WriteOutputData(antPhasePortName, phaseData);
        }

        // 处理每帧输入，生成合成输出
        while(!m_inputBuffer.empty()) {
            const auto& frame = m_inputBuffer.front();

            // 相干求和
            std::complex<double> acc(0.0, 0.0);
            const int frameSize = static_cast<int>(frame.size());
            const int nRead = std::min(frameSize, nAnt_);
            for(int k = 0; k < nRead; ++k) {
                const std::complex<double> xCx = frame[static_cast<size_t>(k)].complex();
                acc += xCx * phaseRotator_(phaseCacheRad_[static_cast<size_t>(k)]);
            }

            m_outputQueue.push(EnvelopeSignal(acc));
            m_inputBuffer.pop_front();
        }

        // 传播载波频率
        UpdateCharacterizationFrequency();
    }

    // —— 步骤 3：从输出队列取一个样本写出 ——
    if(!m_outputQueue.empty()) {
        EnvelopeSignal outValue = m_outputQueue.front();
        m_outputQueue.pop();

        WriteOutputData(outputPortName, std::vector<EnvelopeSignal>{outValue});
    }

    return true;
}

// ============================================================================
// 相位表构建（从 RADAR_Tx_Synthesis.cpp 移植）
// ============================================================================

void RADAR_Tx_Synthesis_Block::buildPhaseTable_()
{
    if(nAnt_ <= 0) {
        nAnt_ = 1;
    }

    if(phaseCacheRad_.size() != static_cast<size_t>(nAnt_)) {
        phaseCacheRad_.assign(static_cast<size_t>(nAnt_), 0.0);
    }

    if(Type == RADAR_Tx_Synthesis::DesiredPhaseShiftType)
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

double RADAR_Tx_Synthesis_Block::getThetaRad_()
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

double RADAR_Tx_Synthesis_Block::getPhiRad_()
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

double RADAR_Tx_Synthesis_Block::computePhaseRad_(int kx, int ky, double thetaRad, double phiRad) const
{
    // 相位公式与 RADAR_PhaseShift 一致：
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

double RADAR_Tx_Synthesis_Block::getDesiredPhaseRad_(int index) const
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

std::complex<double> RADAR_Tx_Synthesis_Block::phaseRotator_(double phaseRad) const
{
    return std::complex<double>(std::cos(phaseRad), std::sin(phaseRad));
}

// ============================================================================
// 工具函数
// ============================================================================

double RADAR_Tx_Synthesis_Block::deg2rad(double x)
{
    return x * M_PI / 180.0;
}

double RADAR_Tx_Synthesis_Block::clampFinite(double x, double fallback)
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

RADAR_Tx_Synthesis::PhaseShiftTypeEnum RADAR_Tx_Synthesis_Block::ConvertStringToTypeEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "desiredphaseshift" || lower == "desiredphaseshifttype" || lower == "1") {
        return RADAR_Tx_Synthesis::DesiredPhaseShiftType;
    }
    return RADAR_Tx_Synthesis::Calculate_by_theta_and_phi;
}

// ============================================================================
// 数组参数解析
// ============================================================================

std::vector<double> RADAR_Tx_Synthesis_Block::ParseDoubleArray(const std::string& str)
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
