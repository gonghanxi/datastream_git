#include "IntegratorCx_Block.h"

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

IntegratorCx_Block::IntegratorCx_Block(const std::string &name)
    :Block(name)
{

}

bool IntegratorCx_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool IntegratorCx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool IntegratorCx_Block::Initialize()
{
    qDebug() << "IntegratorCx_Block --Initialize begin";
    SetBlockType(Block::BlockType::PROCESSOR);

    m_IntegratorCx = std::make_unique<IntegratorCx>();\

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

    SetParameters();

    AddInputPort("reset", m_IntegratorCx->reset, 1, Block::DataType::TIMED_INT);
    AddInputPort("data", m_IntegratorCx->data, 1, Block::DataType::TIMED_DCOMPLEX);
    AddOutputPort("output", m_IntegratorCx->output, 1, Block::DataType::TIMED_DCOMPLEX);

    return true;
}

void IntegratorCx_Block::SetParameters()
{
    if(!m_IntegratorCx) return;
    m_IntegratorCx->IntegrationMethod = m_IntegrationMethod;
    m_IntegratorCx->LimitOutput = m_LimitOutput;
    m_IntegratorCx->Top = m_Top;
    m_IntegratorCx->Bottom = m_Bottom;
    m_IntegratorCx->InitialState = m_InitialState;
    m_IntegratorCx->UseIntegrationWindow = m_UseIntegrationWindow;
    m_IntegratorCx->FeedbackGain = m_FeedbackGain;
    m_IntegratorCx->IntegrationTime = m_IntegrationTime;
    m_IntegratorCx->IntegrationSamples = m_IntegrationSamples;
}

IntegratorCx::IntegrationMethodEnum IntegratorCx_Block::ConvertStringToIntegrationMethodEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "rectangle" || lower == "0") {
        return IntegratorCx::RECTANGLE;
    }
    if (lower == "trapezoidal" || lower == "1") {
        return IntegratorCx::TRAPEZOIDAL;
    }
    return IntegratorCx::RECTANGLE;
}

IntegratorCx::LimitOutputEnum IntegratorCx_Block::ConvertStringToLimitOutputEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no" || lower == "0") {
        return IntegratorCx::LIMIT_NO;
    }
    if (lower == "aturate" || lower == "1") {
        return IntegratorCx::LIMIT_SATURATE;
    }
    if (lower == "wrap" || lower == "2") {
        return IntegratorCx::LIMIT_WRAP;
    }
    return IntegratorCx::LIMIT_NO;
}

IntegratorCx::WindowEnum IntegratorCx_Block::ConvertStringToWindowEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "no" || lower == "0") {
        return IntegratorCx::WIN_NO;
    }
    if (lower == "definedintime" || lower == "1") {
        return IntegratorCx::WIN_DEFINED_IN_TIME;
    }
    if (lower == "definedinsamples" || lower == "2") {
        return IntegratorCx::WIN_DEFINED_IN_SAMPLES;
    }
    return IntegratorCx::WIN_NO;
}

void IntegratorCx_Block::SetDefaultParameters()
{
    m_IntegrationMethod = IntegratorCx::RECTANGLE;
    m_LimitOutput = IntegratorCx::LIMIT_NO;
    m_Top = 0;
    m_Bottom = 0;
    m_InitialState = 0;
    m_UseIntegrationWindow = IntegratorCx::WIN_NO;
    m_FeedbackGain = 1;
    m_IntegrationTime = 100e-6;
    m_IntegrationSamples = 100;
}

bool IntegratorCx_Block::DataStreamRun()
{
    // 获取端口名称
    std::string resetPortName = GetInputPortName(0);
    std::string dataPortName = GetInputPortName(1);
    std::string outputPortName = GetOutputPortName(0);

    // 读取输入数据
    auto dataInput = ReadInputData<std::complex<double>>(dataPortName);
    if (dataInput.empty()) {
        return false;  // 没有数据，等待下次调用
    }

    // 获取输入值
    std::complex<double> x = dataInput[0];

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
    std::vector<std::complex<double>> outputData;
    outputData.push_back(m_IntegratorCx->state_);

    // 写入输出
    WriteOutputData(outputPortName, outputData);

    // 更新原模型的计数
    m_IntegratorCx->Advance();

    return true;
}

bool IntegratorCx_Block::TimeDrivenRun()
{
    // 获取端口名称
    std::string resetPortName = GetInputPortName(0);
    std::string dataPortName = GetInputPortName(1);
    std::string outputPortName = GetOutputPortName(0);

    // 读取输入数据
    auto dataInput = ReadInputData<std::complex<double>>(dataPortName);
    if (dataInput.empty()) return true;

    // 获取输入值
    std::complex<double> x = dataInput[0];
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
        m_outputQueue.push(m_IntegratorCx->state_);

        if (!m_outputQueue.empty())
        {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;
            WriteOutputData(outputPortName, std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;
            m_dataBuffer.clear();
            m_resetBuffer.clear();

            qDebug() << "[IntegratorCx_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
        }
        // 更新原模型的计数
        m_IntegratorCx->Advance();
    }
    return true;
}

void IntegratorCx_Block::ProcessData(std::complex<double> x, bool rActive)
{
    // 获取第一个样本标志
    bool firstSample = (m_IntegratorCx->GetCount() == 0);

    // 获取当前时间
    const double t = simulator_param.startTime + static_cast<double>(m_IntegratorCx->GetCount()) / simulator_param.samplingRate;

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
    m_IntegratorCx->Ts_ = Ts;

    // 计算面积（积分增量）
    std::complex<double> area;
    if (m_IntegrationMethod == IntegratorCx::RECTANGLE) {
        area = std::complex<double>(Ts, 0.0) * x;
    } else { // TRAPEZOIDAL
        std::complex<double> xPrev = firstSample ? x : m_IntegratorCx->prevInput_;
        area = 0.5 * Ts * (xPrev + x);
    }

    // 根据积分窗口模式更新状态
    if (m_UseIntegrationWindow == IntegratorCx::WIN_NO) {
        if (firstSample) {
            if (rActive) {
                m_IntegratorCx->state_ = area;
            } else {
                m_IntegratorCx->state_ = m_InitialState;
            }
        } else {
            if (rActive) {
                m_IntegratorCx->state_ = area;
            } else {
                m_IntegratorCx->state_ = m_FeedbackGain * m_IntegratorCx->state_ + area;
            }
        }
    } else {
        // 有积分窗口的模式
        if (firstSample) {
            m_IntegratorCx->state_ = m_InitialState;
            m_IntegratorCx->areaWindow_.clear();
            m_IntegratorCx->timeWindow_.clear();

            if (rActive) {
                m_IntegratorCx->state_ = area;
                m_IntegratorCx->areaWindow_.push_back(area);
                m_IntegratorCx->timeWindow_.push_back(t);
            }
        } else {
            if (rActive) {
                m_IntegratorCx->state_ = area;
                m_IntegratorCx->areaWindow_.clear();
                m_IntegratorCx->timeWindow_.clear();
                m_IntegratorCx->areaWindow_.push_back(area);
                m_IntegratorCx->timeWindow_.push_back(t);
            } else {
                m_IntegratorCx->state_ += area;
                m_IntegratorCx->areaWindow_.push_back(area);
                m_IntegratorCx->timeWindow_.push_back(t);

                // 根据窗口类型移除旧数据
                if (m_UseIntegrationWindow == IntegratorCx::WIN_DEFINED_IN_TIME) {
                    while (!m_IntegratorCx->areaWindow_.empty() &&
                           (t - m_IntegratorCx->timeWindow_.front()) > m_IntegrationTime) {
                        m_IntegratorCx->state_ -= m_IntegratorCx->areaWindow_.front();
                        m_IntegratorCx->areaWindow_.pop_front();
                        m_IntegratorCx->timeWindow_.pop_front();
                    }
                } else if (m_UseIntegrationWindow == IntegratorCx::WIN_DEFINED_IN_SAMPLES) {
                    while (static_cast<int>(m_IntegratorCx->areaWindow_.size()) > m_IntegrationSamples) {
                        m_IntegratorCx->state_ -= m_IntegratorCx->areaWindow_.front();
                        m_IntegratorCx->areaWindow_.pop_front();
                        m_IntegratorCx->timeWindow_.pop_front();
                    }
                }
            }
        }
    }

    // 保存当前输入作为下一个周期的上一个输入
    m_IntegratorCx->prevInput_ = x;

    // 应用输出限制
    m_IntegratorCx->applyLimits();
}
