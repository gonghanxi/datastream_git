#include "CyclicShiftCx_Block.h"

CyclicShiftCx_Block::CyclicShiftCx_Block(const std::string &name)
    :Block(name)
{

}
bool CyclicShiftCx_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool CyclicShiftCx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool CyclicShiftCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_CyclicShift = std::make_unique<CyclicShiftCx>();
    SetDefaultParameters();
    try { m_BlockSize = std::stoi(getParameter("BlockSize").Value); } catch(...) {}
    try { m_Offset = std::stoi(getParameter("Offset").Value); } catch(...) {}
    SetParameters();
    if(!m_CyclicShift->Setup()) return false;
    AddInputPort("input", m_CyclicShift->input, static_cast<size_t>(m_BlockSize), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_CyclicShift->output, static_cast<size_t>(m_BlockSize), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    return true;
}

void CyclicShiftCx_Block::SetParameters()
{
    if(!m_CyclicShift) return;
    m_CyclicShift->BlockSize = m_BlockSize;
    m_CyclicShift->Offset = m_Offset;
}

void CyclicShiftCx_Block::SetDefaultParameters()
{
    m_BlockSize = 256;
    m_Offset = 0;
}

bool CyclicShiftCx_Block::DataStreamRun()
{
    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    std::vector<std::complex<double>> outputData(inputData.size());
    outputData.reserve(inputData.size());
    for (int i = 0; i < m_BlockSize; i++)
    {
        int outIndex = m_Offset > 0 ? (i + m_Offset) % m_BlockSize : (i + m_Offset + m_BlockSize) % m_BlockSize;
        outputData[outIndex] = inputData[i];
    }
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool CyclicShiftCx_Block::TimeDrivenRun()
{
    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    if(inputData.empty()) return true;
    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= static_cast<size_t>(m_BlockSize)) {
        std::vector<std::complex<double>> outputData(inputData.size());
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
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();

            qDebug() << "[CyclicShiftCx_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
        }
    }
    return true;
}

