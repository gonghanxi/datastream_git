#include "Repeat_Block.h"

Repeat_Block::Repeat_Block(const std::string &name)
    :Block(name)
{

}

bool Repeat_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Repeat_Block::Run()
{
    std::vector<double> inputData = ReadInputData<double>(GetInputPortName(0));
    size_t totalOutputSize = m_BlockSize * m_NumTimes;
    std::vector<double> outputData(totalOutputSize, 0.0);

    for (int n = 0; n < m_NumTimes; n++) {
        for (size_t i = 0; i < m_BlockSize; i++) {
            // 确保索引不越界
            if (i < inputData.size()) {
                outputData[n * m_BlockSize + i] = inputData[i];
            }
        }
    }

    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool Repeat_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Repeat = std::make_unique<Repeat>();
    SetDefaultParameters();
    try { m_BlockSize = std::stod(getParameter("BlockSize").Value); } catch (...) { }
    try { m_NumTimes = std::stod(getParameter("NumTimes").Value); } catch (...) { }
    SetParameters();
    if (m_NumTimes < 1)
    {
        LOG_ERROR("NumTimes must be >= 1.");
        return false;
    }

    if (m_BlockSize < 1)
    {
        LOG_ERROR("BlockSize must be >= 1.");
        return false;
    }
    AddInputPort("input", m_Repeat->input, static_cast<int>(m_BlockSize), DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Repeat->output, static_cast<int>(m_BlockSize * m_NumTimes), DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

void Repeat_Block::SetParameters()
{
    if(!m_Repeat) return;
    m_Repeat->BlockSize = m_BlockSize;
    m_Repeat->NumTimes = m_NumTimes;
}

void Repeat_Block::SetDefaultParameters()
{
    m_BlockSize = 1;
    m_NumTimes = 2;
}
