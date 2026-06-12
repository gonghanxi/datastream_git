#include "RADAR_UnAmbVelocity_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

RADAR_UnAmbVelocity_Block::RADAR_UnAmbVelocity_Block(const std::string& name)
    : Block(name)
    , m_PRI(1, 1)
{
    m_PRI(0) = 1e-4;
}

bool RADAR_UnAmbVelocity_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
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

RADAR_UnAmbVelocity::SelectedDirection RADAR_UnAmbVelocity_Block::ConvertStringToSelectedDirection(const std::string& value)
{
    const std::string lowerValue = ToLowerCopy(TrimCopy(value));
    if (lowerValue == "approachingradar" || lowerValue == "approaching radar" || lowerValue == "0") {
        return RADAR_UnAmbVelocity::ApproachingRadar;
    }
    if (lowerValue == "leavingradar" || lowerValue == "leaving radar" || lowerValue == "1") {
        return RADAR_UnAmbVelocity::LeavingRadar;
    }
    return RADAR_UnAmbVelocity::ApproachingRadar;
}

void RADAR_UnAmbVelocity_Block::SetDefaultParameters()
{
    m_PRI.Resize(1, 1);
    m_PRI(0) = 1e-4;
    m_CPI_Num = 32;
    m_fc = 10e9;
    m_SampleRate = 10e6;
    m_Direction = RADAR_UnAmbVelocity::ApproachingRadar;
}



void RADAR_UnAmbVelocity_Block::SetParameters()
{
    if (!m_radarUnAmbVelocity) {
        return;
    }

    m_radarUnAmbVelocity->PRI = m_PRI;
    m_radarUnAmbVelocity->CPI_Num = m_CPI_Num;
    m_radarUnAmbVelocity->fc = m_fc;
    m_radarUnAmbVelocity->SampleRate = m_SampleRate;
    m_radarUnAmbVelocity->Direction = m_Direction;
}

bool RADAR_UnAmbVelocity_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_radarUnAmbVelocity = std::make_unique<RADAR_UnAmbVelocity>();

    AddInputPort("Index", m_radarUnAmbVelocity->Index, 1, Block::DataType::INT_BUS);
    AddOutputPort("Velocity", m_radarUnAmbVelocity->Velocity, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    SetDefaultParameters();

    try { m_PRI = DataTypesAndParsers::ParseStringToMatrixDouble(getParameter("PRI").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PRI', using default value."); }
    try { m_CPI_Num = std::stoi(getParameter("CPI_Num").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'CPI_Num', using default value."); }
    try { m_fc = std::stod(getParameter("fc").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'fc', using default value."); }
    try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }
    try { m_Direction = ConvertStringToSelectedDirection(getParameter("Direction").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Direction', using default value."); }

    SetParameters();

    return true;
}

bool RADAR_UnAmbVelocity_Block::DataStreamRun()
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
        LOG_ERROR("RADAR_UnAmbVelocity: Num of PRIs must match num of inputs");
        return false;
    }

    if (numInputs < 2) {
        LOG_ERROR("RADAR_UnAmbVelocity: need at least 2 PRI inputs");
        return false;
    }

    const double c = 3e8;

    const double pri0 = m_PRI(0);
    const double pri1 = m_PRI(1);

    const double prf1 = 1.0 / pri0;
    const double prf2 = 1.0 / pri1;

    double fd1 = prf1 * static_cast<double>(inputData[0]) / static_cast<double>(m_CPI_Num);
    double fd2 = prf2 * static_cast<double>(inputData[1]) / static_cast<double>(m_CPI_Num);

    switch (m_Direction) {
    case RADAR_UnAmbVelocity::ApproachingRadar:
        while (std::abs(fd1 - fd2) > prf1 / static_cast<double>(m_CPI_Num)) {
            if (fd1 < fd2) {
                fd1 += prf1;
            } else if (fd1 > fd2) {
                fd2 += prf2;
            }

            if (fd1 > prf1 * m_CPI_Num / 4.0 || fd2 > prf2 * m_CPI_Num / 4.0) {
                LOG_ERROR("RADAR_UnAmbVelocity: The maximum velocity is over the limitation.");
                return false;
            }
        }
        break;
    case RADAR_UnAmbVelocity::LeavingRadar:
        while (std::abs(fd1 - fd2) > prf1 / static_cast<double>(m_CPI_Num)) {
            if (fd1 > fd2) {
                fd1 -= prf1;
            } else if (fd1 < fd2) {
                fd2 -= prf2;
            }

            if (fd1 < -prf1 * m_CPI_Num / 4.0 || fd2 < -prf2 * m_CPI_Num / 4.0) {
                LOG_ERROR("RADAR_UnAmbVelocity: The maximum velocity is over the limitation.");
                return false;
            }
        }
        break;
    default:
        break;
    }


    const double lambda = c / m_fc;
    const double velocityValue = fd1 * lambda / 2.0;

    std::vector<double> outputData;
    outputData.push_back(velocityValue);
    WriteOutputData(outputPortName, outputData);

    return true;
}

bool RADAR_UnAmbVelocity_Block::TimeDrivenRun()
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

    const double pri0 = m_PRI(0);
    const double pri1 = m_PRI(1);

    const double prf1 = 1.0 / pri0;
    const double prf2 = 1.0 / pri1;


    auto it = m_inputBuffer.begin();
    const std::vector<int>& firstChannelData = it->second;

    // 移动到第二个BufferReader
    ++it;
    const std::vector<int>& secondChannelData = it->second;


    if (firstChannelData.empty() || secondChannelData.empty()) {
        LOG_ERROR("Input data is empty");
        return false;
    }


    double fd1 = prf1 * static_cast<double>(firstChannelData[0]) / static_cast<double>(m_CPI_Num);
    double fd2 = prf2 * static_cast<double>(secondChannelData[0]) / static_cast<double>(m_CPI_Num);

    switch (m_Direction) {
    case RADAR_UnAmbVelocity::ApproachingRadar:
        while (std::abs(fd1 - fd2) > prf1 / static_cast<double>(m_CPI_Num)) {
            if (fd1 < fd2) {
                fd1 += prf1;
            } else if (fd1 > fd2) {
                fd2 += prf2;
            }

            if (fd1 > prf1 * m_CPI_Num / 4.0 || fd2 > prf2 * m_CPI_Num / 4.0) {
                LOG_ERROR("RADAR_UnAmbVelocity: The maximum velocity is over the limitation.");
                return false;
            }
        }
        break;
    case RADAR_UnAmbVelocity::LeavingRadar:
        while (std::abs(fd1 - fd2) > prf1 / static_cast<double>(m_CPI_Num)) {
            if (fd1 > fd2) {
                fd1 -= prf1;
            } else if (fd1 < fd2) {
                fd2 -= prf2;
            }

            if (fd1 < -prf1 * m_CPI_Num / 4.0 || fd2 < -prf2 * m_CPI_Num / 4.0) {
                LOG_ERROR("RADAR_UnAmbVelocity: The maximum velocity is over the limitation.");
                return false;
            }
        }
        break;
    default:
        break;
    }


    const double lambda = c / m_fc;
    const double velocityValue = fd1 * lambda / 2.0;

    std::vector<double> outputData;
    outputData.push_back(velocityValue);
    if (!m_outputQueue.empty()) {
        double outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;

        WriteOutputData(outputPort, std::vector<double>{outputValue});
        m_lastOutput = outputValue;

        qDebug() << "[RADAR_UnAmbVelocity_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue;
        m_inputBuffer.clear();
    }
    return true;
}

bool RADAR_UnAmbVelocity_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}
