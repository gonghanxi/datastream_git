#include "BCH_Decoder_Block.h"
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
BCH_Decoder_Block::BCH_Decoder_Block(const std::string &name)
    :Block(name)
{

}

bool BCH_Decoder_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool BCH_Decoder_Block::Setup()
{
    if(!ModelSetup()) return false;
    Block::Setup();
    return true;
}

bool BCH_Decoder_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_bch = std::make_unique<BCH_Decoder>();
    SetDefaultParameters();

    try {
    std::string primString = getParameter("PrimPoly").Value;
    parseArrayString(primString, primdata);

    std::string eraseString = getParameter("ErasePosition").Value;
    parseArrayString(eraseString, erasedata);

    M = std::stoi(getParameter("M").Value);
    K = std::stoi(getParameter("K").Value);
    T = std::stoi(getParameter("T").Value);
    CodeLength = std::stoi(getParameter("CodeLength").Value);
    Erase = ConvertStringToEraseEnum(getParameter("Erase").Value);

    } catch(...) {}

    SetParameters();

    AddInputPort("Code", m_bch->Code, 1, DataType::CIRCULAR_BUFFER_INT);
    AddInputPort("EraseFlag", m_bch->EraseFlag, 1, DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("Msg", m_bch->Msg, 1, DataType::CIRCULAR_BUFFER_INT);

    return true;
}

void BCH_Decoder_Block::SetParameters()
{
    PrimPoly = primdata.data();
    PrimPolySize = static_cast<int>(primdata.size());
    ErasePosition = erasedata.data();
    ErasePositionSize = static_cast<int>(erasedata.size());

    if(!m_bch) return;
    m_bch->M = M;
    m_bch->K = K;
    m_bch->T = T;
    m_bch->CodeLength = CodeLength;
    m_bch->Erase = Erase;
    m_bch->PrimPoly = PrimPoly;
    m_bch->PrimPolySize = PrimPolySize;
    m_bch->ErasePosition = ErasePosition;
    m_bch->ErasePositionSize = ErasePositionSize;
}

void BCH_Decoder_Block::SetDefaultParameters()
{
    primdata.clear();
    PrimPoly = nullptr;
    PrimPolySize = 0;

    erasedata.clear();
    ErasePosition = nullptr;
    ErasePositionSize = 0;

    M = 3;
    K = 4;
    T = 1;
    CodeLength = 0;
    Erase = BCH_Decoder::ERASE_NO;

}

bool BCH_Decoder_Block::parseArrayString(const std::string &arrayStr, std::vector<int> &outArray)
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

BCH_Decoder::EraseEnum BCH_Decoder_Block::ConvertStringToEraseEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "erase_no" || lower == "0") return BCH_Decoder::ERASE_NO;
    if(lower == "erase_yes" || lower == "1") return BCH_Decoder::ERASE_YES;
    return BCH_Decoder::ERASE_NO;
}

bool BCH_Decoder_Block::ModelSetup()
{
    if(!m_bch->Setup()) return false;
    BufferReader* ErasePort = GetInputPort(GetInputPortName(1));
    BufferReader* CodePort = GetInputPort(GetInputPortName(0));
    Buffer* MsgPort = GetOutputPort(GetOutputPortName(0));
    eraseFlagConnected_ = ErasePort->IsConnected();

    if (M < 1)  M = 1;
    if (M > 20) M = 20;

    N_ = (1 << M) - 1;

    int effectiveCodeLength = CodeLength;
    if (effectiveCodeLength <= 0 || effectiveCodeLength > N_)
        effectiveCodeLength = N_;
    Ns_ = std::min(N_, effectiveCodeLength);

    if (CodeLength <= 0 || CodeLength >= N_)
        Ks_ = K;
    else
        Ks_ = K + (CodeLength - N_);

    if (Ks_ < 0)   Ks_ = 0;
    if (Ks_ > Ns_) Ks_ = Ns_;

    CodePort->SetReadSize((unsigned)Ns_);
    MsgPort->SetWriteSize((unsigned)Ks_);
    if (eraseFlagConnected_)
        ErasePort->SetReadSize((unsigned)Ns_);

    buildField();
    return true;
}

void BCH_Decoder_Block::buildField()
{
    const int poly = parsePrimitivePolynomial();

    alpha_to_.assign(N_ + 1, 0);
    index_of_.assign((1 << M), 0);

    alpha_to_[0] = 1;

    for (int i = 1; i < N_; ++i)
    {
        int tmp = alpha_to_[i - 1] << 1;
        if (tmp & (1 << M))
            tmp ^= poly;
        alpha_to_[i] = tmp;
    }
    alpha_to_[N_] = 1;

    for (int i = 0; i < N_; ++i)
        index_of_[alpha_to_[i]] = i;
    index_of_[0] = -1;
}

int BCH_Decoder_Block::parsePrimitivePolynomial() const
{
    if (PrimPolySize <= 0 || PrimPoly == nullptr)
        return m_bch->defaultPrimPolyInt(M);

    int polyInt = 0;

    if (PrimPolySize == 1)
    {
        polyInt = PrimPoly[0];
    }
    else
    {
        bool allZeroOne = true;
        for (int i = 0; i < PrimPolySize; ++i)
        {
            int v = PrimPoly[i];
            if (v < 0 || v > 1)
            {
                allZeroOne = false;
                break;
            }
        }

        if (allZeroOne)
        {
            for (int i = 0; i < PrimPolySize; ++i)
            {
                if (PrimPoly[i] & 1)
                    polyInt |= (1 << i);
            }
        }
        else
        {
            for (int i = 0; i < PrimPolySize; ++i)
            {
                int e = PrimPoly[i];
                if (e >= 0 && e <= M)
                    polyInt |= (1 << e);
            }
        }
    }

    if (polyInt == 0)
        polyInt = m_bch->defaultPrimPolyInt(M);

    if ((polyInt & (1 << M)) == 0)
        polyInt |= (1 << M);
    if ((polyInt & 1) == 0)
        polyInt |= 1;

    return polyInt;
}

void BCH_Decoder_Block::decodeCore(const std::vector<int> &r_in, std::vector<int> &c_out, std::vector<int> &msg_out)
{
    m_bch->decodeCore(r_in, c_out, msg_out);
}

bool BCH_Decoder_Block::DataStreamRun()
{
    auto CodeData = ReadInputData<int>(GetInputPortName(0));
    std::vector<int> EraseData;
    std::vector<int> MsgData(Ks_);
    if(eraseFlagConnected_) {
        EraseData = ReadInputData<int>(GetInputPortName(1));
    }

    const int Ns_run = Ns_;
    if (Ns_run <= 0)
        return true;

    std::vector<int> r_raw(Ns_run, 0);
    for (int i = 0; i < Ns_run; ++i)
        r_raw[i] = (CodeData[i] != 0) ? 1 : 0;

    std::vector<int> decodedMsg;

    if (Erase == BCH_Decoder::ERASE_NO)
    {
        std::vector<int> c;
        decodeCore(r_raw, c, decodedMsg);
    }
    else
    {
        std::vector<int>  erasures;
        std::vector<char> isErased(Ns_run, 0);

        if (eraseFlagConnected_)
        {
            for (int i = 0; i < Ns_run; ++i)
            {
                if (EraseData[i] != 0)
                {
                    erasures.push_back(i);
                    isErased[i] = 1;
                }
            }
        }
        else
        {
            if (ErasePosition && ErasePositionSize > 0)
            {
                for (int k = 0; k < ErasePositionSize; ++k)
                {
                    int pos = ErasePosition[k];
                    if (pos >= 0 && pos < Ns_run)
                    {
                        erasures.push_back(pos);
                        isErased[pos] = 1;
                    }
                }
            }
        }

        if (T <= 0 || (int)erasures.size() > 2 * T)
        {
            std::vector<int> c;
            decodeCore(r_raw, c, decodedMsg);
        }
        else
        {
            std::vector<int> r0 = r_raw;
            std::vector<int> r1 = r_raw;
            for (int pos : erasures)
            {
                if (pos >= 0 && pos < Ns_run)
                {
                    r0[pos] = 0;
                    r1[pos] = 1;
                }
            }

            std::vector<int> c0, m0, c1, m1;
            decodeCore(r0, c0, m0);
            decodeCore(r1, c1, m1);

            auto distanceExcludingErased =
                [&](const std::vector<int> &c) -> int
            {
                int d = 0;
                for (int i = 0; i < Ns_run; ++i)
                {
                    if (!isErased[i] && c[i] != r_raw[i])
                        ++d;
                }
                return d;
            };

            int d0 = distanceExcludingErased(c0);
            int d1 = distanceExcludingErased(c1);

            decodedMsg = (d0 <= d1) ? m0 : m1;
        }
    }

    int Ks_run = Ks_;
    if (Ks_run > (int)decodedMsg.size())
        Ks_run = static_cast<int>(decodedMsg.size());

    for (int i = 0; i < Ks_run; ++i)
        MsgData[i] = decodedMsg[i] ? 1 : 0;
    for (int i = Ks_run; i < Ks_; ++i)
        MsgData[i] = 0;

    WriteOutputData(GetOutputPortName(0), MsgData);
    return true;
}

bool BCH_Decoder_Block::TimeDrivenRun()
{
    std::string CodePort = GetInputPortName(0);
    std::string ErasePort = GetInputPortName(1);
    std::string outputPort = GetOutputPortName(0);

    std::vector<int> CodeData = ReadInputData<int>(CodePort);

    if(CodeData.empty()) return true;
    for(size_t i = 0; i < CodeData.size(); i++) {
        m_CodeBuffer.push_back(CodeData.size());
    }
    if(eraseFlagConnected_) {
        std::vector<int> EraseData = ReadInputData<int>(ErasePort);
        for(size_t i = 0; i < EraseData.size(); i++) {
            m_EraseBuffer.push_back(EraseData.size());
        }
    }

    bool HaseraseConnected = false;
    if(m_CodeBuffer.size() >= (unsigned)Ns_) {
        if(eraseFlagConnected_) {
            if(m_EraseBuffer.size() >= (unsigned)Ks_) {
                HaseraseConnected = true;
            }
            else {
                HaseraseConnected = false;
            }
        }
        HaseraseConnected = true;
    }
    if(HaseraseConnected) {
        std::vector<int> MsgData(Ks_);
        const int Ns_run = Ns_;
        if (Ns_run <= 0)
            return true;

        std::vector<int> r_raw(Ns_run, 0);
        for (int i = 0; i < Ns_run; ++i)
            r_raw[i] = (m_CodeBuffer[i] != 0) ? 1 : 0;

        std::vector<int> decodedMsg;

        if (Erase == BCH_Decoder::ERASE_NO)
        {
            std::vector<int> c;
            decodeCore(r_raw, c, decodedMsg);
        }
        else
        {
            std::vector<int>  erasures;
            std::vector<char> isErased(Ns_run, 0);

            if (eraseFlagConnected_)
            {
                for (int i = 0; i < Ns_run; ++i)
                {
                    if (m_EraseBuffer[i] != 0)
                    {
                        erasures.push_back(i);
                        isErased[i] = 1;
                    }
                }
            }
            else
            {
                if (ErasePosition && ErasePositionSize > 0)
                {
                    for (int k = 0; k < ErasePositionSize; ++k)
                    {
                        int pos = ErasePosition[k];
                        if (pos >= 0 && pos < Ns_run)
                        {
                            erasures.push_back(pos);
                            isErased[pos] = 1;
                        }
                    }
                }
            }

            if (T <= 0 || (int)erasures.size() > 2 * T)
            {
                std::vector<int> c;
                decodeCore(r_raw, c, decodedMsg);
            }
            else
            {
                std::vector<int> r0 = r_raw;
                std::vector<int> r1 = r_raw;
                for (int pos : erasures)
                {
                    if (pos >= 0 && pos < Ns_run)
                    {
                        r0[pos] = 0;
                        r1[pos] = 1;
                    }
                }

                std::vector<int> c0, m0, c1, m1;
                decodeCore(r0, c0, m0);
                decodeCore(r1, c1, m1);

                auto distanceExcludingErased =
                    [&](const std::vector<int> &c) -> int
                {
                    int d = 0;
                    for (int i = 0; i < Ns_run; ++i)
                    {
                        if (!isErased[i] && c[i] != r_raw[i])
                            ++d;
                    }
                    return d;
                };

                int d0 = distanceExcludingErased(c0);
                int d1 = distanceExcludingErased(c1);

                decodedMsg = (d0 <= d1) ? m0 : m1;
            }
        }

        int Ks_run = Ks_;
        if (Ks_run > (int)decodedMsg.size())
            Ks_run = static_cast<int>(decodedMsg.size());

        for (int i = 0; i < Ks_run; ++i)
            MsgData[i] = decodedMsg[i] ? 1 : 0;
        for (int i = Ks_run; i < Ks_; ++i)
            MsgData[i] = 0;

        for (const auto& val : MsgData)
            m_outputQueue.push(val);
        m_CodeBuffer.clear();
        m_EraseBuffer.clear();
        if (!m_outputQueue.empty())
        {
            int outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<int>{outputValue});
            m_lastOutput = outputValue;
        }
    }
    return true;
}
