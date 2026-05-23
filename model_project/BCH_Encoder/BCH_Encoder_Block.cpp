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

    if(!ModelSetup()) return false;
    size_t inputRate = m_bch->Msg.GetRate();
    size_t outputRate = m_bch->Code.GetRate();

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
}

bool BCH_Encoder_Block::ModelSetup()
{
    if(!m_bch->Setup()) return false;
    return true;
}

bool BCH_Encoder_Block::DataStreamRun()
{
    auto MsgData = ReadInputData<int>(GetInputPortName(0));
    size_t outputRate = m_bch->Code.GetRate();
    std::vector<int> CodeData(outputRate);
    if (m_bch->Ks_ <= 0 || m_bch->Ns_ <= 0)
    {
        CodeData[0] = (m_bch->Ks_ > 0 && MsgData[0] != 0) ? 1 : 0;
        return true;
    }

    std::vector<int> u(m_bch->Ks_, 0);
    for (int i = 0; i < m_bch->Ks_; ++i)
        u[i] = (MsgData[i] != 0) ? 1 : 0;

    std::vector<int> c;
    m_bch->encodeOne(u, c);

    const int Ns_run = std::min(m_bch->Ns_, (int)c.size());
    for (int i = 0; i < Ns_run; ++i)
        CodeData[i] = c[i] ? 1 : 0;

    for (int i = Ns_run; i < m_bch->Ns_; ++i)
        CodeData[i] = 0;

    WriteOutputData(GetOutputPortName(0), CodeData);
    return true;
}

bool BCH_Encoder_Block::TimeDrivenRun()
{
    auto MsgData = ReadInputData<int>(GetInputPortName(0));
    if(MsgData.empty()) return true;
    for(size_t i = 0; i < MsgData.size(); i++) {
        m_MsgBuffer.push_back(MsgData.size());
    }
    if(m_MsgBuffer.size() >= GetInputPort(GetInputPortName(0))->GetReadSize()) {
        size_t outputRate = m_bch->Code.GetRate();
        std::vector<int> CodeData(outputRate);
        if (m_bch->Ks_ <= 0 || m_bch->Ns_ <= 0)
        {
            CodeData[0] = (m_bch->Ks_ > 0 && m_MsgBuffer[0] != 0) ? 1 : 0;
            return true;
        }

        std::vector<int> u(m_bch->Ks_, 0);
        for (int i = 0; i < m_bch->Ks_; ++i)
            u[i] = (m_MsgBuffer[i] != 0) ? 1 : 0;

        std::vector<int> c;
        m_bch->encodeOne(u, c);

        const int Ns_run = std::min(m_bch->Ns_, (int)c.size());
        for (int i = 0; i < Ns_run; ++i)
            CodeData[i] = c[i] ? 1 : 0;

        for (int i = Ns_run; i < m_bch->Ns_; ++i)
            CodeData[i] = 0;

        for (const auto& val : MsgData)
            m_outputQueue.push(val);
        m_MsgBuffer.clear();
        if (!m_outputQueue.empty())
        {
            int outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<int>{outputValue});
            m_lastOutput = outputValue;
        }
    }
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
