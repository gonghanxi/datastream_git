#include "TransposeCx_Block.h"

TransposeCx_Block::TransposeCx_Block(const std::string &name)
    :Block(name)
{

}
bool TransposeCx_Block::Setup()
{
    Block::Setup();
    return true;
}

bool TransposeCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Transpose = std::make_unique<TransposeCx>();
    SetDefaultParameters();
    try { SamplesInRow = std::stoi(getParameter("SamplesInRow").Value); } catch(...) {}
    try { NumberOfRows = std::stoi(getParameter("NumberOfRows").Value); } catch(...) {}
    SetParameters();
    if(!ModelSetup()) return false;
    AddInputPort("input", m_Transpose->input, static_cast<size_t>(SamplesInRow * NumberOfRows), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_Transpose->output, static_cast<size_t>(SamplesInRow * NumberOfRows), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    return true;
}

bool TransposeCx_Block::Run()
{
    auto inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    std::vector<std::complex<double>> outputData(SamplesInRow * NumberOfRows);
    for (int cols = 0; cols < SamplesInRow; cols++)
    {
        for (int rows = 0; rows < NumberOfRows; rows++)
        {
            outputData[cols*NumberOfRows + rows] = inputData[rows*SamplesInRow + cols];
        }
    }
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

void TransposeCx_Block::SetParameters()
{
    if(!m_Transpose) return;
    m_Transpose->SamplesInRow = SamplesInRow;
    m_Transpose->NumberOfRows = NumberOfRows;
}

void TransposeCx_Block::SetDefaultParameters()
{
    SamplesInRow = 8;
    NumberOfRows = 8;
}

bool TransposeCx_Block::ModelSetup()
{
    if (SamplesInRow >= 1 && NumberOfRows >= 1)
    {
        return true;
    }
    else
    {
        LOG_ERROR("SamplesInRow and NumberOfRows must not be smaller than 1.");
        return false;
    }
}
