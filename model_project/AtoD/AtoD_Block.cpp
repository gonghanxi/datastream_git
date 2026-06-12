#include "AtoD_Block.h"

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

AtoD_Block::AtoD_Block(const std::string& name)
    : Block(name)
{
}

bool AtoD_Block::parseArrayString(const std::string &arrayStr, std::vector<double> &outArray)
{
        outArray.clear();

    std::string str = arrayStr;
    // 去除首尾空格
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return false;
    size_t end = str.find_last_not_of(" \t\n\r");
    str = str.substr(start, end - start + 1);

    // 检查是否是数组格式
    if (str.empty() || str.front() != '[' || str.back() != ']') {
        return false;
    }

    // 去除外层括号
    std::string content = str.substr(1, str.length() - 2);

    // 去除首尾空格
    start = content.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        // 空数组
        return true;
    }
    end = content.find_last_not_of(" \t\n\r");
    content = content.substr(start, end - start + 1);

    // 按逗号分割
    std::stringstream ss(content);
    std::string item;

    while (std::getline(ss, item, ',')) {
        // 去除空格
        start = item.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue;
        end = item.find_last_not_of(" \t\n\r");
        item = item.substr(start, end - start + 1);

        if (!item.empty()) {
            try {
                int value = std::stoi(item);
                outArray.push_back(value);
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to parse array element: ", item, " - ", e.what());
                return false;
            }
        }
    }

    return true;
}

void AtoD_Block::SetDefaultParamters()
{
    m_NBits = 8;
    m_VRef = 1.0;
    m_OutputDigitalFormat = AtoD::Offset_binary;
    m_DistortionModel = AtoD::Jitter_INL_DNL;
    m_EnableJitter = AtoD::Jitter_No;
    m_RJrms = 0.0;
    m_PhaseNoiseData = nullptr;
    m_PhaseNoiseDataSize = 0;
    m_PN_Type = AtoD::Random_PN;

    m_INL = 0.0;
    m_DNL = 0.0;

    m_ENOB = 7;
    m_SNR_dB = 60.0;
    m_H2_dBc = -400.0;
    m_H3_dBc = -400.0;
    m_H4_dBc = -400.0;
    m_H5_dBc = -400.0;

    m_SINAD_dB = 60.0;
    m_SFDR_dBc = 70.0;
    m_FFT_Size = AtoD::FFT_2_14;

    m_SNR_Model = AtoD::Quantization_and_Jitter;
    m_ThermalNoise_SNR_dBFS = 63;
    m_CenterFreq = 100.0e6;
    m_Level_dBFS = 0.0;

    m_ConversionType = AtoD::Clocked;
    m_Clock = 0.2e6;
    m_Phase = 0.0;

    m_DownsampleFactor = 1;
    m_DownsamplePhase = 0;
    m_AntiAliasingFilter = AtoD::AA_OFF;
    m_ExcessBW = 0.5;

}

void AtoD_Block::SetParameters()
{
    m_PhaseNoiseData = primdata.data();
    m_PhaseNoiseDataSize =  static_cast<double>(primdata.size());
    if(!m_atod) {
        return;
    }
    m_atod->NBits = m_NBits;
    m_atod->VRef = m_VRef;
    m_atod->OutputDigitalFormat = m_OutputDigitalFormat;
    m_atod->DistortionModel = m_DistortionModel;
    m_atod->EnableJitter = m_EnableJitter;
    m_atod->RJrms = m_RJrms;
    m_atod->PhaseNoiseData = m_PhaseNoiseData;
    m_atod->PhaseNoiseDataSize = m_PhaseNoiseDataSize;


    m_atod->PN_Type = m_PN_Type;

    m_atod->INL = m_INL;
    m_atod->DNL = m_DNL;

    m_atod->ENOB = m_ENOB;
    m_atod->SNR_dB = m_SNR_dB;
    m_atod->H2_dBc = m_H2_dBc;
    m_atod->H3_dBc = m_H3_dBc;
    m_atod->H4_dBc = m_H4_dBc;
    m_atod->H5_dBc = m_H5_dBc;

    m_atod->SINAD_dB = m_SINAD_dB;
    m_atod->SFDR_dBc = m_SFDR_dBc;
    m_atod->FFT_Size = m_FFT_Size;

    m_atod->SNR_Model = m_SNR_Model;
    m_atod->ThermalNoise_SNR_dBFS = m_ThermalNoise_SNR_dBFS;
    m_atod->CenterFreq = m_CenterFreq;
    m_atod->Level_dBFS = m_Level_dBFS;

    m_atod->ConversionType = m_ConversionType;
    m_atod->Clock = m_Clock;
    m_atod->Phase = m_Phase;

    m_atod->DownsampleFactor = m_DownsampleFactor;
    m_atod->DownsamplePhase = m_DownsamplePhase;
    m_atod->AntiAliasingFilter = m_AntiAliasingFilter;
    m_atod->ExcessBW = m_ExcessBW;
}

bool AtoD_Block::Setup()
{
    Block::Setup();
    m_inputBuffer.clear();
    while (!m_aoutQueue.empty()) m_aoutQueue.pop();
    while (!m_diQueue.empty()) m_diQueue.pop();
    while (!m_dqQueue.empty()) m_dqQueue.pop();
    return true;
}

bool AtoD_Block::Run()
{
    if (IsVariableStepMode() || m_inputRate > 1) return TimeDrivenRun();
    return DataStreamRun();
}

bool AtoD_Block::DataStreamRun()
{
    // 获取输入输出端口名称
    std::string A_INPortName = GetInputPortName(0);
    std::string A_outPortName = GetOutputPortName(0);
    std::string D_IPortName = GetOutputPortName(1);
    std::string D_QPortName = GetOutputPortName(2);
    qDebug() << "Atod_Block::Run - inputData: 1111";

     auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(A_INPortName);
     if(inputData.empty()) {
         qDebug() << "inputData is empty ";
         if(IsVariableStepMode()) {
             return true;
         }
         return false;
     }
     qDebug() << "Atod_Block::Run - inputData: 2222" << inputData.size();
     m_atod->A_Input = inputData;
     qDebug() << "Atod_Block::Run - inputData: 3333" << inputData.size();

     if (!m_atod->Run()) {
         return false;
     }
     std::vector<SystemVueModelBuilder::EnvelopeSignal> outputA_OUTData;
     std::vector<double> outputD_IData;
     std::vector<double> outputD_QData;

     outputA_OUTData.push_back(m_atod->A_out[0]);
     outputD_IData.push_back(m_atod->D_I[0]);
     outputD_QData.push_back(m_atod->D_Q[0]);

     qDebug() << "outputA_OUTDataread:: " << outputA_OUTData[0].real();
     qDebug() << "outputA_OUTDataimag:: " << outputA_OUTData[0].imag();


     WriteOutputData(A_outPortName, outputA_OUTData);
     WriteOutputData(D_IPortName, outputD_IData);
     WriteOutputData(D_QPortName, outputD_QData);

     if (m_atod) {
         m_atod->Advance();
     }
     return true;
}

bool AtoD_Block::TimeDrivenRun()
{
    // ---- 1. 累积输入 ----
    std::string A_INPortName = GetInputPortName(0);
    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(A_INPortName);
    if (inputData.empty()) {
        if (IsVariableStepMode()) {
            return true;
        }
        return false;
    }
    for (size_t i = 0; i < inputData.size(); ++i) {
        m_inputBuffer.push_back(inputData[i]);
    }

    // ---- 2. 处理：每累计 m_inputRate 个输入 → 运行一次算法 → 3路输出入队 ----
    if (static_cast<int>(m_inputBuffer.size()) >= m_inputRate)
    {
        std::vector<SystemVueModelBuilder::EnvelopeSignal> batch(
            m_inputBuffer.begin(), m_inputBuffer.begin() + m_inputRate);
        m_atod->A_Input = batch;

        if (!m_atod->Run()) {
            return false;
        }

        m_aoutQueue.push(m_atod->A_out[0]);
        m_diQueue.push(static_cast<double>(m_atod->D_I[0]));
        m_dqQueue.push(static_cast<double>(m_atod->D_Q[0]));

        if (m_atod) {
            m_atod->Advance();
        }

        m_inputBuffer.clear();
    }

    // ---- 3. 逐点输出 ----
    std::string A_outPortName = GetOutputPortName(0);
    std::string D_IPortName  = GetOutputPortName(1);
    std::string D_QPortName  = GetOutputPortName(2);

    if (!m_aoutQueue.empty())
    {
        std::vector<SystemVueModelBuilder::EnvelopeSignal> outA{ m_aoutQueue.front() };
        m_aoutQueue.pop();
        WriteOutputData(A_outPortName, outA);
    }
    if (!m_diQueue.empty())
    {
        std::vector<double> outI{ m_diQueue.front() };
        m_diQueue.pop();
        WriteOutputData(D_IPortName, outI);
    }
    if (!m_dqQueue.empty())
    {
        std::vector<double> outQ{ m_dqQueue.front() };
        m_dqQueue.pop();
        WriteOutputData(D_QPortName, outQ);
    }

    return true;
}

bool AtoD_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_atod = std::make_unique<AtoD>();

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_NBits = std::stod(getParameter("NBits").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NBits', using default value."); }
    try { m_VRef = std::stod(getParameter("VRef").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'VRef', using default value."); }
    try { m_OutputDigitalFormat = ConvertStringToOutputDigitalFormatEnum(getParameter("OutputDigitalFormat").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'OutputDigitalFormat', using default value."); }
    try { m_DistortionModel = ConvertStringToDistortionModelEnum(getParameter("DistortionModel").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DistortionModel', using default value."); }
    try { m_EnableJitter = ConvertStringToEnableJitterEnum(getParameter("EnableJitter").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'EnableJitter', using default value."); }
    try { m_RJrms = std::stod(getParameter("RJrms").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'RJrms', using default value."); }
    try {
        std::string PrimString = getParameter("PhaseNoiseData").Value;
        parseArrayString(PrimString, primdata);
    } catch(...) { LOG_WARN("Failed to parse parameter 'PhaseNoiseData', using default value."); }
    try { m_PN_Type = ConvertStringToPN_TypeEnum(getParameter("PN_Type").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PN_Type', using default value."); }

    try { m_INL = std::stod(getParameter("INL").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'INL', using default value."); }
    try { m_DNL = std::stod(getParameter("DNL").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DNL', using default value."); }

    try { m_ENOB = std::stoi(getParameter("ENOB").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ENOB', using default value."); }
    try { m_SNR_dB = std::stod(getParameter("SNR_dB").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SNR_dB', using default value."); }
    try { m_H2_dBc = std::stod(getParameter("H2_dBc").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'H2_dBc', using default value."); }
    try { m_H3_dBc = std::stod(getParameter("H3_dBc").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'H3_dBc', using default value."); }
    try { m_H4_dBc = std::stod(getParameter("H4_dBc").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'H4_dBc', using default value."); }
    try { m_H5_dBc = std::stod(getParameter("H5_dBc").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'H5_dBc', using default value."); }

    try { m_SINAD_dB = std::stod(getParameter("SINAD_dB").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SINAD_dB', using default value."); }
    try { m_SFDR_dBc = std::stod(getParameter("SFDR_dBc").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SFDR_dBc', using default value."); }
    try { m_FFT_Size = ConvertStringToFFT_SizeEnum(getParameter("FFT_Size").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FFT_Size', using default value."); }

    try { m_SNR_Model = ConvertStringToSNR_ModelEnum(getParameter("SNR_Model").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SNR_Model', using default value."); }
    try { m_ThermalNoise_SNR_dBFS = std::stoi(getParameter("ThermalNoise_SNR_dBFS").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ThermalNoise_SNR_dBFS', using default value."); }
    try { m_CenterFreq = std::stod(getParameter("CenterFreq").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'CenterFreq', using default value."); }
    try { m_Level_dBFS = std::stod(getParameter("Level_dBFS").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Level_dBFS', using default value."); }

    try { m_ConversionType = ConvertStringToConversionTypeEnum(getParameter("ConversionType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ConversionType', using default value."); }
    try { m_Clock = std::stod(getParameter("Clock").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Clock', using default value."); }
    try { m_Phase = std::stod(getParameter("Phase").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Phase', using default value."); }

    try { m_DownsampleFactor = std::stoi(getParameter("DownsampleFactor").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DownsampleFactor', using default value."); }
    try { m_DownsamplePhase = std::stoi(getParameter("DownsamplePhase").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DownsamplePhase', using default value."); }
    try { m_AntiAliasingFilter = ConvertStringToAntiAliasingFilterEnum(getParameter("AntiAliasingFilter").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'AntiAliasingFilter', using default value."); }
    try { m_ExcessBW = std::stod(getParameter("ExcessBW").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'ExcessBW', using default value."); }

    SetParameters();

    if (!m_atod->Setup()) {
        return false;
    }

    // ---- 与 AtoD::Setup() 相同的速率判断 ----
    if (m_ConversionType == AtoD::Downsampled) {
        m_inputRate = m_DownsampleFactor;
    } else {
        m_inputRate = 1;
    }

    m_atod->A_in.SetStartTime(simulator_param.startTime);
    m_atod->A_out.SetStartTime(simulator_param.startTime);

    // ---- 端口注册移到末尾 ----
    AddInputPort("A_in", m_atod->A_in, m_inputRate, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("A_out", m_atod->A_out, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("D_I" , m_atod->D_I, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("D_Q",  m_atod->D_Q, 1, Block::DataType::CIRCULAR_BUFFER_INT);

    return true;
}

AtoD::OutputDigitalFormatEnum AtoD_Block::ConvertStringToOutputDigitalFormatEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "offset_binary") {
        return AtoD::Offset_binary;
    }
    if (lower == "twos_complement" || lower == "1") {
        return AtoD::Twos_complement;
    }
    return AtoD::Offset_binary;
}

AtoD::DistortionModelEnum AtoD_Block::ConvertStringToDistortionModelEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "distortion_none") {
        return AtoD::Distortion_None;
    }
    if (lower == "jitter_inl_dnl" || lower == "1") {
        return AtoD::Jitter_INL_DNL;
    }
    if (lower == "enob_value" || lower == "2") {
        return AtoD::ENOB_value;
    }
    if (lower == "snr_and_harmonics" || lower == "3") {
        return AtoD::SNR_and_Harmonics;
    }
    if (lower == "sinad_and_sfdr" || lower == "4") {
        return AtoD::SINAD_and_SFDR;
    }
    return AtoD::Distortion_None;
}

AtoD::EnableJitterEnum AtoD_Block::ConvertStringToEnableJitterEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "jitter_no") {
        return AtoD::Jitter_No;
    }
    if (lower == "time_domain" || lower == "1") {
        return AtoD::Time_Domain;
    }
    if (lower == "frequency_domain" || lower == "2") {
        return AtoD::Frequency_Domain;
    }
    return AtoD::Jitter_No;
}

AtoD::PN_TypeEnum AtoD_Block::ConvertStringToPN_TypeEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "random_pn") {
        return AtoD::Random_PN;
    }
    if (lower == "fixed_freq_offset" || lower == "1") {
        return AtoD::Fixed_freq_offset;
    }
    if (lower == "fixed_freq_offset_and_amplitude" || lower == "2") {
        return AtoD::Fixed_freq_offset_and_amplitude;
    }
    return AtoD::Random_PN;
}

AtoD::FFT_SizeEnum AtoD_Block::ConvertStringToFFT_SizeEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "fft_2_12") {
        return AtoD::FFT_2_12;
    }
    if (lower == "fft_2_13" || lower == "1") {
        return AtoD::FFT_2_13;
    }
    if (lower == "fft_2_14" || lower == "2") {
        return AtoD::FFT_2_14;
    }
    if (lower == "fft_2_15" || lower == "3") {
        return AtoD::FFT_2_15;
    }
    if (lower == "fft_2_16" || lower == "4") {
        return AtoD::FFT_2_16;
    }
    return AtoD::FFT_2_12;
}

AtoD::SNR_ModelEnum AtoD_Block::ConvertStringToSNR_ModelEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "quantization_and_jitter") {
        return AtoD::Quantization_and_Jitter;
    }
    if (lower == "quantization_and_inl_dnl" || lower == "1") {
        return AtoD::Quantization_and_INL_DNL;
    }
    if (lower == "quantization_and_jitter_or_inl_dnl" || lower == "2") {
        return AtoD::Quantization_and_Jitter_or_INL_DNL;
    }
    if (lower == "quantization_jitter_and_thermal_noise" || lower == "3") {
        return AtoD::Quantization_Jitter_and_Thermal_Noise;
    }
    return AtoD::Quantization_and_Jitter;
}

AtoD::ConversionTypeEnum AtoD_Block::ConvertStringToConversionTypeEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "clocked") {
        return AtoD::Clocked;
    }
    if (lower == "downsampled" || lower == "1") {
        return AtoD::Downsampled;
    }
    return AtoD::Clocked;
}
AtoD::AntiAliasingFilterEnum AtoD_Block::ConvertStringToAntiAliasingFilterEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "aa_off") {
        return AtoD::AA_OFF;
    }
    if (lower == "aa_on" || lower == "1") {
        return AtoD::AA_ON;
    }
    return AtoD::AA_OFF;
}
