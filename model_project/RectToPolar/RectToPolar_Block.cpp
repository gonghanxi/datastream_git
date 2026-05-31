#include "RectToPolar_Block.h"

RectToPolar_Block::RectToPolar_Block(const std::string &name)
    :Block(name)
{

}

bool RectToPolar_Block::Setup()
{
    Block::Setup();
    while(!m_phaseQueue.empty()) m_phaseQueue.pop();
    while(!m_magnitudeQueue.empty()) m_magnitudeQueue.pop();
    return true;
}

bool RectToPolar_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RectToPolar_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_RectToPolar = std::make_unique<RectToPolar>();

    AddOutputPort("magnitude", m_RectToPolar->magnitude, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("phase", m_RectToPolar->phase, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("x", m_RectToPolar->x, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("y", m_RectToPolar->y, 1, DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

bool RectToPolar_Block::DataStreamRun()
{
    std::string magnitudePort = GetOutputPortName(0);
    std::string phasePort = GetOutputPortName(1);

    std::string xPort = GetInputPortName(0);
    std::string yPort = GetInputPortName(1);

    auto xData = ReadInputData<double>(xPort);
    auto yData = ReadInputData<double>(yPort);
    if (xData.empty() || yData.empty()) {
        return false;
    }
    std::vector<double> magnitudeData(1);
    std::vector<double> phaseData(1);

    magnitudeData[0] = std::sqrt(std::pow(xData[0], 2) + std::pow(yData[0], 2));
    phaseData[0] = std::atan2(xData[0], yData[0]);

    WriteOutputData(magnitudePort, magnitudeData);
    WriteOutputData(phasePort, phaseData);
    return true;
}

bool RectToPolar_Block::TimeDrivenRun()
{
    std::string magnitudePort = GetOutputPortName(0);
    std::string phasePort = GetOutputPortName(1);

    std::string xPort = GetInputPortName(0);
    std::string yPort = GetInputPortName(1);

    auto xData = ReadInputData<double>(xPort);
    auto yData = ReadInputData<double>(yPort);
    if (xData.empty() || yData.empty()) {
        return true;
    }
    m_xBuffer.push_back(xData[0]);
    m_yBuffer.push_back(yData[0]);
    if(m_xBuffer.size() >= 1 && m_yBuffer.size() >= 1) {
        std::vector<double> magnitudeData(1);
        std::vector<double> phaseData(1);

        magnitudeData[0] = std::sqrt(std::pow(m_xBuffer[0], 2) + std::pow(m_yBuffer[0], 2));
        phaseData[0] = std::atan2(m_xBuffer[0], m_yBuffer[0]);

        m_phaseQueue.push(phaseData[0]);
        m_magnitudeQueue.push(magnitudeData[0]);
        if(!m_phaseQueue.empty() && !m_magnitudeQueue.empty()) {
            double phaseValue = m_phaseQueue.front();
            m_phaseQueue.pop();
            double magnitudeValue = m_magnitudeQueue.front();
            m_magnitudeQueue.pop();
            m_outputCount++;

            WriteOutputData(phasePort, std::vector<double>{phaseValue});
            WriteOutputData(magnitudePort, std::vector<double>{magnitudeValue});

            m_lastphase = phaseValue;
            m_lastmagnitude = magnitudeValue;

            qDebug() << "[RectToPolar_Block] 分发输出:" << m_outputCount
                     << " value:" << phaseValue << "|" << magnitudeValue;
            m_xBuffer.clear();
            m_yBuffer.clear();
        }
    }
    return true;
}
