#include "RADAR_GainCtrl_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

RADAR_GainCtrl_Block::RADAR_GainCtrl_Block(const std::string& name)
    : Block(name)
{
}

bool RADAR_GainCtrl_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

static std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(0, s.find_first_not_of(" \t\n\r"));
    s.erase(s.find_last_not_of(" \t\n\r") + 1);
    return s;
}

static std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

RADAR_GainCtrl::SelectedControlType RADAR_GainCtrl_Block::ConvertStringToSelectedControlType(const std::string& value)
{
    const std::string lowerValue = ToLowerCopy(TrimCopy(value));
    if (lowerValue == "mgc" || lowerValue == "manual gain ctrl" || lowerValue == "0") {
        return RADAR_GainCtrl::MGC;
    }
    if (lowerValue == "stc" || lowerValue == "sensitivity time ctrl" || lowerValue == "1") {
        return RADAR_GainCtrl::STC;
    }
    if (lowerValue == "agc" || lowerValue == "instantaneous automatic gain ctrl/fast time constant(ftc)" || lowerValue == "2") {
        return RADAR_GainCtrl::AGC;
    }
    return RADAR_GainCtrl::MGC;
}

void RADAR_GainCtrl_Block::SetDefaultParameters()
{
    m_ControlType = RADAR_GainCtrl::MGC;
    m_PRI = 10e-3;
    m_Gain = 0.0;
    m_STC_Factor = 4.0;
    m_STC_StartTime = 2e-6;
    m_STC_StopTime = 60e-6;
    m_STC_K_Coef = 1e-4;
}

bool RADAR_GainCtrl_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string gainPortName = GetInputPortName(1);
    std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPortName);
    if (inputData.empty()) {
        return false;
    }

    std::vector<double> gainData = ReadInputData<double>(gainPortName);
    bool gainConnected = !gainData.empty();

    const SimuParameter simulator_param = getSimu();
    const double t = (simulator_param.samplingRate > 0.0)
        ? (simulator_param.startTime + static_cast<double>(m_radarGainCtrl->GetCount()) / simulator_param.samplingRate)
        : 0.0;

    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
    outputData.reserve(inputData.size());

    for (size_t i = 0; i < inputData.size(); ++i) {
        double localGain = m_Gain;
        if (gainConnected && i < gainData.size()) {
            localGain = gainData[i];
        }

        SystemVueModelBuilder::EnvelopeSignal result = inputData[i];

        switch (m_ControlType) {
        case RADAR_GainCtrl::MGC:
            result = result * std::pow(10.0, localGain / 10.0);
            break;
        case RADAR_GainCtrl::STC: {
            const double c = 3e8;
            double Pc = 1.0;
            double tInPRI = std::fmod(t, m_PRI);

            if (tInPRI >= m_STC_StartTime && tInPRI < m_STC_StopTime) {
                double R = c * tInPRI / 2.0;
                Pc = m_STC_K_Coef * std::pow(R, m_STC_Factor) * std::pow(10.0, -4.0 * (m_STC_Factor - 1.0));
            } else if (tInPRI >= m_STC_StopTime) {
                double R = c * m_STC_StopTime / 2.0;
                Pc = m_STC_K_Coef * std::pow(R, m_STC_Factor) * std::pow(10.0, -4.0 * (m_STC_Factor - 1.0));
            }
            result = result * Pc;
            break;
        }
        case RADAR_GainCtrl::AGC:
            result = SystemVueModelBuilder::EnvelopeSignal(0.0);
            break;
        default:
            break;
        }

        outputData.push_back(result);
    }

    WriteOutputData(outputPortName, outputData);
    m_radarGainCtrl->Advance();
    return true;
}

bool RADAR_GainCtrl_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string gainPortName = GetInputPortName(1);
    std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPortName);
    if (inputData.empty()) {
        return true;
    }
    m_inputBuffer.push_back(inputData[0]);
    bool gainConnected = GetInputPort(gainPortName)->IsConnected();
    if(gainConnected) {
        auto gainData = ReadInputData<double>(gainPortName);
        m_gainBuffer.push_back(gainData[0]);
    }

    bool CanprocessData = false;
    if(gainConnected) {
        if(m_inputBuffer.size() >= 1 && m_gainBuffer.size() >= 1) {
            CanprocessData = true;
        }
    }
    else {
        if(m_inputBuffer.size() >= 1) {
            CanprocessData = true;
        }
    }

    if(CanprocessData) {
        const SimuParameter simulator_param = getSimu();
        const double t = (simulator_param.samplingRate > 0.0)
            ? (simulator_param.startTime + static_cast<double>(m_radarGainCtrl->GetCount()) / simulator_param.samplingRate)
            : 0.0;

        std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
        outputData.reserve(m_inputBuffer.size());

        for (size_t i = 0; i < m_inputBuffer.size(); ++i) {
            double localGain = m_Gain;
            if (gainConnected && i < m_gainBuffer.size()) {
                localGain = m_gainBuffer[i];
            }

            SystemVueModelBuilder::EnvelopeSignal result = m_inputBuffer[i];

            switch (m_ControlType) {
            case RADAR_GainCtrl::MGC:
                result = result * std::pow(10.0, localGain / 10.0);
                break;
            case RADAR_GainCtrl::STC: {
                const double c = 3e8;
                double Pc = 1.0;
                double tInPRI = std::fmod(t, m_PRI);

                if (tInPRI >= m_STC_StartTime && tInPRI < m_STC_StopTime) {
                    double R = c * tInPRI / 2.0;
                    Pc = m_STC_K_Coef * std::pow(R, m_STC_Factor) * std::pow(10.0, -4.0 * (m_STC_Factor - 1.0));
                } else if (tInPRI >= m_STC_StopTime) {
                    double R = c * m_STC_StopTime / 2.0;
                    Pc = m_STC_K_Coef * std::pow(R, m_STC_Factor) * std::pow(10.0, -4.0 * (m_STC_Factor - 1.0));
                }
                result = result * Pc;
                break;
            }
            case RADAR_GainCtrl::AGC:
                result = SystemVueModelBuilder::EnvelopeSignal(0.0);
                break;
            default:
                break;
            }

            outputData.push_back(result);

        }
        m_outputQueue.push(outputData[0]);
        if(!m_outputQueue.empty()) {
            EnvelopeSignal outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;
            WriteOutputData(outputPortName, std::vector<EnvelopeSignal>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();
            m_gainBuffer.clear();
            qDebug() << "[RADAR_GainCtrl_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << outputValue.imag();
        }
        m_radarGainCtrl->Advance();
    }
    return true;
}

void RADAR_GainCtrl_Block::SetParameters()
{
    if (!m_radarGainCtrl) {
        return;
    }

    m_radarGainCtrl->ControlType = m_ControlType;
    m_radarGainCtrl->PRI = m_PRI;
    m_radarGainCtrl->Gain = m_Gain;
    m_radarGainCtrl->STC_Factor = m_STC_Factor;
    m_radarGainCtrl->STC_StartTime = m_STC_StartTime;
    m_radarGainCtrl->STC_StopTime = m_STC_StopTime;
    m_radarGainCtrl->STC_K_Coef = m_STC_K_Coef;
}

bool RADAR_GainCtrl_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_radarGainCtrl = std::make_unique<RADAR_GainCtrl>();

    AddInputPort("input", m_radarGainCtrl->input, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddInputPort("gain", m_radarGainCtrl->gain, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_radarGainCtrl->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParameters();

    try { m_ControlType = ConvertStringToSelectedControlType(getParameter("ControlType").Value); } catch (...) { }
    try { m_PRI = std::stod(getParameter("PRI").Value); } catch (...) { }
    try { m_Gain = std::stod(getParameter("Gain").Value); } catch (...) { }
    try { m_STC_Factor = std::stod(getParameter("STC_Factor").Value); } catch (...) { }
    try { m_STC_StartTime = std::stod(getParameter("STC_StartTime").Value); } catch (...) { }
    try { m_STC_StopTime = std::stod(getParameter("STC_StopTime").Value); } catch (...) { }
    try { m_STC_K_Coef = std::stod(getParameter("STC_K_Coef").Value); } catch (...) { }

    SetParameters();

    return true;
}

bool RADAR_GainCtrl_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}


