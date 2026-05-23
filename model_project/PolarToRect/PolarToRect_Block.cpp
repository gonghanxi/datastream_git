#include "PolarToRect_Block.h"

PolarToRect_Block::PolarToRect_Block(const std::string &name)
    :Block(name)
{

}

bool PolarToRect_Block::Setup()
{
    Block::Setup();
    while(!m_xQueue.empty()) m_xQueue.pop();
    while(!m_yQueue.empty()) m_yQueue.pop();
    return true;
}

bool PolarToRect_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool PolarToRect_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_PolarToRect = std::make_unique<PolarToRect>();

    AddInputPort("magnitude", m_PolarToRect->magnitude, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("phase", m_PolarToRect->phase, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("x", m_PolarToRect->x, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("y", m_PolarToRect->y, 1, DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

bool PolarToRect_Block::DataStreamRun()
{
    std::string magnitudePort = GetInputPortName(0);
    std::string phasePort = GetInputPortName(1);
    std::string xPort = GetOutputPortName(0);
    std::string yPort = GetOutputPortName(1);

    auto magnitudeData = ReadInputData<double>(magnitudePort);
    auto phaseData = ReadInputData<double>(phasePort);
    if (magnitudeData.empty() || phaseData.empty()) {
        return false;
    }
    std::vector<double> xData(magnitudeData.size());
    std::vector<double> yData(magnitudeData.size());

    xData[0] = magnitudeData[0] * std::cos(phaseData[0]);
    yData[0] = magnitudeData[0] * std::sin(phaseData[0]);

    WriteOutputData(xPort, xData);
    WriteOutputData(yPort, yData);
    return true;
}

bool PolarToRect_Block::TimeDrivenRun()
{
    std::string magnitudePort = GetInputPortName(0);
    std::string phasePort = GetInputPortName(1);
    std::string xPort = GetOutputPortName(0);
    std::string yPort = GetOutputPortName(1);

    auto magnitudeData = ReadInputData<double>(magnitudePort);
    auto phaseData = ReadInputData<double>(phasePort);
    if (magnitudeData.empty() || phaseData.empty()) {
        return true;
    }
    m_magBuffer.push_back(magnitudeData[0]);
    m_phaseBuffer.push_back(phaseData[0]);

    if(m_magBuffer.size() >= 1 && m_phaseBuffer.size() >= 1) {
        std::vector<double> xData(m_magBuffer.size());
        std::vector<double> yData(m_magBuffer.size());

        xData[0] = m_magBuffer[0] * std::cos(m_phaseBuffer[0]);
        yData[0] = m_magBuffer[0] * std::sin(m_phaseBuffer[0]);

        m_xQueue.push(xData[0]);
        m_yQueue.push(yData[0]);
        //执行写入
        if (!m_xQueue.empty() && !m_yQueue.empty()) {
            double xValue = m_xQueue.front();
            double yValue = m_yQueue.front();
            m_xQueue.pop();
            m_yQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<double>{xValue});
            WriteOutputData(GetOutputPortName(1), std::vector<double>{yValue});
            m_lastx = xValue;
            m_lasty = yValue;

            qDebug() << "[PolarToRect_Block] 分发输出:" << m_outputCount
                     << " value:" << xValue << "|" << yValue;
            m_magBuffer.clear();
            m_phaseBuffer.clear();
        }
    }
    return true;
}
