#include "AtoD_M_Block.h"

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

AtoD_M_Block::AtoD_M_Block(const std::string& name)
    : Block(name)
{
}

bool AtoD_M_Block::parseArrayString(const std::string &arrayStr, std::vector<double> &outArray)
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

void AtoD_M_Block::SetDefaultParamters()
{
    m_NBits = 8;
    m_VRef = 1.0;
    m_OutputDigitalFormat = AtoD_M::Offset_binary;
    m_DistortionModel = AtoD_M::Jitter_INL_DNL;
    m_EnableJitter = AtoD_M::Jitter_No;
    m_RJrms = 0.0;
    m_PhaseNoiseData = nullptr;
    m_PhaseNoiseDataSize = 0;
    m_PN_Type = AtoD_M::Random_PN;

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
    m_FFT_Size = AtoD_M::FFT_2_14;

    m_SNR_Model = AtoD_M::Quantization_and_Jitter;
    m_ThermalNoise_SNR_dBFS = 63;
    m_CenterFreq = 100.0e6;
    m_Level_dBFS = 0.0;

    m_ConversionType = AtoD_M::Clocked;
    m_Clock = 0.2e6;
    m_Phase = 0.0;

    m_DownsampleFactor = 1;
    m_DownsamplePhase = 0;
    m_AntiAliasingFilter = AtoD_M::AA_OFF;
    m_ExcessBW = 0.5;

}

void AtoD_M_Block::SetParameters()
{
    m_PhaseNoiseData = primdata.data();
    m_PhaseNoiseDataSize =  static_cast<double>(primdata.size());
    if(!m_atod_m) {
        return;
    }
    m_atod_m->NBits = m_NBits;
    m_atod_m->VRef = m_VRef;
    m_atod_m->OutputDigitalFormat = m_OutputDigitalFormat;
    m_atod_m->DistortionModel = m_DistortionModel;
    m_atod_m->EnableJitter = m_EnableJitter;
    m_atod_m->RJrms = m_RJrms;
    m_atod_m->PhaseNoiseData = m_PhaseNoiseData;
    m_atod_m->PhaseNoiseDataSize = m_PhaseNoiseDataSize;

    m_atod_m->PN_Type = m_PN_Type;

    m_atod_m->INL = m_INL;
    m_atod_m->DNL = m_DNL;

    m_atod_m->ENOB = m_ENOB;
    m_atod_m->SNR_dB = m_SNR_dB;
    m_atod_m->H2_dBc = m_H2_dBc;
    m_atod_m->H3_dBc = m_H3_dBc;
    m_atod_m->H4_dBc = m_H4_dBc;
    m_atod_m->H5_dBc = m_H5_dBc;

    m_atod_m->SINAD_dB = m_SINAD_dB;
    m_atod_m->SFDR_dBc = m_SFDR_dBc;
    m_atod_m->FFT_Size = m_FFT_Size;

    m_atod_m->SNR_Model = m_SNR_Model;
    m_atod_m->ThermalNoise_SNR_dBFS = m_ThermalNoise_SNR_dBFS;
    m_atod_m->CenterFreq = m_CenterFreq;
    m_atod_m->Level_dBFS = m_Level_dBFS;

    m_atod_m->ConversionType = m_ConversionType;
    m_atod_m->Clock = m_Clock;
    m_atod_m->Phase = m_Phase;

    m_atod_m->DownsampleFactor = m_DownsampleFactor;
    m_atod_m->DownsamplePhase = m_DownsamplePhase;
    m_atod_m->AntiAliasingFilter = m_AntiAliasingFilter;
    m_atod_m->ExcessBW = m_ExcessBW;
}

bool AtoD_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool AtoD_M_Block::Run()
{
    // 获取输入输出端口名称
    std::string A_INPortName = GetInputPortName(0);
    std::string A_outPortName = GetOutputPortName(0);
    std::string D_IPortName = GetOutputPortName(1);
    std::string D_QPortName = GetOutputPortName(2);

     auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeMatrix>(A_INPortName);
     if(inputData.empty()) {
         if(IsVariableStepMode()) {
             return true;
         }
         return false;
     }
     m_atod_m->A_Input = inputData;

     if (!m_atod_m->Run()) {
         return false;
     }
     std::vector<SystemVueModelBuilder::EnvelopeMatrix> outputA_OUTData;
     std::vector<SystemVueModelBuilder::IntMatrix> outputD_IData;
     std::vector<SystemVueModelBuilder::IntMatrix> outputD_QData;

     outputA_OUTData.push_back(m_atod_m->A_out[0]);
     outputD_IData.push_back(m_atod_m->D_I[0]);
     outputD_QData.push_back(m_atod_m->D_Q[0]);

     WriteOutputData(A_outPortName, outputA_OUTData);
     WriteOutputData(D_IPortName, outputD_IData);
     WriteOutputData(D_QPortName, outputD_QData);

     if (m_atod_m) {
         m_atod_m->Advance();
     }
     return true;
}

bool AtoD_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_atod_m = std::make_unique<AtoD_M>();

    SetDefaultParamters();
    simulator_param = getSimu();

    try { m_NBits = std::stod(getParameter("NBits").Value); } catch (...) { }
    try { m_VRef = std::stod(getParameter("VRef").Value); } catch (...) { }
    try { m_OutputDigitalFormat = ConvertStringToOutputDigitalFormatEnum(getParameter("OutputDigitalFormat").Value); } catch (...) { }
    try { m_DistortionModel = ConvertStringToDistortionModelEnum(getParameter("DistortionModel").Value); } catch (...) { }
    try { m_EnableJitter = ConvertStringToEnableJitterEnum(getParameter("EnableJitter").Value); } catch (...) { }
    try { m_RJrms = std::stod(getParameter("RJrms").Value); } catch (...) { }
    try {
        std::string PrimString = getParameter("PhaseNoiseData").Value;
        parseArrayString(PrimString, primdata);
    } catch(...) {}
    try { m_PN_Type = ConvertStringToPN_TypeEnum(getParameter("PN_Type").Value); } catch (...) { }

    try { m_INL = std::stod(getParameter("INL").Value); } catch (...) { }
    try { m_DNL = std::stod(getParameter("DNL").Value); } catch (...) { }

    try { m_ENOB = std::stoi(getParameter("ENOB").Value); } catch (...) { }
    try { m_SNR_dB = std::stod(getParameter("SNR_dB").Value); } catch (...) { }
    try { m_H2_dBc = std::stod(getParameter("H2_dBc").Value); } catch (...) { }
    try { m_H3_dBc = std::stod(getParameter("H3_dBc").Value); } catch (...) { }
    try { m_H4_dBc = std::stod(getParameter("H4_dBc").Value); } catch (...) { }
    try { m_H5_dBc = std::stod(getParameter("H5_dBc").Value); } catch (...) { }

    try { m_SINAD_dB = std::stod(getParameter("SINAD_dB").Value); } catch (...) { }
    try { m_SFDR_dBc = std::stod(getParameter("SFDR_dBc").Value); } catch (...) { }
    try { m_FFT_Size = ConvertStringToFFT_SizeEnum(getParameter("FFT_Size").Value); } catch (...) { }

    try { m_SNR_Model = ConvertStringToSNR_ModelEnum(getParameter("SNR_Model").Value); } catch (...) { }
    try { m_ThermalNoise_SNR_dBFS = std::stoi(getParameter("ThermalNoise_SNR_dBFS").Value); } catch (...) { }
    try { m_CenterFreq = std::stod(getParameter("CenterFreq").Value); } catch (...) { }
    try { m_Level_dBFS = std::stod(getParameter("Level_dBFS").Value); } catch (...) { }

    try { m_ConversionType = ConvertStringToConversionTypeEnum(getParameter("ConversionType").Value); } catch (...) { }
    try { m_Clock = std::stod(getParameter("Clock").Value); } catch (...) { }
    try { m_Phase = std::stod(getParameter("Phase").Value); } catch (...) { }

    try { m_DownsampleFactor = std::stoi(getParameter("DownsampleFactor").Value); } catch (...) { }
    try { m_DownsamplePhase = std::stoi(getParameter("DownsamplePhase").Value); } catch (...) { }
    try { m_AntiAliasingFilter = ConvertStringToAntiAliasingFilterEnum(getParameter("AntiAliasingFilter").Value); } catch (...) { }
    try { m_ExcessBW = std::stod(getParameter("ExcessBW").Value); } catch (...) { }

    SetParameters();

    if (!m_atod_m->Setup()) {
        return false;
    }

    m_atod_m->A_in.SetStartTime(simulator_param.startTime);
    m_atod_m->A_out.SetStartTime(simulator_param.startTime);

    int system_rate = 1;
    if (m_ConversionType == AtoD_M::Downsampled)
    {
        system_rate = m_DownsampleFactor;
    }

    AddInputPort("A_in", m_atod_m->A_in, system_rate, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("A_out", m_atod_m->A_out, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddOutputPort("D_I" , m_atod_m->D_I, 1, Block::DataType::MATRIX_INT);
    AddOutputPort("D_Q",  m_atod_m->D_Q, 1, Block::DataType::MATRIX_INT);

    return true;
}

AtoD_M::OutputDigitalFormatEnum AtoD_M_Block::ConvertStringToOutputDigitalFormatEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "Offset_binary") {
        return AtoD_M::Offset_binary;
    }
    if (lower == "Twos_complement" || lower == "1") {
        return AtoD_M::Twos_complement;
    }
    return AtoD_M::Offset_binary;
}

AtoD_M::DistortionModelEnum AtoD_M_Block::ConvertStringToDistortionModelEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "Distortion_None") {
        return AtoD_M::Distortion_None;
    }
    if (lower == "Jitter_INL_DNL" || lower == "1") {
        return AtoD_M::Jitter_INL_DNL;
    }
    if (lower == "ENOB_value" || lower == "2") {
        return AtoD_M::ENOB_value;
    }
    if (lower == "SNR_and_Harmonics" || lower == "3") {
        return AtoD_M::SNR_and_Harmonics;
    }
    if (lower == "SINAD_and_SFDR" || lower == "4") {
        return AtoD_M::SINAD_and_SFDR;
    }
    return AtoD_M::Distortion_None;
}

AtoD_M::EnableJitterEnum AtoD_M_Block::ConvertStringToEnableJitterEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "Jitter_No") {
        return AtoD_M::Jitter_No;
    }
    if (lower == "Time_Domain" || lower == "1") {
        return AtoD_M::Time_Domain;
    }
    if (lower == "Frequency_Domain" || lower == "2") {
        return AtoD_M::Frequency_Domain;
    }
    return AtoD_M::Jitter_No;
}

AtoD_M::PN_TypeEnum AtoD_M_Block::ConvertStringToPN_TypeEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "Random_PN") {
        return AtoD_M::Random_PN;
    }
    if (lower == "Fixed_freq_offset" || lower == "1") {
        return AtoD_M::Fixed_freq_offset;
    }
    if (lower == "Fixed_freq_offset_and_amplitude" || lower == "2") {
        return AtoD_M::Fixed_freq_offset_and_amplitude;
    }
    return AtoD_M::Random_PN;
}

AtoD_M::FFT_SizeEnum AtoD_M_Block::ConvertStringToFFT_SizeEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "FFT_2_12") {
        return AtoD_M::FFT_2_12;
    }
    if (lower == "FFT_2_13" || lower == "1") {
        return AtoD_M::FFT_2_13;
    }
    if (lower == "FFT_2_14" || lower == "2") {
        return AtoD_M::FFT_2_14;
    }
    if (lower == "FFT_2_15" || lower == "3") {
        return AtoD_M::FFT_2_15;
    }
    if (lower == "FFT_2_16" || lower == "4") {
        return AtoD_M::FFT_2_16;
    }
    return AtoD_M::FFT_2_12;
}

AtoD_M::SNR_ModelEnum AtoD_M_Block::ConvertStringToSNR_ModelEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "Quantization_and_Jitter") {
        return AtoD_M::Quantization_and_Jitter;
    }
    if (lower == "Quantization_and_INL_DNL" || lower == "1") {
        return AtoD_M::Quantization_and_INL_DNL;
    }
    if (lower == "Quantization_and_Jitter_or_INL_DNL" || lower == "2") {
        return AtoD_M::Quantization_and_Jitter_or_INL_DNL;
    }
    if (lower == "Quantization_Jitter_and_Thermal_Noise" || lower == "3") {
        return AtoD_M::Quantization_Jitter_and_Thermal_Noise;
    }
    return AtoD_M::Quantization_and_Jitter;
}

AtoD_M::ConversionTypeEnum AtoD_M_Block::ConvertStringToConversionTypeEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "Clocked") {
        return AtoD_M::Clocked;
    }
    if (lower == "Downsampled" || lower == "1") {
        return AtoD_M::Downsampled;
    }
    return AtoD_M::Clocked;
}
AtoD_M::AntiAliasingFilterEnum AtoD_M_Block::ConvertStringToAntiAliasingFilterEnum(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "AA_OFF") {
        return AtoD_M::AA_OFF;
    }
    if (lower == "AA_ON" || lower == "1") {
        return AtoD_M::AA_ON;
    }
    return AtoD_M::AA_OFF;
}
