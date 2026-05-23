#include "Hermitian_M_Block.h"

Hermitian_M_Block::Hermitian_M_Block(const std::string &name)
    :Block(name)
{

}

bool Hermitian_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Hermitian_M_Block::Run()
{
    std::vector<SystemVueModelBuilder::DComplexMatrix> inputData;
    std::vector<SystemVueModelBuilder::DComplexMatrix> outputData;

    inputData = ReadInputData<SystemVueModelBuilder::DComplexMatrix>(GetInputPortName(0));
    if(inputData.empty()) {
        LOG_INFO("Hermitian_M_Block: inPort no data available");
        return false;
    }
    outputData.resize(inputData.size());
    int outRows = inputData[0].NumColumns();
    int outCols = inputData[0].NumRows();
    for(size_t i = 0; i < inputData.size(); i++) {
        outputData.reserve(i);
    }
    outputData[0].Resize(outRows, outCols);

    for (int m = 0; m < outRows; m++)
    {
        for (int n = 0; n < outCols; n++)
        {
            outputData[0](m, n) = std::conj(inputData[0](n, m));
        }
    }
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool Hermitian_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Hermitian_M = std::make_unique<Hermitian_M>();

    // 添加输入输出端口
    AddInputPort("input", m_Hermitian_M->input, 1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("output", m_Hermitian_M->output, 1, Block::DataType::MATRIX_DCOMPLEX);

    return true;
}
