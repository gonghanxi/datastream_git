#include "Reciprocal_Block.h"

Reciprocal_Block::Reciprocal_Block(const std::string &name)
    :Block(name)
{

}

bool Reciprocal_Block::Setup()
{
     Block::Setup();
     return true;
}

bool Reciprocal_Block::Run()
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

    if (m_MagLimit == 0)
    {
        outputData[0] = 1 / inputData[0];
    }

    if (m_MagLimit != 0 && inputData[0] == 0)
    {
        outputData[0] = m_MagLimit;
    }

    if (m_MagLimit != 0 && inputData[0] != 0)
    {
        if (1 / inputData[0] > m_MagLimit)
        {
            outputData[0] = m_MagLimit;
        }
        else if (1 / inputData[0] < -m_MagLimit)
        {
            outputData[0] = -m_MagLimit;
        }
        else
        {
            outputData[0] = 1 / inputData[0];
        }
    }

    WriteOutputData(outputPort, outputData);
    return true;

}

bool Reciprocal_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Reciprocal = std::make_unique<Reciprocal>();

    SetDefaultParameters();

    try { m_MagLimit = std::stod(getParameter("MagLimit").Value); } catch (...) { }

    SetParameters();

    AddInputPort("input", m_Reciprocal->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Reciprocal->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

void Reciprocal_Block::SetParameters()
{
    if(!m_Reciprocal) return;
    m_Reciprocal->MagLimit = m_MagLimit;
}

void Reciprocal_Block::SetDefaultParameters()
{
    m_MagLimit = 0;
}
