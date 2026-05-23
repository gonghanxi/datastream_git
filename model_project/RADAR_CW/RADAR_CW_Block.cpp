#include "RADAR_CW_Block.h"
#include <string>

RADAR_CW_Block::RADAR_CW_Block(const std::string &name) : Block(name), m_counter(0)
{
}

void RADAR_CW_Block::SetParameters(double sampleRate,
                                        double amplitude,
                                        double lowerFreq,
                                        double deltaFreq,
                                        double period,
                                        double freqDowntime,
                                        double freqUptime,
                                        double freqFixtime,
                                        double offTime,
                                        RADAR_CW::Waveform_typeEnum waveformType)
{
    if(m_radarCW) {
        m_radarCW->SampleRate = sampleRate;
        m_radarCW->Amplitude = amplitude;
        m_radarCW->LowerFreq = lowerFreq;
        m_radarCW->DeltaFreq = deltaFreq;
        m_radarCW->Waveform_type = waveformType;
        m_radarCW->Period = period;


        m_radarCW->FreqDownTime = freqDowntime;
        m_radarCW->FreqUpTime = freqUptime;
        m_radarCW->FreqFixTime = freqFixtime;
        m_radarCW->OffTime = offTime;

    }
}

bool RADAR_CW_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RADAR_CW_Block::Run()
{
    if(!CanProcess()) {
        return false;
    }
    if(m_radarCW->Run()) {
        //----------------数据处理---------------------
        std::vector<std::complex<double>> waveformData;
        waveformData.push_back(std::complex<double>(
            m_radarCW->waveform_out[0].real(),
            m_radarCW->waveform_out[0].imag()
            ));

        double freqData = m_radarCW->freq_out;
        //----------------数据处理---------------------

        // 获取输出端口名称
        std::string waveformPort = GetOutputPortName(0);
        std::string freqPort = GetOutputPortName(1);

        //----------------写入数据---------------------
        if(!waveformPort.empty()) {
            WriteOutputData(waveformPort, waveformData);
        }


        if(!freqPort.empty()) {
            Buffer* outputBuffer = GetOutputPort(freqPort);
            outputBuffer->WriteData(freqData);
        }
        m_counter++;
        return true;
    }
    else {
        return false;
    }
}

bool RADAR_CW_Block::Initialize()
{
    SetBlockType(BlockType::SOURCE);

    m_radarCW = std::make_unique<RADAR_CW>();
    AddOutputPort("waveform_out", m_radarCW->waveform_out, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("freq_out", m_radarCW->freq_out, 1, Block::DataType::DOUBLE);


    SetDefaultParameters();

    m_waveformtype = ConvertStringToWaveformEnum(getParameter("Waveform_type").Value);
    m_sampleRate = std::stod(getParameter("SampleRate").Value);
    m_amplitude = std::stod(getParameter("Amplitude").Value);
    m_lowerFreq = std::stod(getParameter("LowerFreq").Value);
    m_deltaFreq = std::stod(getParameter("DeltaFreq").Value);
    m_period = std::stod(getParameter("Period").Value);

    m_FreqDownTime = std::stod(getParameter("FreqDownTime").Value);
    m_FreqUpTime = std::stod(getParameter("FreqUpTime").Value);
    m_FreqFixTime = std::stod(getParameter("FreqFixTime").Value);
    m_OffTime = std::stod(getParameter("OffTime").Value);

    SetParameters(m_sampleRate,
                  m_amplitude,
                  m_lowerFreq,
                  m_deltaFreq,
                  m_period,
                  m_FreqDownTime,
                  m_FreqUpTime,
                  m_FreqFixTime,
                  m_OffTime,
                  m_waveformtype);

    return true;
}


void RADAR_CW_Block::SetDefaultParameters()
{
    m_sampleRate = 1e6;
    m_amplitude = 1.0;
    m_lowerFreq = 10e3;
    m_deltaFreq = 50e3;
    m_period = 1e-4;
    m_FreqDownTime = 1e-5;
    m_FreqUpTime = 1e-5;
    m_FreqFixTime = 1e-5;
    m_OffTime = 1e-5;
    m_waveformtype = RADAR_CW::Sawtooth;
}

RADAR_CW::Waveform_typeEnum RADAR_CW_Block::ConvertStringToWaveformEnum(const std::string &value)
{
    // 去除字符串前后的空格
    std::string trimmedValue = value;
    trimmedValue.erase(0, trimmedValue.find_first_not_of(" \t\n\r"));
    trimmedValue.erase(trimmedValue.find_last_not_of(" \t\n\r") + 1);

    // 转换为小写以便不区分大小写比较
    std::string lowerValue = trimmedValue;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    // 字符串匹配
    if (lowerValue == "sawtooth" || lowerValue == "0") {
        return RADAR_CW::Sawtooth;
    } else if (lowerValue == "triangle" || lowerValue == "1") {
        return RADAR_CW::Triangle;
    } else if (lowerValue == "userdefined" || lowerValue == "2") {
        return RADAR_CW::UserDefined;
    }
}

int RADAR_CW_Block::GetGeneratedSampleCount() const { return m_counter; }

