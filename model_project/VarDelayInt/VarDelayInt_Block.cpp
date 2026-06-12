#include "VarDelayInt_Block.h"

VarDelayInt_Block::VarDelayInt_Block(const std::string &name)
    :Block(name)
{

}
bool VarDelayInt_Block::Setup()
{
    if(!ModelSetup()) return false;
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    Block::Setup();
    return true;
}

bool VarDelayInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_VarDelay = std::make_unique<VarDelayInt>();
    SetDefaultParameters();
    try { MaxDelay = std::stoi(getParameter("MaxDelay").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'MaxDelay', using default value."); }
    SetParameters();
    AddInputPort("input", m_VarDelay->input, 1, DataType::CIRCULAR_BUFFER_INT);
    AddInputPort("control", m_VarDelay->control, 1, DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_VarDelay->output, 1, DataType::CIRCULAR_BUFFER_INT);
    return true;
}

bool VarDelayInt_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

void VarDelayInt_Block::SetParameters()
{
    if(!m_VarDelay) return;
    m_VarDelay->MaxDelay = MaxDelay;
}

void VarDelayInt_Block::SetDefaultParameters()
{
    MaxDelay = 10;
}

bool VarDelayInt_Block::ModelSetup()
{
    if (MaxDelay < 0)
    {
        LOG_ERROR("VarDelay: MaxDelay must be >= 0.");
        return false;
    }

    m_iMaxDelay = static_cast<size_t>(MaxDelay);
    m_iDelay = m_iMaxDelay;

    m_buffer.ResizeMemory(m_iMaxDelay + 1, true, 0);
    m_buffer.SetHistoryDepth(m_iMaxDelay + 1);

    return true;
}

bool VarDelayInt_Block::DataStreamRun()
{
    auto inputData = ReadInputData<int>(GetInputPortName(0));
    std::vector<int> outputData(1);
    BufferReader* control = GetInputPort(GetInputPortName(1));
    if (control->IsConnected())
    {
        auto controlData = ReadInputData<int>(control->GetName());
        int ctrlVal = controlData[0];

        if (ctrlVal <= 0)
        {
            m_iDelay = 0;
        }
        else
        {
            m_iDelay = static_cast<size_t>(ctrlVal);
            if (m_iDelay > m_iMaxDelay)
                m_iDelay = m_iMaxDelay;
        }
    }
    m_buffer[m_iMaxDelay] = inputData[0];

    outputData[0] = m_buffer[m_iMaxDelay - m_iDelay];

    m_buffer.Advance();
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool VarDelayInt_Block::TimeDrivenRun()
{
    auto inputData = ReadInputData<int>(GetInputPortName(0));
    if(inputData.empty()) return true;

    m_inputBuffer.push_back(inputData[0]);

    std::vector<int> outputData(1);

    bool CanprocessData = false;
    BufferReader* control = GetInputPort(GetInputPortName(1));
    if (control->IsConnected())
    {
        auto controlData = ReadInputData<int>(control->GetName());
        if(controlData.empty()) return true;
        m_controlBuffer.push_back(controlData[0]);
        if(m_inputBuffer.size() >= 1 && m_controlBuffer.size() >= 1) {
            CanprocessData = true;
        }

    }
    else {
        if(m_inputBuffer.size() >= 1) {
            CanprocessData = true;
        }
    }
    if(CanprocessData) {
        if (control->IsConnected()) {
            int ctrlVal = m_controlBuffer[0];

            if (ctrlVal <= 0)
            {
                m_iDelay = 0;
            }
            else
            {
                m_iDelay = static_cast<size_t>(ctrlVal);
                if (m_iDelay > m_iMaxDelay)
                    m_iDelay = m_iMaxDelay;
            }
        }
        m_buffer[m_iMaxDelay] = m_inputBuffer[0];

        outputData[0] = m_buffer[m_iMaxDelay - m_iDelay];
        m_buffer.Advance();

        m_outputQueue.push(outputData[0]);
        if (!m_outputQueue.empty()) {
            int outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<int>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[VarDelayInt_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
            m_inputBuffer.clear();
        }
    }

    return true;
}
