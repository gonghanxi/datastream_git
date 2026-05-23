#include "RADAR_UnAmbRange_Block.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

RADAR_UnAmbRange_Block::RADAR_UnAmbRange_Block(const std::string& name)
    : Block(name)
    , m_PRI(1, 1)
{
    m_PRI(0) = 1e-4;
}

bool RADAR_UnAmbRange_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

void RADAR_UnAmbRange_Block::SetDefaultParameters()
{
    m_PRI.Resize(1, 1);
    m_PRI(0) = 1e-4;
    m_CPI_Num = 32;
    m_SampleRate = 10e6;
}

bool RADAR_UnAmbRange_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    auto inputData = ReadInputData<int>(inputPortName);
    if (inputData.empty()) {
        return false;
    }

    const size_t numInputs = inputData.size();
    const size_t numPRI = static_cast<size_t>(m_PRI.NumElements());
    if (numInputs != numPRI) {
        LOG_ERROR("Num of PRIs must match num of inputs");
        return false;
    }

    if (numInputs < 2) {
        LOG_ERROR("need at least 2 PRI inputs");
        return false;
    }

    const double c = 3e8;
    const double sampleRate = (m_SampleRate > 0.0) ? m_SampleRate : 1.0;

    double t1 = static_cast<double>(inputData[0]) / sampleRate;
    double t2 = static_cast<double>(inputData[1]) / sampleRate;

    while (std::abs(t1 - t2) > 1.0 / sampleRate) {
        if (t1 < t2) {
            t1 += m_PRI(0);
        } else if (t1 > t2) {
            t2 += m_PRI(1);
        }

        if (t1 > m_PRI(0) * m_CPI_Num / 4.0 || t2 > m_PRI(1) * m_CPI_Num / 4.0) {
            LOG_ERROR("The maximum range is over the limitation.");
            return false;
        }
    }

    const double rangeValue = c * t1 / 2.0;

    std::vector<double> outputData;
    outputData.push_back(rangeValue);
    WriteOutputData(outputPortName, outputData);

    return true;
}

bool RADAR_UnAmbRange_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    BufferReader* master_reader = GetInputPort(inputPort);
    auto bridge_readers = master_reader->GetBusConnections();

    for(const auto& bridge_reader : bridge_readers) {
        std::vector<double> inputData;
        bridge_reader.bridgeReader->ReadData(inputData);
        if(inputData.empty()) {
            //上游没产生，
            return true;
        }
        for(size_t i = 0; i < inputData.size();i++) {
            m_inputBuffer[bridge_reader.bridgeReader].push_back(inputData[i]);
        }
    }

    bool CanProcessData = true;
    for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
        if(it->second.size() >= 1) {
            CanProcessData = false;
            break;
        }
    }

    const size_t numInputs = master_reader->GetBusConnectionCount();
    const size_t numPRI = static_cast<size_t>(m_PRI.NumElements());
    if (numInputs != numPRI) {
        LOG_ERROR("Num of PRIs must match num of inputs");
        return false;
    }

    if (numInputs < 2) {
        LOG_ERROR("need at least 2 PRI inputs");
        return false;
    }

    const double c = 3e8;
    const double sampleRate = (m_SampleRate > 0.0) ? m_SampleRate : 1.0;


    auto it = m_inputBuffer.begin();
    const std::vector<int>& firstChannelData = it->second;

    // 移动到第二个BufferReader
    ++it;
    const std::vector<int>& secondChannelData = it->second;


    if (firstChannelData.empty() || secondChannelData.empty()) {
        LOG_ERROR("Input data is empty");
        return false;
    }


    double t1 = firstChannelData[0] / sampleRate;
    double t2 = secondChannelData[0] / sampleRate;

    while (std::abs(t1 - t2) > 1.0 / sampleRate) {
        if (t1 < t2) {
            t1 += m_PRI(0);
        } else if (t1 > t2) {
            t2 += m_PRI(1);
        }

        if (t1 > m_PRI(0) * m_CPI_Num / 4.0 || t2 > m_PRI(1) * m_CPI_Num / 4.0) {
            LOG_ERROR("The maximum range is over the limitation.");
            return false;
        }
    }

    const double rangeValue = c * t1 / 2.0;

    std::vector<double> outputData;
    outputData.push_back(rangeValue);
    if (!m_outputQueue.empty()) {
        double outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;

        WriteOutputData(outputPort, std::vector<double>{outputValue});
        m_lastOutput = outputValue;

        qDebug() << "[RADAR_UnAmbRange_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue;
        m_inputBuffer.clear();
    }
    return true;
}

void RADAR_UnAmbRange_Block::SetParameters()
{
    if (!m_radarUnAmbRange) {
        return;
    }

    m_radarUnAmbRange->PRI = m_PRI;
    m_radarUnAmbRange->CPI_Num = m_CPI_Num;
    m_radarUnAmbRange->SampleRate = m_SampleRate;
}

bool RADAR_UnAmbRange_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_radarUnAmbRange = std::make_unique<RADAR_UnAmbRange>();

    AddInputPort("Index", m_radarUnAmbRange->Index, 1, Block::DataType::INT_BUS);
    AddOutputPort("Range", m_radarUnAmbRange->Range, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    SetDefaultParameters();

    try { m_PRI = DataTypesAndParsers::ParseStringToMatrixDouble(getParameter("PRI").Value); } catch (...) { }
    try { m_CPI_Num = std::stoi(getParameter("CPI_Num").Value); } catch (...) { }
    try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }

    SetParameters();

    return true;
}

bool RADAR_UnAmbRange_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}
