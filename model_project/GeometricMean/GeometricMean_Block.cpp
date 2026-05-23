#include "GeometricMean_Block.h"

GeometricMean_Block::GeometricMean_Block(const std::string &name)
    :Block(name)
{

}

bool GeometricMean_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool GeometricMean_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool GeometricMean_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_GeometricMean = std::make_unique<GeometricMean>();
    SetDefaultParameters();
    try { m_N = std::stoi(getParameter("N").Value); } catch (...) {}
    try { m_Gain = std::stod(getParameter("Gain").Value); } catch (...) {}
    SetParameters();
    if(!m_GeometricMean->Setup()) return false;
    AddInputPort("input", m_GeometricMean->input, static_cast<size_t>(m_N), DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_GeometricMean->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

void GeometricMean_Block::SetParameters()
{
    if(!m_GeometricMean) return;
    m_GeometricMean->N = m_N;
    m_GeometricMean->Gain = m_Gain;
}

void GeometricMean_Block::SetDefaultParameters()
{
    m_N = 10;
    m_Gain = 1;
}

bool GeometricMean_Block::DataStreamRun()
{
    std::vector<double> inputData = ReadInputData<double>(GetInputPortName(0));
    std::vector<double> outputData(1);
    outputData.reserve(1);
    double product = 1.0;
    for (int i = 0; i < m_N; i++)
    {
        product *= inputData[i];
    }
    outputData[0] = m_Gain * std::pow(product, 1.0 / m_N);
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool GeometricMean_Block::TimeDrivenRun()
{
    std::vector<double> inputData = ReadInputData<double>(GetInputPortName(0));
    if(inputData.empty()) return true;
    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= static_cast<size_t>(m_N)) {
        double product = 1.0;
        for (int i = 0; i < m_N; i++)
        {
            product *= m_inputBuffer[i];
        }
        m_outputQueue.push(m_Gain * std::pow(product, 1.0 / m_N));
        if (!m_outputQueue.empty())
        {
            double outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<double>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();

            qDebug() << "[GeometricMean_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
        }
    }
    return true;
}
