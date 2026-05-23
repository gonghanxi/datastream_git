#include "RADAR_MTI_Block.h"
#include <algorithm>
#include <cctype>
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

RADAR_MTI_Block::RADAR_MTI_Block(const std::string& name)
    : Block(name)
{
}

void RADAR_MTI_Block::SetDefaultParamters()
{
    m_pri = 1e-4;
    m_sampleRate = 10e6;
    m_numOfPulse = 32;
    m_mtiType = RADAR_MTI::TwoPulseCanceller;
}

bool RADAR_MTI_Block::DataStreamRun()
{
    if ((m_mtiType == RADAR_MTI::TwoPulseCanceller && m_numOfPulse < 2) ||
        (m_mtiType == RADAR_MTI::ThreePulseCanceller && m_numOfPulse < 3)) {
        return false;
    }
    const std::string inputPort = GetInputPortName(0);
    const std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return false;
    }

    const int samplesPerPulse = static_cast<int>(std::round(m_pri * m_sampleRate));
    const int outputPulseNum = (m_mtiType == RADAR_MTI::TwoPulseCanceller) ? (m_numOfPulse - 1) : (m_numOfPulse - 2);
    const int outputTotalSamples = outputPulseNum * samplesPerPulse;

    std::vector<std::complex<double>> outputData;
    outputData.resize(static_cast<size_t>(outputTotalSamples));

    if (m_mtiType == RADAR_MTI::TwoPulseCanceller) {
        for (int pulse = 1; pulse < m_numOfPulse; ++pulse) {
            for (int sample = 0; sample < samplesPerPulse; ++sample) {
                int inputIdxPrev = (pulse - 1) * samplesPerPulse + sample;
                int inputIdxCurr = pulse * samplesPerPulse + sample;
                int outputIdx = (pulse - 1) * samplesPerPulse + sample;
                outputData[static_cast<size_t>(outputIdx)] = inputData[static_cast<size_t>(inputIdxCurr)] - inputData[static_cast<size_t>(inputIdxPrev)];
            }
        }
    } else {
        for (int pulse = 2; pulse < m_numOfPulse; ++pulse) {
            for (int sample = 0; sample < samplesPerPulse; ++sample) {
                int inputIdxPrev2 = (pulse - 2) * samplesPerPulse + sample;
                int inputIdxPrev1 = (pulse - 1) * samplesPerPulse + sample;
                int inputIdxCurr = pulse * samplesPerPulse + sample;
                int outputIdx = (pulse - 2) * samplesPerPulse + sample;
                outputData[static_cast<size_t>(outputIdx)] = inputData[static_cast<size_t>(inputIdxCurr)]
                    - inputData[static_cast<size_t>(inputIdxPrev1)] * 2.0
                    + inputData[static_cast<size_t>(inputIdxPrev2)];
            }
        }
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

bool RADAR_MTI_Block::TimeDrivenRun()
{
    if ((m_mtiType == RADAR_MTI::TwoPulseCanceller && m_numOfPulse < 2) ||
        (m_mtiType == RADAR_MTI::ThreePulseCanceller && m_numOfPulse < 3)) {
        return false;
    }

    const std::string inputPort = GetInputPortName(0);
    const std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    const int samplesPerPulse = static_cast<int>(std::round(m_pri * m_sampleRate));
    const int outputPulseNum = (m_mtiType == RADAR_MTI::TwoPulseCanceller) ? (m_numOfPulse - 1) : (m_numOfPulse - 2);
    const int outputTotalSamples = outputPulseNum * samplesPerPulse;
    const int inputTotalSamples = m_numOfPulse * samplesPerPulse;


    if(m_inputBuffer.size() >= static_cast<size_t>(inputTotalSamples)) {
        std::vector<std::complex<double>> outputData;
        outputData.resize(static_cast<size_t>(outputTotalSamples));

        if (m_mtiType == RADAR_MTI::TwoPulseCanceller) {
            for (int pulse = 1; pulse < m_numOfPulse; ++pulse) {
                for (int sample = 0; sample < samplesPerPulse; ++sample) {
                    int inputIdxPrev = (pulse - 1) * samplesPerPulse + sample;
                    int inputIdxCurr = pulse * samplesPerPulse + sample;
                    int outputIdx = (pulse - 1) * samplesPerPulse + sample;
                    outputData[static_cast<size_t>(outputIdx)] = m_inputBuffer[static_cast<size_t>(inputIdxCurr)] - m_inputBuffer[static_cast<size_t>(inputIdxPrev)];
                }
            }
        } else {
            for (int pulse = 2; pulse < m_numOfPulse; ++pulse) {
                for (int sample = 0; sample < samplesPerPulse; ++sample) {
                    int inputIdxPrev2 = (pulse - 2) * samplesPerPulse + sample;
                    int inputIdxPrev1 = (pulse - 1) * samplesPerPulse + sample;
                    int inputIdxCurr = pulse * samplesPerPulse + sample;
                    int outputIdx = (pulse - 2) * samplesPerPulse + sample;
                    outputData[static_cast<size_t>(outputIdx)] = m_inputBuffer[static_cast<size_t>(inputIdxCurr)]
                        - m_inputBuffer[static_cast<size_t>(inputIdxPrev1)] * 2.0
                        + m_inputBuffer[static_cast<size_t>(inputIdxPrev2)];
                }
            }
        }
        for(const auto& val : outputData) m_outputQueue.push(val);

        // 步骤5：将处理结果写入输出端口
        if(!m_outputQueue.empty()) {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;
            WriteOutputData(outputPort, std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();
            qDebug() << "[RADAR_MTI_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << outputValue.imag();
        }
    }
    return true;
}

void RADAR_MTI_Block::SetParameters(double pri, double sampleRate, int numOfPulse, RADAR_MTI::SelectedMTI_Type mtiType)
{
    m_pri = pri;
    m_sampleRate = sampleRate;
    m_numOfPulse = numOfPulse;
    m_mtiType = mtiType;

    if (m_radarMti) {
        m_radarMti->PRI = m_pri;
        m_radarMti->SampleRate = m_sampleRate;
        m_radarMti->NumOfPulse = m_numOfPulse;
        m_radarMti->MTI_Type = m_mtiType;
    }
}

bool RADAR_MTI_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool RADAR_MTI_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_MTI_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_radarMti = std::make_unique<RADAR_MTI>();

    SetDefaultParamters();

    try { m_pri = std::stod(getParameter("PRI").Value); } catch (...) { }
    try { m_numOfPulse = std::stoi(getParameter("NumOfPulse").Value); } catch (...) { }
    try { m_mtiType = ConvertStringToMTIType(getParameter("MTI_Type").Value); } catch (...) { }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }

    SetParameters(m_pri, m_sampleRate, m_numOfPulse, m_mtiType);

    const int samplesPerPulse = static_cast<int>(std::round(m_pri * m_sampleRate));
    const int inputTotalSamples = m_numOfPulse * samplesPerPulse;
    const int outputPulseNum = (m_mtiType == RADAR_MTI::TwoPulseCanceller) ? (m_numOfPulse - 1) : (m_numOfPulse - 2);
    const int outputTotalSamples = outputPulseNum * samplesPerPulse;

    AddInputPort("input", m_radarMti->input, static_cast<size_t>(inputTotalSamples), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_radarMti->output, static_cast<size_t>(outputTotalSamples), Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

RADAR_MTI::SelectedMTI_Type RADAR_MTI_Block::ConvertStringToMTIType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "twopulsecanceller") {
        return RADAR_MTI::TwoPulseCanceller;
    }
    if (lower == "threepulsecanceller" || lower == "three pulse canceller" || lower == "1") {
        return RADAR_MTI::ThreePulseCanceller;
    }
    return RADAR_MTI::TwoPulseCanceller;
}
