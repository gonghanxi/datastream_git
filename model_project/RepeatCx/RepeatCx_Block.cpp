#include "RepeatCx_Block.h"

RepeatCx_Block::RepeatCx_Block(const std::string &name)
    :Block(name)
{

}

bool RepeatCx_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RepeatCx_Block::Run()
{
    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    size_t totalOutputSize = BlockSize * NumTimes;
    std::vector<std::complex<double>> outputData(totalOutputSize, 0.0);

    for (int n = 0; n < NumTimes; n++) {
        for (size_t i = 0; i < BlockSize; i++) {
            // 确保索引不越界
            if (i < inputData.size()) {
                outputData[n * BlockSize + i] = inputData[i];
            }
        }
    }

    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool RepeatCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Rep = std::make_unique<RepeatCx>();
    SetDefaultParameters();
    try { BlockSize = std::stod(getParameter("BlockSize").Value); } catch (...) { }
    try { NumTimes = std::stod(getParameter("NumTimes").Value); } catch (...) { }
    SetParameters();
    if (NumTimes < 1)
    {
        LOG_ERROR("NumTimes must be >= 1.");
        return false;
    }

    if (BlockSize < 1)
    {
        LOG_ERROR("BlockSize must be >= 1.");
        return false;
    }
    AddInputPort("input", m_Rep->input, static_cast<int>(BlockSize), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_Rep->output, static_cast<int>(BlockSize * NumTimes), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    return true;
}

void RepeatCx_Block::SetParameters()
{
    if(!m_Rep) return;
    m_Rep->BlockSize = BlockSize;
    m_Rep->NumTimes = NumTimes;
}

void RepeatCx_Block::SetDefaultParameters()
{
    BlockSize = 1;
    NumTimes = 2;
}
