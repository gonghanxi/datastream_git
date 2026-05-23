#include "Integrator_Block.h"

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

Integrator_Block::Integrator_Block(const std::string &name)
    :Block(name)
{

}

bool Integrator_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool Integrator_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool Integrator_Block::DataStreamRun()
{
    // 获取端口名称
    std::string resetPortName = GetInputPortName(0);
    std::string dataPortName = GetInputPortName(1);
    std::string outputPortName = GetOutputPortName(0);

    // 读取输入数据
    auto dataInput = ReadInputData<double>(dataPortName);
    if (dataInput.empty()) {
        return false;  // 没有数据，等待下次调用
    }

    // 获取输入值
    double x = dataInput[0];

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
    std::vector<double> outputData;
    outputData.push_back(m_Integrator->state_);

    // 写入输出
    WriteOutputData(outputPortName, outputData);

    // 更新原模型的计数
    m_Integrator->Advance();

    return true;
}

bool Integrator_Block::TimeDrivenRun()
{
    // 获取端口名称
    std::string resetPortName = GetInputPortName(0);
    std::string dataPortName = GetInputPortName(1);
    std::string outputPortName = GetOutputPortName(0);

    // 读取输入数据
    auto dataInput = ReadInputData<double>(dataPortName);
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
        m_outputQueue.push(m_Integrator->state_);

        if (!m_outputQueue.empty())
        {
            double outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;
            WriteOutputData(outputPortName, std::vector<double>{outputValue});
            m_lastOutput = outputValue;
            m_dataBuffer.clear();
            m_resetBuffer.clear();

            qDebug() << "[Integrator_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue;
        }
        // 更新原模型的计数
        m_Integrator->Advance();
    }
    return true;
}

void Integrator_Block::ProcessData(double x, bool rActive)
{
    // 获取第一个样本标志
    bool firstSample = (m_Integrator->GetCount() == 0);
    // 获取当前时间
    const double t = simulator_param.startTime + static_cast<double>(m_Integrator->GetCount()) / simulator_param.samplingRate;
    // 计算采样间隔 Ts
    double Ts = 0.0;
    if (firstSample) {
        if (simulator_param.samplingRate > 0.0) {
            Ts = 1.0 / simulator_param.samplingRate;
        }
    } else {
        Ts = 1.0 / simulator_param.samplingRate;  // 固定采样率
    }

    // 保存Ts到原模型
    m_Integrator->Ts_ = Ts;

    // 计算面积（积分增量）
    double area;
    if (m_IntegrationMethod == Integrator::RECTANGLE) {
        area = Ts * x;
    } else { // TRAPEZOIDAL
        double xPrev = firstSample ? x : m_Integrator->prevInput_;
        qDebug() << "Integrator_Block --xPrev: " << xPrev;
        area = 0.5 * Ts * (xPrev + x);
    }

    // 根据积分窗口模式更新状态
    if (m_UseIntegrationWindow == Integrator::WIN_NO) {
        if (firstSample) {
            if (rActive) {
                m_Integrator->state_ = area;
            } else {
                m_Integrator->state_ = m_InitialState;
            }
        } else {
            if (rActive) {
                m_Integrator->state_ = area;
            } else {
                m_Integrator->state_ = m_FeedbackGain * m_Integrator->state_ + area;
            }
        }
    } else {
        // 有积分窗口的模式
        if (firstSample) {
            m_Integrator->state_ = m_InitialState;
            m_Integrator->areaWindow_.clear();
            m_Integrator->timeWindow_.clear();

            if (rActive) {
                m_Integrator->state_ = area;
                m_Integrator->areaWindow_.push_back(area);
                m_Integrator->timeWindow_.push_back(t);
            }
        } else {
            if (rActive) {
                m_Integrator->state_ = area;
                m_Integrator->areaWindow_.clear();
                m_Integrator->timeWindow_.clear();
                m_Integrator->areaWindow_.push_back(area);
                m_Integrator->timeWindow_.push_back(t);
            } else {
                m_Integrator->state_ += area;
                m_Integrator->areaWindow_.push_back(area);
                m_Integrator->timeWindow_.push_back(t);

                // 根据窗口类型移除旧数据
                if (m_UseIntegrationWindow == Integrator::WIN_DEFINED_IN_TIME) {
                    while (!m_Integrator->areaWindow_.empty() &&
                           (t - m_Integrator->timeWindow_.front()) > m_IntegrationTime) {
                        m_Integrator->state_ -= m_Integrator->areaWindow_.front();
                        m_Integrator->areaWindow_.pop_front();
                        m_Integrator->timeWindow_.pop_front();
                    }
                } else if (m_UseIntegrationWindow == Integrator::WIN_DEFINED_IN_SAMPLES) {
                    while (static_cast<int>(m_Integrator->areaWindow_.size()) > m_IntegrationSamples) {
                        m_Integrator->state_ -= m_Integrator->areaWindow_.front();
                        m_Integrator->areaWindow_.pop_front();
                        m_Integrator->timeWindow_.pop_front();
                    }
                }
            }
        }
    }

    // 保存当前输入作为下一个周期的上一个输入
    m_Integrator->prevInput_ = x;

    // 应用输出限制
    m_Integrator->applyLimits();
}

bool Integrator_Block::Initialize()
{
    qDebug() << "Integrator_Block --Initialize begin";
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Integrator = std::make_unique<Integrator>();\

    SetDefaultParameters();

    try { m_Top = std::stod(getParameter("Top").Value); } catch (...) { }
    try { m_Bottom = std::stod(getParameter("Bottom").Value); } catch (...) { }
    try { m_InitialState = std::stod(getParameter("InitialState").Value); } catch (...) { }
    try { m_FeedbackGain = std::stod(getParameter("FeedbackGain").Value); } catch (...) { }
    try { m_IntegrationTime = std::stod(getParameter("IntegrationTime").Value); } catch (...) { }
    try { m_IntegrationSamples = std::stod(getParameter("IntegrationSamples").Value); } catch (...) { }
    try { m_IntegrationMethod = ConvertStringToIntegrationMethodEnum(getParameter("IntegrationMethod").Value); } catch (...) { }
    try { m_LimitOutput = ConvertStringToLimitOutputEnum(getParameter("LimitOutput").Value); } catch (...) { }
    try { m_UseIntegrationWindow = ConvertStringToWindowEnum(getParameter("UseIntegrationWindow").Value); } catch (...) { }
    simulator_param = getSimu();

    qDebug() << "m_IntegrationMethod: " << m_IntegrationMethod;
    qDebug() << "m_UseIntegrationWindow: " << m_UseIntegrationWindow;

    SetParameters();

    AddInputPort("reset", m_Integrator->reset, 1, Block::DataType::TIMED_INT);
    AddInputPort("data", m_Integrator->data, 1, Block::DataType::TIMED_DOUBLE);
    AddOutputPort("output", m_Integrator->output, 1, Block::DataType::TIMED_DOUBLE);

    return true;
}

void Integrator_Block::SetParameters()
{
    if(!m_Integrator) return;
    m_Integrator->IntegrationMethod = m_IntegrationMethod;
    m_Integrator->LimitOutput = m_LimitOutput;
    m_Integrator->Top = m_Top;
    m_Integrator->Bottom = m_Bottom;
    m_Integrator->InitialState = m_InitialState;
    m_Integrator->UseIntegrationWindow = m_UseIntegrationWindow;
    m_Integrator->FeedbackGain = m_FeedbackGain;
    m_Integrator->IntegrationTime = m_IntegrationTime;
    m_Integrator->IntegrationSamples = m_IntegrationSamples;
}

Integrator::IntegrationMethodEnum Integrator_Block::ConvertStringToIntegrationMethodEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "rectangle" || lower == "0") {
        return Integrator::RECTANGLE;
    }
    if (lower == "trapezoidal" || lower == "1") {
        return Integrator::TRAPEZOIDAL;
    }
    return Integrator::RECTANGLE;
}

Integrator::LimitOutputEnum Integrator_Block::ConvertStringToLimitOutputEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no" || lower == "0") {
        return Integrator::LIMIT_NO;
    }
    if (lower == "saturate" || lower == "1") {
        return Integrator::LIMIT_SATURATE;
    }
    if (lower == "wrap" || lower == "2") {
        return Integrator::LIMIT_WRAP;
    }
    return Integrator::LIMIT_NO;
}

Integrator::WindowEnum Integrator_Block::ConvertStringToWindowEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no" || lower == "0") {
        return Integrator::WIN_NO;
    }
    if (lower == "definedintime" || lower == "1") {
        return Integrator::WIN_DEFINED_IN_TIME;
    }
    if (lower == "definedinsamples" || lower == "2") {
        return Integrator::WIN_DEFINED_IN_SAMPLES;
    }
    return Integrator::WIN_NO;
}

void Integrator_Block::SetDefaultParameters()
{
    m_IntegrationMethod = Integrator::RECTANGLE;
    m_LimitOutput = Integrator::LIMIT_NO;
    m_Top = 0;
    m_Bottom = 0;
    m_InitialState = 0;
    m_UseIntegrationWindow = Integrator::WIN_NO;
    m_FeedbackGain = 1;
    m_IntegrationTime = 100e-6;
    m_IntegrationSamples = 100;
}


