#include "EventDrivenScheduler.h"
#include "algorithmmanager.h"
#include <QDebug>
#include <QQueue>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>

EventDrivenScheduler::EventDrivenScheduler()
    : m_stopSignal(false)
{
}

EventDrivenScheduler::~EventDrivenScheduler()
{
}

// ========== 核心接口实现 ==========

bool EventDrivenScheduler::InitializeScheduler(
    const QString& linkKey,
    QVector<Block*> blocks,
    SignalFlowLinkSort* topologySorter)
{
    if (blocks.isEmpty()) {
        LOG_ERROR("[EventDrivenScheduler] No blocks to initialize!");
        return false;
    }

    SchedulerContext ctx;
    ctx.linkKey = linkKey;
    ctx.state = SchedulerState::INIT;
    ctx.isPaused = false;
    ctx.pendingCommand = Command::NONE;

    // 1. 构建执行顺序
    ctx.executionOrder = buildExecutionOrder(blocks, topologySorter, linkKey);

    qDebug() << "[EventDrivenScheduler] Initializing for link:" << linkKey;
    qDebug() << "[EventDrivenScheduler] Execution order size:" << ctx.executionOrder.size();
    for (Block* block : ctx.executionOrder) {
        qDebug() << "  -" << QString::fromStdString(block->GetName())
                 << "[" << static_cast<int>(block->GetBlockType()) << "]";
    }

    // 2. 解析时间配置
    initializeTimeConfig(ctx);

    // 3. 初始化模型事件状态
    initializeBlockEventStates(ctx);

    // 4. 统计 Source/Sink
    int sourceCount = 0, sinkCount = 0;
    for (Block* block : ctx.executionOrder) {
        if (block->GetBlockType() == Block::BlockType::SOURCE) sourceCount++;
        else if (block->GetBlockType() == Block::BlockType::SINK) sinkCount++;
    }

    if (sourceCount == 0 || sinkCount == 0) {
        LOG_ERROR("[EventDrivenScheduler] Missing source or sink blocks!");
        return false;
    }

    // 5. 初始化所有块
    for (Block* block : ctx.executionOrder) {
        block->SetDone(false);
    }

    m_schedulers[linkKey] = ctx;

    qDebug() << "[EventDrivenScheduler] Initialized successfully";
    qDebug() << "  Source count:" << sourceCount;
    qDebug() << "  Sink count:" << sinkCount;
    qDebug() << "  Time range:" << ctx.startTime << "s -" << ctx.stopTime << "s";
    qDebug() << "  Time step:" << ctx.timeStep << "s";

    return true;
}

bool EventDrivenScheduler::RunSimulation(
    const QString& linkKey,
    QAtomicInt* pausedFlag,
    QAtomicInt* stopRequestedFlag,
    QMutex* pauseMutex,
    QWaitCondition* pauseCond)
{
    if (!m_schedulers.contains(linkKey)) {
        LOG_ERROR("[EventDrivenScheduler] No scheduler found for link:", linkKey.toStdString());
        return false;
    }

    SchedulerContext& ctx = m_schedulers[linkKey];

    if (ctx.state != SchedulerState::INIT && ctx.state != SchedulerState::PAUSE) {
        LOG_ERROR("[EventDrivenScheduler] Invalid state:", static_cast<int>(ctx.state));
        return false;
    }

    ctx.state = SchedulerState::RUN;
    ctx.isPaused = false;

    qDebug() << "[EventDrivenScheduler] Starting simulation for link:" << linkKey;

    // 主仿真循环
    while (!areAllSinksComplete(ctx) && !m_stopSignal && ctx.currentTime < ctx.stopTime + 1e-9)
    {
        // 停止请求检查
        if (stopRequestedFlag && *stopRequestedFlag) {
            LOG_INFO("[EventDrivenScheduler] Stop requested, terminating...");
            m_stopSignal = true;
            break;
        }

        // 暂停检查
        if (pausedFlag && *pausedFlag) {
            LOG_INFO("[EventDrivenScheduler] Paused, waiting...");
            ctx.state = SchedulerState::PAUSE;
            ctx.isPaused = true;
            flushAllSinks(ctx);

            if (pauseMutex && pauseCond) {
                QMutexLocker locker(pauseMutex);
                while (*pausedFlag && !m_stopSignal) {
                    if (stopRequestedFlag && *stopRequestedFlag) {
                        m_stopSignal = true;
                        break;
                    }
                    pauseCond->wait(pauseMutex, 1000);
                }
            }

            if (m_stopSignal || (stopRequestedFlag && *stopRequestedFlag)) {
                break;
            }

            ctx.state = SchedulerState::RUN;
            ctx.isPaused = false;
            LOG_INFO("[EventDrivenScheduler] Resumed");
        }

        // 待处理命令
        if (ctx.pendingCommand != Command::NONE) {
            applyCommand(ctx, ctx.pendingCommand);
            ctx.pendingCommand = Command::NONE;
            if (ctx.state == SchedulerState::STOP || ctx.isPaused) {
                break;
            }
        }

        // 处理当前时间步
        if (!processTimeStepForContext(ctx)) {
            LOG_ERROR("[EventDrivenScheduler] Failed at step", ctx.currentStep);
            break;
        }
    }

    // 仿真结束处理
    if (ctx.isPaused) {
        qDebug() << "[EventDrivenScheduler] Paused at" << ctx.currentTime << "s";
    } else if (m_stopSignal || (stopRequestedFlag && *stopRequestedFlag)) {
        qDebug() << "[EventDrivenScheduler] Stopped at" << ctx.currentTime << "s";
    } else {
        qDebug() << "[EventDrivenScheduler] Completed at" << ctx.currentTime << "s";
    }

    flushAllSinks(ctx);
    DoneAllModels(ctx);

    return true;
}

bool EventDrivenScheduler::ProcessOneTimeStep(const QString& linkKey)
{
    if (!m_schedulers.contains(linkKey)) {
        return false;
    }

    SchedulerContext& ctx = m_schedulers[linkKey];

    if (ctx.state != SchedulerState::RUN) {
        return false;
    }

    if (areAllSinksComplete(ctx)) {
        return false;
    }

    return processTimeStepForContext(ctx);
}

void EventDrivenScheduler::SendCommand(Command cmd, const QString& linkKey)
{
    if (!linkKey.isEmpty()) {
        if (m_schedulers.contains(linkKey)) {
            applyCommand(m_schedulers[linkKey], cmd);
        }
    } else {
        QMapIterator<QString, SchedulerContext> it(m_schedulers);
        while (it.hasNext()) {
            it.next();
            applyCommand(m_schedulers[it.key()], cmd);
        }
    }
}

// ========== 状态查询实现 ==========

EventDrivenScheduler::SchedulerState EventDrivenScheduler::GetSchedulerState(const QString& linkKey) const
{
    if (m_schedulers.contains(linkKey)) {
        return m_schedulers[linkKey].state;
    }
    return SchedulerState::NONE;
}

double EventDrivenScheduler::GetCurrentSimulationTime(const QString& linkKey) const
{
    if (m_schedulers.contains(linkKey)) {
        return m_schedulers[linkKey].currentTime;
    }
    return 0.0;
}

int EventDrivenScheduler::GetCurrentStep(const QString& linkKey) const
{
    if (m_schedulers.contains(linkKey)) {
        return m_schedulers[linkKey].currentStep;
    }
    return -1;
}

bool EventDrivenScheduler::HasScheduler(const QString& linkKey) const
{
    return m_schedulers.contains(linkKey);
}

bool EventDrivenScheduler::IsSimulationComplete(const QString& linkKey) const
{
    if (!m_schedulers.contains(linkKey)) return true;
    return areAllSinksComplete(m_schedulers[linkKey]);
}

// ========== 配置接口实现 ==========

void EventDrivenScheduler::SetEventThreshold(const QString& linkKey, Block* block, double threshold)
{
    if (m_schedulers.contains(linkKey)) {
        SchedulerContext& ctx = m_schedulers[linkKey];
        if (ctx.blockStates.contains(block)) {
            ctx.blockStates[block].eventThreshold = threshold;
        }
    }
}

void EventDrivenScheduler::SetEventDetectionMode(const QString& linkKey, Block* block, EventDetectionMode mode)
{
    if (m_schedulers.contains(linkKey)) {
        SchedulerContext& ctx = m_schedulers[linkKey];
        if (ctx.blockStates.contains(block)) {
            ctx.blockStates[block].detectionMode = mode;
        }
    }
}

void EventDrivenScheduler::SetStopSignal(bool stopSignal)
{
    m_stopSignal = stopSignal;
    if (stopSignal) {
        QMapIterator<QString, SchedulerContext> it(m_schedulers);
        while (it.hasNext()) {
            it.next();
            SendCommand(Command::STOP, it.key());
        }
    }
}

bool EventDrivenScheduler::GetStopSignal() const
{
    return m_stopSignal;
}

// ========== 核心私有方法实现 ==========

QVector<Block*> EventDrivenScheduler::buildExecutionOrder(
    const QVector<Block*>& blocks,
    SignalFlowLinkSort* sorter,
    const QString& linkKey)
{
    QMap<Block::BlockType, QVector<Block*>> blocksByType;
    for (Block* block : blocks) {
        blocksByType[block->GetBlockType()].append(block);
    }

    QVector<Block*> sortedProcessors;
    if (!blocksByType[Block::BlockType::PROCESSOR].isEmpty() && sorter) {
        sortedProcessors = sorter->sortProcessorsCrossLayer(
            linkKey,
            AlgorithmManager::createInstance()->getBlocksInfo(),
            AlgorithmManager::createInstance()->getConnection()
        );
    } else {
        sortedProcessors = blocksByType[Block::BlockType::PROCESSOR];
    }

    QVector<Block*> executionOrder;
    executionOrder.append(blocksByType[Block::BlockType::SOURCE]);
    executionOrder.append(sortedProcessors);
    executionOrder.append(blocksByType[Block::BlockType::SINK]);
    return executionOrder;
}

void EventDrivenScheduler::initializeBlockEventStates(SchedulerContext& ctx)
{
    for (Block* block : ctx.executionOrder) {
        BlockEventState state;
        state.block = block;
        state.eventThreshold = readEventThreshold(block);
        state.detectionMode = EventDetectionMode::ZERO_CROSSING;
        state.lastOutputValue = 0.0;
        state.hasOutputHistory = false;
        state.eventTriggered = false;
        state.isDone = false;
        state.totalEventsTriggered = 0;
        state.totalStepsExecuted = 0;
        ctx.blockStates[block] = state;
    }
}

void EventDrivenScheduler::initializeTimeConfig(SchedulerContext& ctx)
{
    SimuParameter simParam = AlgorithmManager::createInstance()
        ->getSimuParameters().value(ctx.linkKey);

    ctx.samplingRate = simParam.samplingRate;
    ctx.startTime = simParam.startTime;
    ctx.stopTime = simParam.stopTime;
    ctx.currentTime = ctx.startTime;
    ctx.endTime = ctx.stopTime;

    if (ctx.samplingRate > 0) {
        ctx.timeStep = 1.0 / ctx.samplingRate;
    } else {
        ctx.timeStep = 0.001;
    }
}

double EventDrivenScheduler::readEventThreshold(Block* block)
{
    try {
        std::string val = block->getParameter("EventThreshold").Value;
        if (!val.empty()) {
            return std::stod(val);
        }
    } catch (...) {
        // 参数不存在或解析失败，使用默认值 0.0
    }
    return 0.0;
}

// ========== 事件检测实现 ==========

bool EventDrivenScheduler::detectEvent(BlockEventState& state, double currentOutput)
{
    if (!state.hasOutputHistory) {
        // 第一步，记录输出但不触发事件
        state.lastOutputValue = currentOutput;
        state.hasOutputHistory = true;
        state.eventTriggered = false;
        return false;
    }

    bool triggered = false;

    switch (state.detectionMode) {
    case EventDetectionMode::ZERO_CROSSING:
        triggered = detectZeroCrossing(state.lastOutputValue, currentOutput);
        break;
    case EventDetectionMode::THRESHOLD_RISING:
    case EventDetectionMode::THRESHOLD_FALLING:
    case EventDetectionMode::THRESHOLD_BOTH:
        triggered = detectThresholdCrossing(state.lastOutputValue, currentOutput,
                                            state.eventThreshold, state.detectionMode);
        break;
    }

    state.lastOutputValue = currentOutput;
    state.eventTriggered = triggered;

    if (triggered) {
        state.totalEventsTriggered++;
    }

    return triggered;
}

bool EventDrivenScheduler::detectZeroCrossing(double prevValue, double currentValue)
{
    // sign(y[n]) != sign(y[n-1])
    // 零值视为正方向，避免零点处的误触发
    return (prevValue < 0 && currentValue >= 0) || (prevValue >= 0 && currentValue < 0);
}

bool EventDrivenScheduler::detectThresholdCrossing(
    double prevValue, double currentValue, double threshold, EventDetectionMode mode)
{
    switch (mode) {
    case EventDetectionMode::THRESHOLD_RISING:
        return prevValue < threshold && currentValue >= threshold;
    case EventDetectionMode::THRESHOLD_FALLING:
        return prevValue >= threshold && currentValue < threshold;
    case EventDetectionMode::THRESHOLD_BOTH:
        return (prevValue < threshold && currentValue >= threshold) ||
               (prevValue >= threshold && currentValue < threshold);
    default:
        return false;
    }
}

// ========== 执行逻辑实现 ==========

bool EventDrivenScheduler::processTimeStepForContext(SchedulerContext& ctx)
{
    ctx.currentStep++;
    ctx.totalStepsExecuted++;

    // 向所有模型广播当前时间
    for (Block* block : ctx.executionOrder) {
        block->SetCurrentTime(ctx.currentTime);
    }

    // SOURCE 每步都执行（产生数据）
    for (Block* block : ctx.executionOrder) {
        if (block->GetBlockType() == Block::BlockType::SOURCE && !block->IsDone()) {
            if (!executeBlock(ctx, block)) {
                LOG_ERROR("[EventDrivenScheduler] SOURCE execution failed:", block->GetName());
                return false;
            }
        }
    }

    // 收集事件触发的模型并执行
    collectTriggeredBlocks(ctx);

    if (!executeTriggeredBlocks(ctx)) {
        return false;
    }

    // 检查已完成的Sink
    stopCompletedSinks(ctx);

    // 更新时间
    ctx.currentTime += ctx.timeStep;

    return true;
}

bool EventDrivenScheduler::executeTriggeredBlocks(SchedulerContext& ctx)
{
    for (Block* block : ctx.triggeredBlocks) {
        if (block->IsDone()) continue;

        if (!executeBlock(ctx, block)) {
            LOG_ERROR("[EventDrivenScheduler] Block execution failed:", block->GetName());
            return false;
        }
    }
    return true;
}

bool EventDrivenScheduler::executeBlock(SchedulerContext& ctx, Block* block)
{
    Q_UNUSED(ctx);
    return block->Run();
}

void EventDrivenScheduler::collectTriggeredBlocks(SchedulerContext& ctx)
{
    ctx.triggeredBlocks.clear();

    // PROCESSOR：检查输出端口的事件
    for (Block* block : ctx.executionOrder) {
        if (block->GetBlockType() != Block::BlockType::PROCESSOR) continue;
        if (block->IsDone()) continue;

        if (!ctx.blockStates.contains(block)) continue;
        BlockEventState& state = ctx.blockStates[block];

        // 获取输出端口的第一个数据值用于事件检测
        bool hasOutput = false;
        double outputValue = 0.0;

        if (block->GetOutputPortCount() > 0) {
            std::string portName = block->GetOutputPortName(0);
//            BufferReader* reader = block->GetOutputPort(portName);
//            if (reader && reader->HasDataAvailable()) {
//                std::vector<double> data;
//                reader->ReadData(data);
//                if (!data.empty()) {
//                    outputValue = data[0];
//                    hasOutput = true;
//                }
//            }
        }

        if (hasOutput) {
            if (detectEvent(state, outputValue)) {
                ctx.triggeredBlocks.append(block);
                ctx.totalEventsDetected++;
            }
        }
    }

    // SINK：只要有上游数据就触发
    for (Block* block : ctx.executionOrder) {
        if (block->GetBlockType() != Block::BlockType::SINK) continue;
        if (block->IsDone()) continue;

        bool hasData = false;
        for (size_t i = 0; i < block->GetInputPortCount(); i++) {
            std::string portName = block->GetInputPortName(i);
            BufferReader* reader = block->GetInputPort(portName);
            if (reader && reader->HasDataAvailable()) {
                hasData = true;
                break;
            }
        }

        if (hasData) {
            ctx.triggeredBlocks.append(block);
        }
    }
}

void EventDrivenScheduler::propagateEventToDownstream(SchedulerContext& ctx, Block* source)
{
    Q_UNUSED(ctx);
    Q_UNUSED(source);
    // TODO: 实现基于连接关系的下游事件传播
}

bool EventDrivenScheduler::areAllSinksComplete(const SchedulerContext& ctx) const
{
    for (Block* block : ctx.executionOrder) {
        if (block->GetBlockType() == Block::BlockType::SINK) {
            if (!block->IsCollectionComplete()) {
                return false;
            }
        }
    }
    return true;
}

void EventDrivenScheduler::stopCompletedSinks(SchedulerContext& ctx)
{
    for (Block* block : ctx.executionOrder) {
        if (block->GetBlockType() == Block::BlockType::SINK && !block->IsDone()) {
            if (block->IsCollectionComplete()) {
                qDebug() << "[EventDrivenScheduler] Sink" << QString::fromStdString(block->GetName())
                         << "collection complete. Stopping...";
                block->Stop();
                block->Done();
                block->SetDone(true);
            }
        }
    }
}

void EventDrivenScheduler::flushAllSinks(SchedulerContext& ctx)
{
    for (Block* block : ctx.executionOrder) {
        if (block->GetBlockType() == Block::BlockType::SINK && !block->IsDone()) {
            block->Flush();
        }
    }
}

void EventDrivenScheduler::DoneAllModels(SchedulerContext& ctx)
{
    for (Block* block : ctx.executionOrder) {
        if (!block->IsDone()) {
            block->SetDone(true);
            block->Stop();
            block->Done();
        }
    }
}

void EventDrivenScheduler::applyCommand(SchedulerContext& ctx, Command cmd)
{
    switch (cmd) {
    case Command::START:
        if (ctx.state == SchedulerState::INIT || ctx.state == SchedulerState::PAUSE) {
            ctx.state = SchedulerState::RUN;
            ctx.isPaused = false;
        }
        break;

    case Command::PAUSE:
        if (ctx.state == SchedulerState::RUN) {
            ctx.state = SchedulerState::PAUSE;
            ctx.isPaused = true;
            flushAllSinks(ctx);
        }
        break;

    case Command::STOP:
        ctx.state = SchedulerState::STOP;
        ctx.isPaused = false;
        flushAllSinks(ctx);
        DoneAllModels(ctx);
        break;

    case Command::RESET:
        if (ctx.state == SchedulerState::PAUSE) {
            ctx.state = SchedulerState::RUN;
            ctx.isPaused = false;
        }
        break;

    case Command::NONE:
    default:
        break;
    }
}

void EventDrivenScheduler::resetSchedulerContext(SchedulerContext& ctx)
{
    ctx.currentTime = ctx.startTime;
    ctx.currentStep = 0;
    ctx.totalEventsDetected = 0;
    ctx.totalStepsExecuted = 0;
    ctx.isPaused = false;
    ctx.pendingCommand = Command::NONE;

    for (auto it = ctx.blockStates.begin(); it != ctx.blockStates.end(); ++it) {
        it.value().lastOutputValue = 0.0;
        it.value().hasOutputHistory = false;
        it.value().eventTriggered = false;
        it.value().isDone = false;
        it.value().totalEventsTriggered = 0;
        it.value().totalStepsExecuted = 0;
    }

    for (Block* block : ctx.executionOrder) {
        block->SetDone(false);
    }
}

// ========== 数据流调度接口实现 ==========

bool EventDrivenScheduler::schedule(const QString& linkKey,
                                    QVector<Block*> blocks,
                                    std::shared_ptr<DataStreamVerification> verificationSystem,
                                    const SimuParameter& simuParams)
{
    return eventDrivenSchedulerImpl(linkKey, blocks, verificationSystem, simuParams);
}

void EventDrivenScheduler::setPauseControls(QAtomicInt* paused,
                                            QAtomicInt* stopRequested,
                                            QMutex* pauseMutex,
                                            QWaitCondition* pauseCond)
{
    m_paused = paused;
    m_stopRequested = stopRequested;
    m_pauseMutex = pauseMutex;
    m_pauseCond = pauseCond;
}

int EventDrivenScheduler::generalWork(Block* currentBlock)
{
    if (!currentBlock || currentBlock->IsDone()) {
        return -1;
    }

    if (currentBlock->Run()) {
        if (currentBlock->GetBlockType() == Block::BlockType::SOURCE) {
            std::map<std::string, Buffer*> outports = currentBlock->GetOutputPorts();
            int generate_num = 1;
            for (auto it = outports.begin(); it != outports.end(); ++it) {
                size_t WriteSize = it->second->GetWriteSize();
                generate_num = std::max(generate_num, static_cast<int>(WriteSize));
            }
            return generate_num;
        }
        return 1;
    }
    return 0;
}

int EventDrivenScheduler::calculateMaxProcessCount(QVector<Block*> blocks,
                                                   const QString& linkKey,
                                                   int sourceCount)
{
    int maxProcessCount = 0;

    for (auto block : blocks) {
        if (block->GetBlockType() == Block::BlockType::SINK) {
            std::string option = block->getParameter("StartStopOption").Value;
            std::transform(option.begin(), option.end(), option.begin(), ::tolower);

            int count = 0;
            if (option == "auto") {
                count = AlgorithmManager::createInstance()->getSimuParameters().value(linkKey).num_Samples * sourceCount;
            }
            else if (option == "samples") {
                int sampleStop = std::stoi(block->getParameter("SampleStop").Value);
                count = sampleStop * sourceCount;
            }
            else if (option == "time") {
                double timeStop = std::stod(block->getParameter("TimeStop").Value);
                double timeInterval = AlgorithmManager::createInstance()->getSimuParameters().value(linkKey).time_Interval;
                count = static_cast<int>(timeStop / timeInterval) * sourceCount;
            }

            if (count > maxProcessCount) {
                maxProcessCount = count;
            }
        }
    }

    return maxProcessCount > 0 ? maxProcessCount : 1000;
}

void EventDrivenScheduler::precomputeDownstreamSets(SchedulerContext& ctx)
{
    ctx.zeroCrossBlocks.clear();
    ctx.zeroCrossDownstreamMap.clear();

    // Find all ZeroCross blocks (使用类型标识，不依赖跨DLL虚函数)
    for (Block* block : ctx.executionOrder) {
        if (block->IsZeroCrossType()) {
            ctx.zeroCrossBlocks.append(block);
        }
    }

    if (ctx.zeroCrossBlocks.isEmpty()) {
        return;
    }

    // For each ZeroCross block, BFS forward through connections to find all downstream blocks
    for (Block* zcBlock : ctx.zeroCrossBlocks) {
        QSet<Block*>& downstreamSet = ctx.zeroCrossDownstreamMap[zcBlock];
        QQueue<Block*> bfsQueue;

        // Start from ZeroCross's output ports
        for (size_t i = 0; i < zcBlock->GetOutputPortCount(); i++) {
            std::string portName = zcBlock->GetOutputPortName(i);
            Buffer* buffer = zcBlock->GetOutputPort(portName);
            if (!buffer) continue;

            // Find all blocks that read from this buffer
            for (Block* candidate : ctx.executionOrder) {
                if (candidate == zcBlock) continue;
                for (size_t j = 0; j < candidate->GetInputPortCount(); j++) {
                    std::string inPortName = candidate->GetInputPortName(j);
                    BufferReader* reader = candidate->GetInputPort(inPortName);
                    if (reader && reader->GetConnectedBuffer() == buffer) {
                        if (!downstreamSet.contains(candidate)) {
                            downstreamSet.insert(candidate);
                            bfsQueue.enqueue(candidate);
                        }
                    }
                }
            }
        }

        // BFS: for each downstream block, find its downstream blocks
        while (!bfsQueue.isEmpty()) {
            Block* current = bfsQueue.dequeue();
            for (size_t i = 0; i < current->GetOutputPortCount(); i++) {
                std::string portName = current->GetOutputPortName(i);
                Buffer* buffer = current->GetOutputPort(portName);
                if (!buffer) continue;

                for (Block* candidate : ctx.executionOrder) {
                    if (candidate == current || downstreamSet.contains(candidate)) continue;
                    for (size_t j = 0; j < candidate->GetInputPortCount(); j++) {
                        std::string inPortName = candidate->GetInputPortName(j);
                        BufferReader* reader = candidate->GetInputPort(inPortName);
                        if (reader && reader->GetConnectedBuffer() == buffer) {
                            downstreamSet.insert(candidate);
                            bfsQueue.enqueue(candidate);
                        }
                    }
                }
            }
        }

        qDebug() << "[EventDrivenScheduler] ZeroCross block:" << QString::fromStdString(zcBlock->GetName())
                 << "downstream count:" << downstreamSet.size();
        for (Block* d : downstreamSet) {
            qDebug() << "  ->" << QString::fromStdString(d->GetName());
        }
    }
}

bool EventDrivenScheduler::isDownstreamOfTriggeredZeroCross(const SchedulerContext& ctx, Block* block) const
{
    for (Block* zc : ctx.zeroCrossBlocks) {
        if (zc->IsZeroCrossTriggered()) {
            auto it = ctx.zeroCrossDownstreamMap.find(zc);
            if (it != ctx.zeroCrossDownstreamMap.end() && it.value().contains(block)) {
                return true;
            }
        }
    }
    return false;
}

bool EventDrivenScheduler::eventDrivenSchedulerImpl(const QString& linkKey,
                                                    QVector<Block*> blocks,
                                                    std::shared_ptr<DataStreamVerification> verificationSystem,
                                                    const SimuParameter& simuParams)
{
    if (!verificationSystem) {
        LOG_ERROR("[EventDrivenScheduler] Verification system not initialized!");
        return false;
    }

    // 1. Create verification system
    auto VerificationSystem = Block::GetVerificationSystem();
    if (!VerificationSystem) {
        LOG_ERROR("[EventDrivenScheduler] Cannot get verification system!");
        return false;
    }

    // 2. Register blocks
    for (auto block : blocks) {
        VerificationSystem->registerBlock(block);
    }

    LOG_INFO("[EventDrivenScheduler] Data consistency check passed");

    // 3. Group blocks by type
    QMap<Block::BlockType, QVector<Block*>> blocksByType;
    for (Block* block : blocks) {
        blocksByType[block->GetBlockType()].append(block);
    }

    // 4. Topologically sort processors
    QVector<Block*> sortedProcessors;
    if (!blocksByType[Block::BlockType::PROCESSOR].isEmpty()) {
        sortedProcessors = m_topologySorter.sortProcessorsCrossLayer(
            linkKey,
            AlgorithmManager::createInstance()->getBlocksInfo(),
            AlgorithmManager::createInstance()->getConnection()
        );
    } else {
        sortedProcessors = blocksByType[Block::BlockType::PROCESSOR];
    }

    // 5. Build execution order
    QVector<Block*> executionOrder;
    executionOrder.append(blocksByType[Block::BlockType::SOURCE]);
    executionOrder.append(sortedProcessors);
    executionOrder.append(blocksByType[Block::BlockType::SINK]);

    qDebug() << "[EventDrivenScheduler] Execution order:";
    for (Block* block : executionOrder) {
        qDebug() << "  " << QString::fromStdString(block->GetName());
    }

    // 6. Count sources and sinks
    int sourceCount = 0, sinkCount = 0, OutputBusCount = 0;
    for (Block* block : blocks) {
        if (block->GetBlockType() == Block::BlockType::SOURCE) sourceCount++;
        else if (block->GetBlockType() == Block::BlockType::SINK) sinkCount++;
        for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
            Buffer* buffer = block->GetOutputPort(block->GetOutputPortName(i));
            if (buffer->IsBusType(buffer->GetDataType())) {
                OutputBusCount++;
            }
        }
    }

    if (sourceCount == 0 || sinkCount == 0) {
        LOG_ERROR("[EventDrivenScheduler] Missing source or sink blocks!");
        return false;
    }

    // 7. Calculate max process count
    int maxProcessCount = calculateMaxProcessCount(blocks, linkKey, sourceCount);

    // 8. Initialize all blocks
    for (Block* block : executionOrder) {
        block->SetDone(false);
    }

    // 9. Set event-driven mode on all blocks
    for (Block* block : executionOrder) {
        block->SetEventDrivenMode(true);
    }

    // 10. Build scheduler context and precompute downstream sets
    SchedulerContext ctx;
    ctx.linkKey = linkKey;
    ctx.executionOrder = executionOrder;
    ctx.sourceCount = sourceCount;
    ctx.sinkCount = sinkCount;
    ctx.OutputBusCount = OutputBusCount;
    ctx.currentIteration = 0;

    precomputeDownstreamSets(ctx);

    for (Block* block : blocks) {
        if (block->GetBlockType() == Block::BlockType::SINK) {
            ctx.sinkProcessCount[block->GetName()] = 0;
        }
    }

    // 11. Scheduling state
    unsigned int nalive = blocks.size();
    unsigned int blocks_count = nalive;
    bool made_progress_last_pass = true;
    bool making_progress = false;
    int iteration = 0;
    int lastProgress = -1;
    int noProgressCount = 0;
    int FinalProgress = 0;
    const int MAX_NO_PROGRESS_ITERATIONS = 1000;

    qDebug() << "[EventDrivenScheduler] Starting data-flow scheduling";
    qDebug() << "  ZeroCross blocks:" << ctx.zeroCrossBlocks.size();
    qDebug() << "  Max process count:" << maxProcessCount;

    // ========== Main scheduling loop ==========
    // Event-driven mode: currentIteration is the authoritative counter.
    // Loop terminates when sinks complete naturally (nalive == 0)
    // OR when iteration budget is exhausted.
    while (nalive > 0 && ctx.currentIteration < (unsigned long long)maxProcessCount) {

        // Check stop request
        if (m_stopRequested && (*m_stopRequested)) {
            LOG_INFO("[EventDrivenScheduler] Stop requested, terminating...");
            break;
        }

        // Check pause state
        if (m_paused && (*m_paused)) {
            QMutexLocker locker(m_pauseMutex);
            LOG_INFO("[EventDrivenScheduler] Paused, waiting...");

            while (m_paused && (*m_paused) && m_stopRequested && !(*m_stopRequested)) {
                m_pauseCond->wait(m_pauseMutex, 1000);
            }

            if (m_stopRequested && (*m_stopRequested)) {
                break;
            }
            LOG_INFO("[EventDrivenScheduler] Resumed");
        }

        iteration++;
        making_progress = false;

        // Increment and broadcast iteration count
        ctx.currentIteration++;
        for (Block* b : executionOrder) {
            b->SetCurrentIteration(ctx.currentIteration);
        }

        // Iterate blocks in topological order
        for (Block* currentBlock : executionOrder) {
            if (currentBlock->IsDone()) {
                continue;
            }

            Block::BlockType type = currentBlock->GetBlockType();
            bool blockDone = false;
            bool progressMade = false;

            // Deadlock detection
            if (noProgressCount / blocks_count > 10) {
                LOG_ERROR("[EventDrivenScheduler] Too many iterations without progress");
                blockDone = true;
            } else {
                if (type == Block::BlockType::SOURCE) {
                    // SOURCE in event-driven mode: runs every iteration to produce data.
                    // Terminate when iteration budget is exhausted or downstream is done.
                    if (ctx.currentIteration >= (unsigned long long)maxProcessCount ||
                            currentBlock->IsDownstreamDone()) {
                        blockDone = true;
                    }

                    bool output_ready = true;
                    for (size_t i = 0; i < currentBlock->GetOutputPortCount(); i++) {
                        std::string portName = currentBlock->GetOutputPortName(i);
                        Buffer* buffer = currentBlock->GetOutputPort(portName);
                        if (buffer->GetReaderCount() != 0) {
                            size_t freeSpace = buffer->GetBufferFreeSpace();
                            if (freeSpace == 0) {
                                output_ready = false;
                                break;
                            }
                        }
                    }

                    if (!output_ready) {
                        continue;
                    }

                    int result = generalWork(currentBlock);
                    if (result > 0) {
                        progressMade = true;
                        making_progress = true;
                    }
                }
                else if (type == Block::BlockType::SINK) {
                    // Set skip flag for sink if downstream of triggered ZeroCross
                    // Sink still runs (to advance time) but checks this flag to skip data output
                    bool shouldSkip = isDownstreamOfTriggeredZeroCross(ctx, currentBlock);
                    currentBlock->SetSkipDataOutput(shouldSkip);
                    // SINK: called every iteration
                    // Check termination conditions
                    int max_items_avail = 0;
                    bool input_done = false;
                    bool all_upstream_done = true;

                    for (size_t i = 0; i < currentBlock->GetInputPortCount(); i++) {
                        std::string portName = currentBlock->GetInputPortName(i);
                        BufferReader* reader = currentBlock->GetInputPort(portName);
                        size_t available = reader->GetAvailableDataCount();

                        if (available < 1) {
                            if (reader->IsUpstreamDone()) {
                                input_done = true;
                            }
                        }

                        if (!reader->IsUpstreamDone()) {
                            all_upstream_done = false;
                        }

                        if (max_items_avail < (int)available) {
                            max_items_avail = available;
                        }
                    }

                    // Check if sink should terminate
                    if ((input_done && all_upstream_done) ||
                            (FinalProgress == 100 && nalive / sinkCount == 1 && OutputBusCount > 0)) {
                        blockDone = true;
                    }
                    else {
                        // Always call sink (even if no data available)
                        // Sink's Run() handles event-driven mode internally
                        int result = generalWork(currentBlock);
                        if (result > 0) {
                            progressMade = true;
                            making_progress = true;
                            // Only count data actually received
                            if (max_items_avail > 0) {
                                BufferReader* reader = currentBlock->GetInputPort(currentBlock->GetInputPortName(0));
                                ctx.sinkProcessCount[currentBlock->GetName()] += reader->GetReadSize();
                            }
                        }
                    }

                    if (blockDone) {
                        currentBlock->SetDone(true);
                        currentBlock->Stop();
                        nalive--;
                        sinkCount--;
                        break;
                    }
                }
                else { // PROCESSOR
                    // KEY CHECK: Skip if downstream of triggered ZeroCross
                    if (isDownstreamOfTriggeredZeroCross(ctx, currentBlock)) {
                        // Consume input data to advance read pointers.
                        // This prevents stale data from being read when the block
                        // resumes execution in the next iteration.
                        // Do NOT ResetBuffer output - that resets read/write pointers
                        // and desynchronizes downstream readers.
                        for (size_t i = 0; i < currentBlock->GetInputPortCount(); i++) {
                            std::string portName = currentBlock->GetInputPortName(i);
                            BufferReader* reader = currentBlock->GetInputPort(portName);
                            if (reader && reader->HasValidConnection() && reader->HasDataAvailable()) {
                                // Read and discard data to advance the read pointer
                                std::vector<double> discard;
                                reader->ReadData(discard);
                            }
                        }
                        qDebug() << "[EventDrivenScheduler] SKIP block:" << QString::fromStdString(currentBlock->GetName())
                                 << "iteration:" << ctx.currentIteration;
                        continue;
                    }

                    // In event-driven mode, processors run every iteration.
                    // The block's Run() handles empty input gracefully.
                    // Skip only if output buffer is full (backpressure).
                    bool output_ready = true;
                    for (size_t i = 0; i < currentBlock->GetOutputPortCount(); i++) {
                        std::string portName = currentBlock->GetOutputPortName(i);
                        Buffer* buffer = currentBlock->GetOutputPort(portName);
                        if (buffer->GetReaderCount() == 0) continue;
                        size_t freeSpace = buffer->GetBufferFreeSpace();
                        if (freeSpace <= 0) {
                            output_ready = false;
                            break;
                        }
                    }

                    if (!output_ready) {
                        continue;
                    }

                    // Check if upstream is done (for blockDone detection)
                    bool allUpstreamDone = true;
                    for (size_t i = 0; i < currentBlock->GetInputPortCount(); i++) {
                        std::string portName = currentBlock->GetInputPortName(i);
                        BufferReader* reader = currentBlock->GetInputPort(portName);
                        if (reader->HasValidConnection() && !reader->IsUpstreamDone()) {
                            allUpstreamDone = false;
                            break;
                        }
                    }

                    if (allUpstreamDone) {
                        blockDone = true;
                    }

                    if (!blockDone) {
                        int result = generalWork(currentBlock);
                        if (result > 0) {
                            progressMade = true;
                            making_progress = true;
                            noProgressCount = 0;
                        }
                    }
                }
            }

            if (blockDone) {
                currentBlock->SetDone(true);
                currentBlock->Stop();
                nalive--;
            }

            if (!progressMade && !blockDone) {
                noProgressCount++;
            }
        }

        // End of pass
        made_progress_last_pass = making_progress;
        making_progress = false;

        // Deadlock detection
        if (!made_progress_last_pass) {
            if (++noProgressCount > MAX_NO_PROGRESS_ITERATIONS) {
                LOG_ERROR("[EventDrivenScheduler] Deadlock detected, forcing exit");
                break;
            }
        } else {
            noProgressCount = 0;
        }

        // Progress update (based on iterations for event-driven mode)
        double iterationProgress = (double)ctx.currentIteration / maxProcessCount * 100.0;
        int currentProgress = static_cast<int>(iterationProgress);
        if (currentProgress > lastProgress) {
            lastProgress = currentProgress;
            if (currentProgress % 10 == 0) {
                LOG_DEBUG("[EventDrivenScheduler] Progress:", currentProgress, "%");
                std::cout << "[PROCESS]" << currentProgress << "%" << std::endl;
                fflush(stdout);
                if (currentProgress >= 100) FinalProgress = 100;
            }
        }
    }

    // Final progress
    int finalProgress = 100;
    if (nalive > 0 && ctx.currentIteration < (unsigned long long)maxProcessCount) {
        // Loop exited abnormally (e.g., deadlock), calculate partial progress
        finalProgress = static_cast<int>((double)ctx.currentIteration / maxProcessCount * 100.0);
    }
    LOG_INFO("[EventDrivenScheduler] Final progress:", finalProgress, "%");
    std::cout << "[PROCESS]" << finalProgress << "%" << std::endl;

    // Cleanup
    for (Block* block : blocks) {
        if (!block->IsDone()) {
            block->SetDone(true);
            block->Stop();
        }
        block->Done();
    }

    qDebug() << "[EventDrivenScheduler] Scheduling complete."
             << "Total iterations:" << ctx.currentIteration;
    return true;
}
