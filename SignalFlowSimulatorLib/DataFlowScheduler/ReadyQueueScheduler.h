#ifndef READYQUEUESCHEDULER_H
#define READYQUEUESCHEDULER_H

#include <QQueue>
#include <QMap>
#include <QSet>
#include <QQueue>
#include <memory>
#include "Block.h"
#include "DataStreamVerification.h"

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
     * @param linkKey 链路标识符
     * @param blocks 该链路中包含的所有Block
     * @param verificationSystem 数据流验证系统
     * @param simuParams 仿真参数
     * @return 调度是否成功
     */
    bool schedule(const QString& linkKey,
                  QVector<Block*>& blocks,
                  std::shared_ptr<DataStreamVerification> verificationSystem,
                  const SimuParameter& simuParams);

private:
    // ========== 初始化 ==========
    /**
     * @brief 初始化所有Block的运行时状态结构
     * @param blocks 要初始化的Block列表
     */
    void initBlockStates(QVector<Block*>& blocks);
    /**
     * @brief 初始化就绪队列
     * @param blocks 所有Block列表
     */
    void initReadyQueues(QVector<Block*>& blocks);
    /**
     * @brief 计算每个Block所需的输入数量
     */
    void calculateRequiredInputCounts();
    /**
     * @brief 计算单个Sink Block的目标收集样本数
     * @param sink 目标Sink Block
     * @return 该Sink需要收集的样本数
     */
    int calculateSinkTarget(Block* sink);
    /**
     * @brief 计算所有Sink Block的目标收集样本数
     * @param blocks 所有Block列表
     */
    void calculateSinkTargets(QVector<Block*>& blocks);

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
    /**
     * @brief 执行一个信号源（Source）Block
     * @param state 要执行的Source Block的状态
     * @param globalProcessCount 全局已处理的样本计数
     * @param maxProcessCount 最大可处理样本数
     * @return 是否成功执行
     */
    bool executeSource(BlockRuntimeState& state, int& globalProcessCount, int maxProcessCount);
    /**
     * @brief 执行一个信号源（Source）Block，包含背压控制
     * @param state 要执行的Source Block的状态
     * @return 是否成功执行
     */
    bool executeSourceWithBackpressure(BlockRuntimeState& state);
    /**
     * @brief 执行一个处理器（Processor）Block
     * @param state 要执行的Processor Block的状态
     * @return 是否成功执行
     */
    bool executeProcessor(BlockRuntimeState& state);
    /**
     * @brief 执行一个处理器（Processor）Block，包含背压控制
     * @param state 要执行的Processor Block的状态
     * @return 是否成功执行
     */
    bool executeProcessorWithBackpressure(BlockRuntimeState& state);
    /**
     * @brief 执行一个收集器（Sink）Block，并记录收集数量
     * @param state 要执行的Sink Block的状态
     * @return 本次执行收集到的样本数
     */
    int executeSinkWithCount(BlockRuntimeState& state);
    /**
     * @brief 尝试以批量模式执行一个Block
     * @param state 要执行的Block状态
     * @param maxBatch 最大批量大小
     * @return 实际执行的样本数
     */
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
    QMap<Block*, BlockRuntimeState> m_states; ///< 所有Block的运行时状态映射
    QQueue<Block*> m_readySources;            ///< 就绪的信号源（Source）队列
    QQueue<Block*> m_readyProcessors;         ///< 就绪的处理器（Processor）队列
    QQueue<Block*> m_readySinks;              ///< 就绪的收集器（Sink）队列
    QMap<int, QQueue<Block*>> m_readyByPriority; ///< 按优先级组织的处理器就绪队列

    int m_totalSamples = 0;                     ///< 仿真的总样本数
    double m_timeInterval = 0.0;                 ///< 仿真的时间间隔
    std::shared_ptr<DataStreamVerification> m_verificationSystem; ///< 数据流验证系统
    QVector<Block*> m_executionOrder;           ///< Block的执行顺序
    QMap<Block*, int> m_blockIndex;              ///< Block到其索引的映射

    std::map<std::string, int> m_sinkProcessCount;  ///< 各Sink已处理的样本数
    std::map<std::string, int> m_sinkTargetCounts;  ///< 各Sink的目标处理样本数
    int m_lastProgressPercent = -1;                 ///< 上一次报告的进度百分比

    // Buffer 容量限制
    const size_t DEFAULT_BUFFER_SIZE = 1024;    ///< Buffer的默认容量

    // 用于防止重复传播背压
    QSet<Block*> m_backpressuredBlocks;         ///< 已处于背压状态的Block集合
};

#endif // READYQUEUESCHEDULER_H
