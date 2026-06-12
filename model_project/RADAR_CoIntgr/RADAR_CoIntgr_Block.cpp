#include "RADAR_CoIntgr_Block.h"
#include <complex>
#include <iostream>
#include <vector>

RADAR_CoIntgr_Block::RADAR_CoIntgr_Block(const std::string& name)
    : Block(name)
{
}

void RADAR_CoIntgr_Block::SetDefaultParamters()
{
    m_priOrWaveGate = 10e-3;
    m_numOfPulse = 32;
    m_sampleRate = 10e6;
}

bool RADAR_CoIntgr_Block::DataStreamRun()
{
    const std::string inputPort = GetInputPortName(0);
    const std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return false;
    }

    const size_t prn = static_cast<size_t>(m_priOrWaveGate * m_sampleRate);

    std::vector<std::complex<double>> outputData;
    outputData.resize(prn, std::complex<double>(0.0, 0.0));

    for (size_t i = 0; i < prn; ++i) {
        std::complex<double> acc(0.0, 0.0);
        for (int pulseIndex = 0; pulseIndex < m_numOfPulse; ++pulseIndex) {
            const size_t idx = static_cast<size_t>(pulseIndex) * prn + i;
            acc += inputData[idx];
        }
        outputData[i] = acc;
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

bool RADAR_CoIntgr_Block::TimeDrivenRun()
{
    const std::string inputPort = GetInputPortName(0);
    const std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= GetInputPort(inputPort)->GetReadSize()) {
        const size_t prn = static_cast<size_t>(m_priOrWaveGate * m_sampleRate);

        std::vector<std::complex<double>> outputData;
        outputData.resize(prn, std::complex<double>(0.0, 0.0));

        for (size_t i = 0; i < prn; ++i) {
            std::complex<double> acc(0.0, 0.0);
            for (int pulseIndex = 0; pulseIndex < m_numOfPulse; ++pulseIndex) {
                const size_t idx = static_cast<size_t>(pulseIndex) * prn + i;
                acc += m_inputBuffer[idx];
            }
            outputData[i] = acc;
        }
        for(const auto& val : outputData) m_outputQueue.push(val);
        //执行写入
        if (!m_outputQueue.empty()) {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[RADAR_CoIntgr_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
            m_inputBuffer.clear();
        }
    }
    return true;
}

void RADAR_CoIntgr_Block::SetParameters(double priOrWaveGate, int numOfPulse, double sampleRate)
{
    m_priOrWaveGate = priOrWaveGate;
    m_numOfPulse = numOfPulse;
    m_sampleRate = sampleRate;

    if (m_radarCoIntgr) {
        m_radarCoIntgr->PRI_Or_WaveGate = m_priOrWaveGate;
        m_radarCoIntgr->NumOfPulse = m_numOfPulse;
        m_radarCoIntgr->SampleRate = m_sampleRate;
    }
}

bool RADAR_CoIntgr_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool RADAR_CoIntgr_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_CoIntgr_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_radarCoIntgr = std::make_unique<RADAR_CoIntgr>();

    SetDefaultParamters();

    try { m_priOrWaveGate = std::stod(getParameter("PRI_Or_WaveGate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PRI_Or_WaveGate', using default value."); }
    try { m_numOfPulse = std::stoi(getParameter("NumOfPulse").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumOfPulse', using default value."); }
    try { m_sampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }

    SetParameters(m_priOrWaveGate, m_numOfPulse, m_sampleRate);



    const size_t prn = static_cast<size_t>(m_priOrWaveGate * m_sampleRate);
    const size_t inputRate = prn * static_cast<size_t>(m_numOfPulse);
    const size_t outputRate = prn;

    qDebug() << "RADAR_CoIntgr_Block::Initialize - prn: " << prn;

    if (inputRate == 0 || outputRate == 0) {
        std::cout << "error Port rate must be greater than 0." << std::endl;
        return false;
    }

    AddInputPort("input", m_radarCoIntgr->input, inputRate, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_radarCoIntgr->output, outputRate, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}
