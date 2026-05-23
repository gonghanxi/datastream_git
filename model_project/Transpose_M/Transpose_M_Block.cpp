#include "Transpose_M_Block.h"

Transpose_M_Block::Transpose_M_Block(const std::string &name)
    :Block(name)
{

}

bool Transpose_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Transpose_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Transpose_M = std::make_unique<Transpose_M>();

    AddInputPort("input", m_Transpose_M->input, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_Transpose_M->output, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

bool Transpose_M_Block::Run()
{
    std::vector<SystemVueModelBuilder::DoubleMatrix> inputData;


    inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
    SystemVueModelBuilder::DoubleMatrix inputMatrix = inputData[0];
    SystemVueModelBuilder::DoubleMatrix outputMatrix;

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
    std::vector<SystemVueModelBuilder::DoubleMatrix> outputData;
    outputData.push_back(outputMatrix);
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}
