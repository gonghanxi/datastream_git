#include "IntegratorInt_Block.h"

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

IntegratorInt_Block::IntegratorInt_Block(const std::string &name)
    :Block(name)
{

}

bool IntegratorInt_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool IntegratorInt_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool IntegratorInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_IntegratorInt = std::make_unique<IntegratorInt>();

    SetDefaultParameters();

    try { m_Top = std::stoi(getParameter("Top").Value); } catch (...) { }
    try { m_Bottom = std::stoi(getParameter("Bottom").Value); } catch (...) { }
    try { m_InitialState = std::stoi(getParameter("InitialState").Value); } catch (...) { }
    try { m_IntegrationTime = std::stod(getParameter("IntegrationTime").Value); } catch (...) { }
    try { m_IntegrationSamples = std::stoi(getParameter("IntegrationSamples").Value); } catch (...) { }
    try { m_LimitOutput = ConvertStringToLimitOutputEnum(getParameter("LimitOutput").Value); } catch (...) { }
    try { m_UseIntegrationWindow = ConvertStringToWindowEnum(getParameter("UseIntegrationWindow").Value); } catch (...) { }
    simulator_param = getSimu();

//    qDebug() << "IntegratorInt_Block::Initialize - getParameter: " << QString::fromStdString(getParameter("LimitOutput").Value);
//    qDebug() << "IntegratorInt_Block::Initialize - m_LimitOutput: " << m_LimitOutput;

    SetParameters();

    AddInputPort("reset", m_IntegratorInt->reset, 1, Block::DataType::TIMED_INT);
    AddInputPort("data", m_IntegratorInt->data, 1, Block::DataType::TIMED_INT);
    AddOutputPort("output", m_IntegratorInt->output, 1, Block::DataType::TIMED_INT);

    return true;
}

void IntegratorInt_Block::SetParameters()
{
    if(!m_IntegratorInt) return;
    m_IntegratorInt->LimitOutput = m_LimitOutput;
    m_IntegratorInt->Top = m_Top;
    m_IntegratorInt->Bottom = m_Bottom;
    m_IntegratorInt->InitialState = m_InitialState;
    m_IntegratorInt->UseIntegrationWindow = m_UseIntegrationWindow;
    m_IntegratorInt->IntegrationTime = m_IntegrationTime;
    m_IntegratorInt->IntegrationSamples = m_IntegrationSamples;
}

IntegratorInt::LimitOutputEnum IntegratorInt_Block::ConvertStringToLimitOutputEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    qDebug() << "IntegratorInt_Block::ConvertStringToLimitOutputEnum - lower" << QString::fromStdString(lower);
    if (lower == "no" || lower == "0") {
        return IntegratorInt::LIMIT_NO;
    }
    if (lower == "saturate" || lower == "1") {
        return IntegratorInt::LIMIT_SATURATE;
    }
    if (lower == "wrap" || lower == "2") {
        return IntegratorInt::LIMIT_WRAP;
    }
    return IntegratorInt::LIMIT_NO;
}

IntegratorInt::WindowEnum IntegratorInt_Block::ConvertStringToWindowEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no" || lower == "0") {
        return IntegratorInt::WIN_NO;
    }
    if (lower == "definedintime" || lower == "1") {
        return IntegratorInt::WIN_DEFINED_IN_TIME;
    }
    if (lower == "definedinsamples" || lower == "2") {
        return IntegratorInt::WIN_DEFINED_IN_SAMPLES;
    }
    return IntegratorInt::WIN_NO;
}

void IntegratorInt_Block::SetDefaultParameters()
{
    m_LimitOutput = IntegratorInt::LIMIT_NO;
    m_Top = 0;
    m_Bottom = 0;
    m_InitialState = 0;
    m_UseIntegrationWindow = IntegratorInt::WIN_NO;
    m_IntegrationTime = 100e-6;
    m_IntegrationSamples = 100;
}

bool IntegratorInt_Block::DataStreamRun()
{
    // 获取端口名称
    std::string resetPortName = GetInputPortName(0);
    std::string dataPortName = GetInputPortName(1);
    std::string outputPortName = GetOutputPortName(0);

    // 读取输入数据
    auto dataInput = ReadInputData<int>(dataPortName);
    if (dataInput.empty()) {
        return false;  // 没有数据，等待下次调用
    }

    // 获取输入值
    int x = dataInput[0];

    // 检查reset端口是否连接并读取reset值
    bool resetConnected = GetInputPort(resetPortName)->IsConnected();
    bool rActive = false;

    if (resetConnected) {
        auto resetInput = ReadInputData<int>(resetPortName);
        if (!resetInput.empty()) {
            rActive = (resetInput[0] != 0);
        }
    }

    ProcessData(x, rActive);

    // 创建输出数据
    std::vector<int> outputData;
    outputData.push_back(m_IntegratorInt->state_);

    // 写入输出
    WriteOutputData(outputPortName, outputData);

    // 更新原模型的计数
    m_IntegratorInt->Advance();

    return true;
}

bool IntegratorInt_Block::TimeDrivenRun()
{
    // 获取端口名称
    std::string resetPortName = GetInputPortName(0);
    std::string dataPortName = GetInputPortName(1);
    std::string outputPortName = GetOutputPortName(0);

    // 读取输入数据
    auto dataInput = ReadInputData<int>(dataPortName);
    if (dataInput.empty()) return true;

    // 获取输入值
    double x = dataInput[0];
    m_dataBuffer.push_back(x);

    // 检查reset端口是否连接并读取reset值
    bool resetConnected = GetInputPort(resetPortName)->IsConnected();
    bool rActive = false;

    if (resetConnected) {
        auto resetInput = ReadInputData<int>(resetPortName);
        if (!resetInput.empty()) {
            rActive = (resetInput[0] != 0);
            m_resetBuffer.push_back(rActive);
        }
    }

    bool CanProssData = false;
    if(resetConnected) {
        if(m_dataBuffer.size() >= 1 && m_resetBuffer.size() >= 1) {
            CanProssData = true;
        }
    }
    else {
        if(m_dataBuffer.size() >= 1) {
            CanProssData = true;
        }
    }
    if(CanProssData) {
        ProcessData(m_dataBuffer[0], m_resetBuffer[0]);
        // 创建输出数据
        m_outputQueue.push(m_IntegratorInt->state_);

        if (!m_outputQueue.empty())
        {
            int outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;
            WriteOutputData(outputPortName, std::vector<int>{outputValue});
            m_lastOutput = outputValue;
            m_dataBuffer.clear();
            m_resetBuffer.clear();

            qDebug() << "[IntegratorInt_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
        }
        // 更新原模型的计数
        m_IntegratorInt->Advance();
    }
    return true;
}

void IntegratorInt_Block::ProcessData(int x, bool rActive)
{
    // 获取第一个样本标志
    bool firstOut = !m_IntegratorInt->haveState_;

    // 获取当前时间
    const double t = simulator_param.startTime + static_cast<double>(m_IntegratorInt->GetCount()) / simulator_param.samplingRate;

    // 根据积分窗口模式更新状态
    if (m_UseIntegrationWindow == IntegratorInt::WIN_NO) {
        // 无积分窗口模式
        if (firstOut) {
            if (rActive) {
                m_IntegratorInt->state_ = x;
            } else {
                m_IntegratorInt->state_ = static_cast<long long>(m_InitialState) + x;
            }
        } else {
            if (rActive) {
                m_IntegratorInt->state_ = x;
            } else {
                m_IntegratorInt->state_ += x;
            }
        }
    } else {
        // 有积分窗口模式
        if (firstOut) {
            if (rActive) {
                m_IntegratorInt->state_ = x;
            } else {
                m_IntegratorInt->state_ = static_cast<long long>(m_InitialState) + x;
            }
            m_IntegratorInt->valueWindow_.clear();
            m_IntegratorInt->timeWindow_.clear();
            m_IntegratorInt->valueWindow_.push_back(x);
            m_IntegratorInt->timeWindow_.push_back(t);
        } else {
            if (rActive) {
                m_IntegratorInt->state_ = x;
                m_IntegratorInt->valueWindow_.clear();
                m_IntegratorInt->timeWindow_.clear();
                m_IntegratorInt->valueWindow_.push_back(x);
                m_IntegratorInt->timeWindow_.push_back(t);
            } else {
                m_IntegratorInt->state_ += x;
                m_IntegratorInt->valueWindow_.push_back(x);
                m_IntegratorInt->timeWindow_.push_back(t);

                // 根据窗口类型移除旧数据
                if (m_UseIntegrationWindow == IntegratorInt::WIN_DEFINED_IN_TIME) {
                    while (!m_IntegratorInt->valueWindow_.empty() &&
                           (t - m_IntegratorInt->timeWindow_.front()) > m_IntegrationTime) {
                        m_IntegratorInt->state_ -= m_IntegratorInt->valueWindow_.front();
                        m_IntegratorInt->valueWindow_.pop_front();
                        m_IntegratorInt->timeWindow_.pop_front();
                    }
                } else if (m_UseIntegrationWindow == IntegratorInt::WIN_DEFINED_IN_SAMPLES) {
                    while (static_cast<int>(m_IntegratorInt->valueWindow_.size()) > m_IntegrationSamples) {
                        m_IntegratorInt->state_ -= m_IntegratorInt->valueWindow_.front();
                        m_IntegratorInt->valueWindow_.pop_front();
                        m_IntegratorInt->timeWindow_.pop_front();
                    }
                }
            }
        }
    }

    // 更新状态标志
    m_IntegratorInt->haveState_ = true;

    // 应用输出限制
    m_IntegratorInt->applyLimits();
}
