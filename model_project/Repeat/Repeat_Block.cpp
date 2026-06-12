#include "Repeat_Block.h"

Repeat_Block::Repeat_Block(const std::string &name)
    :Block(name)
{

}

bool Repeat_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool Repeat_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool Repeat_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Repeat = std::make_unique<Repeat>();
    SetDefaultParameters();
    try { m_BlockSize = std::stod(getParameter("BlockSize").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'BlockSize', using default value."); }
    try { m_NumTimes = std::stod(getParameter("NumTimes").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumTimes', using default value."); }
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

bool Repeat_Block::DataStreamRun()
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

bool Repeat_Block::TimeDrivenRun()
{
    std::vector<double> inputData = ReadInputData<double>(GetInputPortName(0));

    if(inputData.empty()) return true;
    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= m_BlockSize) {
        size_t totalOutputSize = m_BlockSize * m_NumTimes;
        std::vector<double> outputData(totalOutputSize, 0.0);
        for (int n = 0; n < m_NumTimes; n++) {
            for (size_t i = 0; i < m_BlockSize; i++) {
                // 确保索引不越界
                if (i < inputData.size()) {
                    outputData[n * m_BlockSize + i] = m_inputBuffer[i];
                }
            }
        }
        for(const auto& val : outputData) m_outputQueue.push(val);
        if (!m_outputQueue.empty()) {
            double outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<double>{outputValue});

            m_lastOutput = outputValue;

            qDebug() << "[Repeat_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
            m_inputBuffer.clear();
        }
    }
    return true;
}
