#include "Modulo_Block.h"

Modulo_Block::Modulo_Block(const std::string &name)
    :Block(name)
{

}

bool Modulo_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Modulo_Block::Run()
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
    std::vector<double> outputData(inputData.size(), 0.0);

    outputData[0] = std::fmod(inputData[0], m_moduloValue);

    WriteOutputData(outputPort, outputData);
    return true;
}

bool Modulo_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Modulo = std::make_unique<Modulo>();

    SetDefaultParameters();

    try { m_moduloValue = std::stod(getParameter("moduloValue").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'moduloValue', using default value."); }

    SetParameters();

    AddInputPort("input", m_Modulo->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Modulo->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

void Modulo_Block::SetParameters()
{
    if(!m_Modulo) return;
    m_Modulo->moduloValue = m_moduloValue;
}


void Modulo_Block::SetDefaultParameters()
{
    m_moduloValue = 1;
}
