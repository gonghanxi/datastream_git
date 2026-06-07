#include "CRC_Decoder_Block.h"
namespace {
std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
}
CRC_Decoder_Block::CRC_Decoder_Block(const std::string &name)
    :Block(name)
    , m_InputFrmLen(0)
    , m_CRCLength(0)
    , m_crcMask(0)
    , m_polyNoMsb(0)
    , m_msgFrame(nullptr)
    , m_msgLogical(nullptr)
    , m_crcRx(nullptr)
    , m_crcExp(nullptr)
    , m_lastOutput(false)
    , m_inputCount(0)
    , m_outputCount(0)
{

}

CRC_Decoder_Block::~CRC_Decoder_Block()
{
    delete[] m_msgFrame;
    delete[] m_msgLogical;
    delete[] m_crcRx;
    delete[] m_crcExp;
    m_msgFrame = nullptr;
    m_msgLogical = nullptr;
    m_crcRx = nullptr;
    m_crcExp = nullptr;
}
bool CRC_Decoder_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    delete[] m_msgFrame;
    delete[] m_msgLogical;
    delete[] m_crcRx;
    delete[] m_crcExp;

    m_msgFrame = new bool[MessageLength];
    m_msgLogical = new bool[MessageLength];
    m_crcRx = new bool[m_CRCLength];
    m_crcExp = new bool[m_CRCLength];

    std::fill(m_msgFrame, m_msgFrame + MessageLength, false);
    std::fill(m_msgLogical, m_msgLogical + MessageLength, false);
    std::fill(m_crcRx, m_crcRx + m_CRCLength, false);
    std::fill(m_crcExp, m_crcExp + m_CRCLength, false);
    return true;
}

bool CRC_Decoder_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool CRC_Decoder_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_crc = std::make_unique<CRC_Decoder>();
    SetDefaultParameters();
    try {

    ParityPosition = ConvertStringToParityPositionEnum(getParameter("ParityPosition").Value);
    ReverseData = ConvertStringToYesNoEnum(getParameter("ReverseData").Value);
    ReverseParity = ConvertStringToYesNoEnum(getParameter("ReverseParity").Value);
    ComplementParity = ConvertStringToYesNoEnum(getParameter("ComplementParity").Value);
    MessageLength = std::stoi(getParameter("MessageLength").Value);
    InitialState = std::stoi(getParameter("InitialState").Value);
    Polynomial = std::stoi(getParameter("Polynomial").Value);

    } catch(...) {}

    SetParameters();

    // ========== 内联 boundaryCheck + 速率计算（原 ModelSetup 逻辑） ==========
    const int chk = boundaryCheckBlock('S');
    if (chk != 0)
    {
        if (chk == -1) LOG_ERROR("MessageLength must be > 0.");
        if (chk == -2) LOG_ERROR("Polynomial must be > 0.");
        if (chk == -3) LOG_ERROR("Invalid Polynomial: cannot determine CRCLength.");
        return false;
    }

    m_InputFrmLen = MessageLength + m_CRCLength;

    size_t inputRate  = static_cast<size_t>(m_InputFrmLen);
    size_t outputRate = static_cast<size_t>(MessageLength);
    qDebug()<<"inputRate"<<inputRate<<"outputRate"<<outputRate;

    AddInputPort("In",  m_crc->In,  inputRate,  DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("Out", m_crc->Out, outputRate, DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("Parity", m_crc->Parity, 1, DataType::CIRCULAR_BUFFER_INT);

    return true;
}

void CRC_Decoder_Block::SetParameters()
{
    if(!m_crc) return;
    m_crc->ParityPosition = ParityPosition;
    m_crc->ReverseData = ReverseData;
    m_crc->ReverseParity = ReverseParity;
    m_crc->ComplementParity = ComplementParity;
    m_crc->MessageLength = MessageLength;
    m_crc->InitialState = InitialState;
    m_crc->Polynomial = Polynomial;
}

void CRC_Decoder_Block::SetDefaultParameters()
{
    ParityPosition = CRC_Decoder::Tail;
    ReverseData = CRC_Decoder::NO;
    ReverseParity = CRC_Decoder::NO;
    ComplementParity = CRC_Decoder::NO;

    MessageLength = 172;
    InitialState = 0;
    Polynomial = 7955;
}

CRC_Decoder::ParityPositionEnum CRC_Decoder_Block::ConvertStringToParityPositionEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "tail" || lower == "0") return CRC_Decoder::Tail;
    if(lower == "head" || lower == "1") return CRC_Decoder::Head;
    return CRC_Decoder::Tail;
}

CRC_Decoder::YesNoEnum CRC_Decoder_Block::ConvertStringToYesNoEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "no" || lower == "0") return CRC_Decoder::NO;
    if(lower == "yes" || lower == "1") return CRC_Decoder::YES;
    return CRC_Decoder::NO;
}

bool CRC_Decoder_Block::DataStreamRun()
{
    size_t outputRate = static_cast<size_t>(MessageLength);
    auto InData = ReadInputData<bool>(GetInputPortName(0));
    std::vector<bool> OutData(outputRate);
    std::vector<int> ParityData(1);

    if (ParityPosition == CRC_Decoder::Tail)
    {
        for (int i = 0; i < MessageLength; ++i)
            m_msgFrame[i] = (InData[i] != 0);

        for (int i = 0; i < m_CRCLength; ++i)
            m_crcRx[i] = (InData[MessageLength + i] != 0);
    }
    else // Head
    {
        for (int i = 0; i < m_CRCLength; ++i)
            m_crcRx[i] = (InData[i] != 0);

        for (int i = 0; i < MessageLength; ++i)
            m_msgFrame[i] = (InData[m_CRCLength + i] != 0);
    }

    for (int i = 0; i < MessageLength; ++i)
        m_msgLogical[i] = m_msgFrame[i];

    if (ReverseData == CRC_Decoder::YES)
        std::reverse(m_msgLogical, m_msgLogical + MessageLength);

    crcComputeRemainderBitsBlock(m_msgLogical, m_crcExp);

    if (ReverseParity == CRC_Decoder::YES)
        std::reverse(m_crcExp, m_crcExp + m_CRCLength);

    if (ComplementParity == CRC_Decoder::YES)
    {
        for (int i = 0; i < m_CRCLength; ++i)
            m_crcExp[i] = !m_crcExp[i];
    }

    bool pass = true;
    for (int i = 0; i < m_CRCLength; ++i)
    {
        if (m_crcExp[i] != m_crcRx[i])
        {
            pass = false;
            break;
        }
    }

    for (int i = 0; i < MessageLength; ++i)
        OutData[i] = m_msgLogical[i];

    ParityData[0] = pass ? 0 : 1;

    WriteOutputData(GetOutputPortName(1), ParityData);
    WriteOutputData(GetOutputPortName(0), OutData);
    return true;
}

bool CRC_Decoder_Block::TimeDrivenRun()
{
    size_t inputRate = static_cast<size_t>(m_InputFrmLen);
    size_t outputRate = static_cast<size_t>(MessageLength);
    auto InData = ReadInputData<bool>(GetInputPortName(0));
    if(InData.empty()) return true;
    std::vector<bool> OutData(outputRate);
    std::vector<int> ParityData(1);
    for(const auto& val : InData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= inputRate) {

        if (ParityPosition == CRC_Decoder::Tail)
        {
            for (int i = 0; i < MessageLength; ++i)
                m_msgFrame[i] = (m_inputBuffer[i] != 0);

            for (int i = 0; i < m_CRCLength; ++i)
                m_crcRx[i] = (m_inputBuffer[MessageLength + i] != 0);
        }
        else // Head
        {
            for (int i = 0; i < m_CRCLength; ++i)
                m_crcRx[i] = (m_inputBuffer[i] != 0);

            for (int i = 0; i < MessageLength; ++i)
                m_msgFrame[i] = (m_inputBuffer[m_CRCLength + i] != 0);
        }

        for (int i = 0; i < MessageLength; ++i)
            m_msgLogical[i] = m_msgFrame[i];

        if (ReverseData == CRC_Decoder::YES)
            std::reverse(m_msgLogical, m_msgLogical + MessageLength);

        crcComputeRemainderBitsBlock(m_msgLogical, m_crcExp);

        if (ReverseParity == CRC_Decoder::YES)
            std::reverse(m_crcExp, m_crcExp + m_CRCLength);

        if (ComplementParity == CRC_Decoder::YES)
        {
            for (int i = 0; i < m_CRCLength; ++i)
                m_crcExp[i] = !m_crcExp[i];
        }

        bool pass = true;
        for (int i = 0; i < m_CRCLength; ++i)
        {
            if (m_crcExp[i] != m_crcRx[i])
            {
                pass = false;
                break;
            }
        }

        for (int i = 0; i < MessageLength; ++i)
            OutData[i] = m_msgLogical[i];

        ParityData[0] = pass ? 0 : 1;
        // 将输出块中的每个样本逐个放入输出队列
        for (const auto& val : OutData)
        {
            m_outputQueue.push(val);
        }
    }
    if (!m_outputQueue.empty())
    {
        bool outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;

        WriteOutputData(GetOutputPortName(0), std::vector<bool>{outputValue});
        WriteOutputData(GetOutputPortName(1), ParityData);
        m_lastOutput = outputValue;
        m_inputBuffer.clear();

        qDebug() << "[CRC_Decoder_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue;
    }
    return true;
}

void CRC_Decoder_Block::computePolynomialMasksBlock()
{
    m_CRCLength = computeCRCLengthBlock(Polynomial);

    if (m_CRCLength <= 0 || m_CRCLength >= 31)
    {
        m_crcMask = 0;
        m_polyNoMsb = 0;
        return;
    }

    m_crcMask = (1u << (uint32_t)m_CRCLength) - 1u;
    m_polyNoMsb = ((uint32_t)Polynomial) & m_crcMask;
}

int CRC_Decoder_Block::computeCRCLengthBlock(int poly) const
{
    if (poly <= 0) return -1;
    int r = 0;
    int p = poly;
    while (p >>= 1) ++r;
    return r;
}

int CRC_Decoder_Block::boundaryCheckBlock(char /*functionTag*/)
{
    if (MessageLength <= 0) return -1;
    if (Polynomial <= 0)    return -2;

    computePolynomialMasksBlock();
    if (m_CRCLength <= 0)   return -3;

    return 0;
}

void CRC_Decoder_Block::crcComputeRemainderBitsBlock(const bool* msgLogical, bool* crcBits)
{
    const int r = m_CRCLength;
    const uint32_t mask = m_crcMask;
    const uint32_t poly = m_polyNoMsb;

    uint32_t reg = ((uint32_t)InitialState) & mask;

    auto update_with_bit = [&](int inBit)
    {
        inBit = (inBit != 0) ? 1 : 0;

        const int msb = (int)((reg >> (r - 1)) & 1u);
        const int fb = msb ^ inBit;

        reg = ((reg << 1) & mask);
        if (fb) reg ^= poly;
    };

    for (int i = 0; i < MessageLength; ++i)
        update_with_bit(msgLogical[i] ? 1 : 0);

    for (int i = 0; i < r; ++i)
        crcBits[i] = (((reg >> (r - 1 - i)) & 1u) != 0);
}
