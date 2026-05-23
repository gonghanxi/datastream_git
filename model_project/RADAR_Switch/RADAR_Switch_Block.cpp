#include "RADAR_Switch_Block.h"

#include <cmath>
#include <string>
#include <vector>

RADAR_Switch_Block::RADAR_Switch_Block(const std::string& name)
    : Block(name)
{
}

bool RADAR_Switch_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

void RADAR_Switch_Block::SetDefaultParameters()
{
    m_PRF = 10e3;
    m_SwitchOff_Time = 5e-6;
}

void RADAR_Switch_Block::SetParameters()
{
    if (!m_radarSwitch) {
        return;
    }

    m_radarSwitch->PRF = m_PRF;
    m_radarSwitch->SwitchOff_Time = m_SwitchOff_Time;
}

bool RADAR_Switch_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_radarSwitch = std::make_unique<RADAR_Switch>();

    AddInputPort("input", m_radarSwitch->input, 1, Block::DataType::ENVELOPE_SIGNAL);
    AddInputPort("PRI", m_radarSwitch->PRI, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_radarSwitch->output, 1, Block::DataType::ENVELOPE_SIGNAL);

    SetDefaultParameters();

    try { m_PRF = std::stod(getParameter("PRF").Value); } catch (...) { }
    try { m_SwitchOff_Time = std::stod(getParameter("SwitchOff_Time").Value); } catch (...) { }

    SetParameters();

    return true;
}

bool RADAR_Switch_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string priPortName = GetInputPortName(1);
    std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPortName);
    if (inputData.empty()) {
        return false;
    }

    std::vector<double> priData = ReadInputData<double>(priPortName);
    bool priConnected = !priData.empty();

    const SimuParameter simulator_param = getSimu();
    const double t = (simulator_param.samplingRate > 0.0)
        ? (simulator_param.startTime + static_cast<double>(m_radarSwitch->GetCount()) / simulator_param.samplingRate + 1e-16)
        : 0.0;

    std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
    outputData.reserve(inputData.size());

    for (size_t i = 0; i < inputData.size(); ++i) {
        double pri = 1.0 / m_PRF;
        if (priConnected && i < priData.size() && priData[i] > 0.0) {
            pri = priData[i];
        }

        if (std::fmod(t, pri) < m_SwitchOff_Time) {
            outputData.push_back(SystemVueModelBuilder::EnvelopeSignal(0.0));
        } else {
            outputData.push_back(inputData[i]);
        }
    }

    WriteOutputData(outputPortName, outputData);
    m_radarSwitch->Advance();
    return true;
}

bool RADAR_Switch_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string priPortName = GetInputPortName(1);
    std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPortName);
    if (inputData.empty()) {
        return true;
    }
    m_inputBuffer.push_back(inputData[0]);


    bool priConnected = GetInputPort(priPortName)->IsConnected();
    if(priConnected) {
        std::vector<double> priData = ReadInputData<double>(priPortName);
        if(priData.empty()) return true;
        m_RPIBuffer.push_back(priData[0]);
    }

    bool Canprocssdata = false;
    if(priConnected) {
        if(m_inputBuffer.size() >= 1 && m_RPIBuffer.size() >= 1) {
            Canprocssdata = true;
        }
    }
    else {
        if(m_inputBuffer.size() >= 1) {
            Canprocssdata = true;
        }
    }
    if(Canprocssdata) {
        std::vector<double> priData = ReadInputData<double>(priPortName);
        bool priConnected = !priData.empty();

        const SimuParameter simulator_param = getSimu();
        const double t = (simulator_param.samplingRate > 0.0)
            ? (simulator_param.startTime + static_cast<double>(m_radarSwitch->GetCount()) / simulator_param.samplingRate + 1e-16)
            : 0.0;

        std::vector<SystemVueModelBuilder::EnvelopeSignal> outputData;
        outputData.reserve(inputData.size());

        for (size_t i = 0; i < inputData.size(); ++i) {
            double pri = 1.0 / m_PRF;
            if (priConnected && i < priData.size() && priData[i] > 0.0) {
                pri = priData[i];
            }

            if (std::fmod(t, pri) < m_SwitchOff_Time) {
                outputData.push_back(SystemVueModelBuilder::EnvelopeSignal(0.0));
            } else {
                outputData.push_back(inputData[i]);
            }
        }

        m_outputQueue.push(outputData[0]);
        // 步骤5：将处理结果写入输出端口
        if(!m_outputQueue.empty()) {
            EnvelopeSignal outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;
            WriteOutputData(outputPortName, std::vector<EnvelopeSignal>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();
            qDebug() << "[RADAR_Switch_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << outputValue.imag();
        }
        m_radarSwitch->Advance();
    }
    return true;
}

bool RADAR_Switch_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

