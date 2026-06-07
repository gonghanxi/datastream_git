#include "BCH_Encoder_Block.h"

BCH_Encoder_Block::BCH_Encoder_Block(const std::string &name)
    :Block(name)
{

}

bool BCH_Encoder_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool BCH_Encoder_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool BCH_Encoder_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_bch = std::make_unique<BCH_Encoder>();
    SetDefaultParameters();

    try {
        std::string genString = getParameter("GenPoly").Value;
        parseArrayString(genString, gendata);

        M = std::stoi(getParameter("M").Value);
        K = std::stoi(getParameter("K").Value);
        MsgLength = std::stoi(getParameter("MsgLength").Value);

    } catch(...) {}

    SetParameters();

    buildGeneratorBlock();

    int effMsgLen = MsgLength;
    if (effMsgLen <= 0 || effMsgLen > K)
        effMsgLen = K;
    Ks_ = std::min(K, effMsgLen);
    int N = (1 << M) - 1;
    int delta = effMsgLen - K;
    Ns_ = N + std::min(0, delta);
    if (Ks_ < 0)   Ks_ = 0;
    if (Ns_ < Ks_) Ns_ = Ks_;

    m_bch->Msg.SetRate(static_cast<unsigned>(Ks_));
    m_bch->Code.SetRate(static_cast<unsigned>(Ns_));

    size_t inputRate  = static_cast<size_t>(Ks_);
    size_t outputRate = static_cast<size_t>(Ns_);

    AddInputPort("Msg", m_bch->Msg, inputRate, DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("Code", m_bch->Code, outputRate, DataType::CIRCULAR_BUFFER_INT);

    return true;
}

void BCH_Encoder_Block::SetParameters()
{
    GenPoly = gendata.data();
    GenPolySize = static_cast<int>(gendata.size());

    if(!m_bch) return;
    m_bch->M = M;
    m_bch->K = K;
    m_bch->MsgLength = MsgLength;
    m_bch->GenPoly = GenPoly;
    m_bch->GenPolySize = GenPolySize;
}

void BCH_Encoder_Block::SetDefaultParameters()
{
    gendata.clear();
    gendata.push_back(0);
    gendata.push_back(1);
    gendata.push_back(3);
    GenPoly = gendata.data();
    GenPolySize = 3;

    M = 3;
    K = 4;
    MsgLength = 0;
    Ks_ = 0;
    Ns_ = 0;
}

bool BCH_Encoder_Block::DataStreamRun()
{
    auto MsgData = ReadInputData<int>(GetInputPortName(0));
    std::vector<int> CodeData(Ns_);
    if (Ks_ <= 0 || Ns_ <= 0)
    {
        CodeData[0] = (Ks_ > 0 && MsgData[0] != 0) ? 1 : 0;
        WriteOutputData(GetOutputPortName(0), CodeData);
        return true;
    }

    std::vector<int> u(Ks_, 0);
    for (int i = 0; i < Ks_; ++i)
        u[i] = (MsgData[i] != 0) ? 1 : 0;

    std::vector<int> c;
    encodeOneBlock(u, c);

    const int Ns_run = std::min(Ns_, (int)c.size());
    for (int i = 0; i < Ns_run; ++i)
        CodeData[i] = c[i] ? 1 : 0;

    for (int i = Ns_run; i < Ns_; ++i)
        CodeData[i] = 0;

    WriteOutputData(GetOutputPortName(0), CodeData);
    return true;
}

bool BCH_Encoder_Block::TimeDrivenRun()
{
    auto MsgData = ReadInputData<int>(GetInputPortName(0));
    if(MsgData.empty()) return true;
    for(size_t i = 0; i < MsgData.size(); i++) {
        m_MsgBuffer.push_back(MsgData[i]);
    }
    if(m_MsgBuffer.size() >= static_cast<size_t>(Ks_)) {
        std::vector<int> CodeData(Ns_);
        if (Ks_ <= 0 || Ns_ <= 0)
        {
            CodeData[0] = (Ks_ > 0 && m_MsgBuffer[0] != 0) ? 1 : 0;
            WriteOutputData(GetOutputPortName(0), CodeData);
            return true;
        }

        std::vector<int> u(Ks_, 0);
        for (int i = 0; i < Ks_; ++i)
            u[i] = (m_MsgBuffer[i] != 0) ? 1 : 0;

        std::vector<int> c;
        encodeOneBlock(u, c);

        const int Ns_run = std::min(Ns_, (int)c.size());
        for (int i = 0; i < Ns_run; ++i)
            CodeData[i] = c[i] ? 1 : 0;

        for (int i = Ns_run; i < Ns_; ++i)
            CodeData[i] = 0;

        for (const auto& val : CodeData)
            m_outputQueue.push(val);
        m_MsgBuffer.clear();
    }
    if (!m_outputQueue.empty())
    {
        int outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;

        WriteOutputData(GetOutputPortName(0), std::vector<int>{outputValue});
        m_lastOutput = outputValue;
    }
    return true;
}


bool BCH_Encoder_Block::parseArrayString(const std::string &arrayStr, std::vector<int> &outArray)
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

void BCH_Encoder_Block::buildGeneratorBlock()
{
    if (M < 1)  M = 1;
    if (M > 20) M = 20;

    parityLen_ = (1 << M) - 1 - K;
    if (parityLen_ < 0)
        parityLen_ = 0;

    g_.assign(parityLen_ + 1, 0);

    if (GenPolySize <= 0 || GenPoly == nullptr)
    {
        if (parityLen_ >= 3)
        {
            g_[0] = 1;
            g_[1] = 1;
            g_[3] = 1;
        }
        else
        {
            g_[0] = 1;
            g_[parityLen_] = 1;
        }
        return;
    }

    bool allZeroOne = true;
    for (int i = 0; i < GenPolySize; ++i)
    {
        if (GenPoly[i] != 0 && GenPoly[i] != 1)
        {
            allZeroOne = false;
            break;
        }
    }

    if (allZeroOne && GenPolySize == parityLen_ + 1)
    {
        for (int i = 0; i <= parityLen_; ++i)
            g_[i] = GenPoly[i] & 1;

        if (g_[0] == 0)          g_[0] = 1;
        if (g_[parityLen_] == 0) g_[parityLen_] = 1;
        return;
    }

    if (allZeroOne && GenPolySize % (M + 1) == 0)
    {
        const int rows = GenPolySize / (M + 1);

        std::vector<int> poly(1, 1);

        for (int r = 0; r < rows; ++r)
        {
            std::vector<int> f(M + 1, 0);
            for (int c = 0; c <= M; ++c)
            {
                f[c] = GenPoly[r * (M + 1) + c] & 1;
            }

            std::vector<int> prod(std::min((int)poly.size() + M, parityLen_ + 1), 0);

            for (int i = 0; i < (int)poly.size(); ++i)
            {
                if (!poly[i]) continue;
                for (int j = 0; j <= M; ++j)
                {
                    if (!f[j]) continue;
                    int deg = i + j;
                    if (deg <= parityLen_)
                        prod[deg] ^= 1;
                }
            }

            poly.swap(prod);
        }

        for (int i = 0; i <= parityLen_ && i < (int)poly.size(); ++i)
            g_[i] = poly[i] & 1;

        if (g_[0] == 0)          g_[0] = 1;
        if (g_[parityLen_] == 0) g_[parityLen_] = 1;
        return;
    }

    for (int i = 0; i < GenPolySize; ++i)
    {
        int e = GenPoly[i];
        if (e >= 0 && e <= parityLen_)
            g_[e] ^= 1;
    }

    if (g_[0] == 0)          g_[0] = 1;
    if (g_[parityLen_] == 0) g_[parityLen_] = 1;
}

void BCH_Encoder_Block::encodeOneBlock(const std::vector<int>& u, std::vector<int>& c_out)
{
    const int Ks_run = (int)u.size();
    const int r = parityLen_;
    const int Ns_run = Ks_run + r;

    if (Ks_run <= 0 || r <= 0)
    {
        c_out = u;
        return;
    }

    std::vector<int> a(Ns_run, 0);

    for (int i = 0; i < Ks_run; ++i)
    {
        const int deg = r + (Ks_run - 1 - i);
        a[deg] = u[i] & 1;
    }

    std::vector<int> tmp = a;

    for (int d = Ns_run - 1; d >= r; --d)
    {
        if (tmp[d] == 0)
            continue;

        const int shift = d - r;
        for (int j = 0; j <= r; ++j)
        {
            if (g_[j])
                tmp[shift + j] ^= 1;
        }
    }

    std::vector<int> b(r, 0);
    for (int i = 0; i < r; ++i)
        b[i] = tmp[r - 1 - i] & 1;

    c_out.resize(Ns_run);
    for (int i = 0; i < Ks_run; ++i)
        c_out[i] = u[i] & 1;
    for (int i = 0; i < r; ++i)
        c_out[Ks_run + i] = b[i] & 1;
}
