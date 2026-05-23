#include "CyclicShift_Block.h"

CyclicShift_Block::CyclicShift_Block(const std::string &name)
    :Block(name)
{

}

bool CyclicShift_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool CyclicShift_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool CyclicShift_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_CyclicShift = std::make_unique<CyclicShift>();
    SetDefaultParameters();
    try { m_BlockSize = std::stoi(getParameter("BlockSize").Value); } catch(...) {}
    try { m_Offset = std::stoi(getParameter("Offset").Value); } catch(...) {}
    SetParameters();
    if(!m_CyclicShift->Setup()) return false;
    AddInputPort("input", m_CyclicShift->input, static_cast<size_t>(m_BlockSize), DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_CyclicShift->output, static_cast<size_t>(m_BlockSize), DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

void CyclicShift_Block::SetParameters()
{
    if(!m_CyclicShift) return;
    m_CyclicShift->BlockSize = m_BlockSize;
    m_CyclicShift->Offset = m_Offset;
}

void CyclicShift_Block::SetDefaultParameters()
{
    m_BlockSize = 256;
    m_Offset = 0;
}

bool CyclicShift_Block::DataStreamRun()
{
    std::vector<double> inputData = ReadInputData<double>(GetInputPortName(0));
    std::vector<double> outputData(inputData.size());
    outputData.reserve(inputData.size());
    for (int i = 0; i < m_BlockSize; i++)
    {
        int outIndex = m_Offset > 0 ? (i + m_Offset) % m_BlockSize : (i + m_Offset + m_BlockSize) % m_BlockSize;
        outputData[outIndex] = inputData[i];
    }
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool CyclicShift_Block::TimeDrivenRun()
{
    std::vector<double> inputData = ReadInputData<double>(GetInputPortName(0));
    if(inputData.empty()) return true;
    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= static_cast<size_t>(m_BlockSize)) {
        std::vector<double> outputData(inputData.size());
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
            double outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<double>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();

            qDebug() << "[CyclicShift_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
        }
    }
    return true;
}
