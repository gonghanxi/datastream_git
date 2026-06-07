#include "ConvolutionalCoder_Block.h"
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
ConvolutionalCoder_Block::ConvolutionalCoder_Block(const std::string &name)
    :Block(name)
{

}

bool ConvolutionalCoder_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    m_Counter_ = 0;
    m_currentState_ = 0;

    const int n = m_convoCodeRateN;
    for (int i = 0; i < 8; ++i) m_polyMask[i] = 0;

    for (int i = 0; i < n; ++i)
    {
            const int p = Polynomial[i];
            const int pr = bitReverseBlock(p, m_constraintLenK);
            m_polyMask[i] = (uint32_t)pr & m_regMaskK;
    }
    return true;
}

bool ConvolutionalCoder_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool ConvolutionalCoder_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_con = std::make_unique<ConvolutionalCoder>();
    SetDefaultParameters();
    try {
    std::string PrimString = getParameter("Polynomial").Value;
    parseArrayString(PrimString, primdata);

    CodingRate = ConvertStringToCodingRateEnum(getParameter("CodingRate").Value);
    ConstraintLength = std::stoi(getParameter("ConstraintLength").Value);
    ZeroTail = ConvertStringToZeroTailEnum(getParameter("ZeroTail").Value);
    BitSequenceLength = std::stoi(getParameter("BitSequenceLength").Value);

    } catch(...) {}

    SetParameters();

    {
        const int chk = boundaryCheckBlock();
        if (chk != 0) {
            if (chk == -1) LOG_ERROR("Polynomial is empty.");
            if (chk == -2) LOG_ERROR("Polynomial size < n (CodingRate=1/n).");
            if (chk == -3) LOG_ERROR("Polynomial has no MSB tap for given ConstraintLength.");
            if (chk == -4) LOG_ERROR("Polynomial contains bits beyond ConstraintLength.");
            return false;
        }

        m_constraintLenK = ConstraintLength;
        m_convoCodeRateN = rateToNBlock(CodingRate);
        m_regMaskK = ((uint32_t)1u << (uint32_t)m_constraintLenK) - 1u;

        if (ZeroTail == ConvolutionalCoder::YES)
        {
            const int tailLen = (m_constraintLenK - 1);
            m_inputFrmLen = BitSequenceLength + tailLen;
        }
        else
        {
            m_inputFrmLen = 1;
        }
    }
    size_t inputRate  = static_cast<size_t>(m_inputFrmLen);
    size_t outputRate = (ZeroTail == ConvolutionalCoder::YES)
        ? static_cast<size_t>(m_convoCodeRateN * m_inputFrmLen)
        : static_cast<size_t>(m_convoCodeRateN);

    AddInputPort("In",  m_con->m_cbInput,  inputRate,  DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("Out", m_con->m_cbOutput, outputRate, DataType::CIRCULAR_BUFFER_BOOL);

    return true;
}

void ConvolutionalCoder_Block::SetParameters()
{
    Polynomial = primdata.data();
    PolynomialSize = static_cast<int>(primdata.size());

    if(!m_con) return;
    m_con->Polynomial = Polynomial;
    m_con->PolynomialSize = PolynomialSize;
    m_con->CodingRate = CodingRate;
    m_con->ZeroTail = ZeroTail;
    m_con->ConstraintLength = ConstraintLength;
    m_con->BitSequenceLength = BitSequenceLength;
}

void ConvolutionalCoder_Block::SetDefaultParameters()
{
    primdata.clear();//[91,121];
    primdata.push_back(91);
    primdata.push_back(121);

    Polynomial = primdata.data();
    PolynomialSize = 2;

    CodingRate = ConvolutionalCoder::rate_1_2;
    ZeroTail = ConvolutionalCoder::NO;

    ConstraintLength = 7;
    BitSequenceLength = 88;
}

bool ConvolutionalCoder_Block::parseArrayString(const std::string &arrayStr, std::vector<int> &outArray)
{
        outArray.clear();

    std::string str = arrayStr;
    // 去除首尾空格
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return false;
    size_t end = str.find_last_not_of(" \t\n\r");
    str = str.substr(start, end - start + 1);

    // 检查是否是数组格式
    if (str.empty() || str.front() != '[' || str.back() != ']') {
        return false;
    }

    // 去除外层括号
    std::string content = str.substr(1, str.length() - 2);

    // 去除首尾空格
    start = content.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        // 空数组
        return true;
    }
    end = content.find_last_not_of(" \t\n\r");
    content = content.substr(start, end - start + 1);

    // 按逗号分割
    std::stringstream ss(content);
    std::string item;

    while (std::getline(ss, item, ',')) {
        // 去除空格
        start = item.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue;
        end = item.find_last_not_of(" \t\n\r");
        item = item.substr(start, end - start + 1);

        if (!item.empty()) {
            try {
                int value = std::stoi(item);
                outArray.push_back(value);
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to parse array element: ", item, " - ", e.what());
                return false;
            }
        }
    }

    return true;
}

ConvolutionalCoder::CodingRateEnum ConvolutionalCoder_Block::ConvertStringToCodingRateEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "rate_1_2" || lower == "0") return ConvolutionalCoder::rate_1_2;
    if(lower == "rate_1_3" || lower == "1") return ConvolutionalCoder::rate_1_3;
    if(lower == "rate_1_4" || lower == "2") return ConvolutionalCoder::rate_1_4;
    if(lower == "rate_1_5" || lower == "3") return ConvolutionalCoder::rate_1_5;
    if(lower == "rate_1_6" || lower == "4") return ConvolutionalCoder::rate_1_6;
    if(lower == "rate_1_7" || lower == "5") return ConvolutionalCoder::rate_1_7;
    if(lower == "rate_1_8" || lower == "6") return ConvolutionalCoder::rate_1_8;
    return ConvolutionalCoder::rate_1_2;
}

ConvolutionalCoder::ZeroTailEnum ConvolutionalCoder_Block::ConvertStringToZeroTailEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "no" || lower == "0") return ConvolutionalCoder::NO;
    if(lower == "yes" || lower == "1") return ConvolutionalCoder::YES;
    return ConvolutionalCoder::NO;
}

bool ConvolutionalCoder_Block::DataStreamRun()
{
    size_t outputRate = (ZeroTail == ConvolutionalCoder::YES)
        ? static_cast<size_t>(m_convoCodeRateN * m_inputFrmLen)
        : static_cast<size_t>(m_convoCodeRateN);
    auto cbInput = ReadInputData<bool>(GetInputPortName(0));
    std::vector<bool> cbOutput(outputRate);
    const int K = m_constraintLenK;
    const int n = m_convoCodeRateN;

    const int tailLen = (K - 1);
    const uint32_t memMask = (tailLen > 0) ? ((1u << (uint32_t)tailLen) - 1u) : 0u;

    uint32_t state = (uint32_t)m_currentState_ & memMask;

    auto encode_one = [&](int u, int outBase)
    {
            u = (u != 0) ? 1 : 0;

            const uint32_t fullReg = ((state << 1) | (uint32_t)u) & m_regMaskK;

            for (int j = 0; j < n; ++j)
            {
                    const int y = parityU32Block(fullReg & m_polyMask[j]);
                    cbOutput[outBase + j] = (y != 0);
            }

            state = ((state << 1) | (uint32_t)u) & memMask;
    };

    if (ZeroTail == ConvolutionalCoder::YES)
    {
            const int Ninfo = BitSequenceLength;

            if (m_inputFrmLen != (Ninfo + tailLen))
            {
                    LOG_ERROR("ZeroTail=YES: internal frame length mismatch.");
                    return false;
            }

            int outIdx = 0;

            for (int i = 0; i < Ninfo; ++i)
            {
                    const int u = cbInput[i] ? 1 : 0;
                    encode_one(u, outIdx);
                    outIdx += n;
            }

            for (int t = 0; t < tailLen; ++t)
            {
                    const int u_tail = cbInput[Ninfo + t] ? 1 : 0;
                    if (u_tail != 0)
                    {
                            LOG_ERROR("Tail bits must be '0' and tail bits number must match the constraint length of convolutional code");
                            return false;
                    }

                    encode_one(0, outIdx);
                    outIdx += n;
            }

            if (state != 0u)
            {
                    LOG_ERROR("ZeroTail=YES: encoder state is not zero after tail bits.");
                    return false;
            }

            m_currentState_ = 0;
            m_Counter_++;
    }
    else
    {
            const int u = cbInput[0] ? 1 : 0;
            encode_one(u, 0);
            m_currentState_ = (int)(state & memMask);
    }
    WriteOutputData(GetOutputPortName(0), cbOutput);
    return true;
}

bool ConvolutionalCoder_Block::TimeDrivenRun()
{

    auto cbInput = ReadInputData<bool>(GetInputPortName(0));
    if(cbInput.empty()) return true;
    for(size_t i = 0; i < cbInput.size(); i++) {
        m_inputBuffer.push_back(cbInput[i]);
    }
    size_t inputRate = static_cast<size_t>(m_inputFrmLen);
    if(m_inputBuffer.size() >= inputRate) {
        size_t outputRate = (ZeroTail == ConvolutionalCoder::YES)
            ? static_cast<size_t>(m_convoCodeRateN * m_inputFrmLen)
            : static_cast<size_t>(m_convoCodeRateN);
        std::vector<bool> cbOutput(outputRate);
        const int K = m_constraintLenK;
        const int n = m_convoCodeRateN;

        const int tailLen = (K - 1);
        const uint32_t memMask = (tailLen > 0) ? ((1u << (uint32_t)tailLen) - 1u) : 0u;

        uint32_t state = (uint32_t)m_currentState_ & memMask;

        auto encode_one = [&](int u, int outBase)
        {
                u = (u != 0) ? 1 : 0;

                const uint32_t fullReg = ((state << 1) | (uint32_t)u) & m_regMaskK;

                for (int j = 0; j < n; ++j)
                {
                        const int y = parityU32Block(fullReg & m_polyMask[j]);
                        cbOutput[outBase + j] = (y != 0);
                }

                state = ((state << 1) | (uint32_t)u) & memMask;
        };

        if (ZeroTail == ConvolutionalCoder::YES)
        {
                const int Ninfo = BitSequenceLength;

                if (m_inputFrmLen != (Ninfo + tailLen))
                {
                        LOG_ERROR("ZeroTail=YES: internal frame length mismatch.");
                        return false;
                }

                int outIdx = 0;

                for (int i = 0; i < Ninfo; ++i)
                {
                        const int u = m_inputBuffer[i] ? 1 : 0;
                        encode_one(u, outIdx);
                        outIdx += n;
                }

                for (int t = 0; t < tailLen; ++t)
                {
                        const int u_tail = m_inputBuffer[Ninfo + t] ? 1 : 0;
                        if (u_tail != 0)
                        {
                                LOG_ERROR("Tail bits must be '0' and tail bits number must match the constraint length of convolutional code");
                                return false;
                        }

                        encode_one(0, outIdx);
                        outIdx += n;
                }

                if (state != 0u)
                {
                        LOG_ERROR("ZeroTail=YES: encoder state is not zero after tail bits.");
                        return false;
                }

                m_currentState_ = 0;
                m_Counter_++;
        }
        else
        {
                const int u = m_inputBuffer[0] ? 1 : 0;
                encode_one(u, 0);
                m_currentState_ = (int)(state & memMask);
        }
        // 将输出块中的每个样本逐个放入输出队列
        for (const auto& val : cbOutput)
        {
            m_outputQueue.push(val);
        }
        m_inputBuffer.erase(m_inputBuffer.begin(), m_inputBuffer.begin() + static_cast<int>(inputRate));
    }
    if (!m_outputQueue.empty())
    {
        bool outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;

        WriteOutputData(GetOutputPortName(0), std::vector<bool>{outputValue});
        m_lastOutput = outputValue;

        qDebug() << "[ConvolutionalCoder_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue;
    }

    return true;
}

int ConvolutionalCoder_Block::boundaryCheckBlock()
{
    if (ConstraintLength < 3)  ConstraintLength = 3;
    if (ConstraintLength > 14) ConstraintLength = 14;

    const int n = rateToNBlock(CodingRate);

    if (Polynomial == nullptr || PolynomialSize <= 0)
        return -1;
    if (PolynomialSize < n)
        return -2;

    if (ZeroTail == ConvolutionalCoder::YES && BitSequenceLength < 1)
        BitSequenceLength = 1;

    const uint32_t K = (uint32_t)ConstraintLength;
    const uint32_t maxMask = ((1u << K) - 1u);
    const uint32_t msb = 1u << (K - 1);

    for (int i = 0; i < n; ++i)
    {
        const uint32_t p = (uint32_t)Polynomial[i];
        if ((p & msb) == 0u)      return -3;
        if ((p & ~maxMask) != 0u) return -4;
    }

    return 0;
}

int ConvolutionalCoder_Block::bitReverseBlock(int mask, int constraintLen) const
{
    uint32_t x = (uint32_t)mask;
    uint32_t r = 0;
    for (int i = 0; i < constraintLen; ++i)
    {
        r = (r << 1) | (x & 1u);
        x >>= 1;
    }
    return (int)r;
}

int ConvolutionalCoder_Block::rateToNBlock(ConvolutionalCoder::CodingRateEnum r)
{
    switch (r)
    {
    case ConvolutionalCoder::rate_1_2: return 2;
    case ConvolutionalCoder::rate_1_3: return 3;
    case ConvolutionalCoder::rate_1_4: return 4;
    case ConvolutionalCoder::rate_1_5: return 5;
    case ConvolutionalCoder::rate_1_6: return 6;
    case ConvolutionalCoder::rate_1_7: return 7;
    case ConvolutionalCoder::rate_1_8: return 8;
    default:                           return 2;
    }
}

int ConvolutionalCoder_Block::parityU32Block(uint32_t v)
{
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v &= 0xF;
    return (0x6996u >> v) & 1u;
}
