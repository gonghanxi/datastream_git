#ifndef TIMEDRIVENSCHEDULER_H
#define TIMEDRIVENSCHEDULER_H

#include <QMap>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <QVector>
#include <QElapsedTimer>
#include <memory>
#include <functional>

#include "Block.h"
#include "signalflowlinksort.h"

using namespace SystemVueModelBuilder;

class TimeDrivenScheduler {
public:
    // 驱动模式枚举
    enum class DriveMode {
        FIXED_STEP,     // 固定步长
        VARIABLE_STEP   // 变步长（预留）
    };

    // 时序不匹配处理策略
    enum class TimingStrategy {
        HOLD_LAST,      // 保持上一个值
        ZERO_FILL,      // 零值填充
        INTERPOLATE     // 插值（预留）
    };

    // Flush策略
    enum class FlushStrategy {
        BY_STEP_COUNT,  // 按步数
        BY_TIME,        // 按时间间隔
        BY_DATA_COUNT   // 按数据量
    };

    // 进度计算策略
    enum class ProgressStrategy {
        BY_TIME,        // 按时间比例
        BY_STEPS        // 按步数比例
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

    // 调度器上下文
    struct SchedulerContext {
        QString linkKey;
        QVector<Block*> executionOrder;
        int sourceCount;
        int sinkCount;

        // 时间相关
        double currentTime;           // 当前仿真时间
        double endTime;               // 仿真结束时间
        double timeStep;              // 当前步长
        int currentStep;              // 当前步数
        int totalEstimatedSteps;      // 预估总步数

        // 变步长相关（预留）
        DriveMode driveMode;
        bool isVariableStep;
        double minTimeStep;
        double maxTimeStep;
        std::map<Block*, double> blockNextExecTime;

        // SINK刷新相关
        FlushStrategy flushStrategy;
        int flushInterval;            // 刷新间隔（步数或数据量）
        double flushTimeInterval;     // 刷新时间间隔（秒）
        int stepsSinceLastFlush;      // 距上次刷新的步数
        double lastFlushTime;         // 上次刷新时间
        int dataPointsSinceLastFlush; // 距上次刷新的数据点数

        // 时序策略
        TimingStrategy timingStrategy;

        // 进度相关
        ProgressStrategy progressStrategy;
        int currentProgress;

        // 控制状态
        SchedulerState state;
        Command pendingCommand;
        bool isPaused;

        // 统计信息
        int totalDataPointsProcessed;
        int totalStepsExecuted;
        int totalStepsSkipped;
        std::map<Block*, unsigned long long> sinkDataPoints;  // 每个Sink的收集点数

        // 配置
        double samplingRateUs;        // 仿真器采样率
        double startTimeUs;           // 仿真器起始时间
        double stopTimeUs;            // 仿真器终止时间

        SchedulerContext()
            : sourceCount(0), sinkCount(0),
              currentTime(0.0), endTime(1.0), timeStep(0.001),
              currentStep(0), totalEstimatedSteps(0),
              driveMode(DriveMode::FIXED_STEP), isVariableStep(false),
              minTimeStep(1e-6), maxTimeStep(0.1),
              flushStrategy(FlushStrategy::BY_STEP_COUNT),
              flushInterval(100), flushTimeInterval(0.1),
              stepsSinceLastFlush(0), lastFlushTime(0.0),
              dataPointsSinceLastFlush(0),
              timingStrategy(TimingStrategy::HOLD_LAST),
              progressStrategy(ProgressStrategy::BY_TIME),
              currentProgress(0),
              state(SchedulerState::INIT), pendingCommand(Command::NONE),
              isPaused(false),
              totalDataPointsProcessed(0), totalStepsExecuted(0),
              totalStepsSkipped(0),
              samplingRateUs(1.0), startTimeUs(0.0), stopTimeUs(1.0)
        {}
    };

    TimeDrivenScheduler();
    ~TimeDrivenScheduler();

    // ========== 调度器核心接口 ==========

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
    double GetCurrentProgress(const QString& linkKey) const;
    double GetCurrentSimulationTime(const QString& linkKey) const;
    int GetCurrentStep(const QString& linkKey) const;
    double GetCurrentTimeStep(const QString& linkKey) const;
    bool IsSimulationComplete(const QString& linkKey) const;
    bool HasScheduler(const QString& linkKey) const;

    // ========== 配置接口 ==========

    void SetFlushStrategy(const QString& linkKey, FlushStrategy strategy, int interval = 100);
    void SetTimingStrategy(const QString& linkKey, TimingStrategy strategy);
    void SetProgressStrategy(const QString& linkKey, ProgressStrategy strategy);
    void SetDriveMode(const QString& linkKey, DriveMode mode);
    void SetStepBounds(const QString& linkKey, double minStep, double maxStep);
    void SetEndTime(const QString& linkKey, double endTime);

    // 停止信号（外部控制）
    void SetStopSignal(bool stopSignal);
    bool GetStopSignal() const;

    // 节拍设置（兼容原接口）
    void SetCurStep(int curStep);
    int GetCurStep() const;

private:
    // ========== 核心私有方法 ==========

    // 构建执行顺序
    QVector<Block*> buildExecutionOrder(const QVector<Block*>& blocks,
                                        SignalFlowLinkSort* sorter,
                                        const QString& linkKey);

    // 处理单个时间步
    bool processTimeStepForContext(SchedulerContext& ctx);

    // 执行一个时间步内的所有模型
    bool executeOneTimeStep(SchedulerContext& ctx, const std::vector<bool>& executionMask);

    // 向所有模型广播当前时间
    void notifyCurrentTime(SchedulerContext& ctx);

    // 检查是否所有Sink执行完毕
    bool areAllSinksComplete(const SchedulerContext& ctx) const;

    // 将已执行完的Sink停止
    void stopCompletedSinks(SchedulerContext& ctx);

    // ========== 模型处理方法 ==========

    // 在时间步中执行SOURCE
    bool processSourceInTimeStep(SchedulerContext& ctx, Block* block);

    // 在时间步中执行PROCESSOR
    bool processProcessorInTimeStep(SchedulerContext& ctx, Block* block);

    // 在时间步中执行SINK
    bool processSinkInTimeStep(SchedulerContext& ctx, Block* block);

    // ========== 辅助方法 ==========

    // 解析时间配置
    void initializeTimeConfig(SchedulerContext& ctx);

    // 判断仿真是否结束
    bool isSimulationComplete(const SchedulerContext& ctx) const;

    // 判断是否需要刷新SINK
    bool shouldFlushSink(const SchedulerContext& ctx);

    // 刷新指定SINK
    void flushSinkBlock(Block* block);

    // 刷新所有SINK
    void flushAllSinks(SchedulerContext& ctx);

    // 所有模型 执行Done
    void DoneAllModels(SchedulerContext& ctx);

    // 应用控制命令
    void applyCommand(SchedulerContext& ctx, Command cmd);

    // 更新进度
    void updateProgress(SchedulerContext& ctx);

    // 重置调度器上下文
    void resetSchedulerContext(SchedulerContext& ctx);

    // 检查模型是否完成
    bool checkBlockCompletion(SchedulerContext& ctx, Block* block);

    // 设置所有SINK的输出路径
    void collectSinkOutputPaths();

    // ========== 变步长相关（预留） ==========

    // 计算下一个时间步长
    double calculateNextTimeStep(SchedulerContext& ctx);

    // 收集模型时间需求
    void collectBlockTimeRequests(SchedulerContext& ctx, std::map<Block*, double>& requests);

    // 构建执行掩码
    std::vector<bool> buildExecutionMask(SchedulerContext& ctx);

    // 处理时序不匹配
    void resolveTimingMismatch(SchedulerContext& ctx, Block* block);

    // 步长稳定化（完整实现）
    double stabilizeTimeStep(SchedulerContext& ctx, double candidateStep);

    // 验证步长合法性
    bool validateTimeStep(double step, const SchedulerContext& ctx);

    // 处理SINK刷新策略
    bool shouldFlushByTimeElapsed(const SchedulerContext& ctx);
    bool shouldFlushByDataCount(const SchedulerContext& ctx);

    // 更新模型执行时间
    void updateBlockExecutionTimes(SchedulerContext& ctx);

    // 处理数据速率不匹配
    void handleDataRateMismatch(SchedulerContext& ctx, Block* source, Block* target);

    // 确保时间单调性
    bool assertTimeMonotonic(double previousTime, double newTime);

    // 内存压力检测（返回当前进程内存使用MB，失败返回0）
    static size_t getProcessMemoryMB();

    // 检查内存是否超限（默认阈值2GB）
    bool checkMemoryPressure(size_t thresholdMB = 2048) const;

    // 预估总步数
    void estimateTotalSteps(SchedulerContext& ctx);

    // ========== 成员变量 ==========

    QMap<QString, SchedulerContext> m_schedulers;
    std::map<std::string, std::string> m_sinkOutputPaths;
    bool m_stopSignal;
    int m_curStep;  // 兼容原节拍接口

    // 单模型执行超时（毫秒），默认60秒
    static constexpr int BLOCK_EXEC_TIMEOUT_MS = 60000;
    // 内存检查间隔（步数），每100步检查一次
    static constexpr int MEMORY_CHECK_INTERVAL = 100;
    int m_stepsSinceLastMemoryCheck = 0;
};

#endif // TIMEDRIVENSCHEDULER_H
