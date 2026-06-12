#include "AverageCx_Block.h"

AverageCx_Block::AverageCx_Block(const std::string &name)
    :Block(name)
{

}

bool AverageCx_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool AverageCx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool AverageCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Average = std::make_unique<AverageCx>();
    SetDefaultParameters();
    try { m_NumInputsToAverage = std::stoi(getParameter("NumInputsToAverage").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'NumInputsToAverage', using default value."); }
    try { m_BlockSize = std::stoi(getParameter("BlockSize").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'BlockSize', using default value."); }
    SetParameters();
    if(!m_Average->Setup()) return false;
    AddInputPort("input", m_Average->input, static_cast<size_t>(m_NumInputsToAverage * m_BlockSize), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_Average->output, static_cast<size_t>(m_BlockSize), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    return true;
}

void AverageCx_Block::SetParameters()
{
    if(!m_Average) return;
    m_Average->NumInputsToAverage = m_NumInputsToAverage;
    m_Average->BlockSize = m_BlockSize;
}

void AverageCx_Block::SetDefaultParameters()
{
    m_NumInputsToAverage = 8;
    m_BlockSize = 1;
}

bool AverageCx_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(inputPort);
    std::vector<std::complex<double>> outputData(m_BlockSize);
    outputData.reserve(m_BlockSize);
    for (int n = 0; n < m_BlockSize; n++)
    {
        outputData[n] = 0.0;
        for (int i = 0; i < m_NumInputsToAverage; i++)
        {
            outputData[n] += inputData[i*m_BlockSize + n];
        }
        outputData[n] /= m_NumInputsToAverage;
    }
    WriteOutputData(outputPort, outputData);
    return true;
}

bool AverageCx_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(inputPort);

    if(inputData.empty()) return true;
    for(size_t i = 0; i < inputData.size(); i++) {
        m_inputBuffer.push_back(inputData.size());
    }
    if(m_inputBuffer.size() >= static_cast<size_t>(m_NumInputsToAverage * m_BlockSize)) {
        std::vector<std::complex<double>> outputData(m_BlockSize);
        for (int n = 0; n < m_BlockSize; n++)
        {
            outputData[n] = 0.0;
            for (int i = 0; i < m_NumInputsToAverage; i++)
            {
                outputData[n] += m_inputBuffer[i*m_BlockSize + n];
            }
            outputData[n] /= m_NumInputsToAverage;
        }
        for (const auto& val : outputData)
            m_outputQueue.push(val);
        m_inputBuffer.clear();
        if (!m_outputQueue.empty())
        {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;
        }
    }
    return true;
}
