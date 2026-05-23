#include "VarDelayInt_Block.h"

VarDelayInt_Block::VarDelayInt_Block(const std::string &name)
    :Block(name)
{

}
bool VarDelayInt_Block::Setup()
{
    if(!ModelSetup()) return false;
    Block::Setup();
    return true;
}

bool VarDelayInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_VarDelay = std::make_unique<VarDelayInt>();
    SetDefaultParameters();
    try { MaxDelay = std::stoi(getParameter("MaxDelay").Value); } catch(...) {}
    SetParameters();
    AddInputPort("input", m_VarDelay->input, 1, DataType::CIRCULAR_BUFFER_INT);
    AddInputPort("control", m_VarDelay->control, 1, DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_VarDelay->output, 1, DataType::CIRCULAR_BUFFER_INT);
    return true;
}

bool VarDelayInt_Block::Run()
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
