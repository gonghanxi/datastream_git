#ifndef READYQUEUESCHEDULER_H
#define READYQUEUESCHEDULER_H

#include <QQueue>
#include <QMap>
#include <QSet>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <memory>
#include "Block.h"
#include "DataStreamVerification.h"
#include "../signalflowlinksort.h"

using namespace SystemVueModelBuilder;

/**
 * @brief Block运行时状态缓存
 */
struct BlockRuntimeState {
    Block* block = nullptr;

    // 就绪状态
    bool isReady = false;       // Block当前是否准备好执行
    bool inputsReady = false;   // Block的所有输入端口是否准备好（有足够数据）
    bool outputsReady = false;  // Block的所有输出端口是否准备好（有足够空间）

    // 缓存的数据计数
    QMap<int, size_t> cachedInputCount;   // 各输入端口的缓存数据量
    QMap<int, size_t> cachedOutputSpace; // 各输出端口的可用空间

    // 执行统计
    int executedCount = 0;      // 已执行的次数

    // 批量处理相关
    int batchSize = 1;           // 批量处理大小
    int requiredInputCount = 1;  // Block执行一次所需的最少输入样本数

    // 标志位
    bool isDone = false;         // 该Block是否已执行完成
    bool inReadyQueue = false;   // 该Block是否已在就绪队列中

    // ========== 背压控制 ==========
    bool isBackpressured = false; // 该Block是否因下游（输出）缓冲区已满而被阻塞
    int backpressureLevel = 0;    // 背压等级
    bool upstreamFinished = false; // 上游Block是否已全部完成
    int currentDynamicBatch = 1;       // 当前动态批量大小
    int consecutiveFailures = 0;       // 连续写入失败次数
};

/**
 * @brief 就绪队列调度器
 */
class ReadyQueueScheduler {
public:
    ReadyQueueScheduler();
    ~ReadyQueueScheduler();

    /**
     * @brief 主调度函数，执行给定链路的调度
     */
    bool schedule(const QString& linkKey,
                  QVector<Block*>& blocks,
                  std::shared_ptr<DataStreamVerification> verificationSystem,
                  const SimuParameter& simuParams,
                  SignalFlowLinkSort* topologySorter = nullptr);

    // ========== 暂停/恢复/停止控制 ==========
    void setPauseControls(QAtomicInt* paused, QAtomicInt* stopRequested,
                          QMutex* pauseMutex, QWaitCondition* pauseCond);
    void pause();
    void resume();
    void requestStop();
    bool isPaused() const;

private:
    // ========== 初始化 ==========
    void initBlockStates(QVector<Block*>& blocks);
    void initReadyQueues(QVector<Block*>& blocks);
    void calculateRequiredInputCounts();
    int calculateSinkTarget(Block* sink);
    void calculateSinkTargets(QVector<Block*>& blocks);

    // ========== 全局仿真时钟 ==========
    void initSimulationClock(const SimuParameter& simuParams, QVector<Block*>& blocks);
    void advanceSimulationTime();

    // ========== 下游Sink引用计数 ==========
    void buildDownstreamSinkRefCounts();
    bool allSinksDone() const;

    // ========== 状态管理 ==========
    /**
     * @brief 更新Block的就绪状态
     * @param state 要更新的Block状态
     */
    void updateBlockReadyState(BlockRuntimeState& state);
    /**
     * @brief 检查Block的输入端口是否准备好
     * @param state 要检查的Block状态
     * @return 输入端口是否准备好
     */
    bool checkInputsReady(BlockRuntimeState& state);
    /**
     * @brief 检查Block的输出端口是否准备好
     * @param state 要检查的Block状态
     * @return 输出端口是否准备好
     */
    bool checkOutputsReady(BlockRuntimeState& state);
    /**
     * @brief 更新Block的缓存输入数据计数
     * @param state 要更新的Block状态
     */
    void updateCachedInputCounts(BlockRuntimeState& state);
    /**
     * @brief 更新Block的缓存输出空间计数
     * @param state 要更新的Block状态
     */
    void updateCachedOutputSpace(BlockRuntimeState& state);

    // ========== 背压控制 ==========
    /**
     * @brief 检查一个Block是否因下游阻塞（背压）而无法执行
     * @param block 要检查的Block
     * @return 该Block是否因背压而被阻塞
     */
    bool isBackpressured(Block* block);
    /**
     * @brief 检查Block的所有输出是否已被完全消费
     * @param state 要检查的Block状态
     * @return Block的所有输出是否已被完全消费
     */
    bool isOutputAllConsumed(BlockRuntimeState& state);
    /**
     * @brief 计算动态批量大小（根据下游Buffer使用率）
     */
    int calculateDynamicBatch(BlockRuntimeState& state);

    /**
     * @brief 传播背压到上游
     */
    void propagateBackpressureToUpstream(Block* block);

    /**
     * @brief 解除背压
     */
    void releaseBackpressure(Block* block);

    /**
     * @brief 设置背压回调
     */
    void setupBackpressureCallbacks(BlockRuntimeState& state);

    // ========== 通知机制 ==========
    /**
     * @brief 通知指定Block的所有下游Block更新状态
     * @param block 触发通知的源Block
     */
    void notifyDownstream(Block* block);
    /**
     * @brief 通知指定Block的所有上游Block更新状态
     * @param block 触发通知的源Block
     */
    void notifyUpstream(Block* block);
    /**
     * @brief 通知指定Sink的所有上游Block，该Sink已结束，上游应停止生产
     * @param sink 已完成收集的Sink Block
     */
    void notifyUpstreamFinished(Block* sink);
    /**
     * @brief 强制停止所有上游（非Sink）Block
     */
    void stopAllUpstreamBlocks();
    /**
     * @brief 将一个Block根据其类型和优先级加入相应的就绪队列
     * @param state 要加入队列的Block状态
     */
    void addToReadyQueue(BlockRuntimeState& state);
    /**
     * @brief 将一个Block从就绪队列中移除
     * @param state 要从队列中移除的Block状态
     */
    void removeFromReadyQueue(BlockRuntimeState& state);

    // ========== 执行逻辑 ==========
    bool executeSource(BlockRuntimeState& state, int& globalProcessCount, int maxProcessCount);
    bool executeSourceWithBackpressure(BlockRuntimeState& state);
    bool executeProcessor(BlockRuntimeState& state);
    bool executeProcessorWithBackpressure(BlockRuntimeState& state);
    int executeSinkWithCount(BlockRuntimeState& state);
    int tryExecuteBatch(BlockRuntimeState& state, int maxBatch);

    // ========== 辅助方法 ==========
    /**
     * @brief 获取指定Block的运行时状态
     * @param block 目标Block
     * @return 指向其运行时状态的指针，若未找到则返回nullptr
     */
    BlockRuntimeState* getState(Block* block);
    /**
     * @brief 获取指定Block的优先级
     * @param block 目标Block
     * @return Block的优先级，数值越大优先级越高
     */
    int getBlockPriority(Block* block);
    /**
     * @brief 报告调度进度
     * @param linkKey 链路标识符
     * @param sinkCounts 各Sink已收集的样本数
     * @param sinkTargets 各Sink的目标收集数
     */
    void reportProgress(const QString& linkKey,
                        const std::map<std::string, int>& sinkCounts,
                        const std::map<std::string, int>& sinkTargets);

    // ========== 成员变量 ==========
    QMap<Block*, BlockRuntimeState> m_states;
    QQueue<Block*> m_readySources;
    QQueue<Block*> m_readyProcessors;
    QQueue<Block*> m_readySinks;
    QMap<int, QQueue<Block*>> m_readyByPriority;

    int m_totalSamples = 0;
    double m_timeInterval = 0.0;
    std::shared_ptr<DataStreamVerification> m_verificationSystem;
    QVector<Block*> m_executionOrder;
    QMap<Block*, int> m_blockIndex;

    std::map<std::string, int> m_sinkProcessCount;
    std::map<std::string, int> m_sinkTargetCounts;
    int m_lastProgressPercent = -1;

    // Buffer 容量限制
    const size_t DEFAULT_BUFFER_SIZE = 1024;

    // 背压控制
    QSet<Block*> m_backpressuredBlocks;

    // ========== 全局仿真时钟 (Task 1) ==========
    double m_simulationTime = 0.0;
    double m_timeStep = 0.0;
    unsigned long long m_sourceFireCount = 0;
    bool m_hasTimeDrivenSink = false;

    // ========== 暂停/停止控制 (Task 2) ==========
    QAtomicInt* m_paused = nullptr;
    QAtomicInt* m_stopRequested = nullptr;
    QMutex* m_pauseMutex = nullptr;
    QWaitCondition* m_pauseCond = nullptr;
    bool m_stopSignal = false;  // 内部停止信号

    // ========== 下游Sink引用计数 (Task 4) ==========
    std::map<Block*, int> m_downstreamSinkRefCount;

    // ========== 拓扑排序 (Task 6) ==========
    SignalFlowLinkSort* m_topologySorter = nullptr;
    QMap<Block*, int> m_processorPriority;
};

#endif // READYQUEUESCHEDULER_H
