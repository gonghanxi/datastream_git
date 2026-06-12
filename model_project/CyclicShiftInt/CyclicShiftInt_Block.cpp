#include "CyclicShiftInt_Block.h"

CyclicShiftInt_Block::CyclicShiftInt_Block(const std::string &name)
    :Block(name)
{

}
bool CyclicShiftInt_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool CyclicShiftInt_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool CyclicShiftInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_CyclicShift = std::make_unique<CyclicShiftInt>();
    SetDefaultParameters();
    try { m_BlockSize = std::stoi(getParameter("BlockSize").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'BlockSize', using default value."); }
    try { m_Offset = std::stoi(getParameter("Offset").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'Offset', using default value."); }
    SetParameters();
    if(!m_CyclicShift->Setup()) return false;
    AddInputPort("input", m_CyclicShift->input, static_cast<size_t>(m_BlockSize), DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_CyclicShift->output, static_cast<size_t>(m_BlockSize), DataType::CIRCULAR_BUFFER_INT);
    return true;
}

void CyclicShiftInt_Block::SetParameters()
{
    if(!m_CyclicShift) return;
    m_CyclicShift->BlockSize = m_BlockSize;
    m_CyclicShift->Offset = m_Offset;
}

void CyclicShiftInt_Block::SetDefaultParameters()
{
    m_BlockSize = 256;
    m_Offset = 0;
}

bool CyclicShiftInt_Block::DataStreamRun()
{
    std::vector<int> inputData = ReadInputData<int>(GetInputPortName(0));
    std::vector<int> outputData(inputData.size());
    outputData.reserve(inputData.size());
    for (int i = 0; i < m_BlockSize; i++)
    {
        int outIndex = m_Offset > 0 ? (i + m_Offset) % m_BlockSize : (i + m_Offset + m_BlockSize) % m_BlockSize;
        outputData[outIndex] = inputData[i];
    }
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool CyclicShiftInt_Block::TimeDrivenRun()
{
    std::vector<int> inputData = ReadInputData<int>(GetInputPortName(0));
    if(inputData.empty()) return true;
    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= static_cast<size_t>(m_BlockSize)) {
        std::vector<int> outputData(inputData.size());
        outputData.reserve(inputData.size());
        for (int i = 0; i < m_BlockSize; i++)
        {
            int outIndex = m_Offset > 0 ? (i + m_Offset) % m_BlockSize : (i + m_Offset + m_BlockSize) % m_BlockSize;
            outputData[outIndex] = m_inputBuffer[i];
        }
        for (const auto& val : outputData)
        {
            m_outputQueue.push(val);
        }
        if (!m_outputQueue.empty())
        {
            int outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<int>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();

            qDebug() << "[CyclicShiftInt_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
        }
    }
    return true;
}
