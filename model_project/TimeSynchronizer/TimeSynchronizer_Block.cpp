#include "TimeSynchronizer_Block.h"
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
TimeSynchronizer_Block::TimeSynchronizer_Block(const std::string& name)
    : Block(name)
{
}

bool TimeSynchronizer_Block::Setup()
{
    Block::Setup();
    return true;
}

bool TimeSynchronizer_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);
    BufferReader* inputReader = GetInputPort(inputPort);
    Buffer* outputBuffer = GetOutputPort(outputPort);

    // 获取总线连接数量
    size_t InputNumConnections = inputReader->GetBusConnectionCount();
    size_t OutputNumConnections = outputBuffer->GetBusConnectionCount();

    const int nin = InputNumConnections;
    const int nout = OutputNumConnections;
    N_ = std::min(nin, nout);

    fifos_.assign(std::max(0, N_), {});
    lastValue_.assign(std::max(0, N_), 0.0);

    return true;
}

bool TimeSynchronizer_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_TimeSynchronizer = std::make_unique<TimeSynchronizer>();

    SetDefaultParameters();

    // 读取参数
    try { m_Mode = ConvertStringToModeEnum(getParameter("Mode").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Mode', using default value."); }

    SetParameters();

    if (!m_TimeSynchronizer->Setup()) {
        LOG_ERROR("TimeSynchronizer Setup failed");
        return false;
    }
    AddInputPort("input", m_TimeSynchronizer->input, 1, DataType::DOUBLE_BUS);
    AddOutputPort("output", m_TimeSynchronizer->output, 1, DataType::DOUBLE_BUS);

    return true;
}

TimeSynchronizer::ModeEnum TimeSynchronizer_Block::ConvertStringToModeEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "zeropadding" || lower == "0") {
        return TimeSynchronizer::ZeroPadding;
    }
    if (lower == "timedelay" || lower == "1") {
        return TimeSynchronizer::TimeDelay;
    }
    return TimeSynchronizer::ZeroPadding;
}

void TimeSynchronizer_Block::SetParameters()
{
    if (!m_TimeSynchronizer) return;

    // 设置模式
    m_TimeSynchronizer->Mode = m_Mode;
}

void TimeSynchronizer_Block::SetDefaultParameters()
{
    m_Mode = TimeSynchronizer::ZeroPadding;
}
