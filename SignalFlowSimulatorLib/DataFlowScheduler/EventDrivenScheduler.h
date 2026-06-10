#ifndef EVENTDRIVENSCHEDULER_H
#define EVENTDRIVENSCHEDULER_H

#include <QMap>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <QVector>
#include <QElapsedTimer>
#include <memory>
#include <functional>
#include <cmath>

#include "Block.h"
#include "signalflowlinksort.h"

using namespace SystemVueModelBuilder;

class EventDrivenScheduler {
public:
    // 事件检测模式
    enum class EventDetectionMode {
        ZERO_CROSSING,      // 过零检测：sign(y[n]) != sign(y[n-1])
        THRESHOLD_RISING,   // 上升沿穿越：y[n-1] < threshold && y[n] >= threshold
        THRESHOLD_FALLING,  // 下降沿穿越：y[n-1] >= threshold && y[n] < threshold
        THRESHOLD_BOTH      // 双向穿越（上升沿或下降沿）
    };

    // 调度器状态
    enum class SchedulerState {
        INIT = 0,
        RUN = 1,
        PAUSE = 2,
        STOP = 3,
        NONE = 4
    };

    // 控制命令
    enum class Command {
        NONE = 0,
        START = 1,
        PAUSE = 2,
        STOP = 3,
        RESET = 4
    };

    // 单个模型的事件状态
    struct BlockEventState {
        Block* block = nullptr;
        double eventThreshold = 0.0;            // 事件触发阈值
        EventDetectionMode detectionMode = EventDetectionMode::ZERO_CROSSING;
        double lastOutputValue = 0.0;           // 上一步的输出值
        bool hasOutputHistory = false;          // 是否有历史输出
        bool eventTriggered = false;            // 本步是否触发了事件
        bool isDone = false;                    // 是否已完成

        // 事件统计
        int totalEventsTriggered = 0;           // 累计触发事件次数
        int totalStepsExecuted = 0;             // 累计执行步数
    };

    // 调度器上下文
    struct SchedulerContext {
        QString linkKey;
        QVector<Block*> executionOrder;

        // 时间相关
        double currentTime = 0.0;
        double endTime = 1.0;
        double timeStep = 0.001;
        int currentStep = 0;

        // 模型事件状态
        QMap<Block*, BlockEventState> blockStates;

        // 事件触发的模型队列（本步需要执行的模型）
        QVector<Block*> triggeredBlocks;

        // 统计信息
        int totalEventsDetected = 0;
        int totalStepsExecuted = 0;

        // 控制状态
        SchedulerState state = SchedulerState::INIT;
        Command pendingCommand = Command::NONE;
        bool isPaused = false;

        // 仿真参数
        double samplingRate = 1.0;
        double startTime = 0.0;
        double stopTime = 1.0;
    };

    EventDrivenScheduler();
    ~EventDrivenScheduler();

    // ========== 核心接口 ==========

    // 初始化调度器
    bool InitializeScheduler(const QString& linkKey,
                             QVector<Block*> blocks,
                             SignalFlowLinkSort* topologySorter = nullptr);

    // 运行仿真（完整循环）
    bool RunSimulation(const QString& linkKey,
                       QAtomicInt* pausedFlag = nullptr,
                       QAtomicInt* stopRequestedFlag = nullptr,
                       QMutex* pauseMutex = nullptr,
                       QWaitCondition* pauseCond = nullptr);

    // 推进单个时间步
    bool ProcessOneTimeStep(const QString& linkKey);

    // 控制命令
    void SendCommand(Command cmd, const QString& linkKey = QString());

    // ========== 状态查询接口 ==========

    SchedulerState GetSchedulerState(const QString& linkKey) const;
    double GetCurrentSimulationTime(const QString& linkKey) const;
    int GetCurrentStep(const QString& linkKey) const;
    bool HasScheduler(const QString& linkKey) const;
    bool IsSimulationComplete(const QString& linkKey) const;

    // ========== 配置接口 ==========

    // 设置指定模型的事件阈值
    void SetEventThreshold(const QString& linkKey, Block* block, double threshold);

    // 设置指定模型的事件检测模式
    void SetEventDetectionMode(const QString& linkKey, Block* block, EventDetectionMode mode);

    // 停止信号
    void SetStopSignal(bool stopSignal);
    bool GetStopSignal() const;

private:
    // ========== 核心私有方法 ==========

    // 构建执行顺序
    QVector<Block*> buildExecutionOrder(const QVector<Block*>& blocks,
                                        SignalFlowLinkSort* sorter,
                                        const QString& linkKey);

    // 初始化模型事件状态
    void initializeBlockEventStates(SchedulerContext& ctx);

    // 解析时间配置
    void initializeTimeConfig(SchedulerContext& ctx);

    // 从模型参数读取 EventThreshold
    double readEventThreshold(Block* block);

    // ========== 事件检测 ==========

    // 检测单个模型是否产生事件
    bool detectEvent(BlockEventState& state, double currentOutput);

    // 过零检测
    bool detectZeroCrossing(double prevValue, double currentValue);

    // 阈值穿越检测
    bool detectThresholdCrossing(double prevValue, double currentValue,
                                 double threshold, EventDetectionMode mode);

    // ========== 执行逻辑 ==========

    // 处理单个时间步
    bool processTimeStepForContext(SchedulerContext& ctx);

    // 执行事件触发的模型
    bool executeTriggeredBlocks(SchedulerContext& ctx);

    // 执行单个模型
    bool executeBlock(SchedulerContext& ctx, Block* block);

    // 收集事件触发的模型列表
    void collectTriggeredBlocks(SchedulerContext& ctx);

    // 传播事件到下游
    void propagateEventToDownstream(SchedulerContext& ctx, Block* source);

    // 检查是否所有Sink完成
    bool areAllSinksComplete(const SchedulerContext& ctx) const;

    // 停止已完成的Sink
    void stopCompletedSinks(SchedulerContext& ctx);

    // 刷新所有Sink
    void flushAllSinks(SchedulerContext& ctx);

    // 所有模型执行Done
    void DoneAllModels(SchedulerContext& ctx);

    // 应用控制命令
    void applyCommand(SchedulerContext& ctx, Command cmd);

    // 重置调度器上下文
    void resetSchedulerContext(SchedulerContext& ctx);

    // ========== 成员变量 ==========

    QMap<QString, SchedulerContext> m_schedulers;
    bool m_stopSignal = false;
};

#endif // EVENTDRIVENSCHEDULER_H
