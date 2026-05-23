#include "SubCx_Block.h"

SubCx_Block::SubCx_Block(const std::string& name)
    : Block(name)
{
}

void SubCx_Block::SetDefaultParamters()
{
}

bool SubCx_Block::Setup()
{
    Block::Setup();
    return true;
}

bool SubCx_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    std::string posPort = GetInputPortName(0);
    std::string negPort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    auto posData = ReadInputData<std::complex<double>>(posPort);
    if (posData.empty()) {
        return true;
    }

    auto negData = ReadInputData<std::complex<double>>(negPort);

    std::complex<double> acc = posData[0];
    for (size_t i = 0; i < negData.size(); ++i) {
        acc -= negData[i];
    }

    std::vector<std::complex<double>> outputData;
    outputData.push_back(acc);
    WriteOutputData(outputPort, outputData);

    return true;
}

bool SubCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_subCx = std::make_unique<SubCx>();

    AddInputPort("pos", m_subCx->pos, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddInputPort("neg", m_subCx->neg, 1, Block::DataType::DCOMPLEX_BUS);
    AddOutputPort("output", m_subCx->output, 1, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

    SetDefaultParamters();

    return true;
}
