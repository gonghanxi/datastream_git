#include "TimeSynchronizer_Block.h"
#include <algorithm>
#include <cmath>

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
{
}

// ============================================================================
// Setup — 仿真开始前重置
// ============================================================================

bool TimeSynchronizer_Block::Setup()
{
    Block::Setup();

    m_firingCount = 0;
    m_fifos.clear();
    m_lastValue.clear();
    m_inputChannelBuffer.clear();
    while(!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }

    return true;
}

// ============================================================================
// Run — 运行模式分发
// ============================================================================

bool TimeSynchronizer_Block::Run()
{
    if(!CanProcess()) {
        return false;
    }

    if(IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

// ============================================================================
// Initialize — 模块初始化
// ============================================================================
//
// 创建算法实例并注册端口：
//  - 输入端口（1 个）：
//      input : DOUBLE_BUS 类型，多路带时间戳 double 信号总线
//  - 输出端口（1 个）：
//      output : DOUBLE_BUS 类型，同步后的多路 double 信号总线
//  输入输出端口速率均为 1（每次 firing 每通道处理 1 个样本）
//
// 注意：原始算法使用 CircularBufferBusT<TimedCircularBuffer<double>>，
// 框架将其映射为 DOUBLE_BUS 类型。

bool TimeSynchronizer_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_TimeSynchronizer = std::make_unique<TimeSynchronizer>();

    SetDefaultParameters();

    // 读取参数
    try { m_Mode = ConvertStringToModeEnum(getParameter("Mode").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Mode', using default value."); }

    SetParameters();

    // 注册端口（Bus 类型）
    AddInputPort("input", m_TimeSynchronizer->input, 1, DataType::DOUBLE_BUS);
    AddOutputPort("output", m_TimeSynchronizer->output, 1, DataType::DOUBLE_BUS);

    return true;
}

// ============================================================================
// DataStreamRun — 固定步长多通道时间同步模式
// ============================================================================
//
// 读取 input BUS 端口的全部 double 样本数据，展开为 flat 向量。
// 将 flat 数据分配到各通道的 FIFO 队列中，
// 然后逐帧处理：计算每通道时间戳，确定对齐目标时间，
// 根据 ZeroPadding / TimeDelay 模式输出同步结果。
//
// ReadInputData<double> 将所有总线通道的数据展开为一个一维向量，
// 数据布局为 [ch0_all_samples, ch1_all_samples, ...]（按通道顺序排列）。

bool TimeSynchronizer_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    BufferReader* inputReader = GetInputPort(inputPortName);
    Buffer* outputBuffer = GetOutputPort(outputPortName);
    if(!inputReader || !outputBuffer) {
        return false;
    }

    // 获取输入/输出 bus 通道数
    size_t numInputChannels = inputReader->GetBusConnectionCount();
    size_t numOutputChannels = outputBuffer->GetBusConnectionCount();
    int N = static_cast<int>(std::min(numInputChannels, numOutputChannels));
    if(N <= 0) {
        return true;
    }

    // 展开读取所有 BUS 通道的 double 数据
    std::vector<double> inputData = ReadInputData<double>(inputPortName);
    if(inputData.empty()) {
        return true;
    }

    // 每通道的样本数 = 总数据量 / 通道数
    size_t totalSamples = inputData.size();
    size_t samplesPerChannel = totalSamples / static_cast<size_t>(N);
    if(samplesPerChannel == 0) {
        return true;
    }

    // 初始化 FIFO 和 lastValue（通道数变化时重新分配）
    if(static_cast<int>(m_fifos.size()) != N) {
        m_fifos.assign(N, {});
        m_lastValue.assign(N, 0.0);
    }

    // 将 flat 数据分配到各通道的 FIFO 队列
    // 数据布局：按通道顺序排列 [ch0_all_samples, ch1_all_samples, ...]
    for(int i = 0; i < N; ++i) {
        for(size_t s = 0; s < samplesPerChannel; ++s) {
            size_t idx = static_cast<size_t>(i) * samplesPerChannel + s;
            if(idx < inputData.size()) {
                SampleD sample;
                sample.v = inputData[idx];
                sample.t = 0.0;  // 时间戳将在下面计算
                m_fifos[i].push_back(sample);
            }
        }
    }

    // 获取仿真时间参数用于计算每帧时间戳
    SimuParameter simuParam = getSimu();
    double startTime = simuParam.startTime;
    double timeStep = (simuParam.samplingRate > 0.0)
                      ? (1.0 / simuParam.samplingRate) : 1.0;

    // 逐帧处理同步算法
    for(size_t s = 0; s < samplesPerChannel; ++s) {
        double currentFiringTime = startTime + static_cast<double>(m_firingCount + s) * timeStep;

        // 为当前帧的各通道样本设置时间戳
        for(int i = 0; i < N; ++i) {
            if(!m_fifos[i].empty()) {
                m_fifos[i].front().t = currentFiringTime;
            }
        }

        // 确定目标对齐时间
        double target = m_fifos[0].front().t;
        for(int i = 1; i < N; ++i) {
            if(!m_fifos[i].empty()) {
                double t = m_fifos[i].front().t;
                if(m_Mode == TimeSynchronizer::ZeroPadding) {
                    target = std::min(target, t);
                } else {
                    target = std::max(target, t);
                }
            }
        }

        // 根据模式输出同步结果
        std::vector<double> frameOutput(N);
        if(m_Mode == TimeSynchronizer::ZeroPadding) {
            for(int i = 0; i < N; ++i) {
                double y = 0.0;
                if(!m_fifos[i].empty() &&
                    std::fabs(m_fifos[i].front().t - target) <= eps()) {
                    y = m_fifos[i].front().v;
                    m_lastValue[i] = y;
                    m_fifos[i].pop_front();
                }
                frameOutput[i] = y;
            }
        } else {
            // TimeDelay 模式
            for(int i = 0; i < N; ++i) {
                while(!m_fifos[i].empty() &&
                      m_fifos[i].front().t <= target + eps()) {
                    m_lastValue[i] = m_fifos[i].front().v;
                    m_fifos[i].pop_front();
                }
                frameOutput[i] = m_lastValue[i];
            }
        }

        // 逐通道写入输出 bus
        for(int i = 0; i < N; ++i) {
            outputBuffer->WriteDataToChannel(i, std::vector<double>{frameOutput[i]});
        }
    }

    m_firingCount += samplesPerChannel;
    return true;
}

// ============================================================================
// TimeDrivenRun — 变步长逐点累积时间同步模式
// ============================================================================
//
// 工作于变步长仿真模式（VariableStep），样本以非固定时间间隔到达。
// 处理流程分为三步：
//
//   1. 【累积】遍历 input bus 上的所有桥接 Reader，
//      将每个 Reader 到达的 double 样本累积到对应的通道缓冲区中。
//
//   2. 【判断】检查所有通道缓冲区是否都已累积了至少 1 个样本，
//      若未全部就绪则跳过本帧，等待下次 firing 继续累积。
//
//   3. 【同步输出】从每个通道缓冲区取出一个样本，
//      应用 ZeroPadding / TimeDelay 同步逻辑，
//      将同步结果推入输出队列，逐点写出。

bool TimeSynchronizer_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    BufferReader* inputMaster = GetInputPort(inputPortName);
    Buffer* outputBuffer = GetOutputPort(outputPortName);
    if(!inputMaster || !outputBuffer) {
        return true;
    }

    // —— 步骤 1：从 input BUS 的各桥接 Reader 读取并累积 double 样本 ——
    auto bridgeReaders = inputMaster->GetBusConnections();
    for(const auto& conn : bridgeReaders) {
        std::vector<double> channelData;
        conn.bridgeReader->ReadData(channelData);
        for(size_t i = 0; i < channelData.size(); i++) {
            m_inputChannelBuffer[conn.bridgeReader].push_back(channelData[i]);
        }
    }

    // —— 步骤 2：检查所有通道是否都至少累积了 1 个样本 ——
    bool canProcess = !bridgeReaders.empty();
    for(const auto& conn : bridgeReaders) {
        if(m_inputChannelBuffer[conn.bridgeReader].empty()) {
            canProcess = false;
            break;
        }
    }

    if(!canProcess) {
        return true;  // 等待更多通道数据到达
    }

    // —— 步骤 3：应用同步逻辑并输出 ——
    int N = static_cast<int>(bridgeReaders.size());

    // 初始化 FIFO（首次或通道数变化时）
    if(static_cast<int>(m_fifos.size()) != N) {
        m_fifos.assign(N, {});
        m_lastValue.assign(N, 0.0);
    }

    // 将累积数据转入 FIFO 队列
    int chIdx = 0;
    for(const auto& conn : bridgeReaders) {
        auto& buf = m_inputChannelBuffer[conn.bridgeReader];
        for(size_t j = 0; j < buf.size(); ++j) {
            SampleD s;
            s.v = buf[j];
            s.t = 0.0;  // 时间驱动模式下使用帧时间
            m_fifos[chIdx].push_back(s);
        }
        buf.clear();
        ++chIdx;
    }

    // 计算当前帧时间戳（使用全局仿真参数）
    SimuParameter simuParam = getSimu();
    double currentTime = simuParam.startTime;
    if(simuParam.samplingRate > 0.0) {
        double dt = 1.0 / simuParam.samplingRate;
        currentTime += static_cast<double>(m_firingCount) * dt;
    }

    // 为各通道 FIFO 前端样本设置时间戳
    for(int i = 0; i < N; ++i) {
        if(!m_fifos[i].empty()) {
            m_fifos[i].front().t = currentTime;
        }
    }

    // 确定目标对齐时间
    double target = m_fifos[0].front().t;
    for(int i = 1; i < N; ++i) {
        if(!m_fifos[i].empty()) {
            double t = m_fifos[i].front().t;
            if(m_Mode == TimeSynchronizer::ZeroPadding) {
                target = std::min(target, t);
            } else {
                target = std::max(target, t);
            }
        }
    }

    // 根据模式计算同步输出
    std::vector<double> frameOutput(N);
    if(m_Mode == TimeSynchronizer::ZeroPadding) {
        for(int i = 0; i < N; ++i) {
            double y = 0.0;
            if(!m_fifos[i].empty() &&
                std::fabs(m_fifos[i].front().t - target) <= eps()) {
                y = m_fifos[i].front().v;
                m_lastValue[i] = y;
                m_fifos[i].pop_front();
            }
            frameOutput[i] = y;
        }
    } else {
        // TimeDelay 模式
        for(int i = 0; i < N; ++i) {
            while(!m_fifos[i].empty() &&
                  m_fifos[i].front().t <= target + eps()) {
                m_lastValue[i] = m_fifos[i].front().v;
                m_fifos[i].pop_front();
            }
            frameOutput[i] = m_lastValue[i];
        }
    }

    // 将同步结果推入输出队列
    m_outputQueue.push(frameOutput);

    // 从输出队列取一帧结果写出
    if(!m_outputQueue.empty()) {
        std::vector<double> outputFrame = m_outputQueue.front();
        m_outputQueue.pop();

        for(int i = 0; i < N && i < static_cast<int>(outputFrame.size()); ++i) {
            outputBuffer->WriteDataToChannel(i, std::vector<double>{outputFrame[i]});
        }
    }

    m_firingCount++;
    return true;
}

// ============================================================================
// 参数处理
// ============================================================================

TimeSynchronizer::ModeEnum TimeSynchronizer_Block::ConvertStringToModeEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "zeropadding" || lower == "0") {
        return TimeSynchronizer::ZeroPadding;
    }
    if(lower == "timedelay" || lower == "1") {
        return TimeSynchronizer::TimeDelay;
    }
    return TimeSynchronizer::ZeroPadding;
}

void TimeSynchronizer_Block::SetParameters()
{
    if(!m_TimeSynchronizer) return;
    m_TimeSynchronizer->Mode = m_Mode;
}

void TimeSynchronizer_Block::SetDefaultParameters()
{
    m_Mode = TimeSynchronizer::ZeroPadding;
}
