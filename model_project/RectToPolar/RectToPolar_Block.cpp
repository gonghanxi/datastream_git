#include "RectToPolar_Block.h"

RectToPolar_Block::RectToPolar_Block(const std::string &name)
    :Block(name)
{

}

bool RectToPolar_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RectToPolar_Block::Run()
{

    if (!CanProcess()) {
        return false;
    }

    qDebug() << "RectToPolar_Block --Run begin";
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
