#include "EnvToCx_M_Block.h"

EnvToCx_M_Block::EnvToCx_M_Block(const std::string &name)
    :Block(name)
{

}

bool EnvToCx_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool EnvToCx_M_Block::Run()
{

    std::string inputPortName = GetInputPortName(0);
    BufferReader* inputport = GetInputPort(inputPortName);
    Buffer* fcOutputport = GetOutputPort("fc");

    auto inputData = ReadInputData<EnvelopeMatrix>(inputPortName);
    std::vector<DComplexMatrix> outputData(1);

    double fc = inputport->getCharacterizationFrequency();
    fcOutputport->setCharacterizationFrequency(fc);


    int NRow = inputData[0].NumRows();
    int NCol = inputData[0].NumColumns();
    outputData[0].Resize(NRow, NCol);

    for (int row = 0; row < NRow; row++)
    {
        for (int col = 0; col < NCol; col++)
        {
            outputData[0](col, row) = inputData[0](col, row).complex();
        }
    }

    WriteOutputData("output", outputData);
    return true;
}

bool EnvToCx_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_env = std::make_unique<EnvToCx_M>();

    AddInputPort("input", m_env->input, 1, Block::DataType::MATRIX_ENVELOPE);
    AddOutputPort("output", m_env->output, 1, Block::DataType::MATRIX_DCOMPLEX);
    AddOutputPort("fc", m_env->fc, 1, Block::DataType::ENVELOPE_SIGNAL);

    return true;
}
