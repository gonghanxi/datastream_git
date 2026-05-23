#include "RADAR_VelocityMeas_Block.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

RADAR_VelocityMeas_Block::RADAR_VelocityMeas_Block(const std::string& name)
    : Block(name)
{
}

bool RADAR_VelocityMeas_Block::Setup()
{
    Block::Setup();
    return true;
}

void RADAR_VelocityMeas_Block::SetDefaultParameters()
{
    m_PRI = 1e-4;
    m_CPI_Num = 32;
    m_SampleRate = 10e6;
    m_fc = 10e9;
    m_PRINum = 0;
    m_portRate = 0;
}

bool RADAR_VelocityMeas_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputVelocityPortName = GetOutputPortName(0);
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

    int fmaxIndex = 0;
    if (m_PRINum > 0) {
        fmaxIndex = static_cast<int>(std::floor(static_cast<double>(maxIndex) / static_cast<double>(m_PRINum)));
    }

    const double prf = 1.0 / m_PRI;
    const double fd = prf * static_cast<double>(fmaxIndex) / static_cast<double>(m_CPI_Num);

    const double c = 3e8;
    const double lambda = c / m_fc;
    const double velocityValue = fd * lambda / 2.0;

    std::vector<double> velocityData;
    velocityData.push_back(velocityValue);
    WriteOutputData(outputVelocityPortName, velocityData);

    std::vector<int> indexData;
    indexData.push_back(fmaxIndex);
    WriteOutputData(outputIndexPortName, indexData);

    return true;
}

bool RADAR_VelocityMeas_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputVelocityPortName = GetOutputPortName(0);
    std::string outputIndexPortName = GetOutputPortName(1);

    auto inputData = ReadInputData<double>(inputPortName);
    if (inputData.empty()) {
        return true;
    }
    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= static_cast<size_t>(m_portRate)) {
        const int maxCount = static_cast<int>(std::min<size_t>(m_inputBuffer.size(), static_cast<size_t>(m_portRate)));

        double maxValue = 0.0;
        int maxIndex = 0;
        for (int i = 0; i < maxCount; ++i) {
            if (m_inputBuffer[static_cast<size_t>(i)] > maxValue) {
                maxValue = m_inputBuffer[static_cast<size_t>(i)];
                maxIndex = i;
            }
        }

        int fmaxIndex = 0;
        if (m_PRINum > 0) {
            fmaxIndex = static_cast<int>(std::floor(static_cast<double>(maxIndex) / static_cast<double>(m_PRINum)));
        }

        const double prf = 1.0 / m_PRI;
        const double fd = prf * static_cast<double>(fmaxIndex) / static_cast<double>(m_CPI_Num);

        const double c = 3e8;
        const double lambda = c / m_fc;
        const double velocityValue = fd * lambda / 2.0;

        m_VelocitytQueue.push(velocityValue);
        m_IndextQueue.push(fmaxIndex);
    }


    return true;
}

void RADAR_VelocityMeas_Block::SetParameters()
{
    if (!m_radarVelocityMeas) {
        return;
    }

    m_radarVelocityMeas->PRI = m_PRI;
    m_radarVelocityMeas->CPI_Num = m_CPI_Num;
    m_radarVelocityMeas->SampleRate = m_SampleRate;
    m_radarVelocityMeas->fc = m_fc;
    m_radarVelocityMeas->PRINum = m_PRINum;
    m_radarVelocityMeas->portRate = m_portRate;
}

bool RADAR_VelocityMeas_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_radarVelocityMeas = std::make_unique<RADAR_VelocityMeas>();

    SetDefaultParameters();

    try { m_PRI = std::stod(getParameter("PRI").Value); } catch (...) { }
    try { m_CPI_Num = std::stoi(getParameter("CPI_Num").Value); } catch (...) { }
    try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { }
    try { m_fc = std::stod(getParameter("fc").Value); } catch (...) { }

    m_PRINum = static_cast<int>(m_PRI * m_SampleRate);
    m_portRate = m_PRINum * m_CPI_Num;
    if (m_portRate <= 0) {
        LOG_ERROR("RADAR_VelocityMeas: input port rate PRI * SampleRate * CPI_Num must be greater than 0.");
        return false;
    }

    AddInputPort("input", m_radarVelocityMeas->input, static_cast<size_t>(m_portRate), Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Velocity", m_radarVelocityMeas->Velocity, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Index", m_radarVelocityMeas->Index, 1, Block::DataType::CIRCULAR_BUFFER_INT);

    SetParameters();

    return true;
}

bool RADAR_VelocityMeas_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}
