#include "TransposeEnv_Block.h"

TransposeEnv_Block::TransposeEnv_Block(const std::string &name)
    :Block(name)
{

}
bool TransposeEnv_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool TransposeEnv_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Transpose = std::make_unique<TransposeEnv>();
    SetDefaultParameters();
    try { SamplesInRow = std::stoi(getParameter("SamplesInRow").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'SamplesInRow', using default value."); }
    try { NumberOfRows = std::stoi(getParameter("NumberOfRows").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'NumberOfRows', using default value."); }
    SetParameters();
    if(!ModelSetup()) return false;
    AddInputPort("input", m_Transpose->input, static_cast<size_t>(SamplesInRow * NumberOfRows), DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_Transpose->output, static_cast<size_t>(SamplesInRow * NumberOfRows), DataType::ENVELOPE_SIGNAL);
    return true;
}

bool TransposeEnv_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

void TransposeEnv_Block::SetParameters()
{
    if(!m_Transpose) return;
    m_Transpose->SamplesInRow = SamplesInRow;
    m_Transpose->NumberOfRows = NumberOfRows;
}

void TransposeEnv_Block::SetDefaultParameters()
{
    SamplesInRow = 8;
    NumberOfRows = 8;
}

bool TransposeEnv_Block::ModelSetup()
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

bool TransposeEnv_Block::DataStreamRun()
{
    auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(0));
    std::vector<EnvelopeSignal> outputData(SamplesInRow * NumberOfRows);
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

bool TransposeEnv_Block::TimeDrivenRun()
{
    auto inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(0));
    if(inputData.empty()) return true;

    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= static_cast<size_t>(SamplesInRow * NumberOfRows)) {
        std::vector<EnvelopeSignal> outputData(SamplesInRow * NumberOfRows);
        for (int cols = 0; cols < SamplesInRow; cols++)
        {
            for (int rows = 0; rows < NumberOfRows; rows++)
            {
                outputData[cols*NumberOfRows + rows] = m_inputBuffer[rows*SamplesInRow + cols];
            }
        }
        for(const auto& val : outputData) m_outputQueue.push(val);

        if (!m_outputQueue.empty()) {
            EnvelopeSignal outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<EnvelopeSignal>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[TransposeEnv_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
            m_inputBuffer.clear();
        }
    }
    return true;
}
