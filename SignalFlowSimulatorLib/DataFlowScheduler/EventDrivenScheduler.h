#ifndef EVENTDRIVENSCHEDULER_H
#define EVENTDRIVENSCHEDULER_H

#include <QMap>
#include <QSet>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <QVector>
#include <QElapsedTimer>
#include <memory>
#include <functional>
#include <cmath>
#include <map>
#include <set>

#include "Block.h"
#include "signalflowlinksort.h"
#include "DataStreamVerification.h"

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
        double eventThreshold = 0.0;
        EventDetectionMode detectionMode = EventDetectionMode::ZERO_CROSSING;
        double lastOutputValue = 0.0;
        bool hasOutputHistory = false;
        bool eventTriggered = false;
        bool isDone = false;
        int totalEventsTriggered = 0;
        int totalStepsExecuted = 0;
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

        // 数据流调度相关
        unsigned long long currentIteration = 0;
        int sourceCount = 0;
        int sinkCount = 0;
        int OutputBusCount = 0;
        std::map<std::string, int> sinkProcessCount;

        // ZeroCross 下游映射
        QMap<Block*, QSet<Block*>> zeroCrossDownstreamMap;
        QVector<Block*> zeroCrossBlocks;

        // 模型事件状态
        QMap<Block*, BlockEventState> blockStates;
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

    // ========== 数据流调度接口（类似 SimpleScheduler） ==========

    bool schedule(const QString& linkKey,
                  QVector<Block*> blocks,
                  std::shared_ptr<DataStreamVerification> verificationSystem,
                  const SimuParameter& simuParams = SimuParameter());

    void setPauseControls(QAtomicInt* paused,
                          QAtomicInt* stopRequested,
                          QMutex* pauseMutex,
                          QWaitCondition* pauseCond);

    // ========== 时间驱动接口（保留） ==========

    bool InitializeScheduler(const QString& linkKey,
                             QVector<Block*> blocks,
                             SignalFlowLinkSort* topologySorter = nullptr);

    bool RunSimulation(const QString& linkKey,
                       QAtomicInt* pausedFlag = nullptr,
                       QAtomicInt* stopRequestedFlag = nullptr,
                       QMutex* pauseMutex = nullptr,
                       QWaitCondition* pauseCond = nullptr);

    bool ProcessOneTimeStep(const QString& linkKey);
    void SendCommand(Command cmd, const QString& linkKey = QString());

    // ========== 状态查询接口 ==========

    SchedulerState GetSchedulerState(const QString& linkKey) const;
    double GetCurrentSimulationTime(const QString& linkKey) const;
    int GetCurrentStep(const QString& linkKey) const;
    bool HasScheduler(const QString& linkKey) const;
    bool IsSimulationComplete(const QString& linkKey) const;

    // ========== 配置接口 ==========

    void SetEventThreshold(const QString& linkKey, Block* block, double threshold);
    void SetEventDetectionMode(const QString& linkKey, Block* block, EventDetectionMode mode);
    void SetStopSignal(bool stopSignal);
    bool GetStopSignal() const;

private:
    // ========== 数据流调度核心方法 ==========

    bool eventDrivenSchedulerImpl(const QString& linkKey,
                                  QVector<Block*> blocks,
                                  std::shared_ptr<DataStreamVerification> verificationSystem,
                                  const SimuParameter& simuParams);

    void precomputeDownstreamSets(SchedulerContext& ctx);
    bool isDownstreamOfTriggeredZeroCross(const SchedulerContext& ctx, Block* block) const;
    bool isBlockedByUntriggeredZeroCross(const SchedulerContext& ctx, Block* block) const;
    int generalWork(Block* currentBlock);
    int calculateMaxProcessCount(QVector<Block*> blocks, const QString& linkKey, int sourceCount);

    // ========== 时间驱动核心方法（保留） ==========

    QVector<Block*> buildExecutionOrder(const QVector<Block*>& blocks,
                                        SignalFlowLinkSort* sorter,
                                        const QString& linkKey);
    void initializeBlockEventStates(SchedulerContext& ctx);
    void initializeTimeConfig(SchedulerContext& ctx);
    double readEventThreshold(Block* block);

    bool detectEvent(BlockEventState& state, double currentOutput);
    bool detectZeroCrossing(double prevValue, double currentValue);
    bool detectThresholdCrossing(double prevValue, double currentValue,
                                 double threshold, EventDetectionMode mode);

    bool processTimeStepForContext(SchedulerContext& ctx);
    bool executeTriggeredBlocks(SchedulerContext& ctx);
    bool executeBlock(SchedulerContext& ctx, Block* block);
    void collectTriggeredBlocks(SchedulerContext& ctx);
    void propagateEventToDownstream(SchedulerContext& ctx, Block* source);
    bool areAllSinksComplete(const SchedulerContext& ctx) const;
    void stopCompletedSinks(SchedulerContext& ctx);
    void flushAllSinks(SchedulerContext& ctx);
    void DoneAllModels(SchedulerContext& ctx);
    void applyCommand(SchedulerContext& ctx, Command cmd);
    void resetSchedulerContext(SchedulerContext& ctx);

    // ========== 成员变量 ==========

    QMap<QString, SchedulerContext> m_schedulers;
    bool m_stopSignal = false;

    // 暂停控制（数据流调度用）
    QAtomicInt* m_paused = nullptr;
    QAtomicInt* m_stopRequested = nullptr;
    QMutex* m_pauseMutex = nullptr;
    QWaitCondition* m_pauseCond = nullptr;

    // 拓扑排序器
    SignalFlowLinkSort m_topologySorter;
};

#endif // EVENTDRIVENSCHEDULER_H
