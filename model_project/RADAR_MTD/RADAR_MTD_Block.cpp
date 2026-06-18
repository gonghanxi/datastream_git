#include "RADAR_MTD_Block.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fftw3.h>  // FFTW库头文件

#ifndef M_PI
#define M_PI 3.14159265358979323846
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

// 解析数组字符串，格式如 "[1,2,3,4]" 或 "1,2,3,4"
std::vector<double> ParseArrayString(const std::string& value)
{
    std::vector<double> result;
    std::string s = TrimCopy(value);

    // 去掉开头和结尾的方括号
    if (!s.empty() && s.front() == '[') {
        s = s.substr(1);
    }
    if (!s.empty() && s.back() == ']') {
        s = s.substr(0, s.length() - 1);
    }

    // 按逗号分割
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = TrimCopy(token);
        if (!token.empty()) {
            try {
                result.push_back(std::stod(token));
            } catch (...) {
                // 解析失败时使用默认值
                result.push_back(1.0);
            }
        }
    }

    return result;
}

// 备用FFT实现（递归Cooley-Tukey算法）
void BackupFFT(std::vector<std::complex<double>>& x)
{
    const size_t N = x.size();
    if (N <= 1) return;

    std::vector<std::complex<double>> even(N / 2);
    std::vector<std::complex<double>> odd(N / 2);

    for (size_t i = 0; i < N / 2; ++i) {
        even[i] = x[i * 2];
        odd[i] = x[i * 2 + 1];
    }

    BackupFFT(even);
    BackupFFT(odd);

    for (size_t k = 0; k < N / 2; ++k) {
        std::complex<double> t = std::polar(1.0, -2.0 * M_PI * k / N) * odd[k];
        x[k] = even[k] + t;
        x[k + N / 2] = even[k] - t;
    }
}
}

RADAR_MTD_Block::RADAR_MTD_Block(const std::string& name)
    : Block(name)
    , m_PRI(1e-4)
    , m_SampleRate(10e6)
    , m_NumOfPulse(8)
    , m_WindowType(RADAR_MTD::Rectangle)
    , m_Freq_Weight(nullptr)
    , m_Freq_Weight_Size(0)
    , m_WindowParameters(nullptr)
    , m_WindowParameters_Size(0)
    , m_samplesPerPulse(0)
    , m_totalSamples(0)
    , m_fft_plan(nullptr)
    , m_fftw_input(nullptr)
    , m_fftw_output(nullptr)
    , m_fftw_initialized(false)
    , m_inputCount(0)
    , m_outputCount(0)
    , m_lastOutput(0.0, 0.0)
{
}

RADAR_MTD_Block::~RADAR_MTD_Block()
{
    // 释放动态分配的内存
    if (m_Freq_Weight != nullptr) {
        delete[] m_Freq_Weight;
        m_Freq_Weight = nullptr;
    }
    if (m_WindowParameters != nullptr) {
        delete[] m_WindowParameters;
        m_WindowParameters = nullptr;
    }

    // 清理FFTW资源
    if (m_fft_plan != nullptr) {
        fftw_destroy_plan(m_fft_plan);
        m_fft_plan = nullptr;
    }
    if (m_fftw_input != nullptr) {
        fftw_free(m_fftw_input);
        m_fftw_input = nullptr;
    }
    if (m_fftw_output != nullptr) {
        fftw_free(m_fftw_output);
        m_fftw_output = nullptr;
    }
}

bool RADAR_MTD_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();

    // 计算每个脉冲的采样点数和总采样点数
    m_samplesPerPulse = static_cast<int>(std::round(m_PRI * m_SampleRate));
    m_totalSamples = m_NumOfPulse * m_samplesPerPulse;

    // 初始化FFTW
    m_fftw_initialized = InitializeFFTW();

    return true;
}

bool RADAR_MTD_Block::InitializeFFTW()
{
    // 清理现有的FFT计划
    if (m_fft_plan != nullptr) {
        fftw_destroy_plan(m_fft_plan);
        m_fft_plan = nullptr;
    }

    // 释放现有的FFTW数组
    if (m_fftw_input != nullptr) {
        fftw_free(m_fftw_input);
        m_fftw_input = nullptr;
    }
    if (m_fftw_output != nullptr) {
        fftw_free(m_fftw_output);
        m_fftw_output = nullptr;
    }

    // 分配FFTW输入输出数组
    int fft_size = m_NumOfPulse;
    m_fftw_input = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fft_size);
    m_fftw_output = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fft_size);

    // 检查内存分配是否成功
    if (m_fftw_input == nullptr || m_fftw_output == nullptr) {
        return false;
    }

    // 创建FFT计划
    m_fft_plan = fftw_plan_dft_1d(fft_size, m_fftw_input, m_fftw_output,
                                   FFTW_FORWARD, FFTW_ESTIMATE);

    return (m_fft_plan != nullptr);
}

bool RADAR_MTD_Block::ExternalFFT(std::vector<std::complex<double>>& x)
{
    // 检查FFTW资源是否已正确初始化
    if (m_fft_plan == nullptr || m_fftw_input == nullptr || m_fftw_output == nullptr) {
        return false;
    }

    const int N = x.size();
    if (N <= 0 || N != m_NumOfPulse) return false;  // 确保大小匹配

    // 将数据复制到FFTW输入数组
    for (int i = 0; i < N; ++i) {
        m_fftw_input[i][0] = x[i].real();
        m_fftw_input[i][1] = x[i].imag();
    }

    // 执行FFT变换
    fftw_execute(m_fft_plan);

    // 将结果复制回输出向量
    for (int i = 0; i < N; ++i) {
        x[i] = std::complex<double>(m_fftw_output[i][0], m_fftw_output[i][1]);
    }

    return true;
}

bool RADAR_MTD_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    // 步骤1：读取输入数据
    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(inputPortName);

    if (inputData.empty()) {
        return false; // 没有数据时直接返回
    }

    // 步骤2：重塑数据为脉冲矩阵格式（列优先，兼容MATLAB）
    std::vector<std::complex<double>> pulseMatrix(m_samplesPerPulse * m_NumOfPulse);
    for (int pulse = 0; pulse < m_NumOfPulse; ++pulse) {
        for (int sample = 0; sample < m_samplesPerPulse; ++sample) {
            int input_idx = pulse * m_samplesPerPulse + sample;        // 输入数据索引（行优先）
            int matrix_idx = sample * m_NumOfPulse + pulse;            // 矩阵索引（列优先）
            pulseMatrix[matrix_idx] = inputData[input_idx];
        }
    }

    // 步骤3：三脉冲对消器（MTI滤波器）
    std::vector<std::complex<double>> cancelledData(m_samplesPerPulse * m_NumOfPulse);

    if (m_NumOfPulse >= 3) {
        // 初始化前两个脉冲的处理结果为0
        for (int sample = 0; sample < m_samplesPerPulse; ++sample) {
            int idx1 = sample * m_NumOfPulse + 0;
            cancelledData[idx1] = std::complex<double>(0, 0);

            int idx2 = sample * m_NumOfPulse + 1;
            cancelledData[idx2] = std::complex<double>(0, 0);
        }

        // 应用三脉冲对消器公式：y[n] = x[n] - 2*x[n-1] + x[n-2]
        for (int pulse = 2; pulse < m_NumOfPulse; ++pulse) {
            for (int sample = 0; sample < m_samplesPerPulse; ++sample) {
                int idx_n = sample * m_NumOfPulse + pulse;
                int idx_n1 = sample * m_NumOfPulse + (pulse - 1);
                int idx_n2 = sample * m_NumOfPulse + (pulse - 2);

                cancelledData[idx_n] = pulseMatrix[idx_n]
                    - 2.0 * pulseMatrix[idx_n1]
                    + pulseMatrix[idx_n2];
            }
        }
    } else {
        // 脉冲数量不足3个，直接复制原始数据
        cancelledData = pulseMatrix;
    }

    // 步骤4：对每个距离门进行FFT处理（多普勒处理）
    std::vector<std::complex<double>> outputBuffer(m_totalSamples);

    for (int range_gate = 0; range_gate < m_samplesPerPulse; ++range_gate) {
        // 提取该距离门在所有脉冲上的数据
        std::vector<std::complex<double>> range_gate_data(m_NumOfPulse);
        for (int pulse = 0; pulse < m_NumOfPulse; ++pulse) {
            int idx = range_gate * m_NumOfPulse + pulse;
            range_gate_data[pulse] = cancelledData[idx];
        }

        // 尝试使用外部FFT库（FFTW）进行变换，如果失败则使用备用FFT
        bool fft_success = false;
        if (m_fftw_initialized) {
            fft_success = ExternalFFT(range_gate_data);
        }

        if (!fft_success) {
            // 如果外部FFT失败或未初始化，使用备用FFT
            BackupFFT(range_gate_data);
        }

        // 将FFT结果存储到输出缓冲区，恢复为行优先格式
        for (int pulse = 0; pulse < m_NumOfPulse; ++pulse) {
            int output_idx = pulse * m_samplesPerPulse + range_gate;
            outputBuffer[output_idx] = range_gate_data[pulse];
        }
    }

    // 步骤5：将处理结果写入输出端口
    WriteOutputData(outputPortName, outputBuffer);

    return true;
}

bool RADAR_MTD_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);
    BufferReader* master_reader = GetInputPort(inputPortName);
    std::vector<BufferReader*> channelReaders;
    auto bridge_readers = master_reader->GetBusConnections();
    //保证多输入同时读取数据
    for(const auto& bridge_reader : bridge_readers) {
        std::vector<double> inputData;
        bridge_reader.bridgeReader->ReadData(inputData);
        channelReaders.push_back(bridge_reader.bridgeReader);
        if(inputData.empty()) {
            //上游没产生，
            return true;
        }
        for(size_t i = 0; i < inputData.size();i++) {
            m_inputBuffer[bridge_reader.bridgeReader].push_back(inputData[i]);
        }
    }

    bool CanProcessData = !m_inputBuffer.empty();
    for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
        if(it->second.size() < static_cast<size_t>(m_totalSamples)) {
            CanProcessData = false;
            break;
        }
    }

    if(CanProcessData) {
        // 步骤2：重塑数据为脉冲矩阵格式（列优先，兼容MATLAB）

        std::vector<std::complex<double>> pulseMatrix(m_samplesPerPulse * m_NumOfPulse, 0.0);
        int channelIndex = 0;

        // 遍历所有通道
        for (const auto& reader : channelReaders) {
            auto it = m_inputBuffer.find(reader);
            if (it != m_inputBuffer.end()) {
                const std::vector<std::complex<double>>& channelData = it->second;

                // 将 channelData.size() 显式转换为 int
                int dataCount = std::min(static_cast<int>(channelData.size()),
                                         static_cast<int>(m_samplesPerPulse * m_NumOfPulse / channelReaders.size()));

                // 将通道数据填充到pulseMatrix
                for (int i = 0; i < dataCount; ++i) {
                    int pulse = i / m_samplesPerPulse;
                    int sample = i % m_samplesPerPulse;
                    int matrix_idx = sample * m_NumOfPulse + pulse;
                    pulseMatrix[matrix_idx] = channelData[i];
                }
            }
            channelIndex++;
        }

        // 步骤3：三脉冲对消器（MTI滤波器）
        std::vector<std::complex<double>> cancelledData(m_samplesPerPulse * m_NumOfPulse);

        if (m_NumOfPulse >= 3) {
            // 初始化前两个脉冲的处理结果为0
            for (int sample = 0; sample < m_samplesPerPulse; ++sample) {
                int idx1 = sample * m_NumOfPulse + 0;
                cancelledData[idx1] = std::complex<double>(0, 0);

                int idx2 = sample * m_NumOfPulse + 1;
                cancelledData[idx2] = std::complex<double>(0, 0);
            }

            // 应用三脉冲对消器公式：y[n] = x[n] - 2*x[n-1] + x[n-2]
            for (int pulse = 2; pulse < m_NumOfPulse; ++pulse) {
                for (int sample = 0; sample < m_samplesPerPulse; ++sample) {
                    int idx_n = sample * m_NumOfPulse + pulse;
                    int idx_n1 = sample * m_NumOfPulse + (pulse - 1);
                    int idx_n2 = sample * m_NumOfPulse + (pulse - 2);

                    cancelledData[idx_n] = pulseMatrix[idx_n]
                        - 2.0 * pulseMatrix[idx_n1]
                        + pulseMatrix[idx_n2];
                }
            }
        } else {
            // 脉冲数量不足3个，直接复制原始数据
            cancelledData = pulseMatrix;
        }

        // 步骤4：对每个距离门进行FFT处理（多普勒处理）
        std::vector<std::complex<double>> outputBuffer(m_totalSamples);

        for (int range_gate = 0; range_gate < m_samplesPerPulse; ++range_gate) {
            // 提取该距离门在所有脉冲上的数据
            std::vector<std::complex<double>> range_gate_data(m_NumOfPulse);
            for (int pulse = 0; pulse < m_NumOfPulse; ++pulse) {
                int idx = range_gate * m_NumOfPulse + pulse;
                range_gate_data[pulse] = cancelledData[idx];
            }

            // 尝试使用外部FFT库（FFTW）进行变换，如果失败则使用备用FFT
            bool fft_success = false;
            if (m_fftw_initialized) {
                fft_success = ExternalFFT(range_gate_data);
            }

            if (!fft_success) {
                // 如果外部FFT失败或未初始化，使用备用FFT
                BackupFFT(range_gate_data);
            }

            // 将FFT结果存储到输出缓冲区，恢复为行优先格式
            for (int pulse = 0; pulse < m_NumOfPulse; ++pulse) {
                int output_idx = pulse * m_samplesPerPulse + range_gate;
                outputBuffer[output_idx] = range_gate_data[pulse];
            }
        }
        for(const auto& val : outputBuffer) m_outputQueue.push(val);

        // 步骤5：将处理结果写入输出端口
        if(!m_outputQueue.empty()) {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;
            WriteOutputData(outputPortName, std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();
            qDebug() << "[RADAR_MTD_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << outputValue.imag();
        }
    }
    return true;
}

bool RADAR_MTD_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_MTD_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    // 创建算法实例并设置默认参数
    m_RADAR_MTD = std::make_unique<RADAR_MTD>();
    SetDefaultParameters();

    // 从参数系统获取参数值
    try { m_PRI = std::stod(getParameter("PRI").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PRI', using default value."); }
    try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    try { m_NumOfPulse = std::stoi(getParameter("NumOfPulse").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumOfPulse', using default value."); }
    try { m_WindowType = ConvertStringToSelectedWindowType(getParameter("WindowType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'WindowType', using default value."); }

    // 解析数组参数：频率权重
    try {
        std::string freqWeightStr = getParameter("Freq_Weight").Value;
        std::vector<double> freqWeightVec = ParseArrayString(freqWeightStr);

        // 释放原有内存并重新分配
        if (m_Freq_Weight != nullptr) {
            delete[] m_Freq_Weight;
        }

        m_Freq_Weight_Size = freqWeightVec.size();
        if (m_Freq_Weight_Size > 0) {
            m_Freq_Weight = new double[m_Freq_Weight_Size];
            for (int i = 0; i < m_Freq_Weight_Size; ++i) {
                m_Freq_Weight[i] = freqWeightVec[i];
            }
        } else {
            m_Freq_Weight = nullptr;
        }
    } catch (...) {
        // 解析失败时使用默认值：全1数组
        m_Freq_Weight_Size = m_NumOfPulse;
        m_Freq_Weight = new double[m_Freq_Weight_Size];
        for (int i = 0; i < m_Freq_Weight_Size; ++i) {
            m_Freq_Weight[i] = 1.0;
        }
    }

    // 解析数组参数：窗函数参数
    try {
        std::string windowParamsStr = getParameter("WindowParameters").Value;
        std::vector<double> windowParamsVec = ParseArrayString(windowParamsStr);

        // 释放原有内存并重新分配
        if (m_WindowParameters != nullptr) {
            delete[] m_WindowParameters;
        }

        m_WindowParameters_Size = windowParamsVec.size();
        if (m_WindowParameters_Size > 0) {
            m_WindowParameters = new double[m_WindowParameters_Size];
            for (int i = 0; i < m_WindowParameters_Size; ++i) {
                m_WindowParameters[i] = windowParamsVec[i];
            }
        } else {
            m_WindowParameters = nullptr;
        }
    } catch (...) {
        // 解析失败时使用默认值
        m_WindowParameters = nullptr;
        m_WindowParameters_Size = 0;
    }

    // 计算每个脉冲的采样点数和总采样点数
    m_samplesPerPulse = static_cast<int>(std::round(m_PRI * m_SampleRate));
    m_totalSamples = m_NumOfPulse * m_samplesPerPulse;

    // 设置模型参数（如果需要传递给模型）
    if (m_RADAR_MTD) {
        SetParameters();
    }

    // 添加输入输出端口
    // bus类型输入端口：readSize为单通道速率，不需要乘以通道数
    AddInputPort("input", m_RADAR_MTD->input, static_cast<size_t>(m_samplesPerPulse),
                 Block::DataType::DCOMPLEX_BUS);
    AddOutputPort("output", m_RADAR_MTD->output, static_cast<size_t>(m_totalSamples),
                  Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

void RADAR_MTD_Block::SetParameters()
{
    if (!m_RADAR_MTD) {
        return;
    }

    m_RADAR_MTD->PRI = m_PRI;
    m_RADAR_MTD->SampleRate = m_SampleRate;
    m_RADAR_MTD->NumOfPulse = m_NumOfPulse;
    m_RADAR_MTD->WindowType = m_WindowType;
    m_RADAR_MTD->Freq_Weight = m_Freq_Weight;
    m_RADAR_MTD->Freq_Weight_Size = m_Freq_Weight_Size;
    m_RADAR_MTD->WindowParameters = m_WindowParameters;
    m_RADAR_MTD->WindowParameters_Size = m_WindowParameters_Size;
}

RADAR_MTD::SelectedWindowType RADAR_MTD_Block::ConvertStringToSelectedWindowType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "rectangle" || lower == "0") {
        return RADAR_MTD::Rectangle;
    }
    if (lower == "bartlett" || lower == "1") {
        return RADAR_MTD::Bartlett;
    }
    if (lower == "hanning" || lower == "2") {
        return RADAR_MTD::Hanning;
    }
    if (lower == "hamming" || lower == "3") {
        return RADAR_MTD::Hamming;
    }
    if (lower == "blackman" || lower == "4") {
        return RADAR_MTD::Blackman;
    }
    if (lower == "steepblackman" || lower == "5") {
        return RADAR_MTD::SteepBlackman;
    }
    if (lower == "kaiser" || lower == "6") {
        return RADAR_MTD::Kaiser;
    }
    return RADAR_MTD::Rectangle;
}

void RADAR_MTD_Block::SetDefaultParameters()
{
    m_PRI = 1e-4;           // 0.1ms
    m_SampleRate = 10e6;    // 10MHz
    m_NumOfPulse = 8;       // 8个脉冲
    m_WindowType = RADAR_MTD::Rectangle;

    // 初始化频率权重数组（全1）
    m_Freq_Weight_Size = m_NumOfPulse;
    if (m_Freq_Weight != nullptr) {
        delete[] m_Freq_Weight;
    }
    m_Freq_Weight = new double[m_Freq_Weight_Size];
    for (int i = 0; i < m_Freq_Weight_Size; ++i) {
        m_Freq_Weight[i] = 1.0;
    }

    // 窗函数参数数组默认为空
    m_WindowParameters = nullptr;
    m_WindowParameters_Size = 0;

    // 计算采样点数
    m_samplesPerPulse = static_cast<int>(std::round(m_PRI * m_SampleRate));
    m_totalSamples = m_NumOfPulse * m_samplesPerPulse;
}
