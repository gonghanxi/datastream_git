#include "CoderRS_Block.h"

CoderRS_Block::CoderRS_Block(const std::string &name)
    :Block(name)
{

}

bool CoderRS_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool CoderRS_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool CoderRS_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_code = std::make_unique<CoderRS>();
    SetDefaultParameters();

    try {
        std::string PrimString = getParameter("PrimPoly").Value;
        parseArrayString(PrimString, primdata);
    } catch(...) { LOG_WARN("Failed to parse parameter 'PrimPoly', using default value."); }
    try { GF = std::stoi(getParameter("GF").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'GF', using default value."); }
    try { CodeLength = std::stoi(getParameter("CodeLength").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'CodeLength', using default value."); }
    try { MessageLength = std::stoi(getParameter("MessageLength").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'MessageLength', using default value."); }

    SetParameters();

    n_ = CodeLength;
    k_ = MessageLength;
    if (n_ < 3)  n_ = 3;
    if (k_ < 1)  k_ = 1;
    if (k_ > n_ - 2) k_ = n_ - 2;
    int maxN = (1 << GF) - 1;
    if (n_ > maxN) n_ = maxN;
    if (k_ > n_ - 2) k_ = n_ - 2;

    buildFieldBlock();
    buildGeneratorBlock();

    m_code->in.SetRate(static_cast<unsigned>(k_));
    m_code->out.SetRate(static_cast<unsigned>(n_));

    size_t inputRate  = static_cast<size_t>(k_);
    size_t outputRate = static_cast<size_t>(n_);

    AddInputPort("in", m_code->in, inputRate, DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("out", m_code->out, outputRate, DataType::CIRCULAR_BUFFER_INT);

    return true;
}

void CoderRS_Block::SetParameters()
{
    PrimPoly = primdata.data();
    PrimPolySize = static_cast<int>(primdata.size());

    if(!m_code) return;
    m_code->GF = GF;
    m_code->CodeLength = CodeLength;
    m_code->MessageLength = MessageLength;
    m_code->PrimPoly = PrimPoly;
    m_code->PrimPolySize = PrimPolySize;
    m_code->Root = Root;
}

void CoderRS_Block::SetDefaultParameters()
{
    primdata.clear();//[1,0,1,1,1,0,0,0,1];
    primdata.push_back(1);
    primdata.push_back(0);
    primdata.push_back(1);
    primdata.push_back(1);
    primdata.push_back(1);
    primdata.push_back(0);
    primdata.push_back(0);
    primdata.push_back(0);
    primdata.push_back(1);

    PrimPoly = primdata.data();
    PrimPolySize = 9;

    GF = 8;
    CodeLength = 255;
    MessageLength = 223;
    Root = 1;
}

bool CoderRS_Block::parseArrayString(const std::string &arrayStr, std::vector<int> &outArray)
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

bool CoderRS_Block::DataStreamRun()
{
    auto inData = ReadInputData<int>(GetInputPortName(0));
    std::vector<int> outData(n_);

    const int parity = n_ - k_;
    if (parity <= 0)
    {
        for (int i = 0; i < k_; ++i)
            outData[i] = inData[i];
        WriteOutputData(GetOutputPortName(0), outData);
        return true;
    }

    std::vector<int> p(parity, 0);

    for (int i = 0; i < k_; ++i)
    {
        int sym = inData[i] & fieldMask_;
        int feedback = gf_add(sym, p[parity - 1]);

        for (int j = parity - 1; j > 0; --j)
        {
            if (feedback != 0 && g_[j] != 0)
            {
                p[j] = gf_add(p[j - 1], gf_mul(feedback, g_[j]));
            }
            else
            {
                p[j] = p[j - 1];
            }
        }

        if (feedback != 0 && g_[0] != 0)
            p[0] = gf_mul(feedback, g_[0]);
        else
            p[0] = 0;
    }

    for (int i = 0; i < k_; ++i)
        outData[i] = inData[i];

    for (int j = 0; j < parity; ++j)
        outData[k_ + j] = p[parity - 1 - j];

    WriteOutputData(GetOutputPortName(0),outData);
    return true;
}

bool CoderRS_Block::TimeDrivenRun()
{
    auto inData = ReadInputData<int>(GetInputPortName(0));
    if(inData.empty()) return true;
    for(size_t i = 0; i < inData.size(); i++) {
        m_inputBuffer.push_back(inData[i]);
    }
    if(m_inputBuffer.size() >= static_cast<size_t>(k_)) {
        std::vector<int> outData(n_);

        const int parity = n_ - k_;
        if (parity <= 0)
        {
            for (int i = 0; i < k_; ++i)
                outData[i] = m_inputBuffer[i];
            WriteOutputData(GetOutputPortName(0), outData);
            return true;
        }

        std::vector<int> p(parity, 0);

        for (int i = 0; i < k_; ++i)
        {
            int sym = m_inputBuffer[i] & fieldMask_;
            int feedback = gf_add(sym, p[parity - 1]);

            for (int j = parity - 1; j > 0; --j)
            {
                if (feedback != 0 && g_[j] != 0)
                {
                    p[j] = gf_add(p[j - 1], gf_mul(feedback, g_[j]));
                }
                else
                {
                    p[j] = p[j - 1];
                }
            }

            if (feedback != 0 && g_[0] != 0)
                p[0] = gf_mul(feedback, g_[0]);
            else
                p[0] = 0;
        }

        for (int i = 0; i < k_; ++i)
            outData[i] = m_inputBuffer[i];

        for (int j = 0; j < parity; ++j)
            outData[k_ + j] = p[parity - 1 - j];
        for (const auto& val : outData)
            m_outputQueue.push(val);
        m_inputBuffer.clear();
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

int CoderRS_Block::gf_add(int a, int b) const
{
    return a ^ b;
}

int CoderRS_Block::gf_mul(int a, int b) const
{
    if (a == 0 || b == 0)
        return 0;
    int ia = index_of_[a];
    int ib = index_of_[b];
    if (ia < 0 || ib < 0)
        return 0;
    int ie = ia + ib;
    if (ie >= maxExp_)
        ie -= maxExp_;
    return alpha_to_[ie];
}

void CoderRS_Block::buildFieldBlock()
{
    int m = GF;
    if (m < 2)  m = 2;
    if (m > 16) m = 16;

    int fieldSize = 1 << m;
    fieldMask_ = fieldSize - 1;
    maxExp_ = fieldSize - 1;

    bool useUserPoly = false;
    int  primPolyMask = 0;
    bool highestOk = false;
    bool constantOk = false;

    if (PrimPoly != nullptr && PrimPolySize > 0)
    {
        int highestIdx = -1;
        for (int i = 0; i < PrimPolySize; ++i)
        {
            if (PrimPoly[i] != 0)
                highestIdx = i;
        }

        if (highestIdx == m)
        {
            highestOk = true;

            if ((PrimPoly[0] & 1) != 0)
                constantOk = true;

            int cm = (m < PrimPolySize) ? PrimPoly[m] : 0;
            if ((cm & 1) == 0)
                highestOk = false;

            if (highestOk && constantOk)
            {
                primPolyMask = 0;
                for (int i = 0; i < m; ++i)
                {
                    if (i < PrimPolySize && (PrimPoly[i] & 1))
                        primPolyMask |= (1 << i);
                }
                primPolyMask &= fieldMask_;
                useUserPoly = true;
            }
        }
    }

    if (!useUserPoly)
    {
        switch (m)
        {
        case 2:  primPolyMask = 0x7;    break;
        case 3:  primPolyMask = 0xB;    break;
        case 4:  primPolyMask = 0x13;   break;
        case 5:  primPolyMask = 0x25;   break;
        case 6:  primPolyMask = 0x43;   break;
        case 7:  primPolyMask = 0x89;   break;
        case 8:  primPolyMask = 0x11D;  break;
        default:
            primPolyMask = (1 << m) | (1 << 1) | 1;
            break;
        }
        primPolyMask &= fieldMask_;
    }

    alpha_to_.assign(maxExp_ + 1, 0);
    index_of_.assign(fieldSize, -1);

    int alpha = 1;
    for (int i = 0; i < maxExp_; ++i)
    {
        alpha_to_[i] = alpha;
        index_of_[alpha] = i;

        alpha <<= 1;
        if (alpha & fieldSize)
            alpha ^= primPolyMask;
        alpha &= fieldMask_;
    }

    alpha_to_[maxExp_] = 1;
    index_of_[0] = -1;
}

void CoderRS_Block::buildGeneratorBlock()
{
    int parity = n_ - k_;
    if (parity <= 0)
    {
        g_.assign(1, 1);
        return;
    }

    int root = Root;
    if (root < 0)
        root = 0;
    if (maxExp_ > 0)
        root %= maxExp_;

    g_.clear();
    g_.push_back(1);

    for (int i = 0; i < parity; ++i)
    {
        int exp_i = root + i;
        while (exp_i >= maxExp_)
            exp_i -= maxExp_;

        int alpha_i = alpha_to_[exp_i];

        int deg = static_cast<int>(g_.size()) - 1;
        std::vector<int> new_g(deg + 2, 0);

        for (int j = 0; j <= deg; ++j)
        {
            new_g[j + 1] ^= g_[j];
        }

        for (int j = 0; j <= deg; ++j)
        {
            if (g_[j] != 0)
            {
                int prod = gf_mul(g_[j], alpha_i);
                new_g[j] ^= prod;
            }
        }

        g_.swap(new_g);
    }

    if (static_cast<int>(g_.size()) > parity + 1)
        g_.resize(parity + 1);
}
