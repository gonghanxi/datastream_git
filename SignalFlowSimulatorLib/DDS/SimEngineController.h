// SimEngineController.h
#ifndef SIMENGINECONTROLLER_H
#define SIMENGINECONTROLLER_H

#include <QMap>
#include <QVector>
#include <memory>
#include <functional>
#include "Block.h"
#include "DataStreamVerification.h"
#include "signalflowlinksort.h"
using namespace SystemVueModelBuilder;
class SimEngineController {
public:
    enum class EngineState{
        INIT = 0,    // 初始化
        RUN = 1,     // 运行中
        PAUSE = 2,   // 暂停（收到节拍但不执行）
        STOP = 3     // 停止
    };

    // 调度器控制命令
    enum class Command{
        NONE = 0,
        START = 1,      // 开始/继续运行
        PAUSE = 2,      // 暂停
        STOP = 3,       // 停止
        RESET = 4       // 复位
    };

    SimEngineController();
    ~SimEngineController();

    // 原有接口
    int GetModelStatus(const QString &modelName);
    void SetCurrentState(const QString &modelName, SimEngineController::EngineState state);

    // ========== 新调度器接口 ==========
    // 初始化调度器
    bool InitializeScheduler(const QString& linkKey,
                            QVector<Block*> blocks,
                            std::shared_ptr<DataStreamVerification> verificationSystem,
                            SignalFlowLinkSort* topologySorter = nullptr);

    // 处理一个节拍（1ms）
    // 返回：true-成功处理，false-跳过（暂停或停止状态）
    bool ProcessOneBeat(int beatNumber, double beatDurationMs = 1.0);

    // 发送控制命令
    void SendCommand(Command cmd);
    void SendCommand(const QString& linkKey, Command cmd);

    // 获取调度器状态
    EngineState GetSchedulerState(const QString& linkKey) const;
    double GetCurrentProgress(const QString& linkKey) const;
    int GetCurrentBeat(const QString& linkKey) const;
    bool IsAllBlocksDone(const QString& linkKey) const;

private:
    QMap<QString, EngineState> m_currentState;

    // ========== 节拍调度器数据结构 ==========
    struct SchedulerContext {
        QString linkKey;
        QVector<Block*> executionOrder;
        int sourceCount;
        int sinkCount;

        // 进度跟踪
        std::map<std::string, int> sinkProcessCount;
        int currentProgress;           // 0-100
        int currentBeat;               // 当前节拍号
        int totalExpectedBeats;        // 预期总节拍数（从计算得出）

        // 控制状态
        EngineState state;             // 当前状态
        Command pendingCommand;        // 待处理的命令

        // 统计信息
        int totalDataPointsProcessed;
        int totalBeatsSkipped;         // 因暂停跳过的节拍数

        // 配置
        double samplingRateUs;         // 采样率（微秒）
        QMap<Block*, int> blockProcessTargets; // 每个块本拍节处理目标

        SchedulerContext()
            : sourceCount(0), sinkCount(0), currentProgress(0),
              currentBeat(0), totalExpectedBeats(0),
              state(EngineState::INIT), pendingCommand(Command::NONE),
              totalDataPointsProcessed(0), totalBeatsSkipped(0),
              samplingRateUs(1.0) {}
    };

    QMap<QString, SchedulerContext> m_schedulers;

    // 辅助方法
    QVector<Block*> buildExecutionOrder(const QVector<Block*>& blocks,
                                        SignalFlowLinkSort* sorter);
    bool processBeatForContext(SchedulerContext& ctx, int beatNumber, double beatDurationMs);
    bool processSourceInBeat(SchedulerContext& ctx, Block* block);
    bool processProcessorInBeat(SchedulerContext& ctx, Block* block);
    bool processSinkInBeat(SchedulerContext& ctx, Block* block);
    void applyCommand(SchedulerContext& ctx, Command cmd);
    void updateProgress(SchedulerContext& ctx);
    double getBlockSamplingRate(Block* block);
    bool checkBlockCompletion(SchedulerContext& ctx, Block* block);
};

#endif
