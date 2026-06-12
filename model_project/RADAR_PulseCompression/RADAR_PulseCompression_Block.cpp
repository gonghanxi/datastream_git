#include "RADAR_PulseCompression_Block.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <iostream>
#include <vector>

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

RADAR_PulseCompression_Block::RADAR_PulseCompression_Block(const std::string& name)
    : Block(name)
{
}

void RADAR_PulseCompression_Block::SetDefaultParamters()
{
    m_samplenum = 1000;
    m_fftSize = 1024;
    m_bandwidth = 5e6;
    m_sampleRate = 10e6;
    m_windowType = RADAR_PulseCompression::Rectangle;
    m_windowParameter = 1.0;
}

bool RADAR_PulseCompression_Block::ProcessData(Matrix<std::complex<double> > fullSequence)
{
    const double freqResolution = m_sampleRate / m_fftSize;
    const int windowLen = static_cast<int>(m_bandwidth / freqResolution);
    const int windowN = windowLen - 1;
    const double PI = std::acos(-1);

    SystemVueModelBuilder::Matrix<std::complex<double>> windowSequence(1, static_cast<size_t>(m_fftSize));

    switch (m_windowType) {
    case RADAR_PulseCompression::Rectangle:
        for (int i = 0; i < m_fftSize; ++i) {
            windowSequence(i) = 1.0;
        }
        break;
    case RADAR_PulseCompression::Bartlett:
        for (int i = 0; i < m_fftSize; ++i) {
            if (i < windowLen / 2) {
                windowSequence(i) = 2.0 * i / windowN;
            } else if (i >= windowLen / 2 && i < windowLen) {
                windowSequence(i) = 2.0 - 2.0 * i / windowN;
            } else {
                windowSequence(i) = 0.0;
            }
        }
        break;
    case RADAR_PulseCompression::Hanning:
        for (int i = 0; i < m_fftSize; ++i) {
            if (i < windowLen) {
                windowSequence(i) = 0.5 * (1.0 - std::cos(2.0 * PI * i / windowN));
            } else {
                windowSequence(i) = 0.0;
            }
        }
        break;
    case RADAR_PulseCompression::Hamming:
        for (int i = 0; i < m_fftSize; ++i) {
            if (i < windowLen) {
                windowSequence(i) = 0.54 - 0.46 * std::cos(2.0 * PI * i / windowN);
            } else {
                windowSequence(i) = 0.0;
            }
        }
        break;
    case RADAR_PulseCompression::Blackman:
        for (int i = 0; i < m_fftSize; ++i) {
            if (i < windowLen / 2) {
                windowSequence(i) = 0.42 - 0.5 * std::cos(2.0 * PI * i / windowN) + 0.08 * std::cos(4.0 * PI * i / windowN);
            } else if (i >= windowLen / 2 && i < windowLen) {
                windowSequence(i) = 0.42 - 0.5 * std::cos(2.0 * PI * (windowLen - i) / windowN) + 0.08 * std::cos(4.0 * PI * (windowLen - i) / windowN);
            } else {
                windowSequence(i) = 0.0;
            }
        }
        break;
    case RADAR_PulseCompression::SteepBlackman:
        for (int i = 0; i < m_fftSize; ++i) {
            if (i < windowLen / 2) {
                windowSequence(i) = 0.35875 - 0.48829 * std::cos(2.0 * PI * i / windowN) + 0.14128 * std::cos(4.0 * PI * i / windowN) - 0.01168 * std::cos(6.0 * PI * i / windowN);
            } else if (i >= windowLen / 2 && i < windowLen) {
                windowSequence(i) = 0.35875 - 0.48829 * std::cos(2.0 * PI * (windowLen - i) / windowN) + 0.14128 * std::cos(4.0 * PI * (windowLen - i) / windowN) - 0.01168 * std::cos(6.0 * PI * (windowLen - i) / windowN);
            } else {
                windowSequence(i) = 0.0;
            }
        }
        break;
    case RADAR_PulseCompression::Kaiser:
        for (int i = 0; i < m_fftSize; ++i) {
            if (i < windowLen) {
                const double t = 1.0 - std::pow(2.0 * i / windowN - 1.0, 2);
                windowSequence(i) = m_radarPulseCompression->I0(20, m_windowParameter * std::sqrt(t)) /
                                   m_radarPulseCompression->I0(20, m_windowParameter);
            } else {
                windowSequence(i) = 0.0;
            }
        }
        break;
    default:
        break;
    }

    for (int i = 0; i < m_fftSize; i++)
    {
        fullSequence(i) *= windowSequence(i + windowLen / 2 < m_fftSize ? i + windowLen / 2 : i + windowLen / 2 - m_fftSize);
    }

    m_radarPulseCompression->fft(fullSequence, m_fftSize, -1);

    std::vector<std::complex<double>> outputData;
    outputData.reserve(static_cast<size_t>(m_samplenum));
    for (int i = 0; i < m_samplenum; ++i) {
        outputData.push_back(fullSequence(m_fftSize - i < m_fftSize ? m_fftSize - i : 0));
    }
    if(IsVariableStepMode()) {
        for(const auto& val : outputData) m_outputQueue.push(val);
        if(!m_outputQueue.empty()) {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;
            WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;
            m_signalBuffer.clear();
            m_referenceBuffer.clear();
            qDebug() << "[RADAR_PulseCompression_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << outputValue.imag();
        }
    }
    else {
        WriteOutputData(GetOutputPortName(0), outputData);
    }
    return true;
}

bool RADAR_PulseCompression_Block::DataStreamRun()
{
    const std::string signalPort = GetInputPortName(0);
    const std::string referencePort = GetInputPortName(1);
    const std::string outputPort = GetOutputPortName(0);

    auto signalData = ReadInputData<std::complex<double>>(signalPort);
    auto referenceData = ReadInputData<std::complex<double>>(referencePort);

    if (signalData.empty() || referenceData.empty()) {
        return false;
    }

    SystemVueModelBuilder::Matrix<std::complex<double>> fullSequence(1, static_cast<size_t>(m_fftSize));
    for (int i = 0; i < m_fftSize; ++i) {
        if (i < m_samplenum) {
            fullSequence(i) = signalData[static_cast<size_t>(i)];
        } else {
            fullSequence(i) = 0.0;
        }
    }

    m_radarPulseCompression->fft(fullSequence, m_fftSize, 1);
    fullSequence *= m_fftSize;

    for (int i = 0; i < m_fftSize; ++i) {
        fullSequence(i) *= referenceData[static_cast<size_t>(i)];
    }
    return ProcessData(fullSequence);
}

bool RADAR_PulseCompression_Block::TimeDrivenRun()
{
    const std::string signalPort = GetInputPortName(0);
    const std::string referencePort = GetInputPortName(1);
    const std::string outputPort = GetOutputPortName(0);

    auto signalData = ReadInputData<std::complex<double>>(signalPort);
    auto referenceData = ReadInputData<std::complex<double>>(referencePort);

    if (signalData.empty() || referenceData.empty()) {
        return true;
    }
    for(const auto& val : signalData) m_signalBuffer.push_back(val);
    for(const auto& val : referenceData) m_referenceBuffer.push_back(val);

    if(m_signalBuffer.size() >= static_cast<size_t>(m_samplenum) && m_referenceBuffer.size() >= static_cast<size_t>(m_fftSize)) {
        SystemVueModelBuilder::Matrix<std::complex<double>> fullSequence(1, static_cast<size_t>(m_fftSize));
        for (int i = 0; i < m_fftSize; ++i) {
            if (i < m_samplenum) {
                fullSequence(i) = m_signalBuffer[static_cast<size_t>(i)];
            } else {
                fullSequence(i) = 0.0;
            }
        }

        m_radarPulseCompression->fft(fullSequence, m_fftSize, 1);
        fullSequence *= m_fftSize;

        for (int i = 0; i < m_fftSize; ++i) {
            fullSequence(i) *= m_referenceBuffer[static_cast<size_t>(i)];
        }
        return ProcessData(fullSequence);
    }
    return true;
}

void RADAR_PulseCompression_Block::SetParameters(int samplenum, int fftSize, double bandwidth, double sampleRate,
    RADAR_PulseCompression::SelectedWindowType windowType, double windowParameter)
{
    m_samplenum = samplenum;
    m_fftSize = fftSize;
    m_bandwidth = bandwidth;
    m_sampleRate = sampleRate;
    m_windowType = windowType;
    m_windowParameter = windowParameter;

    if (m_radarPulseCompression) {
        m_radarPulseCompression->Samplenum = m_samplenum;
        m_radarPulseCompression->FFTSize = m_fftSize;
        m_radarPulseCompression->Bandwidth = m_bandwidth;
        m_radarPulseCompression->SampleRate = m_sampleRate;
        m_radarPulseCompression->WindowType = m_windowType;
        m_radarPulseCompression->WindowParameter = m_windowParameter;
    }
}

bool RADAR_PulseCompression_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool RADAR_PulseCompression_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_PulseCompression_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_radarPulseCompression = std::make_unique<RADAR_PulseCompression>();

    SetDefaultParamters();

    try { m_samplenum = std::stoi(getParameter("Samplenum").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Samplenum', using default value."); }
    try { m_fftSize = std::stoi(getParameter("FFTSize").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FFTSize', using default value."); }
    try { m_bandwidth = std::stod(getParameter("Bandwidth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Bandwidth', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    try { m_windowType = ConvertStringToWindowType(getParameter("WindowType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'WindowType', using default value."); }
    try { m_windowParameter = std::stod(getParameter("WindowParameter").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'WindowParameter', using default value."); }

    SetParameters(m_samplenum, m_fftSize, m_bandwidth, m_sampleRate, m_windowType, m_windowParameter);

    if (m_samplenum < 1 || m_fftSize < m_samplenum) {
        LOG_ERROR("error FFTSize and Size should meet this condition: FFTSize >= Size >= 1");
        return false;
    }

    if ((m_fftSize & (m_fftSize - 1)) != 0) {
        LOG_ERROR("warning Only 2^N FFTSize is supported now. For FFTSize not equels to 2^N, performance may be insufficient.");
    }

    AddInputPort("signal", m_radarPulseCompression->signal, static_cast<size_t>(m_samplenum), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddInputPort("reference", m_radarPulseCompression->reference, static_cast<size_t>(m_fftSize), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_radarPulseCompression->output, static_cast<size_t>(m_samplenum), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

RADAR_PulseCompression::SelectedWindowType RADAR_PulseCompression_Block::ConvertStringToWindowType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "rectangle") {
        return RADAR_PulseCompression::Rectangle;
    }
    if (lower == "bartlett" || lower == "1") {
        return RADAR_PulseCompression::Bartlett;
    }
    if (lower == "hanning" || lower == "2") {
        return RADAR_PulseCompression::Hanning;
    }
    if (lower == "hamming" || lower == "3") {
        return RADAR_PulseCompression::Hamming;
    }
    if (lower == "blackman" || lower == "4") {
        return RADAR_PulseCompression::Blackman;
    }
    if (lower == "steepblackman" || lower == "5") {
        return RADAR_PulseCompression::SteepBlackman;
    }
    if (lower == "kaiser" || lower == "6") {
        return RADAR_PulseCompression::Kaiser;
    }
    return RADAR_PulseCompression::Rectangle;
}
