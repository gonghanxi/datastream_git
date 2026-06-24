#include "RADAR_Tx_Block.h"
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
    // 去除括号
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

RADAR_Tx_Block::RADAR_Tx_Block(const std::string& name)
    : Block(name)
{
}

RADAR_Tx_Block::~RADAR_Tx_Block()
{
    // 确保算法实例先于数组存储销毁
    m_tx.reset();
}

// ============================================================================
// Setup — 仿真开始前重置
// ============================================================================

bool RADAR_Tx_Block::Setup()
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

bool RADAR_Tx_Block::Run()
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
//      BB_Signal : CIRCULAR_BUFFER_DCOMPLEX 类型，基带复数信号
//  - 输出端口（1 个）：
//      RF_Signal : ENVELOPE_SIGNAL 类型，射频包络信号
//  输入速率 = 1，输出速率 = BB_UpSamplingRatio * DAC_UpSamplingRatio（变速率）

bool RADAR_Tx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_tx = std::make_unique<RADAR_Tx>();

    SetDefaultParameters();

    // ========== 解析基本参数 ==========
    try { m_TStep = std::stod(getParameter("TStep").Value); } catch (...) { LOG_WARN("Failed to parse 'TStep'."); }
    try { m_RF_Freq = std::stod(getParameter("RF_Freq").Value); } catch (...) { LOG_WARN("Failed to parse 'RF_Freq'."); }
    try {
        std::complex<double> g = ParseComplex(getParameter("RF_Gain").Value);
        m_RF_Gain_Re = g.real();
        m_RF_Gain_Im = g.imag();
    } catch (...) { LOG_WARN("Failed to parse 'RF_Gain'."); }
    try { m_IF_Freq = std::stod(getParameter("IF_Freq").Value); } catch (...) { LOG_WARN("Failed to parse 'IF_Freq'."); }
    try {
        std::complex<double> g = ParseComplex(getParameter("IF_Gain").Value);
        m_IF_Gain_Re = g.real();
        m_IF_Gain_Im = g.imag();
    } catch (...) { LOG_WARN("Failed to parse 'IF_Gain'."); }
    try { m_IF_SamplingRate = std::stod(getParameter("IF_SamplingRate").Value); } catch (...) { LOG_WARN("Failed to parse 'IF_SamplingRate'."); }
    try { m_BandWidth = std::stod(getParameter("BandWidth").Value); } catch (...) { LOG_WARN("Failed to parse 'BandWidth'."); }
    try { m_In_CenterFreq = std::stod(getParameter("In_CenterFreq").Value); } catch (...) { LOG_WARN("Failed to parse 'In_CenterFreq'."); }
    try { m_BB_UpSamplingRatio = std::stoi(getParameter("BB_UpSamplingRatio").Value); } catch (...) { LOG_WARN("Failed to parse 'BB_UpSamplingRatio'."); }
    try { m_RC_ExcessBW = std::stod(getParameter("RC_ExcessBW").Value); } catch (...) { LOG_WARN("Failed to parse 'RC_ExcessBW'."); }
    try { m_PhaseImbalance = std::stod(getParameter("PhaseImbalance").Value); } catch (...) { LOG_WARN("Failed to parse 'PhaseImbalance'."); }
    try { m_DAC_NBits = std::stoi(getParameter("DAC_NBits").Value); } catch (...) { LOG_WARN("Failed to parse 'DAC_NBits'."); }
    try { m_DAC_UpSamplingRatio = std::stoi(getParameter("DAC_UpSamplingRatio").Value); } catch (...) { LOG_WARN("Failed to parse 'DAC_UpSamplingRatio'."); }

    // ========== 解析噪声参数 ==========
    try { m_NoiseFigure_RF_Gain = std::stod(getParameter("NoiseFigure_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'NoiseFigure_RF_Gain'."); }
    try { m_NoiseFigure_IF_Gain = std::stod(getParameter("NoiseFigure_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'NoiseFigure_IF_Gain'."); }
    try { m_NoiseFigure_Mixer = std::stod(getParameter("NoiseFigure_Mixer").Value); } catch (...) { LOG_WARN("Failed to parse 'NoiseFigure_Mixer'."); }

    // ========== 解析 RF 增益压缩参数 ==========
    try { m_GCType_RF_Gain = static_cast<int>(ConvertStringToGCType(getParameter("GCType_RF_Gain").Value)); } catch (...) { LOG_WARN("Failed to parse 'GCType_RF_Gain'."); }
    try { m_TOIout_RF_Gain = std::stod(getParameter("TOIout_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'TOIout_RF_Gain'."); }
    try { m_dBc1out_RF_Gain = std::stod(getParameter("dBc1out_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'dBc1out_RF_Gain'."); }
    try { m_PSat_RF_Gain = std::stod(getParameter("PSat_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'PSat_RF_Gain'."); }
    try { m_GCSat_RF_Gain = std::stod(getParameter("GCSat_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'GCSat_RF_Gain'."); }
    try { m_RappS_RF_Gain = std::stoi(getParameter("RappS_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'RappS_RF_Gain'."); }
    try { m_GComp_RF_Gain_Data = ParseDoubleArray(getParameter("GComp_RF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'GComp_RF_Gain'."); }

    // ========== 解析 IF 增益压缩参数 ==========
    try { m_GCType_IF_Gain = static_cast<int>(ConvertStringToGCType(getParameter("GCType_IF_Gain").Value)); } catch (...) { LOG_WARN("Failed to parse 'GCType_IF_Gain'."); }
    try { m_TOIout_IF_Gain = std::stod(getParameter("TOIout_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'TOIout_IF_Gain'."); }
    try { m_dBc1out_IF_Gain = std::stod(getParameter("dBc1out_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'dBc1out_IF_Gain'."); }
    try { m_PSat_IF_Gain = std::stod(getParameter("PSat_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'PSat_IF_Gain'."); }
    try { m_GCSat_IF_Gain = std::stod(getParameter("GCSat_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'GCSat_IF_Gain'."); }
    try { m_RappS_IF_Gain = std::stoi(getParameter("RappS_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'RappS_IF_Gain'."); }
    try { m_GComp_IF_Gain_Data = ParseDoubleArray(getParameter("GComp_IF_Gain").Value); } catch (...) { LOG_WARN("Failed to parse 'GComp_IF_Gain'."); }

    // 设置参数到算法实例
    SetParameters();

    // 调用算法 Setup 以初始化内部状态
    if(!ModelSetup()) return false;

    // 注册端口（变速率：输入 1，输出 outRate）
    AddInputPort("BB_Signal", m_tx->BB_Signal, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("RF_Signal", m_tx->RF_Signal, static_cast<size_t>(m_outRate), DataType::ENVELOPE_SIGNAL);

    // 设置输出端口写入大小
    GetOutputPort(GetOutputPortName(0))->SetWriteSize(static_cast<unsigned>(m_outRate));

    return true;
}

// ============================================================================
// ModelSetup — 算法初始化
// ============================================================================

bool RADAR_Tx_Block::ModelSetup()
{
    if(!m_tx->Setup()) {
        LOG_ERROR("RADAR_Tx Setup failed");
        return false;
    }

    // 从算法获取输出速率
    int bbUp = (m_BB_UpSamplingRatio > 0) ? m_BB_UpSamplingRatio : 1;
    int dacUp = (m_DAC_UpSamplingRatio > 0) ? m_DAC_UpSamplingRatio : 1;
    m_outRate = bbUp * dacUp;
    if(m_outRate < 1) m_outRate = 1;

    return true;
}

// ============================================================================
// DataStreamRun — 固定步长发射机处理模式
// ============================================================================
//
// 读取 1 个基带复数样本，写入算法内部输入缓冲区，
// 调用算法 Run() 执行完整的发射机信号处理链路
// （DUC → IF放大 → Mixer → RF放大 → FcChange），
// 然后从算法输出缓冲区读取 outRate 个包络样本写出。

bool RADAR_Tx_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    // 读取 1 个基带复数输入样本
    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(inputPortName);
    if(inputData.empty()) {
        return true;
    }

    // 将输入样本写入算法内部缓冲区
    m_tx->BB_Signal[0U] = inputData[0];

    // 调用算法 Run() 执行完整发射机处理链路
    m_tx->Run();
    m_tx->Advance();

    // 从算法输出缓冲区读取 outRate 个包络样本
    std::vector<EnvelopeSignal> outputData;
    outputData.reserve(static_cast<size_t>(m_outRate));
    for(int i = 0; i < m_outRate; ++i) {
        outputData.push_back(m_tx->RF_Signal[static_cast<unsigned>(i)]);
    }

    // 传播载波频率
    UpdateCharacterizationFrequency();

    // 写出输出数据
    WriteOutputData(outputPortName, outputData);

    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长逐点累积发射机模式
// ============================================================================
//
// 工作于变步长仿真模式（VariableStep）。
// 每次 firing 读取 1 个输入样本，调用算法处理，
// 将 outRate 个输出样本推入队列，逐点写出。

bool RADAR_Tx_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    // 读取输入样本并累积到队列
    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(inputPortName);
    for(const auto& sample : inputData) {
        m_inputQueue.push(sample);
    }

    // 如果有输入数据可处理
    if(!m_inputQueue.empty()) {
        std::complex<double> inputSample = m_inputQueue.front();
        m_inputQueue.pop();

        // 写入算法缓冲区并执行
        m_tx->BB_Signal[0U] = inputSample;
        m_tx->Run();
        m_tx->Advance();

        // 将输出样本推入队列
        for(int i = 0; i < m_outRate; ++i) {
            m_outputQueue.push(m_tx->RF_Signal[static_cast<unsigned>(i)]);
        }
    }

    // 从输出队列取一个样本写出
    if(!m_outputQueue.empty()) {
        EnvelopeSignal outValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;

        WriteOutputData(outputPortName, std::vector<EnvelopeSignal>{outValue});

        // 传播载波频率
        UpdateCharacterizationFrequency();
    }

    return true;
}

// ============================================================================
// UpdateCharacterizationFrequency — 传播载波频率
// ============================================================================

void RADAR_Tx_Block::UpdateCharacterizationFrequency()
{
    if(!m_tx) return;
    auto* outputBuffer = GetOutputPort(GetOutputPortName(0));
    if(outputBuffer) {
        outputBuffer->setCharacterizationFrequency(m_tx->RF_Freq);
    }
}

// ============================================================================
// 参数处理
// ============================================================================

void RADAR_Tx_Block::SetDefaultParameters()
{
    m_TStep = 0.0;
    m_RF_Freq = 1000000000.0;
    m_RF_Gain_Re = 1.0;
    m_RF_Gain_Im = 0.0;
    m_IF_Freq = 20000000.0;
    m_IF_Gain_Re = 1.0;
    m_IF_Gain_Im = 0.0;
    m_IF_SamplingRate = 50000000.0;
    m_BandWidth = 5000000.0;
    m_In_CenterFreq = 0.0;
    m_BB_UpSamplingRatio = 5;
    m_RC_ExcessBW = 0.22;
    m_PhaseImbalance = 0.0;
    m_DAC_NBits = 8;
    m_DAC_UpSamplingRatio = 1;

    m_NoiseFigure_RF_Gain = 0.0;
    m_NoiseFigure_IF_Gain = 0.0;
    m_NoiseFigure_Mixer = 0.0;

    m_GCType_RF_Gain = 0;  // none
    m_TOIout_RF_Gain = 0.1;
    m_dBc1out_RF_Gain = 0.01;
    m_PSat_RF_Gain = 0.032;
    m_GCSat_RF_Gain = 3.0;
    m_RappS_RF_Gain = 3;
    m_GComp_RF_Gain_Data = {0.0, 0.0, 0.0};

    m_GCType_IF_Gain = 0;  // none
    m_TOIout_IF_Gain = 0.1;
    m_dBc1out_IF_Gain = 0.01;
    m_PSat_IF_Gain = 0.032;
    m_GCSat_IF_Gain = 3.0;
    m_RappS_IF_Gain = 3;
    m_GComp_IF_Gain_Data = {0.0, 0.0, 0.0};

    m_outRate = 5;
    m_outputCount = 0;
}

void RADAR_Tx_Block::SetParameters()
{
    if(!m_tx) return;

    m_tx->TStep = m_TStep;
    m_tx->RF_Freq = m_RF_Freq;
    m_tx->RF_Gain = std::complex<double>(m_RF_Gain_Re, m_RF_Gain_Im);
    m_tx->IF_Freq = m_IF_Freq;
    m_tx->IF_Gain = std::complex<double>(m_IF_Gain_Re, m_IF_Gain_Im);
    m_tx->IF_SamplingRate = m_IF_SamplingRate;
    m_tx->BandWidth = m_BandWidth;
    m_tx->In_CenterFreq = m_In_CenterFreq;
    m_tx->BB_UpSamplingRatio = m_BB_UpSamplingRatio;
    m_tx->RC_ExcessBW = m_RC_ExcessBW;
    m_tx->PhaseImbalance = m_PhaseImbalance;
    m_tx->DAC_NBits = m_DAC_NBits;
    m_tx->DAC_UpSamplingRatio = m_DAC_UpSamplingRatio;

    m_tx->NoiseFigure_RF_Gain = m_NoiseFigure_RF_Gain;
    m_tx->NoiseFigure_IF_Gain = m_NoiseFigure_IF_Gain;
    m_tx->NoiseFigure_Mixer = m_NoiseFigure_Mixer;

    m_tx->GCType_RF_Gain = static_cast<RADAR_Tx::SelectedGCType>(m_GCType_RF_Gain);
    m_tx->TOIout_RF_Gain = m_TOIout_RF_Gain;
    m_tx->dBc1out_RF_Gain = m_dBc1out_RF_Gain;
    m_tx->PSat_RF_Gain = m_PSat_RF_Gain;
    m_tx->GCSat_RF_Gain = m_GCSat_RF_Gain;
    m_tx->RappS_RF_Gain = m_RappS_RF_Gain;
    if(!m_GComp_RF_Gain_Data.empty()) {
        m_tx->GComp_RF_Gain = m_GComp_RF_Gain_Data.data();
        m_tx->GComp_RF_Gain_Size = static_cast<int>(m_GComp_RF_Gain_Data.size());
    }

    m_tx->GCType_IF_Gain = static_cast<RADAR_Tx::SelectedGCType>(m_GCType_IF_Gain);
    m_tx->TOIout_IF_Gain = m_TOIout_IF_Gain;
    m_tx->dBc1out_IF_Gain = m_dBc1out_IF_Gain;
    m_tx->PSat_IF_Gain = m_PSat_IF_Gain;
    m_tx->GCSat_IF_Gain = m_GCSat_IF_Gain;
    m_tx->RappS_IF_Gain = m_RappS_IF_Gain;
    if(!m_GComp_IF_Gain_Data.empty()) {
        m_tx->GComp_IF_Gain = m_GComp_IF_Gain_Data.data();
        m_tx->GComp_IF_Gain_Size = static_cast<int>(m_GComp_IF_Gain_Data.size());
    }
}

// ============================================================================
// 枚举解析
// ============================================================================

RADAR_Tx::SelectedGCType RADAR_Tx_Block::ConvertStringToGCType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "none" || lower == "0") {
        return RADAR_Tx::none;
    }
    if(lower == "toi" || lower == "1") {
        return RADAR_Tx::TOI;
    }
    if(lower == "dbc1" || lower == "2") {
        return RADAR_Tx::dBc1;
    }
    if(lower == "toi_dbc1" || lower == "3") {
        return RADAR_Tx::TOI_dBc1;
    }
    if(lower == "psat_gcsat_toi" || lower == "4") {
        return RADAR_Tx::PSat_GCSat_TOI;
    }
    if(lower == "psat_gcsat_dbc1" || lower == "5") {
        return RADAR_Tx::PSat_GCSat_dBc1;
    }
    if(lower == "psat_gcsat_toi_dbc1" || lower == "6") {
        return RADAR_Tx::PSat_GCSat_TOI_dBc1;
    }
    if(lower == "rappnonlinearity" || lower == "7") {
        return RADAR_Tx::RappNonlinearity;
    }
    if(lower == "gain_compression_vs_input_power" || lower == "8") {
        return RADAR_Tx::Gain_compression_vs_input_power;
    }
    if(lower == "am_am_and_ampm_vs_input_power" || lower == "9") {
        return RADAR_Tx::AM_AM_and_AMPM_vs_input_power;
    }
    return RADAR_Tx::none;
}
