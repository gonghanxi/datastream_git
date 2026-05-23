#include "DeScrambler_Block.h"

DeScrambler_Block::DeScrambler_Block(const std::string &name)
    :Block(name)
{

}
bool DeScrambler_Block::Setup()
{
    Block::Setup();
    if (Polynomial == 0)
    {
        LOG_ERROR("Polynomial must be non-zero.");
        return false;
    }

    unsigned int poly = static_cast<unsigned int>(Polynomial);

    // low-order bit must be set
    if ((poly & 0x1U) == 0U)
    {
        LOG_WARN("Polynomial low-order bit (bit0) should be set; forcing bit0=1.");
        poly |= 0x1U;
    }

    m_de->order = m_de->highestSetBitIndex(poly);
    if (m_de->order < 0)
    {
        LOG_ERROR("Polynomial is invalid (no set bits).");
        return false;
    }

    // Do not allow using sign bit (portable behavior similar to Keysight doc)
    const int maxOrder = static_cast<int>(8 * sizeof(int)) - 2; // e.g. 30 for 32-bit int
    if (m_de->order > maxOrder)
    {
        LOG_ERROR("Polynomial order too large for 'int' implementation (sign bit not allowed).");
        return false;
    }

    if (m_de->order == 0)
    {
        m_de->mask = 0;
        m_de->tapsMask = 0U;
        m_de->state = 0U;
        return true;
    }

    m_de->mask = (1 << m_de->order) - 1;

    // Polynomial bit i (i>=1) corresponds to delay i -> state bit (i-1)
    m_de->tapsMask = (poly >> 1) & static_cast<unsigned int>(m_de->mask);

    // Initial delay-line state from ShiftReg (masked to 'order' bits)
    m_de->state = static_cast<unsigned int>(ShiftReg) & static_cast<unsigned int>(m_de->mask);
    return true;
}

bool DeScrambler_Block::Run()
{
    std::string input = GetInputPortName(0);
    std::string output = GetOutputPortName(0);

    std::vector<bool> inputData = ReadInputData<bool>(input);
    std::vector<bool> outputData(1);

    const unsigned int inBit = inputData[0] ? 1U : 0U;

    const unsigned int p = static_cast<unsigned int>(m_de->parity32(m_de->state & m_de->tapsMask));
    const unsigned int outBit = inBit ^ p;
    outputData[0] = (outBit != 0U);

    // update delay line: shift in CURRENT INPUT (not output)
    if (m_de->mask != 0)
    {
        m_de->state = ((m_de->state << 1) | inBit) & static_cast<unsigned int>(m_de->mask);
    }

    WriteOutputData(output, outputData);
    return true;
}

bool DeScrambler_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_de = std::make_unique<DeScrambler>();

    SetDefaultParameters();

    try {
        Polynomial = std::stoi(getParameter("Polynomial").Value);
        ShiftReg = std::stoi(getParameter("ShiftReg").Value);
    } catch (...) {}

    SetParameters();

    AddInputPort("input", m_de->input, 1, DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("output", m_de->output, 1, DataType::CIRCULAR_BUFFER_BOOL);

    return true;
}

void DeScrambler_Block::SetParameters()
{
    if(!m_de) return;
    m_de->Polynomial = Polynomial;
    m_de->ShiftReg = ShiftReg;
}

void DeScrambler_Block::SetDefaultParameters()
{
    Polynomial = 147457;
    ShiftReg = 1;
}
