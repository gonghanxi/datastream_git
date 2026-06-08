#include "Inverse_M_Block.h"

Inverse_M_Block::Inverse_M_Block(const std::string &name)
    :Block(name)
{

}

bool Inverse_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Inverse_M_Block::Run()
{
    std::vector<SystemVueModelBuilder::DoubleMatrix> inputData;
    std::vector<SystemVueModelBuilder::DoubleMatrix> outputData;

    inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
    if(inputData.empty()) {
        LOG_INFO("Hermitian_M_Block: inPort no data available");
        return false;
    }
    outputData.resize(inputData.size());

    const Matrix<double>& inMat = inputData[0];

    if (!inMat.IsMatrix())
    {
        LOG_ERROR("Inverse_M: input is not a matrix.");
        return false;
    }
    const size_t numRows = inputData[0].NumRows();
    const size_t numCols = inputData[0].NumColumns();

    if (numRows == 0 || numCols == 0 || numRows != numCols)
    {
        std::stringstream msg;
        LOG_ERROR("Inverse_M: input matrix must be square and non-empty. Received (",numRows,", ",numCols,").");
        return false;
    }

    outputData[0].Resize(numRows,numCols);
    Matrix<double>& outMat = outputData[0];
    const bool ok = Matrix_Inverse<double>(inMat, outMat);
    if (!ok)
    {
        LOG_ERROR("Inverse_M: the input matrix is singular and does not have an inverse.");
        return false;
    }

    WriteOutputData(GetOutputPortName(0), outputData);

    return true;

}

bool Inverse_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Inverse_M = std::make_unique<Inverse_M>();


    AddInputPort("input", m_Inverse_M->input,1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_Inverse_M->output,1, Block::DataType::MATRIX_DOUBLE);

    return true;
}
