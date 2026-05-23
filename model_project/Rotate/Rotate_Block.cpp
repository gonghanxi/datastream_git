#include "Rotate_Block.h"

Rotate_Block::Rotate_Block(const std::string &name)
    :Block(name)
{

}

bool Rotate_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Rotate_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return false;
    }
    std::vector<std::complex<double>> outputData(inputData.size());

    const std::complex<double> j(0.0, 1.0);
    outputData[0] = inputData[0] * std::exp(j*m_RotationAngle);

    WriteOutputData(outputPort, outputData);
    return true;
}

bool Rotate_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Rotate = std::make_unique<Rotate>();

    SetDefaultParameters();

    try { m_RotationAngle = std::stod(getParameter("RotationAngle").Value); } catch (...) { }

    SetParameters();

    AddInputPort("input", m_Rotate->input, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_Rotate->output, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

void Rotate_Block::SetParameters()
{
    if(!m_Rotate) return;
    m_Rotate->RotationAngle = m_RotationAngle;
}

void Rotate_Block::SetDefaultParameters()
{
    m_RotationAngle = 0;
}
