#include "InverseCx_M_Block.h"

InverseCx_M_Block::InverseCx_M_Block(const std::string &name)
    :Block(name)
{

}

bool InverseCx_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool InverseCx_M_Block::Run()
{
    std::vector<SystemVueModelBuilder::DComplexMatrix> inputData;
    std::vector<SystemVueModelBuilder::DComplexMatrix> outputData;

    inputData = ReadInputData<SystemVueModelBuilder::DComplexMatrix>(GetInputPortName(0));
    if(inputData.empty()) {
        LOG_INFO("Hermitian_M_Block: inPort no data available");
        return false;
    }
    outputData.resize(inputData.size());

    const Matrix<std::complex<double>>& inMat = inputData[0];

    if (!inMat.IsMatrix())
    {
        LOG_ERROR("Inverse_M: input is not a matrix.");
        return false;
    }
    const size_t numRows = inMat.NumRows();
    const size_t numCols = inMat.NumColumns();

    if (numRows == 0 || numCols == 0 || numRows != numCols)
    {
        std::stringstream msg;
        LOG_ERROR("Inverse_M: input matrix must be square and non-empty. Received (",numRows,", ",numCols,").");
        return false;
    }
    outputData.resize(0);
    outputData[0].Resize(numRows,numCols);
    Matrix<std::complex<double>>& outMat = outputData[0];
    const bool ok = Matrix_Inverse<std::complex<double>>(inMat, outMat);
    if (!ok)
    {
        LOG_ERROR("Inverse_M: the input matrix is singular and does not have an inverse.");
        return false;
    }
    WriteOutputData(GetOutputPortName(0), outputData);

    return true;

}

bool InverseCx_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_InverseCx_M = std::make_unique<InverseCx_M>();


    AddInputPort("input", m_InverseCx_M->input,1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("output", m_InverseCx_M->output,1, Block::DataType::MATRIX_DCOMPLEX);

    return true;
}
