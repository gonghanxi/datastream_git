#include "TimeDrivenScheduler.h"
#include "algorithmmanager.h"
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <iostream>

TimeDrivenScheduler::TimeDrivenScheduler()
    : m_stopSignal(false)
    , m_curStep(0)
{
}

TimeDrivenScheduler::~TimeDrivenScheduler()
{
}

// ========== 调度器核心接口实现 ==========

bool TimeDrivenScheduler::InitializeScheduler(
    const QString& linkKey,
    QVector<Block*> blocks,
    SignalFlowLinkSort* topologySorter)
{
    if (blocks.isEmpty()) {
        LOG_ERROR("[TimeDrivenScheduler] No blocks to initialize!");
        return false;
    }

    // 创建调度器上下文
    SchedulerContext ctx;
    ctx.linkKey = linkKey;
    ctx.state = SchedulerState::INIT;
    ctx.isPaused = false;
    ctx.pendingCommand = Command::NONE;

    // 1. 构建执行顺序
    ctx.executionOrder = buildExecutionOrder(blocks, topologySorter, linkKey);

    qDebug() << "[TimeDrivenScheduler] Initializing for link:" << linkKey;
    qDebug() << "[TimeDrivenScheduler] Execution order size:" << ctx.executionOrder.size();
    qDebug() << "[TimeDrivenScheduler] Execution order:";
    for (Block* block : ctx.executionOrder) {
        qDebug() << "  -" << QString::fromStdString(block->GetName())
                 << "[" << static_cast<int>(block->GetBlockType()) << "]";
    }

    // 2. 解析时间配置
    initializeTimeConfig(ctx);

    // 3. 统计并分类块
    ctx.sourceCount = 0;
    ctx.sinkCount = 0;

    for (Block* block : ctx.executionOrder) {
        Block::BlockType type = block->GetBlockType();

        if (type == Block::BlockType::SOURCE) {
            ctx.sourceCount++;
        } else if (type == Block::BlockType::SINK) {
            ctx.sinkCount++;
        }
    }

    if (ctx.sourceCount == 0 || ctx.sinkCount == 0) {
        LOG_ERROR("[TimeDrivenScheduler] Missing source or sink blocks!");
        return false;
    }

    // 4. 预估总步数
    estimateTotalSteps(ctx);

    // 5. 初始化所有块
    for (Block* block : ctx.executionOrder) {
        block->SetDone(false);
    }

    // 6. 收集SINK输出路径
    collectSinkOutputPaths();

    // 存储调度器上下文
    m_schedulers[linkKey] = ctx;

    qDebug() << "[TimeDrivenScheduler] Initialized successfully";
    qDebug() << "  Source count:" << ctx.sourceCount;
    qDebug() << "  Sink count:" << ctx.sinkCount;
    qDebug() << "  Time range:" << ctx.startTimeUs << "s -" << ctx.stopTimeUs << "s";
    qDebug() << "  Time step:" << ctx.timeStep << "s";
    qDebug() << "  Estimated steps:" << ctx.totalEstimatedSteps;
    qDebug() << "  Drive mode:" << (ctx.isVariableStep ? "Variable" : "Fixed");

    return true;
}

bool TimeDrivenScheduler::RunSimulation(const QString& linkKey,
    QAtomicInt *pausedFlag,
    QAtomicInt *stopRequestedFlag,
    QMutex* pauseMutex,
    QWaitCondition* pauseCond)
{
    if (!m_schedulers.contains(linkKey)) {
        LOG_ERROR("[TimeDrivenScheduler] No scheduler found for link:", linkKey.toStdString());
        return false;
    }

    SchedulerContext& ctx = m_schedulers[linkKey];

    if (ctx.state != SchedulerState::INIT && ctx.state != SchedulerState::PAUSE) {
        LOG_ERROR("[TimeDrivenScheduler] Invalid state for simulation:", static_cast<int>(ctx.state));
        return false;
    }

    // 设置状态为运行
    ctx.state = SchedulerState::RUN;
    ctx.isPaused = false;

    qDebug() << "[TimeDrivenScheduler] Starting simulation for link:" << linkKey;
    qDebug() << "  From" << ctx.currentTime << "s to" << ctx.endTime << "s";

    // ======================================================================
    // 主仿真循环
    // 与 SimpleScheduler 的设计一致：循环内部检测暂停/停止标志
    // ======================================================================
    while (!isSimulationComplete(ctx) && !m_stopSignal)
    {
        // ========== 检查点1: 停止请求检查 ==========
        // 与 SimpleScheduler 中 m_stopRequested 检测逻辑一致
        // 检测到停止标志后立即 break 退出主循环，执行后续清理
        if (stopRequestedFlag && *stopRequestedFlag) {
            LOG_INFO("[TimeDrivenScheduler] 检测到停止命令，正在终止仿真...");
            qDebug() << "[TimeDrivenScheduler] 停止标志已触发，退出调度循环";
            m_stopSignal = true;
            break;
        }

        // ========== 检查点2: 暂停检查 ==========
        // 与 SimpleScheduler 中 m_paused 检测逻辑一致
        // 检测到暂停标志后进入阻塞等待
        if (pausedFlag && *pausedFlag) {
            LOG_INFO("[TimeDrivenScheduler] 仿真已暂停，等待继续或停止命令...");
            qDebug() << "[TimeDrivenScheduler] 进入暂停等待状态";

            ctx.state = SchedulerState::PAUSE;
            ctx.isPaused = true;

            // 暂停前刷新 SINK，保存当前数据
            flushAllSinks(ctx);

            // 阻塞等待，直到被唤醒或超时
            // 与 SimpleScheduler 中的 wait() 逻辑一致
            if (pauseMutex && pauseCond) {
                QMutexLocker locker(pauseMutex);
                while (*pausedFlag && !m_stopSignal) {
                    // 也检查 stopRequestedFlag，防止暂停期间收到停止命令
                    if (stopRequestedFlag && *stopRequestedFlag) {
                        m_stopSignal = true;
                        break;
                    }
                    pauseCond->wait(pauseMutex, 1000);
                }
            }

            // ========== 被唤醒后的处理 ==========
            // 两种情况:
            //   1. 收到继续命令: pausedFlag=0, stopRequestedFlag=0 → 继续循环
            //   2. 收到停止命令: stopRequestedFlag=1 → break 退出
            if (m_stopSignal || (stopRequestedFlag && *stopRequestedFlag)) {
                LOG_INFO("[TimeDrivenScheduler] 暂停期间收到停止命令，终止仿真...");
                qDebug() << "[TimeDrivenScheduler] 暂停期间检测到停止标志，退出调度循环";
                break;
            }

            // 不是停止命令，是继续命令
            ctx.state = SchedulerState::RUN;
            ctx.isPaused = false;
            LOG_INFO("[TimeDrivenScheduler] 仿真继续执行");
            qDebug() << "[TimeDrivenScheduler] 已从暂停状态恢复，继续调度";
        }

        // ========== 检查点3: 待处理命令 ==========
        if (ctx.pendingCommand != Command::NONE) {
            applyCommand(ctx, ctx.pendingCommand);
            ctx.pendingCommand = Command::NONE;

            if (ctx.state == SchedulerState::STOP || ctx.isPaused) {
                break;
            }
        }

        // ========== 变步长：动态计算步长 ==========
        if (ctx.isVariableStep) {
            double newStep = calculateNextTimeStep(ctx);
            if (!validateTimeStep(newStep, ctx)) {
                LOG_ERROR("[TimeDrivenScheduler] 无效步长");
                break;
            }
            ctx.timeStep = newStep;

            qDebug() << "[TimeDrivenScheduler] 变步长:" << ctx.timeStep
                     << "时间:" << ctx.currentTime;
        }

        // ========== 处理当前时间步 ==========
        if (!processTimeStepForContext(ctx)) {
            LOG_ERROR("[TimeDrivenScheduler] Failed to process time step at t =", ctx.currentTime);
            break;
        }

        // ========== 更新进度 ==========
        updateProgress(ctx);

        // ========== 进度输出（每10%输出一次） ==========
        if (ctx.totalEstimatedSteps > 0) {
            int currentDecile = (ctx.currentProgress / 10) * 10;
            static int lastDecile = -1;
            if (currentDecile > lastDecile) {
                lastDecile = currentDecile;
                qDebug() << "[Progress]" << ctx.currentProgress << "% - Time:"
                         << ctx.currentTime << "s /" << ctx.endTime << "s";
                std::cout << "[PROCESS]" << ctx.currentProgress << "%" << std::endl;
                fflush(stdout);
            }
        }
    }

    // ======================================================================
    // 仿真结束处理
    // ======================================================================
    if (ctx.isPaused) {
        qDebug() << "[TimeDrivenScheduler] 仿真暂停于" << ctx.currentTime << "s";
    } else if (m_stopSignal || (stopRequestedFlag && *stopRequestedFlag)) {
        qDebug() << "[TimeDrivenScheduler] 仿真被停止于" << ctx.currentTime << "s";
    } else {
        qDebug() << "[TimeDrivenScheduler] 仿真完成于" << ctx.currentTime << "s";
    }

    // 最终刷新 SINK
    flushAllSinks(ctx);

    // 最终进度
    updateProgress(ctx);
    LOG_INFO("最终进度:" , ctx.currentProgress, "%");
    std::cout << "[PROCESS]" << ctx.currentProgress << "%" << std::endl;
    fflush(stdout);

    // 最终Done所有模型
    DoneAllModels(ctx);

    return true;
}

bool TimeDrivenScheduler::ProcessOneTimeStep(const QString& linkKey)
{
    if (!m_schedulers.contains(linkKey)) {
        qDebug() << "[TimeDrivenScheduler] No scheduler found for link:" << linkKey;
        return false;
    }

    SchedulerContext& ctx = m_schedulers[linkKey];

    if (ctx.state != SchedulerState::RUN) {
        qDebug() << "[TimeDrivenScheduler] Scheduler not running, state:" << static_cast<int>(ctx.state);
        return false;
    }

    if (isSimulationComplete(ctx)) {
        LOG_INFO("[TimeDrivenScheduler] Simulation already complete");
        return false;
    }

    // 变步长模式：重新计算步长
    if (ctx.isVariableStep) {
        ctx.timeStep = calculateNextTimeStep(ctx);
        if (!validateTimeStep(ctx.timeStep, ctx)) {
            return false;
        }
    }

    return processTimeStepForContext(ctx);
}

void TimeDrivenScheduler::SendCommand(Command cmd, const QString& linkKey)
{
    qDebug() << "[TimeDrivenScheduler] SendCommand:" << static_cast<int>(cmd);

    if (!linkKey.isEmpty()) {
        if (m_schedulers.contains(linkKey)) {
            applyCommand(m_schedulers[linkKey], cmd);
        } else {
            qDebug() << "[TimeDrivenScheduler] No scheduler for link:" << linkKey;
        }
    } else {
        // 向所有链路广播命令
        QMapIterator<QString, SchedulerContext> it(m_schedulers);
        while (it.hasNext()) {
            it.next();
            applyCommand(m_schedulers[it.key()], cmd);
        }
    }
}

// ========== 状态查询接口实现 ==========

TimeDrivenScheduler::SchedulerState TimeDrivenScheduler::GetSchedulerState(const QString& linkKey) const
{
    if (m_schedulers.contains(linkKey)) {
        return m_schedulers[linkKey].state;
    }
    return SchedulerState::NONE;
}

double TimeDrivenScheduler::GetCurrentProgress(const QString& linkKey) const
{
    if (m_schedulers.contains(linkKey)) {
        return m_schedulers[linkKey].currentProgress;
    }
    return 0.0;
}

double TimeDrivenScheduler::GetCurrentSimulationTime(const QString& linkKey) const
{
    if (m_schedulers.contains(linkKey)) {
        return m_schedulers[linkKey].currentTime;
    }
    return 0.0;
}

int TimeDrivenScheduler::GetCurrentStep(const QString& linkKey) const
{
    if (m_schedulers.contains(linkKey)) {
        return m_schedulers[linkKey].currentStep;
    }
    return -1;
}

double TimeDrivenScheduler::GetCurrentTimeStep(const QString& linkKey) const
{
    if (m_schedulers.contains(linkKey)) {
        return m_schedulers[linkKey].timeStep;
    }
    return 0.0;
}

bool TimeDrivenScheduler::IsSimulationComplete(const QString& linkKey) const
{
    if (!m_schedulers.contains(linkKey)) {
        return true;
    }
    return isSimulationComplete(m_schedulers[linkKey]);
}

bool TimeDrivenScheduler::HasScheduler(const QString& linkKey) const
{
    return m_schedulers.contains(linkKey);
}

// ========== 配置接口实现 ==========

void TimeDrivenScheduler::SetFlushStrategy(const QString& linkKey, FlushStrategy strategy, int interval)
{
    if (m_schedulers.contains(linkKey)) {
        m_schedulers[linkKey].flushStrategy = strategy;
        m_schedulers[linkKey].flushInterval = interval;
    }
}

void TimeDrivenScheduler::SetTimingStrategy(const QString& linkKey, TimingStrategy strategy)
{
    if (m_schedulers.contains(linkKey)) {
        m_schedulers[linkKey].timingStrategy = strategy;
    }
}

void TimeDrivenScheduler::SetProgressStrategy(const QString& linkKey, ProgressStrategy strategy)
{
    if (m_schedulers.contains(linkKey)) {
        m_schedulers[linkKey].progressStrategy = strategy;
    }
}

void TimeDrivenScheduler::SetDriveMode(const QString& linkKey, DriveMode mode)
{
    if (m_schedulers.contains(linkKey)) {
        m_schedulers[linkKey].driveMode = mode;
        m_schedulers[linkKey].isVariableStep = (mode == DriveMode::VARIABLE_STEP);
    }
}

void TimeDrivenScheduler::SetStepBounds(const QString& linkKey, double minStep, double maxStep)
{
    if (m_schedulers.contains(linkKey)) {
        m_schedulers[linkKey].minTimeStep = minStep;
        m_schedulers[linkKey].maxTimeStep = maxStep;
    }
}

void TimeDrivenScheduler::SetEndTime(const QString& linkKey, double endTime)
{
    if (m_schedulers.contains(linkKey)) {
        m_schedulers[linkKey].endTime = endTime;
        estimateTotalSteps(m_schedulers[linkKey]);
    }
}

void TimeDrivenScheduler::SetStopSignal(bool stopSignal)
{
    m_stopSignal = stopSignal;

    if (stopSignal) {
        // 向所有调度器发送停止命令
        QMapIterator<QString, SchedulerContext> it(m_schedulers);
        while (it.hasNext()) {
            it.next();
            SchedulerContext& ctx = m_schedulers[it.key()];
            if (ctx.state != SchedulerState::STOP) {
                SendCommand(Command::STOP, it.key());
            }
        }
    }
}

bool TimeDrivenScheduler::GetStopSignal() const
{
    return m_stopSignal;
}

void TimeDrivenScheduler::SetCurStep(int curStep)
{
    m_curStep = curStep;
}

int TimeDrivenScheduler::GetCurStep() const
{
    return m_curStep;
}

// ========== 核心私有方法实现 ==========

QVector<Block*> TimeDrivenScheduler::buildExecutionOrder(
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

bool TimeDrivenScheduler::processTimeStepForContext(SchedulerContext& ctx)
{
    ctx.currentStep++;
    ctx.totalStepsExecuted++;

    qDebug() << "[TimeDrivenScheduler] Processing step" << ctx.currentStep
             << "at time" << ctx.currentTime << "s"
             << "with step size" << ctx.timeStep << "s";

    // 在执行前保存当前时间
    double previousTime = ctx.currentTime;
    
    // 向所有模型广播当前时间
    notifyCurrentTime(ctx);

    // 2. 构建执行掩码（唯一一次）
    std::vector<bool> executionMask;
    if (ctx.isVariableStep) {
        executionMask = buildExecutionMask(ctx);
    } else {
        executionMask.assign(ctx.executionOrder.size(), true);
    }

    // 3. 执行本时间步，使用构建好的掩码
    bool success = executeOneTimeStep(ctx, executionMask);

    if (success) {
        // 更新时间
        ctx.currentTime += ctx.timeStep;
        
        // 确保时间单调性
        assertTimeMonotonic(ctx, ctx.currentTime);

        // 检查是否需要刷新SINK(执行Flush)
        if (shouldFlushSink(ctx)) {
            flushAllSinks(ctx);
        }

        ctx.stepsSinceLastFlush++;
        ctx.dataPointsSinceLastFlush++;
    } else {
        qDebug() << "[TimeDrivenScheduler] Failed to execute time step at" << ctx.currentTime << "s";
    }

    return success;
}

bool TimeDrivenScheduler::executeOneTimeStep(SchedulerContext& ctx,
                                              const std::vector<bool>& executionMask)
{
    for (size_t i = 0; i < ctx.executionOrder.size(); i++) {
        Block* block = ctx.executionOrder[i];

        if (block->IsDone()) {
            continue;
        }

        // 使用外部传入的掩码
        if (!executionMask[i]) {
            qDebug() << "[TimeDrivenScheduler] 跳过模型:"
                     << QString::fromStdString(block->GetName());
            continue;
        }

        bool success = false;
        switch (block->GetBlockType()) {
        case Block::BlockType::SOURCE:
            success = processSourceInTimeStep(ctx, block);
            break;
        case Block::BlockType::PROCESSOR:
            success = processProcessorInTimeStep(ctx, block);
            break;
        case Block::BlockType::SINK:
            success = processSinkInTimeStep(ctx, block);
            break;
        default:
            continue;
        }

        if (!success) {
            LOG_ERROR("[TimeDrivenScheduler] 模型执行失败:", block->GetName(), "时间:", ctx.currentTime);
            return false;
        }
    }
    return true;
}

void TimeDrivenScheduler::notifyCurrentTime(SchedulerContext& ctx)
{
    for (Block* block : ctx.executionOrder) {
        block->SetCurrentTime(ctx.currentTime);

//        if (ctx.isVariableStep) {
//            // 变步长模式下更新下次执行时间
//            double desiredStep = block->GetDesiredTimeStep();
//            if (desiredStep > 0 && block->ShouldExecuteAt(ctx.currentTime)) {
//                block->SetNextExecutionTime(ctx.currentTime + desiredStep);
//            } else if (desiredStep <= 0) {
//                // 每步执行的模型
//                block->SetNextExecutionTime(ctx.currentTime + ctx.timeStep);
//            }
//        }
    }

    qDebug() << "[TimeDrivenScheduler] 时间通知完成: t=" << ctx.currentTime;
}

// ========== 模型处理方法实现 ==========

bool TimeDrivenScheduler::processSourceInTimeStep(SchedulerContext& ctx, Block* block)
{
    qDebug() << "[TimeDrivenScheduler] Processing SOURCE:" 
             << QString::fromStdString(block->GetName())
             << "at time" << ctx.currentTime;

    // 时间驱动下，SOURCE在预定时间点必须产生数据
    // 不再检查输出缓冲区空间
    
    bool result = block->Run();
    
    if (!result) {
        qDebug() << "[TimeDrivenScheduler] SOURCE" << QString::fromStdString(block->GetName())
                 << "failed to produce data at time" << ctx.currentTime;
    }

    return result;
}

bool TimeDrivenScheduler::processProcessorInTimeStep(SchedulerContext& ctx, Block* block)
{
    qDebug() << "[TimeDrivenScheduler] Processing PROCESSOR:" 
             << QString::fromStdString(block->GetName())
             << "at time" << ctx.currentTime;

    // 检查上游模型是否在本步执行了
    bool upstreamExecuted = true;
    for (size_t i = 0; i < block->GetInputPortCount(); i++) {
        std::string portName = block->GetInputPortName(i);
        BufferReader* reader = block->GetInputPort(portName);

        // 查找连接的上游模型
        // 这里需要根据实际连接关系判断上游是否执行
        // 简化处理：检查缓冲区是否有新数据
        if (reader->HasValidConnection() && !reader->HasDataAvailable()) {
            upstreamExecuted = false;
            break;
        }
    }

    // 处理时序不匹配
    if (!upstreamExecuted) {
        resolveTimingMismatch(ctx, block);
    }
    
    bool result = block->Run();
    
    if (!result) {
        qDebug() << "[TimeDrivenScheduler] PROCESSOR" << QString::fromStdString(block->GetName())
                 << "failed at time" << ctx.currentTime;
    }

    return result;
}

bool TimeDrivenScheduler::processSinkInTimeStep(SchedulerContext& ctx, Block* block)
{
    qDebug() << "[TimeDrivenScheduler] Processing SINK:" 
             << QString::fromStdString(block->GetName())
             << "at time" << ctx.currentTime;

    // 时间驱动下，SINK每步都收集数据
    // 不检查数据可用性，数据应该由前面的PROCESSOR准备好
    
    bool result = block->Run();
    
    if (result) {
        ctx.totalDataPointsProcessed++;
        ctx.dataPointsSinceLastFlush++;
        
        qDebug() << "[TimeDrivenScheduler] SINK" << QString::fromStdString(block->GetName())
                 << "collected data at time" << ctx.currentTime
                 << "total points:" << ctx.totalDataPointsProcessed;
    } else {
        qDebug() << "[TimeDrivenScheduler] SINK" << QString::fromStdString(block->GetName())
                  << "failed to collect data at time" << ctx.currentTime;
    }

    return result;
}

// ========== 辅助方法实现 ==========

void TimeDrivenScheduler::initializeTimeConfig(SchedulerContext& ctx)
{
    // 从仿真参数获取基础配置
    SimuParameter simParam = AlgorithmManager::createInstance()
        ->getSimuParameters().value(ctx.linkKey);

    ctx.samplingRateUs = simParam.samplingRate;
    ctx.startTimeUs = simParam.startTime;
    ctx.stopTimeUs = simParam.stopTime;

    ctx.currentTime = ctx.startTimeUs;
    ctx.endTime = ctx.stopTimeUs;

    // 计算基础步长
    if (ctx.samplingRateUs > 0) {
        ctx.timeStep = 1.0 / ctx.samplingRateUs;
    } else {
        ctx.timeStep = 0.001;
    }

    // ========== 新增：自动检测是否需要变步长 ==========
    bool hasVariableRateModel = false;
    double minRequestedStep = ctx.timeStep;
    double maxRequestedStep = ctx.timeStep;

    for (Block* block : ctx.executionOrder) {
        double desiredStep = block->GetDesiredTimeStep();

        if (desiredStep > 0) {
            hasVariableRateModel = true;

            // 更新最小/最大步长
            if (desiredStep < minRequestedStep) {
                minRequestedStep = desiredStep;
            }
            if (desiredStep > maxRequestedStep) {
                maxRequestedStep = desiredStep;
            }

            // 设置模型的采样周期
            block->SetSamplePeriod(desiredStep);

            qDebug() << "[TimeDrivenScheduler] 检测到变采样模型:"
                     << QString::fromStdString(block->GetName())
                     << "期望步长:" << desiredStep
                     << "抽取因子:" << block->GetDecimationFactor();
        }
    }

    // 设置变步长模式
    ctx.isVariableStep = hasVariableRateModel;

    if (ctx.isVariableStep) {
        // 计算步长范围
        ctx.minTimeStep = std::min(ctx.timeStep, minRequestedStep);
        ctx.maxTimeStep = std::max(ctx.timeStep, maxRequestedStep);

        // 确保合理范围
        if (ctx.minTimeStep < 1e-9) ctx.minTimeStep = 1e-9;
        if (ctx.maxTimeStep > ctx.endTime - ctx.startTimeUs) {
            ctx.maxTimeStep = ctx.endTime - ctx.startTimeUs;
        }

        // 设置时序不匹配策略
        ctx.timingStrategy = TimingStrategy::HOLD_LAST;

        // 调整刷新策略
        ctx.flushStrategy = FlushStrategy::BY_TIME;
        ctx.flushTimeInterval = ctx.timeStep * 100;  // 每100个基础步长刷新

        qDebug() << "[TimeDrivenScheduler] 启用变步长模式";
        qDebug() << "  基础步长:" << ctx.timeStep;
        qDebug() << "  最小步长:" << ctx.minTimeStep;
        qDebug() << "  最大步长:" << ctx.maxTimeStep;
    } else {
        qDebug() << "[TimeDrivenScheduler] 使用固定步长模式";
    }

    // 初始化所有模型的执行时间
    for (Block* block : ctx.executionOrder) {
        block->SetNextExecutionTime(ctx.currentTime);
        block->SetCurrentTime(ctx.currentTime);
    }

    estimateTotalSteps(ctx);
}

bool TimeDrivenScheduler::isSimulationComplete(const SchedulerContext& ctx) const
{
    return ctx.currentTime >= ctx.endTime;
}

bool TimeDrivenScheduler::shouldFlushSink(const SchedulerContext& ctx)
{
    switch (ctx.flushStrategy) {
    case FlushStrategy::BY_STEP_COUNT:
        return ctx.stepsSinceLastFlush >= ctx.flushInterval;

    case FlushStrategy::BY_TIME:
        return shouldFlushByTimeElapsed(ctx);

    case FlushStrategy::BY_DATA_COUNT:
        return shouldFlushByDataCount(ctx);

    default:
        return false;
    }
}

bool TimeDrivenScheduler::shouldFlushByTimeElapsed(const SchedulerContext& ctx)
{
    return (ctx.currentTime - ctx.lastFlushTime) >= ctx.flushTimeInterval;
}

bool TimeDrivenScheduler::shouldFlushByDataCount(const SchedulerContext& ctx)
{
    return ctx.dataPointsSinceLastFlush >= ctx.flushInterval;
}

void TimeDrivenScheduler::flushSinkBlock(Block* block)
{
    if (block && block->GetBlockType() == Block::BlockType::SINK) {
        qDebug() << "[TimeDrivenScheduler] Flushing SINK:" 
                 << QString::fromStdString(block->GetName());
        block->Flush();
    }
}

void TimeDrivenScheduler::flushAllSinks(SchedulerContext& ctx)
{
    qDebug() << "[TimeDrivenScheduler] Flushing all SINKs at time" << ctx.currentTime;
    
    for (Block* block : ctx.executionOrder) {
        if (block->GetBlockType() == Block::BlockType::SINK && !block->IsDone()) {
            flushSinkBlock(block);
        }
    }
    
    // 重置刷新计数器
    ctx.stepsSinceLastFlush = 0;
    ctx.dataPointsSinceLastFlush = 0;
    ctx.lastFlushTime = ctx.currentTime;
}

void TimeDrivenScheduler::DoneAllModels(TimeDrivenScheduler::SchedulerContext &ctx)
{
    qDebug() << "[TimeDrivenScheduler] Done all SINKs at time" << ctx.currentTime;

    for (Block* block : ctx.executionOrder) {
        if (!block->IsDone()) {
            block->SetDone(true);
            block->Stop();
            block->Done();
            qDebug() << "[TimeDrivenScheduler] Block"
                     << QString::fromStdString(block->GetName()) << "stopped";
        }
    }
}

void TimeDrivenScheduler::applyCommand(SchedulerContext& ctx, Command cmd)
{
    qDebug() << "[TimeDrivenScheduler] Applying command" << static_cast<int>(cmd)
             << "to link:" << ctx.linkKey
             << "current state:" << static_cast<int>(ctx.state);

    switch (cmd) {
    case Command::START:
        if (ctx.state == SchedulerState::INIT || ctx.state == SchedulerState::PAUSE) {
            ctx.state = SchedulerState::RUN;
            ctx.isPaused = false;
            qDebug() << "[TimeDrivenScheduler] Started for link:" << ctx.linkKey;
        } else {
            qDebug() << "[TimeDrivenScheduler] Cannot start in state:" << static_cast<int>(ctx.state);
        }
        break;

    case Command::PAUSE:
        if (ctx.state == SchedulerState::RUN) {
            ctx.state = SchedulerState::PAUSE;
            ctx.isPaused = true;
            qDebug() << "[TimeDrivenScheduler] Paused at time" << ctx.currentTime
                     << "s, step" << ctx.currentStep;
            
            // 暂停时刷新SINK，保存当前数据
            flushAllSinks(ctx);
        }
        break;

    case Command::STOP:
        if (ctx.state == SchedulerState::STOP) {
            LOG_INFO("[TimeDrivenScheduler] Already stopped");
            return;
        }
        
        ctx.state = SchedulerState::STOP;
        ctx.isPaused = false;
        
        qDebug() << "[TimeDrivenScheduler] Stopped at time" << ctx.currentTime
                 << "s, step" << ctx.currentStep
                 << "data points:" << ctx.totalDataPointsProcessed;
        
        // 停止前刷新所有SINK
        flushAllSinks(ctx);
        
        // 停止所有活跃的块
        for (Block* block : ctx.executionOrder) {
            if (!block->IsDone()) {
                block->SetDone(true);
                block->Stop();
                block->Done();
                qDebug() << "[TimeDrivenScheduler] Block" 
                         << QString::fromStdString(block->GetName()) << "stopped";
            }
        }
        
        updateProgress(ctx);
        break;

    case Command::RESET:
        if (ctx.state == SchedulerState::PAUSE) {
            ctx.state = SchedulerState::RUN;
            ctx.isPaused = false;
            qDebug() << "[TimeDrivenScheduler] Reset - Continuing simulation at time"
                     << ctx.currentTime << "s";
        } else {
            qDebug() << "[TimeDrivenScheduler] Reset ignored - invalid state:"
                     << static_cast<int>(ctx.state);
        }
        break;

    case Command::NONE:
    default:
        break;
    }
}

void TimeDrivenScheduler::updateProgress(SchedulerContext& ctx)
{
    switch (ctx.progressStrategy) {
    case ProgressStrategy::BY_TIME:
        if (ctx.endTime > ctx.startTimeUs) {
            double elapsed = ctx.currentTime - ctx.startTimeUs;
            double total = ctx.endTime - ctx.startTimeUs;
            ctx.currentProgress = static_cast<int>((elapsed / total) * 100.0);
        } else {
            ctx.currentProgress = 100;
        }
        break;

    case ProgressStrategy::BY_STEPS:
        if (ctx.totalEstimatedSteps > 0) {
            ctx.currentProgress = (ctx.currentStep * 100) / ctx.totalEstimatedSteps;
        } else {
            ctx.currentProgress = 100;
        }
        break;

    default:
        ctx.currentProgress = 100;
        break;
    }

    // 限制进度在0-100
    if (ctx.currentProgress > 100) {
        ctx.currentProgress = 100;
    }
    if (ctx.currentProgress < 0) {
        ctx.currentProgress = 0;
    }
}

void TimeDrivenScheduler::resetSchedulerContext(SchedulerContext& ctx)
{
    ctx.currentTime = ctx.startTimeUs;
    ctx.currentStep = 0;
    ctx.currentProgress = 0;
    ctx.totalStepsExecuted = 0;
    ctx.totalStepsSkipped = 0;
    ctx.totalDataPointsProcessed = 0;
    ctx.stepsSinceLastFlush = 0;
    ctx.dataPointsSinceLastFlush = 0;
    ctx.lastFlushTime = ctx.startTimeUs;
    ctx.isPaused = false;
    ctx.pendingCommand = Command::NONE;

    // 重新初始化块
    for (Block* block : ctx.executionOrder) {
        block->SetDone(false);
    }

    qDebug() << "[TimeDrivenScheduler] Context reset for link:" << ctx.linkKey;
}

bool TimeDrivenScheduler::checkBlockCompletion(SchedulerContext& ctx, Block* block)
{
    // 时间驱动下，完成状态基于当前时间判断
    if (isSimulationComplete(ctx)) {
        return true;
    }

    Block::BlockType type = block->GetBlockType();

    if (type == Block::BlockType::SOURCE) {
        // SOURCE在时间到达后完成
        return ctx.currentTime >= ctx.endTime;
    }

    if (type == Block::BlockType::PROCESSOR) {
        // PROCESSOR在时间到达且上游完成时完成
        return ctx.currentTime >= ctx.endTime;
    }

    if (type == Block::BlockType::SINK) {
        // SINK在仿真完成时完成
        return ctx.currentTime >= ctx.endTime;
    }

    return false;
}

void TimeDrivenScheduler::collectSinkOutputPaths()
{
    // TODO: 收集SINK输出路径用于DDS上传
    m_sinkOutputPaths.clear();
}

// ========== 变步长相关方法实现（预留） ==========

double TimeDrivenScheduler::calculateNextTimeStep(SchedulerContext& ctx)
{
    if (!ctx.isVariableStep) {
        return ctx.timeStep;  // 固定步长直接返回
    }

    // 收集所有模型的时间需求
    std::map<Block*, double> requests;
    collectBlockTimeRequests(ctx, requests);

    if (requests.empty()) {
        return ctx.timeStep;  // 无特殊需求，使用基础步长
    }

    // 找到最小的请求步长（满足最快的模型）
    double nextStep = ctx.maxTimeStep;

    for (const auto& pair : requests) {
        if (pair.second > 0 && pair.second < nextStep) {
            nextStep = pair.second;
        }
    }

    // 确保在范围内
    if (nextStep < ctx.minTimeStep) {
        nextStep = ctx.minTimeStep;
    }
    if (nextStep > ctx.maxTimeStep) {
        nextStep = ctx.maxTimeStep;
    }

    // 也要考虑基础步长的模型（它们需要每步执行）
    // 不能让步长大于基础步长太多
    double maxStepForContinuous = ctx.timeStep * 2.0;  // 最多两倍基础步长
    if (nextStep > maxStepForContinuous && !ctx.executionOrder.isEmpty()) {
        nextStep = maxStepForContinuous;
    }

    // ----- 修复：事件对齐检测，绕过平滑 -----
    bool isAlignmentStep = false;
    for (const auto& pair : requests) {
        // 如果候选步长正好等于某个模型的请求（即对齐到该模型的执行点）
        if (std::abs(pair.second - nextStep) < 1e-12 && nextStep > 0) {
            isAlignmentStep = true;
            break;
        }
    }

    if (isAlignmentStep && nextStep >= ctx.minTimeStep) {
        // 对齐事件的步长不能被平滑截断，直接返回
        qDebug() << "[TimeDrivenScheduler] 对齐事件步长，跳过平滑:" << nextStep;
        return nextStep;
    }

    // 非对齐步长，进行稳定化平滑
    nextStep = stabilizeTimeStep(ctx, nextStep);

    return nextStep;
}

void TimeDrivenScheduler::collectBlockTimeRequests(
    SchedulerContext& ctx,
    std::map<Block*, double>& requests)
{
    for (Block* block : ctx.executionOrder) {
        if (block->IsDone()) {
            continue;
        }

        double desiredStep = block->GetDesiredTimeStep();

        if (desiredStep > 0) {
            // 计算距离下次执行还需要的时间
            double timeToNextExec = block->GetNextExecutionTime() - ctx.currentTime;

            if (timeToNextExec > 0) {
                // 还没到执行时间，请求步长至少要到执行时间
                requests[block] = timeToNextExec;
            } else {
                // 已经到了或过了执行时间，尽快执行
                requests[block] = ctx.minTimeStep;
            }
        }
    }

    qDebug() << "[TimeDrivenScheduler] 收集到" << requests.size() << "个步长请求";
    for (const auto& pair : requests) {
        qDebug() << "  模型:" << QString::fromStdString(pair.first->GetName())
                 << "请求步长:" << pair.second;
    }
}

std::vector<bool> TimeDrivenScheduler::buildExecutionMask(SchedulerContext& ctx)
{
    std::vector<bool> mask(ctx.executionOrder.size(), false);

    if (!ctx.isVariableStep) {
        // 固定步长：所有模型都执行
        std::fill(mask.begin(), mask.end(), true);
        return mask;
    }

    // 变步长：检查每个模型是否需要执行
    for (size_t i = 0; i < ctx.executionOrder.size(); i++) {
        Block* block = ctx.executionOrder[i];

        if (block->IsDone()) {
            continue;
        }

        // 检查是否到达模型的执行时间
        bool shouldExec = block->ShouldExecuteAt(ctx.currentTime);

        if (shouldExec) {
            mask[i] = true;

            // 更新下次执行时间
            double desiredStep = block->GetDesiredTimeStep();
            if (desiredStep > 0) {
                double nextExec = ctx.currentTime + desiredStep;
                block->SetNextExecutionTime(nextExec);
                ctx.blockNextExecTime[block] = nextExec;
            } else {
                // 每步都执行的模型
                block->SetNextExecutionTime(ctx.currentTime + ctx.timeStep);
            }
        }

        qDebug() << "[TimeDrivenScheduler] 模型:"
                 << QString::fromStdString(block->GetName())
                 << (shouldExec ? "执行" : "跳过")
                 << "下次执行时间:" << block->GetNextExecutionTime();
    }

    return mask;
}

void TimeDrivenScheduler::resolveTimingMismatch(
    SchedulerContext& ctx, Block* block)
{
    switch (ctx.timingStrategy) {
    case TimingStrategy::HOLD_LAST:
        // 保持上一个值 - 模型内部缓冲区不刷新即可
        qDebug() << "[TimeDrivenScheduler] HOLD_LAST: 模型"
                 << QString::fromStdString(block->GetName())
                 << "使用上次数据";
        break;

    case TimingStrategy::ZERO_FILL:
        // 零值填充 - 向输入缓冲区写入0
        qDebug() << "[TimeDrivenScheduler] ZERO_FILL: 模型"
                 << QString::fromStdString(block->GetName())
                 << "使用零值";
        // 这里可以向模型的输入端口写入0
        break;

    case TimingStrategy::INTERPOLATE:
        // 插值（预留）
        qDebug() << "[TimeDrivenScheduler] INTERPOLATE: 模型"
                 << QString::fromStdString(block->GetName())
                 << "使用插值";
        break;
    }
}

double TimeDrivenScheduler::stabilizeTimeStep(SchedulerContext& ctx, double candidateStep)
{
    // 防止步长剧烈变化
    double lastStep = ctx.timeStep;

    if (lastStep <= 0) {
        return candidateStep;
    }

    // 限制变化率：不超过上次的2倍，不小于上次的0.5倍
    double maxChange = lastStep * 2.0;
    double minChange = lastStep * 0.5;

    double stabilizedStep = candidateStep;

    if (stabilizedStep > maxChange) {
        stabilizedStep = maxChange;
        qDebug() << "[TimeDrivenScheduler] 步长被限制（上限）:"
                 << candidateStep << "→" << stabilizedStep;
    }

    if (stabilizedStep < minChange) {
        stabilizedStep = minChange;
        qDebug() << "[TimeDrivenScheduler] 步长被限制（下限）:"
                 << candidateStep << "→" << stabilizedStep;
    }

    // 可选：滑动平均平滑
    // stabilizedStep = lastStep * 0.7 + candidateStep * 0.3;

    return stabilizedStep;
}

bool TimeDrivenScheduler::validateTimeStep(double step, const SchedulerContext& ctx)
{
    if (step <= 0) {
        qDebug() << "[TimeDrivenScheduler] Invalid time step:" << step;
        return false;
    }

    if (step < ctx.minTimeStep) {
        qDebug() << "[TimeDrivenScheduler] Time step" << step << "below minimum" << ctx.minTimeStep;
        return false;
    }

    if (step > ctx.maxTimeStep) {
        qDebug() << "[TimeDrivenScheduler] Time step" << step << "above maximum" << ctx.maxTimeStep;
        return false;
    }

    return true;
}

void TimeDrivenScheduler::assertTimeMonotonic(const SchedulerContext& ctx, double newTime)
{
    if (newTime < ctx.currentTime) {
        qDebug() << "[TimeDrivenScheduler] Time monotonicity violation:"
                 << "current" << ctx.currentTime << "-> new" << newTime;
    }
}

void TimeDrivenScheduler::estimateTotalSteps(SchedulerContext& ctx)
{
    double totalTime = ctx.endTime - ctx.startTimeUs;
    
    if (ctx.timeStep > 0) {
        ctx.totalEstimatedSteps = static_cast<int>(std::ceil(totalTime / ctx.timeStep));
    } else {
        ctx.totalEstimatedSteps = 0;
    }
    
    qDebug() << "[TimeDrivenScheduler] Estimated total steps:" << ctx.totalEstimatedSteps;
}
