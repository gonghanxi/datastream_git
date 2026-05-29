#include "TimeSynchronizer_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>

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

// ============================================================================
// 构造函数
// ============================================================================

TimeSynchronizer_Block::TimeSynchronizer_Block(const std::string& name)
    : Block(name)
    , m_Mode(TimeSynchronizer::ZeroPadding)
    , m_sampleCount(0)
    , m_sampleRate(0.0)
    , N_(0)
{
}

// ============================================================================
// 默认参数
// ============================================================================

void TimeSynchronizer_Block::SetDefaultParameters()
{
    m_Mode = TimeSynchronizer::ZeroPadding;
}

// ============================================================================
// 参数同步到算法实例
// ============================================================================

void TimeSynchronizer_Block::SetParameters()
{
    if (!m_TimeSynchronizer) return;
    m_TimeSynchronizer->Mode = m_Mode;
}

// ============================================================================
// Setup / Run / Initialize
// ============================================================================

bool TimeSynchronizer_Block::Setup()
{
    Block::Setup();
    return true;
}

bool TimeSynchronizer_Block::Run()
{
    return DataStreamRun();
}

bool TimeSynchronizer_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_TimeSynchronizer = std::make_unique<TimeSynchronizer>();
    SetDefaultParameters();

    // 读取参数
    try { m_Mode = ConvertStringToModeEnum(getParameter("Mode").Value); } catch (...) {}

    SetParameters();

    if (!m_TimeSynchronizer->Setup()) {
        LOG_ERROR("TimeSynchronizer Setup failed");
        return false;
    }

    // 获取仿真参数
    m_simuParam = getSimu();
    m_sampleRate = m_simuParam.samplingRate;
    m_sampleCount = 0;

    // 注册端口
    AddInputPort("input", m_TimeSynchronizer->input, 1, DataType::DOUBLE_BUS);
    AddOutputPort("output", m_TimeSynchronizer->output, 1, DataType::DOUBLE_BUS);

    return true;
}

// ============================================================================
// DataStreamRun：核心运行逻辑
// ============================================================================

bool TimeSynchronizer_Block::DataStreamRun()
{
    SetParameters();

    // 获取端口数量
    auto* inputReader = GetInputPort(GetInputPortName(0));
    auto* outputBuffer = GetOutputPort(GetOutputPortName(0));
    if (!inputReader || !outputBuffer) return false;

    const int nin = static_cast<int>(inputReader->GetBusConnectionCount());
    const int nout = static_cast<int>(outputBuffer->GetBusConnectionCount());
    N_ = std::min(nin, nout);

    if (N_ <= 0) return true;

    // 初始化 FIFO 和 lastValue
    if (fifos_.size() != static_cast<size_t>(N_)) {
        fifos_.assign(static_cast<size_t>(N_), {});
        lastValue_.assign(static_cast<size_t>(N_), 0.0);
    }

    // 计算当前仿真时间
    const double t = m_simuParam.startTime + static_cast<double>(m_sampleCount) / m_sampleRate;

    // 读取所有输入通道的数据并加入 FIFO
    for (int i = 0; i < N_; ++i) {
        auto inputData = ReadInputData<double>(GetInputPortName(0));
        if (!inputData.empty()) {
            SampleD s;
            s.v = inputData[0];
            s.t = t;
            fifos_[static_cast<size_t>(i)].push_back(s);
        }
    }

    // 找到目标时间
    double target = fifos_[0].front().t;
    for (int i = 1; i < N_; ++i) {
        if (!fifos_[static_cast<size_t>(i)].empty()) {
            const double ti = fifos_[static_cast<size_t>(i)].front().t;
            if (m_Mode == TimeSynchronizer::ZeroPadding) {
                target = std::min(target, ti);
            } else {
                target = std::max(target, ti);
            }
        }
    }

    // 根据模式计算所有通道的输出数据
    std::vector<double> outputData;
    outputData.reserve(static_cast<size_t>(N_));

    if (m_Mode == TimeSynchronizer::ZeroPadding) {
        for (int i = 0; i < N_; ++i) {
            double y = 0.0;
            if (!fifos_[static_cast<size_t>(i)].empty() &&
                std::fabs(fifos_[static_cast<size_t>(i)].front().t - target) <= eps()) {
                y = fifos_[static_cast<size_t>(i)].front().v;
                lastValue_[static_cast<size_t>(i)] = y;
                fifos_[static_cast<size_t>(i)].pop_front();
            }
            outputData.push_back(y);
        }
    } else {
        for (int i = 0; i < N_; ++i) {
            while (!fifos_[static_cast<size_t>(i)].empty() &&
                   fifos_[static_cast<size_t>(i)].front().t <= target + eps()) {
                lastValue_[static_cast<size_t>(i)] = fifos_[static_cast<size_t>(i)].front().v;
                fifos_[static_cast<size_t>(i)].pop_front();
            }
            outputData.push_back(lastValue_[static_cast<size_t>(i)]);
        }
    }

    if (!outputData.empty()) {
        WriteOutputData(GetOutputPortName(0), outputData);
    }

    // 更新采样计数
    m_sampleCount += 1;

    return true;
}

// ============================================================================
// 枚举转换
// ============================================================================

TimeSynchronizer::ModeEnum TimeSynchronizer_Block::ConvertStringToModeEnum(const std::string& value)
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
