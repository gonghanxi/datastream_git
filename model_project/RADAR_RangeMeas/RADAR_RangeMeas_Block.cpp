#include "RADAR_RangeMeas_Block.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

RADAR_RangeMeas_Block::RADAR_RangeMeas_Block(const std::string& name)
    : Block(name)
{
}

bool RADAR_RangeMeas_Block::Setup()
{
    Block::Setup();
    while(!m_RangeQueue.empty()) m_RangeQueue.pop();
    while(!m_IndexQueue.empty()) m_IndexQueue.pop();
    return true;
}

void RADAR_RangeMeas_Block::SetDefaultParameters()
{
    m_PRI = 1e-4;
    m_CPI_Num = 32;
    m_SampleRate = 10e6;
    m_PRINum = 0;
    m_portRate = 0;
}

bool RADAR_RangeMeas_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputRangePortName = GetOutputPortName(0);
    std::string outputIndexPortName = GetOutputPortName(1);

    auto inputData = ReadInputData<double>(inputPortName);
    if (inputData.empty()) {
        return false;
    }

    const int maxCount = static_cast<int>(std::min<size_t>(inputData.size(), static_cast<size_t>(m_portRate)));

    double maxValue = 0.0;
    int maxIndex = 0;
    for (int i = 0; i < maxCount; ++i) {
        if (inputData[static_cast<size_t>(i)] > maxValue) {
            maxValue = inputData[static_cast<size_t>(i)];
            maxIndex = i;
        }
    }

    if (m_PRINum > 0) {
        maxIndex = static_cast<int>(std::fmod(static_cast<double>(maxIndex), static_cast<double>(m_PRINum)));
    }

    const double c = 3e8;
    const double rangeValue = c * (static_cast<double>(maxIndex) / m_SampleRate) / 2.0;

    std::vector<double> rangeData;
    rangeData.push_back(rangeValue);
    WriteOutputData(outputRangePortName, rangeData);

    std::vector<int> indexData;
    indexData.push_back(maxIndex);
    WriteOutputData(outputIndexPortName, indexData);

    return true;
}

bool RADAR_RangeMeas_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputRangePortName = GetOutputPortName(0);
    std::string outputIndexPortName = GetOutputPortName(1);

    auto inputData = ReadInputData<double>(inputPortName);
    if (inputData.empty()) {
        return true;
    }

    const int maxCount = static_cast<int>(std::min<size_t>(inputData.size(), static_cast<size_t>(m_portRate)));

    double maxValue = 0.0;
    int maxIndex = 0;
    for (int i = 0; i < maxCount; ++i) {
        if (inputData[static_cast<size_t>(i)] > maxValue) {
            maxValue = inputData[static_cast<size_t>(i)];
            maxIndex = i;
        }
    }
    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= static_cast<size_t>(m_portRate)) {
        if (m_PRINum > 0) {
            maxIndex = static_cast<int>(std::fmod(static_cast<double>(maxIndex), static_cast<double>(m_PRINum)));
        }

        const double c = 3e8;
        const double rangeValue = c * (static_cast<double>(maxIndex) / m_SampleRate) / 2.0;

        m_RangeQueue.push(rangeValue);
        m_IndexQueue.push(maxIndex);

        if(!m_RangeQueue.empty() && !m_IndexQueue.empty()) {
            double RangeValue = m_RangeQueue.front();
            m_RangeQueue.pop();
            int IndexValue = m_IndexQueue.front();
            m_IndexQueue.pop();

            WriteOutputData(outputRangePortName, std::vector<double>{RangeValue});
            WriteOutputData(outputIndexPortName, std::vector<int>{IndexValue});
            m_outputCount++;

            m_lastRange = RangeValue;

            m_inputBuffer.clear();
            qDebug() << "[RADAR_RangeMeas_Block] 分发输出:" << m_outputCount
                     << " value:" << RangeValue  << "|" << IndexValue;
        }
    }
    return true;
}

void RADAR_RangeMeas_Block::SetParameters()
{
    if (!m_radarRangeMeas) {
        return;
    }

    m_radarRangeMeas->PRI = m_PRI;
    m_radarRangeMeas->CPI_Num = m_CPI_Num;
    m_radarRangeMeas->SampleRate = m_SampleRate;
    m_radarRangeMeas->PRINum = m_PRINum;
    m_radarRangeMeas->portRate = m_portRate;
}

bool RADAR_RangeMeas_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_radarRangeMeas = std::make_unique<RADAR_RangeMeas>();

    SetDefaultParameters();

    try { m_PRI = std::stod(getParameter("PRI").Value); } catch (...) { }
    try { m_CPI_Num = std::stoi(getParameter("CPI_Num").Value); } catch (...) { }
    try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }

    m_PRINum = static_cast<int>(m_PRI * m_SampleRate);
    m_portRate = m_PRINum * m_CPI_Num;
    if (m_portRate <= 0) {
        LOG_ERROR("input port rate PRI * SampleRate * CPI_Num must be greater than 0.");
        return false;
    }

    AddInputPort("input", m_radarRangeMeas->input, static_cast<size_t>(m_portRate), Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Range", m_radarRangeMeas->Range, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Index", m_radarRangeMeas->Index, 1, Block::DataType::CIRCULAR_BUFFER_INT);

    SetParameters();

    return true;
}

bool RADAR_RangeMeas_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}
