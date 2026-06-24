#include "RADAR_Rx_Block.h"
#include <cmath>
#include <algorithm>

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

// 解析 complex<double> 参数：支持 "re" 或 "re,im" 或 "(re,im)" 格式
std::complex<double> ParseComplex(const std::string& value)
{
    std::string s = TrimCopy(value);
    if(!s.empty() && s.front() == '(') s = s.substr(1);
    if(!s.empty() && s.back() == ')') s.pop_back();

    auto commaPos = s.find(',');
    if(commaPos == std::string::npos) {
        return std::complex<double>(std::stod(s), 0.0);
    }
    double re = std::stod(s.substr(0, commaPos));
    double im = std::stod(s.substr(commaPos + 1));
    return std::complex<double>(re, im);
}

// 解析数组参数：支持 "[a, b, c]" 或 "a, b, c" 格式
std::vector<double> ParseDoubleArray(const std::string& value)
{
    std::string s = TrimCopy(value);
    if(!s.empty() && s.front() == '[') s = s.substr(1);
    if(!s.empty() && s.back() == ']') s.pop_back();

    std::vector<double> result;
    std::string token;
    for(char c : s) {
        if(c == ',') {
            std::string t = TrimCopy(token);
            if(!t.empty()) result.push_back(std::stod(t));
            token.clear();
        } else {
            token += c;
        }
    }
    std::string t = TrimCopy(token);
    if(!t.empty()) result.push_back(std::stod(t));
    return result;
}
}

// ============================================================================
// 构造函数 / 析构函数
// ============================================================================

RADAR_Rx_Block::RADAR_Rx_Block(const std::string& name)
    : Block(name)
{
}

RADAR_Rx_Block::~RADAR_Rx_Block()
{
    m_rx.reset();
}

// ============================================================================
// Setup — 仿真开始前重置
// ============================================================================

bool RADAR_Rx_Block::Setup()
{
    Block::Setup();
    while(!m_inputQueue.empty()) m_inputQueue.pop();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    m_outputCount = 0;
    return true;
}

// ============================================================================
// Run — 运行模式分发
// ============================================================================

bool RADAR_Rx_Block::Run()
{
    if(!CanProcess()) {
        return false;
    }

    if(IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

// ============================================================================
// Initialize — 模块初始化
// ============================================================================
//
// 创建算法实例并注册端口：
//  - 输入端口（1 个）：
//      RF_Signal : ENVELOPE_SIGNAL 类型，射频包络信号
//  - 输出端口（1 个）：
//      BB_Signal : CIRCULAR_BUFFER_DCOMPLEX 类型，基带复数信号
//  输入速率 = BB_DownSamplingRatio（变速率读取），输出速率 = 1

bool RADAR_Rx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_rx = std::make_unique<RADAR_Rx>();

    SetDefaultParameters();

    // ========== 解析基本参数 ==========
    try { m_TStep = std::stod(getParameter("TStep").Value); } catch (...) { LOG_WARN("Failed to parse 'TStep'."); }
    try { m_RF_Freq = std::stod(getParameter("RF_Freq").Value); } catch (...) { LOG_WARN("Failed to parse 'RF_Freq'."); }
    try {
        m_RF_Gain = ParseComplex(getParameter("RF_Gain").Value);
    } catch (...) { LOG_WARN("Failed to parse 'RF_Gain'."); }
    try { m_IF_Freq = std::stod(getParameter("IF_Freq").Value); } catch (...) { LOG_WARN("Failed to parse 'IF_Freq'."); }
    try {
        m_IF_Gain = ParseComplex(getParameter("IF_Gain").Value);
    } catch (...) { LOG_WARN("Failed to parse 'IF_Gain'."); }
    try { m_IF_SamplingRate = std::stod(getParameter("IF_SamplingRate").Value); } catch (...) { LOG_WARN("Failed to parse 'IF_SamplingRate'."); }
    try { m_BandWidth = std::stod(getParameter("BandWidth").Value); } catch (...) { LOG_WARN("Failed to parse 'BandWidth'."); }
    try { m_ADC_NBits = std::stoi(getParameter("ADC_NBits").Value); } catch (...) { LOG_WARN("Failed to parse 'ADC_NBits'."); }
    try { m_PhaseImbalance = std::stod(getParameter("PhaseImbalance").Value); } catch (...) { LOG_WARN("Failed to parse 'PhaseImbalance'."); }
    try { m_BB_DownSamplingRatio = std::stoi(getParameter("BB_DownSamplingRatio").Value); } catch (...) { LOG_WARN("Failed to parse 'BB_DownSamplingRatio'."); }
    try { m_RC_ExcessBW = std::stod(getParameter("RC_ExcessBW").Value); } catch (...) { LOG_WARN("Failed to parse 'RC_ExcessBW'."); }
    try { m_Out_CenterFreq = std::stod(getParameter("Out_CenterFreq").Value); } catch (...) { LOG_WARN("Failed to parse 'Out_CenterFreq'."); }

    // ========== 解析噪声参数 ==========
    try { m_NoiseFigure_RFGain = std::stod(getParameter("NoiseFigure_RFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'NoiseFigure_RFGain'."); }
    try { m_NoiseFigure_IFGain = std::stod(getParameter("NoiseFigure_IFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'NoiseFigure_IFGain'."); }
    try { m_NoiseFigure_Mixer = std::stod(getParameter("NoiseFigure_Mixer").Value); } catch (...) { LOG_WARN("Failed to parse 'NoiseFigure_Mixer'."); }

    // ========== 解析 RF 增益压缩参数 ==========
    try { m_GCType_RFGain = static_cast<int>(ConvertStringToGCType(getParameter("GCType_RFGain").Value)); } catch (...) { LOG_WARN("Failed to parse 'GCType_RFGain'."); }
    try { m_TOIout_RFGain = std::stod(getParameter("TOIout_RFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'TOIout_RFGain'."); }
    try { m_dBc1out_RFGain = std::stod(getParameter("dBc1out_RFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'dBc1out_RFGain'."); }
    try { m_PSat_RFGain = std::stod(getParameter("PSat_RFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'PSat_RFGain'."); }
    try { m_GCSat_RFGain = std::stod(getParameter("GCSat_RFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'GCSat_RFGain'."); }
    try { m_GComp_RFGain_Data = ParseDoubleArray(getParameter("GComp_RFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'GComp_RFGain'."); }

    // ========== 解析 IF 增益压缩参数 ==========
    try { m_GCType_IFGain = static_cast<int>(ConvertStringToGCType(getParameter("GCType_IFGain").Value)); } catch (...) { LOG_WARN("Failed to parse 'GCType_IFGain'."); }
    try { m_TOIout_IFGain = std::stod(getParameter("TOIout_IFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'TOIout_IFGain'."); }
    try { m_dBc1out_IFGain = std::stod(getParameter("dBc1out_IFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'dBc1out_IFGain'."); }
    try { m_PSat_IFGain = std::stod(getParameter("PSat_IFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'PSat_IFGain'."); }
    try { m_GCSat_IFGain = std::stod(getParameter("GCSat_IFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'GCSat_IFGain'."); }
    try { m_GComp_IFGain_Data = ParseDoubleArray(getParameter("GComp_IFGain").Value); } catch (...) { LOG_WARN("Failed to parse 'GComp_IFGain'."); }

    // 设置参数到算法实例
    SetParameters();

    // 调用算法 Setup 以初始化内部状态
    if(!ModelSetup()) return false;

    // 注册端口（变速率：输入 outRate，输出 1）
    AddInputPort("RF_Signal", m_rx->RF_Signal, static_cast<size_t>(m_outRate), DataType::ENVELOPE_SIGNAL);
    AddOutputPort("BB_Signal", m_rx->BB_Signal, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);

    // 设置抽取因子
    SetDecimationFactor(m_outRate);

    return true;
}

// ============================================================================
// ModelSetup — 算法初始化
// ============================================================================

bool RADAR_Rx_Block::ModelSetup()
{
    if(!m_rx->Setup()) {
        LOG_ERROR("RADAR_Rx Setup failed");
        return false;
    }

    // 从算法获取输入速率
    int decim = (m_BB_DownSamplingRatio > 0) ? m_BB_DownSamplingRatio : 1;
    m_outRate = decim;
    if(m_outRate < 1) m_outRate = 1;

    return true;
}

// ============================================================================
// DataStreamRun — 固定步长接收机处理模式
// ============================================================================
//
// 读取 outRate 个 RF envelope 输入样本，写入算法内部输入缓冲区，
// 调用算法 Run() 执行完整的接收机信号处理链路
// （RF Gain → Mixer → BPF → IF Gain → DDC → PhaseImbalance → ADC），
// 然后从算法输出缓冲区读取 1 个基带复数样本写出。

bool RADAR_Rx_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    // 读取 outRate 个 RF envelope 输入样本
    std::vector<EnvelopeSignal> inputData = ReadInputData<EnvelopeSignal>(inputPortName);
    if(inputData.empty()) {
        return true;
    }

    // 将输入样本写入算法内部缓冲区
    for(int i = 0; i < m_outRate && i < static_cast<int>(inputData.size()); ++i) {
        m_rx->RF_Signal[static_cast<unsigned>(i)] = inputData[static_cast<size_t>(i)];
    }

    // 调用算法 Run() 执行完整接收机处理链路
    m_rx->Run();
    m_rx->Advance();

    // 从算法输出缓冲区读取 1 个基带复数样本
    std::complex<double> y = m_rx->BB_Signal[0U];

    // 传播载波频率
    UpdateCharacterizationFrequency();

    // 写出输出数据
    WriteOutputData(outputPortName, std::vector<std::complex<double>>{y});

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长逐点累积接收机模式
// ============================================================================
//
// 工作于变步长仿真模式（VariableStep）。
// 每次 firing 读取输入样本，累积到 outRate 个后调用算法处理，
// 将结果推入队列，逐点写出。

bool RADAR_Rx_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    // 读取输入样本并累积到队列
    std::vector<EnvelopeSignal> inputData = ReadInputData<EnvelopeSignal>(inputPortName);
    for(const auto& sample : inputData) {
        m_inputQueue.push(sample);
    }

    // 如果有足够输入数据可处理（outRate 个）
    if(static_cast<int>(m_inputQueue.size()) >= m_outRate) {
        // 将 outRate 个输入样本写入算法缓冲区
        for(int i = 0; i < m_outRate; ++i) {
            m_rx->RF_Signal[static_cast<unsigned>(i)] = m_inputQueue.front();
            m_inputQueue.pop();
        }

        // 调用算法处理
        m_rx->Run();
        m_rx->Advance();

        // 将输出样本推入队列
        m_outputQueue.push(m_rx->BB_Signal[0U]);
    }

    // 从输出队列取一个样本写出
    if(!m_outputQueue.empty()) {
        std::complex<double> outValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;

        WriteOutputData(outputPortName, std::vector<std::complex<double>>{outValue});

        // 传播载波频率
        UpdateCharacterizationFrequency();
    }

    return true;
}

// ============================================================================
// UpdateCharacterizationFrequency — 传播载波频率
// ============================================================================

void RADAR_Rx_Block::UpdateCharacterizationFrequency()
{
    if(!m_rx) return;
    // 从输入端口获取 Fc
    double fc = 0.0;
    if(GetInputPortCount() > 0) {
        BufferReader* inputReader = GetInputPort(GetInputPortName(0));
        if(inputReader) {
            fc = inputReader->getCharacterizationFrequency();
        }
    }

    // 传播到输出端口
    auto* outputBuffer = GetOutputPort(GetOutputPortName(0));
    if(outputBuffer) {
        outputBuffer->setCharacterizationFrequency(fc);
    }
}

// ============================================================================
// 参数处理
// ============================================================================

void RADAR_Rx_Block::SetDefaultParameters()
{
    m_TStep = 0.0;
    m_RF_Freq = 1000000000.0;
    m_RF_Gain = std::complex<double>(1.0, 0.0);
    m_IF_Freq = 20000000.0;
    m_IF_Gain = std::complex<double>(1.0, 0.0);
    m_IF_SamplingRate = 50000000.0;
    m_BandWidth = 5000000.0;
    m_ADC_NBits = 8;
    m_PhaseImbalance = 0.0;
    m_BB_DownSamplingRatio = 5;
    m_RC_ExcessBW = 0.22;
    m_Out_CenterFreq = 0.0;

    m_NoiseFigure_RFGain = 0.0;
    m_NoiseFigure_IFGain = 0.0;
    m_NoiseFigure_Mixer = 0.0;

    m_GCType_RFGain = 0;  // none
    m_TOIout_RFGain = 3.0;
    m_dBc1out_RFGain = 1.0;
    m_PSat_RFGain = 1.0;
    m_GCSat_RFGain = 1.0;
    m_GComp_RFGain_Data = {0.0, 0.0, 0.0};

    m_GCType_IFGain = 0;  // none
    m_TOIout_IFGain = 3.0;
    m_dBc1out_IFGain = 1.0;
    m_PSat_IFGain = 1.0;
    m_GCSat_IFGain = 1.0;
    m_GComp_IFGain_Data = {0.0, 0.0, 0.0};

    m_outRate = 5;
    m_outputCount = 0;
}

void RADAR_Rx_Block::SetParameters()
{
    if(!m_rx) return;

    m_rx->TStep = m_TStep;
    m_rx->RF_Freq = m_RF_Freq;
    m_rx->RF_Gain = m_RF_Gain;
    m_rx->IF_Freq = m_IF_Freq;
    m_rx->IF_Gain = m_IF_Gain;
    m_rx->IF_SamplingRate = m_IF_SamplingRate;
    m_rx->BandWidth = m_BandWidth;
    m_rx->ADC_NBits = m_ADC_NBits;
    m_rx->PhaseImbalance = m_PhaseImbalance;
    m_rx->BB_DownSamplingRatio = m_BB_DownSamplingRatio;
    m_rx->RC_ExcessBW = m_RC_ExcessBW;
    m_rx->Out_CenterFreq = m_Out_CenterFreq;

    m_rx->NoiseFigure_RFGain = m_NoiseFigure_RFGain;
    m_rx->NoiseFigure_IFGain = m_NoiseFigure_IFGain;
    m_rx->NoiseFigure_Mixer = m_NoiseFigure_Mixer;

    m_rx->GCType_RFGain = static_cast<RADAR_Rx::SelectedGCType>(m_GCType_RFGain);
    m_rx->TOIout_RFGain = m_TOIout_RFGain;
    m_rx->dBc1out_RFGain = m_dBc1out_RFGain;
    m_rx->PSat_RFGain = m_PSat_RFGain;
    m_rx->GCSat_RFGain = m_GCSat_RFGain;
    if(!m_GComp_RFGain_Data.empty()) {
        m_rx->GComp_RFGain = m_GComp_RFGain_Data.data();
        m_rx->GComp_RFGain_Size = static_cast<int>(m_GComp_RFGain_Data.size());
    }

    m_rx->GCType_IFGain = static_cast<RADAR_Rx::SelectedGCType>(m_GCType_IFGain);
    m_rx->TOIout_IFGain = m_TOIout_IFGain;
    m_rx->dBc1out_IFGain = m_dBc1out_IFGain;
    m_rx->PSat_IFGain = m_PSat_IFGain;
    m_rx->GCSat_IFGain = m_GCSat_IFGain;
    if(!m_GComp_IFGain_Data.empty()) {
        m_rx->GComp_IFGain = m_GComp_IFGain_Data.data();
        m_rx->GComp_IFGain_Size = static_cast<int>(m_GComp_IFGain_Data.size());
    }
}

// ============================================================================
// 枚举解析
// ============================================================================

RADAR_Rx::SelectedGCType RADAR_Rx_Block::ConvertStringToGCType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "none" || lower == "0") {
        return RADAR_Rx::none;
    }
    if(lower == "toi" || lower == "1") {
        return RADAR_Rx::TOI;
    }
    if(lower == "dbc1" || lower == "2") {
        return RADAR_Rx::dBc1;
    }
    if(lower == "toi_dbc1" || lower == "3") {
        return RADAR_Rx::TOI_dBc1;
    }
    if(lower == "psat_gcsat_toi" || lower == "4") {
        return RADAR_Rx::PSat_GCSat_TOI;
    }
    if(lower == "psat_gcsat_dbc1" || lower == "5") {
        return RADAR_Rx::PSat_GCSat_dBc1;
    }
    if(lower == "psat_gcsat_toi_dbc1" || lower == "6") {
        return RADAR_Rx::PSat_GCSat_TOI_dBc1;
    }
    if(lower == "rappnonlinearity" || lower == "7") {
        return RADAR_Rx::RappNonlinearity;
    }
    if(lower == "gain_compression_vs_input_power" || lower == "8") {
        return RADAR_Rx::Gain_compression_vs_input_power;
    }
    if(lower == "am_am_and_ampm_vs_input_power" || lower == "9") {
        return RADAR_Rx::AM_AM_and_AMPM_vs_input_power;
    }
    return RADAR_Rx::none;
}
