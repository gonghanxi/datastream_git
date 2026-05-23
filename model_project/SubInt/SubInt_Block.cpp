#include "SubInt_Block.h"

SubInt_Block::SubInt_Block(const std::string& name)
    : Block(name)
{
}

void SubInt_Block::SetDefaultParamters()
{
}

bool SubInt_Block::Setup()
{
    Block::Setup();
    return true;
}

bool SubInt_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    std::string posPort = GetInputPortName(0);
    std::string negPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto posData = ReadInputData<int>(posPort);
    if (posData.empty()) {
        return true;
    }

    auto negData = ReadInputData<int>(negPort);

    long long acc = posData[0];
    for (size_t i = 0; i < negData.size(); ++i) {
        acc -= static_cast<long long>(negData[i]);
    }

    std::vector<int> outputData;
    outputData.push_back(static_cast<int>(acc));
    WriteOutputData(outputPort, outputData);

    return true;
}

bool SubInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_subInt = std::make_unique<SubInt>();

    AddInputPort("pos", m_subInt->pos, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddInputPort("neg", m_subInt->neg, 1, Block::DataType::INT_BUS);
    AddOutputPort("output", m_subInt->output, 1, Block::DataType::CIRCULAR_BUFFER_INT);

    SetDefaultParamters();

    return true;
}
