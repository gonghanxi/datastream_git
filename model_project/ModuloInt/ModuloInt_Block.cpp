#include "ModuloInt_Block.h"

ModuloInt_Block::ModuloInt_Block(const std::string &name)
    :Block(name)
{

}

bool ModuloInt_Block::Setup()
{
    Block::Setup();
    return true;
}

bool ModuloInt_Block::Run()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<int>(inputPort);
    if (inputData.empty()) {
        return false;
    }
    std::vector<int> outputData(inputData.size(), 0);

    outputData[0] = inputData[0] % m_moduloValue;

    WriteOutputData(outputPort, outputData);
    return true;
}

bool ModuloInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_ModuloInt = std::make_unique<ModuloInt>();

    SetDefaultParameters();

    try { m_moduloValue = std::stoi(getParameter("moduloValue").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'moduloValue', using default value."); }

    SetParameters();

    AddInputPort("input", m_ModuloInt->input, 1, DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_ModuloInt->output, 1, DataType::CIRCULAR_BUFFER_INT);

    return true;
}

void ModuloInt_Block::SetParameters()
{
    if(!m_ModuloInt) return;
    m_ModuloInt->moduloValue = m_moduloValue;
}


void ModuloInt_Block::SetDefaultParameters()
{
    m_moduloValue = 1;
}
