#include "TransposeCx_M_Block.h"

TransposeCx_M_Block::TransposeCx_M_Block(const std::string &name)
    :Block(name)
{

}

bool TransposeCx_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool TransposeCx_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Transpose_M = std::make_unique<TransposeCx_M>();

    AddInputPort("input", m_Transpose_M->input, 1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("output", m_Transpose_M->output, 1, Block::DataType::MATRIX_DCOMPLEX);

    return true;
}

bool TransposeCx_M_Block::Run()
{
    std::vector<SystemVueModelBuilder::DComplexMatrix> inputData;


    inputData = ReadInputData<SystemVueModelBuilder::DComplexMatrix>(GetInputPortName(0));
    SystemVueModelBuilder::DComplexMatrix inputMatrix = inputData[0];
    SystemVueModelBuilder::DComplexMatrix outputMatrix;

    int M = inputMatrix.NumRows();
    int N = inputMatrix.NumColumns();
    outputMatrix.Resize(N, M);
    for (int m = 0; m < M; m++)
    {
        for (int n = 0; n < N; n++)
        {
            outputMatrix(n, m) = inputMatrix(m, n);
        }
    }
    std::vector<SystemVueModelBuilder::DComplexMatrix> outputData;
    outputData.push_back(outputMatrix);
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}
