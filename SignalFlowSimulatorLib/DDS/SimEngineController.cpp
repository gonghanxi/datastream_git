#include "SimEngineController.h"
#include <Block.h>
#include "algorithmmanager.h"
#include "signalflowlinksort.h"
#include <QDebug>
#include <algorithm>

SimEngineController::SimEngineController()
{
}

SimEngineController::~SimEngineController()
{
}

// ========== 原有接口实现 ==========

int SimEngineController::GetModelStatus(const QString &modelName)
{
    EngineState state = m_currentState.value(modelName, EngineState::INIT);
    return static_cast<int>(state);
}

void SimEngineController::SetCurrentState(const QString &modelName, SimEngineController::EngineState state)
{
    m_currentState[modelName] = state;

    // 同步更新调度器状态
    if (m_schedulers.contains(modelName)) {
        m_schedulers[modelName].state = state;
    }
}

// ========== 新调度器实现 ==========

bool SimEngineController::InitializeScheduler(
    const QString& linkKey,
    QVector<Block*> blocks,
    std::shared_ptr<DataStreamVerification> verificationSystem,
    SignalFlowLinkSort* topologySorter)
{
    if (!verificationSystem) {
        qDebug() << "[Scheduler] Verification system is null!";
        return false;
    }

    // 创建调度器上下文
    SchedulerContext ctx;
    ctx.linkKey = linkKey;
    ctx.state = EngineState::INIT;
    ctx.pendingCommand = Command::NONE;

    // 1. 数据一致性校验
    auto verSys = Block::GetVerificationSystem();
    if (!verSys) {
        qDebug() << "[Scheduler] Cannot get verification system!";
        return false;
    }

    for (auto block : blocks) {
        verSys->registerBlock(block);
    }

    if (!verificationSystem->CheckFeasibility()) {
        qDebug() << "[Scheduler] Data consistency check failed!";
        return false;
    }

    qDebug() << "[Scheduler] Data consistency check passed";

    // 2. 构建执行顺序
    ctx.executionOrder = buildExecutionOrder(blocks, topologySorter);

    // 3. 统计并分类块
    ctx.sourceCount = 0;
    ctx.sinkCount = 0;

    for (Block* block : ctx.executionOrder) {
        Block::BlockType type = block->GetBlockType();

        if (type == Block::BlockType::SOURCE) {
            ctx.sourceCount++;
            // 获取信号源的采样率
            ctx.samplingRateUs = getBlockSamplingRate(block);
        } else if (type == Block::BlockType::SINK) {
            ctx.sinkCount++;
            ctx.sinkProcessCount[block->GetName()] = 0;
        }
    }

    if (ctx.sourceCount == 0 || ctx.sinkCount == 0) {
        qDebug() << "[Scheduler] Missing source or sink blocks!";
        return false;
    }

    // 4. 计算预期总节拍数（基于总仿真时间和采样率）
    // 假设总仿真时间为10ms（10个1ms节拍）
    ctx.totalExpectedBeats = 10;  // 可以从配置中读取

    // 5. 初始化所有块
    for (Block* block : ctx.executionOrder) {
        block->SetDone(false);
    }

    // 存储调度器上下文
    m_schedulers[linkKey] = ctx;

    qDebug() << "[Scheduler] Initialized for link:" << linkKey;
    qDebug() << "[Scheduler] Source count:" << ctx.sourceCount;
    qDebug() << "[Scheduler] Sink count:" << ctx.sinkCount;
    qDebug() << "[Scheduler] Sampling rate:" << ctx.samplingRateUs << "us";
    qDebug() << "[Scheduler] Expected beats:" << ctx.totalExpectedBeats;
    qDebug() << "[Scheduler] Execution order:";
    for (int i = 0; i < ctx.executionOrder.size(); i++) {
        qDebug() << "  " << i << ":" << QString::fromStdString(ctx.executionOrder[i]->GetName());
    }

    return true;
}

bool SimEngineController::ProcessOneBeat(int beatNumber, double beatDurationMs)
{
    // 遍历所有调度器（使用迭代器避免结构化绑定问题）
    QMapIterator<QString, SchedulerContext> it(m_schedulers);
    bool anyProcessed = false;

    while (it.hasNext()) {
        it.next();
        SchedulerContext& ctx = m_schedulers[it.key()];  // 获取可修改的引用

        // 先处理待处理的命令
        if (ctx.pendingCommand != Command::NONE) {
            applyCommand(ctx, ctx.pendingCommand);
            ctx.pendingCommand = Command::NONE;
        }

        // 只有RUN状态才处理节拍
        if (ctx.state != EngineState::RUN) {
            if (ctx.state == EngineState::PAUSE) {
                ctx.totalBeatsSkipped++;
                qDebug() << "[Scheduler] Beat" << beatNumber << "skipped due to PAUSE state";
            } else if (ctx.state == EngineState::STOP) {
                qDebug() << "[Scheduler] Beat" << beatNumber << "skipped due to STOP state";
                continue;
            } else if (ctx.state == EngineState::INIT) {
                qDebug() << "[Scheduler] Beat" << beatNumber << "skipped (not started yet)";
                continue;
            }
            continue;
        }

        // 处理本拍节
        bool processed = processBeatForContext(ctx, beatNumber, beatDurationMs);
        if (processed) {
            anyProcessed = true;
        }
    }

    return anyProcessed;
}

void SimEngineController::SendCommand(Command cmd)
{
    // 发送命令给所有调度器
    QMapIterator<QString, SchedulerContext> it(m_schedulers);
    while (it.hasNext()) {
        it.next();
        SendCommand(it.key(), cmd);
    }
}

void SimEngineController::SendCommand(const QString& linkKey, Command cmd)
{
    if (!m_schedulers.contains(linkKey)) {
        qDebug() << "[Scheduler] No scheduler found for link:" << linkKey;
        return;
    }

    SchedulerContext& ctx = m_schedulers[linkKey];

    qDebug() << "[Scheduler] Received command:" << static_cast<int>(cmd)
             << "for link:" << linkKey;

    // 立即应用命令
    applyCommand(ctx, cmd);
}

SimEngineController::EngineState SimEngineController::GetSchedulerState(const QString& linkKey) const
{
    if (m_schedulers.contains(linkKey)) {
        return m_schedulers[linkKey].state;
    }
    return EngineState::STOP;
}

double SimEngineController::GetCurrentProgress(const QString& linkKey) const
{
    if (m_schedulers.contains(linkKey)) {
        return m_schedulers[linkKey].currentProgress;
    }
    return 0.0;
}

int SimEngineController::GetCurrentBeat(const QString& linkKey) const
{
    if (m_schedulers.contains(linkKey)) {
        return m_schedulers[linkKey].currentBeat;
    }
    return -1;
}

bool SimEngineController::IsAllBlocksDone(const QString& linkKey) const
{
    if (!m_schedulers.contains(linkKey)) {
        return true;
    }

    const SchedulerContext& ctx = m_schedulers[linkKey];
    return ctx.currentProgress >= 100;
}

// ========== 私有方法实现 ==========

QVector<Block*> SimEngineController::buildExecutionOrder(
    const QVector<Block*>& blocks,
    SignalFlowLinkSort* sorter)
{
    QMap<Block::BlockType, QVector<Block*>> blocksByType;

    // 按类型分组
    for (Block* block : blocks) {
        blocksByType[block->GetBlockType()].append(block);
    }

    // 处理器排序
    QVector<Block*> sortedProcessors;
    if (!blocksByType[Block::BlockType::PROCESSOR].isEmpty() && sorter) {
        sortedProcessors = sorter->sortProcessorsCrossLayer(
            "",
            AlgorithmManager::createInstance()->getBlocksInfo(),
            AlgorithmManager::createInstance()->getConnection()
        );
    } else {
        sortedProcessors = blocksByType[Block::BlockType::PROCESSOR];
    }

    // 构建最终执行顺序：SOURCE → PROCESSOR → SINK
    QVector<Block*> executionOrder;
    executionOrder.append(blocksByType[Block::BlockType::SOURCE]);
    executionOrder.append(sortedProcessors);
    executionOrder.append(blocksByType[Block::BlockType::SINK]);

    return executionOrder;
}

bool SimEngineController::processBeatForContext(
    SchedulerContext& ctx,
    int beatNumber,
    double beatDurationMs)
{
    bool progressMade = false;
    ctx.currentBeat = beatNumber;

    // 计算本拍节需要处理的数据点数
    // 1ms / 采样率(us) = 本拍节数据点数
    double samplesPerMs = 1000.0 / ctx.samplingRateUs;
    int targetSamples = static_cast<int>(samplesPerMs * beatDurationMs);

    qDebug() << "[Scheduler] Beat" << beatNumber
             << "| Target samples:" << targetSamples;

    // 为每个SOURCE设置本拍节的目标
    for (Block* block : ctx.executionOrder) {
        if (block->GetBlockType() == Block::BlockType::SOURCE) {
            ctx.blockProcessTargets[block] = targetSamples;
        }
    }

    // 按拓扑顺序处理每个块
    for (Block* block : ctx.executionOrder) {
        if (block->IsDone()) {
            continue;
        }

        bool blockProgress = false;

        switch (block->GetBlockType()) {
        case Block::BlockType::SOURCE:
            blockProgress = processSourceInBeat(ctx, block);
            break;
        case Block::BlockType::PROCESSOR:
            blockProgress = processProcessorInBeat(ctx, block);
            break;
        case Block::BlockType::SINK:
            blockProgress = processSinkInBeat(ctx, block);
            break;
        }

        if (blockProgress) {
            progressMade = true;
        }
    }

    // 更新进度
    updateProgress(ctx);

    qDebug() << "[Scheduler] Beat" << beatNumber
             << "completed | Progress:" << ctx.currentProgress
             << "% | Data points:" << ctx.totalDataPointsProcessed;

    return progressMade;
}

bool SimEngineController::processSourceInBeat(SchedulerContext& ctx, Block* block)
{
    int targetCount = ctx.blockProcessTargets.value(block, 0);
    int processedCount = 0;

    qDebug() << "[Source]" << QString::fromStdString(block->GetName())
             << "| Target:" << targetCount;

    // 尝试处理目标次数
    for (int i = 0; i < targetCount; i++) {
        // 检查输出缓冲区空间
        bool outputReady = true;
        for (size_t portIdx = 0; portIdx < block->GetOutputPortCount(); portIdx++) {
            std::string portName = block->GetOutputPortName(portIdx);
            Buffer* buffer = block->GetOutputPort(portName);

            if (buffer->GetReaderCount() > 0) {
                size_t freeSpace = buffer->GetBufferFreeSpace();
                if (freeSpace == 0) {
                    outputReady = false;
                    qDebug() << "[Source] Output buffer full, stopping at" << i;
                    break;
                }
            }
        }

        if (!outputReady) {
            break;  // 缓冲区满，本拍节不再处理
        }

        // 执行一次运行
        if (block->Run()) {
            processedCount++;
            ctx.totalDataPointsProcessed++;
        } else {
            break;  // 运行失败
        }
    }

    qDebug() << "[Source] Processed:" << processedCount << "/" << targetCount;
    return processedCount > 0;
}

bool SimEngineController::processProcessorInBeat(SchedulerContext& ctx, Block* block)
{
    bool progressMade = false;
    int processedCount = 0;

    // 处理器持续处理直到输入数据耗尽或输出缓冲区满
    const int MAX_ITERATIONS = 10000;  // 防止死循环
    for (int i = 0; i < MAX_ITERATIONS; i++) {
        // 检查输入数据可用性
        bool inputReady = true;
        bool hasActiveInput = false;

        for (size_t portIdx = 0; portIdx < block->GetInputPortCount(); portIdx++) {
            std::string portName = block->GetInputPortName(portIdx);
            BufferReader* reader = block->GetInputPort(portName);

            if (!reader->HasValidConnection() &&
                !reader->IsBusType(reader->GetDataType())) {
                continue;  // 跳过无效端口
            }

            hasActiveInput = true;

            if (!reader->HasDataAvailable()) {
                if (reader->IsUpstreamDone() && reader->HasValidConnection()) {
                    // 上游已完成，但还有未处理的数据
                    continue;
                }
                inputReady = false;
                break;
            }
        }

        if (!hasActiveInput || !inputReady) {
            break;  // 没有输入数据
        }

        // 检查输出缓冲区空间
        bool outputReady = true;
        for (size_t portIdx = 0; portIdx < block->GetOutputPortCount(); portIdx++) {
            std::string portName = block->GetOutputPortName(portIdx);
            Buffer* buffer = block->GetOutputPort(portName);

            if (buffer->GetReaderCount() > 0) {
                size_t freeSpace = buffer->GetBufferFreeSpace();
                if (freeSpace == 0) {
                    outputReady = false;
                    break;
                }
            }
        }

        if (!outputReady) {
            break;  // 输出缓冲区满
        }

        // 执行处理
        if (block->Run()) {
            processedCount++;
            progressMade = true;
        } else {
            break;  // 处理失败
        }
    }

    if (processedCount > 0) {
        qDebug() << "[Processor]" << QString::fromStdString(block->GetName())
                 << "| Processed:" << processedCount;
    }

    return progressMade;
}

bool SimEngineController::processSinkInBeat(SchedulerContext& ctx, Block* block)
{
    bool progressMade = false;
    int collectedCount = 0;

    // 收集所有可用数据
    const int MAX_ITERATIONS = 10000;  // 防止死循环
    for (int i = 0; i < MAX_ITERATIONS; i++) {
        // 检查输入数据
        bool hasData = false;
        int maxAvailable = 0;

        for (size_t portIdx = 0; portIdx < block->GetInputPortCount(); portIdx++) {
            std::string portName = block->GetInputPortName(portIdx);
            BufferReader* reader = block->GetInputPort(portName);

            size_t available = reader->GetAvailableDataCount();
            if (available > 0) {
                hasData = true;
                if (available > maxAvailable) {
                    maxAvailable = available;
                }
            }
        }

        if (!hasData) {
            break;  // 没有数据可收集
        }

        // 执行收集
        if (block->Run()) {
            collectedCount += maxAvailable;
            ctx.sinkProcessCount[block->GetName()] += maxAvailable;
            progressMade = true;
        } else {
            break;  // 收集失败
        }
    }

    if (collectedCount > 0) {
        qDebug() << "[Sink]" << QString::fromStdString(block->GetName())
                 << "| Collected:" << collectedCount;
    }

    return progressMade;
}

void SimEngineController::applyCommand(SchedulerContext& ctx, Command cmd)
{
    switch (cmd) {
    case Command::START:
        if (ctx.state == EngineState::INIT || ctx.state == EngineState::PAUSE) {
            ctx.state = EngineState::RUN;
            qDebug() << "[Scheduler] State -> RUN for:" << ctx.linkKey;
        }
        break;

    case Command::PAUSE:
        if (ctx.state == EngineState::RUN) {
            ctx.state = EngineState::PAUSE;
            qDebug() << "[Scheduler] State -> PAUSE for:" << ctx.linkKey;
        }
        break;

    case Command::STOP:
        ctx.state = EngineState::STOP;
        qDebug() << "[Scheduler] State -> STOP for:" << ctx.linkKey;

        // 停止所有块
        for (Block* block : ctx.executionOrder) {
            if (!block->IsDone()) {
                block->SetDone(true);
                block->Stop();
            }
        }

        // 最终进度
        updateProgress(ctx);
        qDebug() << "[Scheduler] Final progress:" << ctx.currentProgress << "%";
        break;

    case Command::RESET:
        qDebug() << "[Scheduler] State -> INIT (Reset) for:" << ctx.linkKey;

        // 重置所有状态
        ctx.state = EngineState::INIT;
        ctx.currentBeat = 0;
        ctx.currentProgress = 0;
        ctx.totalDataPointsProcessed = 0;
        ctx.totalBeatsSkipped = 0;
        ctx.sinkProcessCount.clear();
        ctx.blockProcessTargets.clear();

        // 重新初始化块
        for (Block* block : ctx.executionOrder) {
            block->SetDone(false);
        }

        // 重新初始化SINK计数
        for (Block* block : ctx.executionOrder) {
            if (block->GetBlockType() == Block::BlockType::SINK) {
                ctx.sinkProcessCount[block->GetName()] = 0;
            }
        }
        break;

    case Command::NONE:
    default:
        break;
    }
}

void SimEngineController::updateProgress(SchedulerContext& ctx)
{
    if (ctx.totalExpectedBeats <= 0) {
        ctx.currentProgress = 0;
        return;
    }

    // 基于节拍计算进度
    int effectiveBeats = ctx.currentBeat - ctx.totalBeatsSkipped + 1;
    ctx.currentProgress = (effectiveBeats * 100) / ctx.totalExpectedBeats;

    if (ctx.currentProgress > 100) {
        ctx.currentProgress = 100;
    }

    // 检查所有块是否完成
    if (ctx.currentProgress >= 100) {
        bool allDone = true;
        for (Block* block : ctx.executionOrder) {
            if (!block->IsDone() && block->GetBlockType() != Block::BlockType::SINK) {
                allDone = false;
                break;
            }
        }

        if (allDone) {
            ctx.currentProgress = 100;
        }
    }
}

double SimEngineController::getBlockSamplingRate(Block* block)
{
    // 从块的配置中获取采样率
    // 这里需要根据实际的Block接口来获取
    // 示例：返回默认值1us
    double samplingRate = 1.0;  // 默认1微秒

    // TODO: 实际实现需要读取块的采样率参数
    // 例如：samplingRate = block->GetParameter("sampling_rate").toDouble();

    return samplingRate;
}

bool SimEngineController::checkBlockCompletion(SchedulerContext& ctx, Block* block)
{
    // SOURCE块完成条件：达到总数据点
    if (block->GetBlockType() == Block::BlockType::SOURCE) {
        return ctx.currentProgress >= 100;
    }

    // PROCESSOR块完成条件：上游完成且没有待处理数据
    if (block->GetBlockType() == Block::BlockType::PROCESSOR) {
        bool upstreamDone = true;
        for (size_t i = 0; i < block->GetInputPortCount(); i++) {
            BufferReader* reader = block->GetInputPort(block->GetInputPortName(i));
            if (reader->HasValidConnection() && !reader->IsUpstreamDone()) {
                upstreamDone = false;
                break;
            }
        }
        return upstreamDone && ctx.currentProgress >= 100;
    }

    // SINK块完成条件：收集完所有数据
    if (block->GetBlockType() == Block::BlockType::SINK) {
        return ctx.currentProgress >= 100;
    }

    return false;
}
