#include "Sinc_Block.h"

Sinc_Block::Sinc_Block(const std::string& name)
    :Block(name)
{

}

bool Sinc_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Sinc_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) {
        return false;
    }
    std::vector<double> outputData(inputData.size(),0.0);
    if (inputData[0] == 0)
    {
        outputData[0] = 1;
    }
    else
    {
        outputData[0] = sin(inputData[0]) / inputData[0];
    }

    WriteOutputData(outputPort, outputData);
    return true;
}

bool Sinc_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Sinc = std::make_unique<Sinc>();

    AddInputPort("input", m_Sinc->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Sinc->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}
