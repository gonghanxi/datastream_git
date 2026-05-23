#include "VarDelay_Block.h"

VarDelay_Block::VarDelay_Block(const std::string &name)
    :Block(name)
{

}

bool VarDelay_Block::Setup()
{
    if(!ModelSetup()) return false;
    Block::Setup();
    return true;
}

bool VarDelay_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_VarDelay = std::make_unique<VarDelay>();
    SetDefaultParameters();
    try { MaxDelay = std::stoi(getParameter("MaxDelay").Value); } catch(...) {}
    SetParameters();
    AddInputPort("input", m_VarDelay->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddInputPort("control", m_VarDelay->control, 1, DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_VarDelay->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    return true;
}

bool VarDelay_Block::Run()
{
    auto inputData = ReadInputData<double>(GetInputPortName(0));
    std::vector<double> outputData(1);
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

void VarDelay_Block::SetParameters()
{
    if(!m_VarDelay) return;
    m_VarDelay->MaxDelay = MaxDelay;
}

void VarDelay_Block::SetDefaultParameters()
{
    MaxDelay = 10;
}

bool VarDelay_Block::ModelSetup()
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
