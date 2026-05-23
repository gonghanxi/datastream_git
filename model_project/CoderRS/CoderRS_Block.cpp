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

    GF = std::stoi(getParameter("GF").Value);
    CodeLength = std::stoi(getParameter("CodeLength").Value);
    MessageLength = std::stoi(getParameter("MessageLength").Value);

    } catch(...) {}

    SetParameters();

    if(!ModelSetup()) return false;
    size_t inputRate = m_code->in.GetRate();
    size_t outputRate = m_code->out.GetRate();

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

bool CoderRS_Block::ModelSetup()
{
    if(!m_code->Setup()) return false;
    return true;
}

bool CoderRS_Block::DataStreamRun()
{
    auto inData = ReadInputData<int>(GetInputPortName(0));
    size_t outputRate = m_code->out.GetRate();
    std::vector<int> outData(outputRate);

    const int parity = m_code->n_ - m_code->k_;
    if (parity <= 0)
    {
        for (int i = 0; i < m_code->k_; ++i)
            outData[i] = inData[i];
        return true;
    }

    std::vector<int> p(parity, 0);

    for (int i = 0; i < m_code->k_; ++i)
    {
        int sym = inData[i] & m_code->fieldMask_;
        int feedback = m_code->gf_add(sym, p[parity - 1]);

        for (int j = parity - 1; j > 0; --j)
        {
            if (feedback != 0 && m_code->g_[j] != 0)
            {
                p[j] = m_code->gf_add(p[j - 1], m_code->gf_mul(feedback, m_code->g_[j]));
            }
            else
            {
                p[j] = p[j - 1];
            }
        }

        if (feedback != 0 && m_code->g_[0] != 0)
            p[0] = m_code->gf_mul(feedback, m_code->g_[0]);
        else
            p[0] = 0;
    }

    for (int i = 0; i < m_code->k_; ++i)
        outData[i] = inData[i];

    for (int j = 0; j < parity; ++j)
        outData[m_code->k_ + j] = p[parity - 1 - j];

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
    if(m_inputBuffer.size() >= GetInputPort(GetInputPortName(0))->GetReadSize()) {
        size_t outputRate = m_code->out.GetRate();
        std::vector<int> outData(outputRate);

        const int parity = m_code->n_ - m_code->k_;
        if (parity <= 0)
        {
            for (int i = 0; i < m_code->k_; ++i)
                outData[i] = inData[i];
            return true;
        }

        std::vector<int> p(parity, 0);

        for (int i = 0; i < m_code->k_; ++i)
        {
            int sym = inData[i] & m_code->fieldMask_;
            int feedback = m_code->gf_add(sym, p[parity - 1]);

            for (int j = parity - 1; j > 0; --j)
            {
                if (feedback != 0 && m_code->g_[j] != 0)
                {
                    p[j] = m_code->gf_add(p[j - 1], m_code->gf_mul(feedback, m_code->g_[j]));
                }
                else
                {
                    p[j] = p[j - 1];
                }
            }

            if (feedback != 0 && m_code->g_[0] != 0)
                p[0] = m_code->gf_mul(feedback, m_code->g_[0]);
            else
                p[0] = 0;
        }

        for (int i = 0; i < m_code->k_; ++i)
            outData[i] = inData[i];

        for (int j = 0; j < parity; ++j)
            outData[m_code->k_ + j] = p[parity - 1 - j];
        for (const auto& val : outData)
            m_outputQueue.push(val);
        m_inputBuffer.clear();
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
