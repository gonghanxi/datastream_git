#include "SampleMean_M_Block.h"

SampleMean_M_Block::SampleMean_M_Block(const std::string& name)
    : Block(name)
{
}

bool SampleMean_M_Block::Setup()
{
    Block::Setup();
    return true;
}

// ============================================================================
// Run
// ============================================================================

bool SampleMean_M_Block::Run()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    const SystemVueModelBuilder::DoubleMatrix& inMat = inputData[0];

    const int N = inMat.NumElements();
    double sum = 0.0;
    for (int i = 0; i < N; ++i)
    {
        sum += inMat(i);
    }

    std::vector<double> outputData;
    outputData.push_back(sum / static_cast<double>(N));
    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

bool SampleMean_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_SampleMean_M = std::make_unique<SampleMean_M>();

    AddInputPort("input", m_SampleMean_M->input, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_SampleMean_M->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}
