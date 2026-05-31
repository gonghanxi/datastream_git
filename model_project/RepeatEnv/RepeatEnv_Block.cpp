#include "RepeatEnv_Block.h"

RepeatEnv_Block::RepeatEnv_Block(const std::string &name)
    :Block(name)
{

}
bool RepeatEnv_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool RepeatEnv_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RepeatEnv_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Rep = std::make_unique<RepeatEnv>();
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
    AddInputPort("input", m_Rep->input, static_cast<int>(BlockSize), DataType::ENVELOPE_SIGNAL);
    AddOutputPort("output", m_Rep->output, static_cast<int>(BlockSize * NumTimes), DataType::ENVELOPE_SIGNAL);
    return true;
}

void RepeatEnv_Block::SetParameters()
{
    if(!m_Rep) return;
    m_Rep->BlockSize = BlockSize;
    m_Rep->NumTimes = NumTimes;
}

void RepeatEnv_Block::SetDefaultParameters()
{
    BlockSize = 1;
    NumTimes = 2;
}

bool RepeatEnv_Block::DataStreamRun()
{
    std::vector<EnvelopeSignal> inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(0));
    size_t totalOutputSize = BlockSize * NumTimes;
    std::vector<EnvelopeSignal> outputData(totalOutputSize, 0.0);

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

bool RepeatEnv_Block::TimeDrivenRun()
{
    std::vector<EnvelopeSignal> inputData = ReadInputData<EnvelopeSignal>(GetInputPortName(0));

    if(inputData.empty()) return true;
    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= BlockSize) {
        size_t totalOutputSize = BlockSize * NumTimes;
        std::vector<EnvelopeSignal> outputData(totalOutputSize, 0.0);
        for (int n = 0; n < NumTimes; n++) {
            for (size_t i = 0; i < BlockSize; i++) {
                // 确保索引不越界
                if (i < inputData.size()) {
                    outputData[n * BlockSize + i] = m_inputBuffer[i];
                }
            }
        }
        for(const auto& val : outputData) m_outputQueue.push(val);
        if (!m_outputQueue.empty()) {
            EnvelopeSignal outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<EnvelopeSignal>{outputValue});

            m_lastOutput = outputValue;

            qDebug() << "[RepeatEnv_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
            m_inputBuffer.clear();
        }
    }
    return true;
}
