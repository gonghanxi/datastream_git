#include "CRC_Coder_Block.h"
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
CRC_Coder_Block::CRC_Coder_Block(const std::string &name)
    :Block(name)
    , m_OutFrmLen(0)
    , m_CRCLength(0)
    , m_crcMask(0)
    , m_polyNoMsb(0)
    , m_frameP(nullptr)
    , m_CRC_P(nullptr)
    , m_lastOutput(false)
    , m_inputCount(0)
    , m_outputCount(0)
{

}

CRC_Coder_Block::~CRC_Coder_Block()
{
    delete[] m_frameP;
    delete[] m_CRC_P;
    m_frameP = nullptr;
    m_CRC_P = nullptr;
}
bool CRC_Coder_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    delete[] m_frameP;
    delete[] m_CRC_P;
    m_frameP = nullptr;
    m_CRC_P = nullptr;

    m_frameP = new bool[MessageLength];
    m_CRC_P = new bool[m_CRCLength];

    std::fill(m_frameP, m_frameP + MessageLength, false);
    std::fill(m_CRC_P, m_CRC_P + m_CRCLength, false);
    return true;
}

bool CRC_Coder_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool CRC_Coder_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_crc = std::make_unique<CRC_Coder>();
    SetDefaultParameters();
    try { ParityPosition = ConvertStringToParityPositionEnum(getParameter("ParityPosition").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'ParityPosition', using default value."); }
    try { ReverseData = ConvertStringToYesNoEnum(getParameter("ReverseData").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'ReverseData', using default value."); }
    try { ReverseParity = ConvertStringToYesNoEnum(getParameter("ReverseParity").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'ReverseParity', using default value."); }
    try { ComplementParity = ConvertStringToYesNoEnum(getParameter("ComplementParity").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'ComplementParity', using default value."); }
    try { MessageLength = std::stoi(getParameter("MessageLength").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'MessageLength', using default value."); }
    try { InitialState = std::stoi(getParameter("InitialState").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'InitialState', using default value."); }
    try { Polynomial = std::stoi(getParameter("Polynomial").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'Polynomial', using default value."); }

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

    m_OutFrmLen = MessageLength + m_CRCLength;

    size_t inputRate  = static_cast<size_t>(MessageLength);
    size_t outputRate = static_cast<size_t>(m_OutFrmLen);

    AddInputPort("In",  m_crc->In,  inputRate,  DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("Out", m_crc->Out, outputRate, DataType::CIRCULAR_BUFFER_BOOL);

    return true;
}

void CRC_Coder_Block::SetParameters()
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

void CRC_Coder_Block::SetDefaultParameters()
{
    ParityPosition = CRC_Coder::Tail;
    ReverseData = CRC_Coder::NO;
    ReverseParity = CRC_Coder::NO;
    ComplementParity = CRC_Coder::NO;

    MessageLength = 172;
    InitialState = 0;
    Polynomial = 7955;
}

CRC_Coder::ParityPositionEnum CRC_Coder_Block::ConvertStringToParityPositionEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "tail" || lower == "0") return CRC_Coder::Tail;
    if(lower == "head" || lower == "1") return CRC_Coder::Head;
    return CRC_Coder::Tail;
}

CRC_Coder::YesNoEnum CRC_Coder_Block::ConvertStringToYesNoEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "no" || lower == "0") return CRC_Coder::NO;
    if(lower == "yes" || lower == "1") return CRC_Coder::YES;
    return CRC_Coder::NO;
}

bool CRC_Coder_Block::DataStreamRun()
{
    size_t outputRate = static_cast<size_t>(m_OutFrmLen);
    auto InData = ReadInputData<bool>(GetInputPortName(0));
    std::vector<bool> OutData(outputRate);
    for (int i = 0; i < MessageLength; ++i)
        m_frameP[i] = (InData[i] != 0);

    crcEncodeOneFrameBlock(m_frameP, m_CRC_P);

    if (ReverseParity == CRC_Coder::YES)
        std::reverse(m_CRC_P, m_CRC_P + m_CRCLength);

    if (ComplementParity == CRC_Coder::YES)
    {
        for (int i = 0; i < m_CRCLength; ++i)
            m_CRC_P[i] = !m_CRC_P[i];
    }

    auto writeData = [&](int &outIdx)
    {
        if (ReverseData == CRC_Coder::YES)
        {
            for (int i = MessageLength - 1; i >= 0; --i)
                OutData[outIdx++] = m_frameP[i];
        }
        else
        {
            for (int i = 0; i < MessageLength; ++i)
                OutData[outIdx++] = m_frameP[i];
        }
    };

    int outIdx = 0;

    if (ParityPosition == CRC_Coder::Head)
    {
        for (int i = 0; i < m_CRCLength; ++i)
            OutData[outIdx++] = m_CRC_P[i];

        writeData(outIdx);
    }
    else
    {
        writeData(outIdx);

        for (int i = 0; i < m_CRCLength; ++i)
            OutData[outIdx++] = m_CRC_P[i];
    }
    WriteOutputData(GetOutputPortName(0), OutData);
    return true;
}

bool CRC_Coder_Block::TimeDrivenRun()
{
    size_t inputRate = static_cast<size_t>(MessageLength);
    size_t outputRate = static_cast<size_t>(m_OutFrmLen);
    auto InData = ReadInputData<bool>(GetInputPortName(0));
    if(InData.empty()) return true;
    for(const auto& val : InData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= inputRate) {
        std::vector<bool> OutData(outputRate);
        for (int i = 0; i < MessageLength; ++i)
            m_frameP[i] = (m_inputBuffer[i] != 0);

        crcEncodeOneFrameBlock(m_frameP, m_CRC_P);

        if (ReverseParity == CRC_Coder::YES)
            std::reverse(m_CRC_P, m_CRC_P + m_CRCLength);

        if (ComplementParity == CRC_Coder::YES)
        {
            for (int i = 0; i < m_CRCLength; ++i)
                m_CRC_P[i] = !m_CRC_P[i];
        }

        auto writeData = [&](int &outIdx)
        {
            if (ReverseData == CRC_Coder::YES)
            {
                for (int i = MessageLength - 1; i >= 0; --i)
                    OutData[outIdx++] = m_frameP[i];
            }
            else
            {
                for (int i = 0; i < MessageLength; ++i)
                    OutData[outIdx++] = m_frameP[i];
            }
        };

        int outIdx = 0;

        if (ParityPosition == CRC_Coder::Head)
        {
            for (int i = 0; i < m_CRCLength; ++i)
                OutData[outIdx++] = m_CRC_P[i];

            writeData(outIdx);
        }
        else
        {
            writeData(outIdx);

            for (int i = 0; i < m_CRCLength; ++i)
                OutData[outIdx++] = m_CRC_P[i];
        }
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
        m_lastOutput = outputValue;
        m_inputBuffer.clear();

        qDebug() << "[CRC_Coder_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue;
    }
    return true;
}

void CRC_Coder_Block::computePolynomialMasksBlock()
{
    m_CRCLength = computeCRCLengthBlock(Polynomial);

    if (m_CRCLength <= 0 || m_CRCLength >= 31)
    {
        m_crcMask = 0;
        m_polyNoMsb = 0;
        return;
    }

    m_crcMask = (1u << (uint32_t)m_CRCLength) - 1u;
    m_polyNoMsb = (uint32_t)Polynomial & m_crcMask;
}

int CRC_Coder_Block::computeCRCLengthBlock(int poly) const
{
    if (poly <= 0)
        return -1;

    int r = 0;
    int p = poly;
    while (p >>= 1) ++r;
    return r;
}

int CRC_Coder_Block::boundaryCheckBlock(char /*functionTag*/)
{
    if (MessageLength <= 0)
        return -1;

    if (Polynomial <= 0)
        return -2;

    computePolynomialMasksBlock();

    if (m_CRCLength <= 0)
        return -3;

    return 0;
}

void CRC_Coder_Block::crcEncodeOneFrameBlock(const bool* dataBits, bool* crcBits)
{
    const int r = m_CRCLength;
    const uint32_t mask = m_crcMask;
    const uint32_t poly = m_polyNoMsb;

    uint32_t reg = (uint32_t)InitialState & mask;

    auto update_with_bit = [&](int inBit)
    {
        inBit = (inBit != 0) ? 1 : 0;

        const int msb = (int)((reg >> (r - 1)) & 1u);
        const int fb = msb ^ inBit;

        reg = ((reg << 1) & mask);
        if (fb)
            reg ^= poly;
    };

    for (int i = 0; i < MessageLength; ++i)
        update_with_bit(dataBits[i] ? 1 : 0);

    for (int i = 0; i < r; ++i)
        crcBits[i] = (((reg >> (r - 1 - i)) & 1u) != 0);
}
