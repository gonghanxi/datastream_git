#include "ReadyQueueScheduler.h"
#include <QDebug>
#include <QThread>
#include <algorithm>
#include "../Common/LogExport.h"

ReadyQueueScheduler::ReadyQueueScheduler() {
}

ReadyQueueScheduler::~ReadyQueueScheduler() {
}

bool ReadyQueueScheduler::schedule(const QString& linkKey,
                                    QVector<Block*>& blocks,
                                    std::shared_ptr<DataStreamVerification> verificationSystem,
                                    const SimuParameter& simuParams) {

    if (!verificationSystem) {
        LOG_ERROR("Verification system not initialized!");
        return false;
    }

    m_verificationSystem = verificationSystem;
    m_totalSamples = simuParams.num_Samples;
    m_timeInterval = simuParams.time_Interval;

    // 1. 初始化状态和就绪队列
    initBlockStates(blocks);
    initReadyQueues(blocks);
    calculateRequiredInputCounts();

    // 2. 验证可行性
    if (!m_verificationSystem->CheckFeasibility()) {
        LOG_ERROR("数据一致性校验失败!");
        return false;
    }

    // 3. 统计信号源和收集器数量
    int sourceCount = 0, sinkCount = 0;
    for (auto block : blocks) {
        if (block->GetBlockType() == Block::BlockType::SOURCE) sourceCount++;
        else if (block->GetBlockType() == Block::BlockType::SINK) sinkCount++;
    }

    if (sourceCount == 0 || sinkCount == 0) {
        LOG_ERROR("链路：", linkKey.toStdString(), "，缺少必要的信号源或收集器");
        return false;
    }

    // 4. 设置 Buffer 容量限制
    for (auto block : blocks) {
        for (auto& port : block->GetOutputPorts()) {
            Buffer* buffer = port.second;
            if (buffer) {
                buffer->SetMaxSize(DEFAULT_BUFFER_SIZE);
            }
        }
    }

    // 5. 计算 Sink 目标收集数
    calculateSinkTargets(blocks);
    int totalTargetCollect = 0;
    for (auto& pair : m_sinkTargetCounts) {
        totalTargetCollect += pair.second;
    }

    // 6. 初始化 Sink 计数
    m_sinkProcessCount.clear();
    for (auto block : blocks) {
        if (block->GetBlockType() == Block::BlockType::SINK) {
            m_sinkProcessCount[block->GetName()] = 0;
        }
    }

    // 7. 主调度循环（基于 Sink 收集计数）
    int totalCollected = 0;
    int noProgressCount = 0;
    const int MAX_NO_PROGRESS = 1000;
    bool allSinksDone = false;

//    LOG_INFO("[调度]开始执行，Sink目标总数:", totalTargetCollect);

    while (!allSinksDone && totalCollected < totalTargetCollect) {
        bool madeProgress = false;
        static int cycleCount = 0;  // 添加循环计数
        cycleCount++;

        // 每 500 个周期打印一次状态
        if (cycleCount == 1 || cycleCount == 2 || cycleCount % 1000 == 0 || cycleCount == 4 || cycleCount == 5
                || cycleCount == 6 || cycleCount == 7 || cycleCount == 8 || cycleCount == 9 || cycleCount == 10) {
            qDebug() <<"=== Scheduler State at cycle" << cycleCount << "===";
            qDebug() <<"Total collected:" << totalCollected << "/" << totalTargetCollect;
            qDebug() <<"Ready sources:" << m_readySources.size();
            qDebug() <<"Ready processors:" << m_readyByPriority.values();
            qDebug() <<"Ready sinks:" << m_readySinks.size();
            for (auto it = m_readyByPriority.begin(); it != m_readyByPriority.end(); ++it) {
                if (!it.value().isEmpty()) {
                    qDebug() <<"Priority" << it.key() << "queue size:" << it.value().size();
                }
            }

            // 打印各 Block 状态
            for (auto& state : m_states) {
                if (!state.isDone) {
                    qDebug() <<"Block [" <<  QString::fromStdString(state.block->GetName())
                             << "]: ready=" << state.isReady
                             << ", inQueue="<< state.inReadyQueue
                             << ", backpressured="<< state.isBackpressured
                             << ", upstreamFinished="<< state.upstreamFinished
                             << ", execCount="<< state.executedCount;
                }
            }
            qDebug() <<"=====================================";
        }

        // 阶段1：信号源（带背压检测）
        while (!m_readySources.isEmpty()) {
            Block* block = m_readySources.dequeue();
            BlockRuntimeState& state = m_states[block];
            state.inReadyQueue = false;

            if (state.isDone) continue;

            // 检查是否被背压阻塞
            if (isBackpressured(block)) {
                state.isBackpressured = true;
                continue;
            }

            if (executeSourceWithBackpressure(state)) {
                madeProgress = true;
                qDebug() << QString::fromStdString(block->GetName()) <<" Source produced data: " << state.executedCount;
                updateBlockReadyState(state);
                if (state.isReady && !state.isDone && !state.isBackpressured) {
                    addToReadyQueue(state);
                }
                notifyDownstream(block);
            }
        }

        // 阶段2：处理器（带背压检测）
        while (!m_readyByPriority.isEmpty()) {
            // 找到最高优先级的非空队列
            int highestPriority = -1;
            for (auto it = m_readyByPriority.begin(); it != m_readyByPriority.end(); ++it) {
                if (!it->isEmpty() && it.key() > highestPriority) {
                    highestPriority = it.key();
                }
            }
            if (highestPriority == -1) break;

            QQueue<Block*>& queue = m_readyByPriority[highestPriority];
            if (queue.isEmpty()) continue;

            Block* block = queue.dequeue();
            BlockRuntimeState& state = m_states[block];
            state.inReadyQueue = false;

            if (state.isDone) continue;

            // 使用带背压检测的执行方法
            if (executeProcessorWithBackpressure(state)) {
                madeProgress = true;
//                qDebug() << QString::fromStdString(block->GetName()) <<" Proccessor processed data";
//                qDebug() << QString::fromStdString(block->GetName()) <<" Proccessor current deal round: " << state.executedCount;
                updateBlockReadyState(state);
//                if(state.executedCount >= totalTargetCollect - 4) {
//                    qDebug() << QString::fromStdString(block->GetName()) <<" Proccessor 状态更新后: ";
//                    qDebug() << "是否在队列中: " << state.inReadyQueue;
//                    qDebug() << "是否在就绪中: " << state.isReady;
//                    qDebug() << "是否完成: " << state.isDone;
//                    qDebug() << "是否背压: " << state.isBackpressured;
//                    qDebug() << "此刻的仿真次数: " << totalCollected << "/" << totalTargetCollect;
//                }
                if (state.isReady && !state.isDone && !state.isBackpressured) {
                    addToReadyQueue(state);


                }
                notifyDownstream(block);
                notifyUpstream(block);
//                if(state.executedCount >= totalTargetCollect - 4) {
//                    qDebug() << QString::fromStdString(block->GetName()) << "加入了队列";
//                    qDebug() << "是否在队列中: " << state.inReadyQueue;
//                    qDebug() << "是否在就绪中: " << state.isReady;
//                    qDebug() << "是否完成: " << state.isDone;
//                    qDebug() << "是否背压: " << state.isBackpressured;
//                    qDebug() <<"Ready processors:" << m_readyByPriority.values();
//                    qDebug() << "此刻的仿真次数: " << totalCollected << "/" << totalTargetCollect;
//                }
            } else if (state.isBackpressured) {
                // 被背压阻塞，不加入就绪队列，等待空间释放通知
                // 已经在回调中处理重新入队
            }
        }
//        if(totalTargetCollect >= 6394 && cycleCount <= 7) {
//            qDebug() <<"执行器执行完成后";
//            qDebug() <<"Ready processors:" << m_readyByPriority.values();
//        }

        // 阶段3：收集器（更新计数）
        while (!m_readySinks.isEmpty()) {
            Block* block = m_readySinks.dequeue();
            BlockRuntimeState& state = m_states[block];
            state.inReadyQueue = false;

            if (state.isDone) continue;

            int collected = executeSinkWithCount(state);
            if (collected > 0) {
                madeProgress = true;
                totalCollected += collected;
//                qDebug() << QString::fromStdString(block->GetName()) << "Sink collected data, count: "
//                         << totalCollected;
                m_sinkProcessCount[block->GetName()] += collected;

                // 检查 Sink 是否达到目标
                if (m_sinkProcessCount[block->GetName()] >= m_sinkTargetCounts[block->GetName()]) {
                    state.isDone = true;
                    block->SetDone(true);
                    block->Stop();

                    // 通知上游停止生产
                    notifyUpstreamFinished(block);
                }

                updateBlockReadyState(state);
                if (state.isReady && !state.isDone) {
                    addToReadyQueue(state);
                }
//                if(state.executedCount >= totalTargetCollect - 4) {
//                    qDebug() << QString::fromStdString(block->GetName()) << "状态更新";
//                    qDebug() << "是否在队列中: " << state.inReadyQueue;
//                    qDebug() << "是否在就绪中: " << state.isReady;
//                    qDebug() << "是否完成: " << state.isDone;
//                    qDebug() << "是否背压: " << state.isBackpressured;
//                    qDebug() <<"Ready processors:" << m_readyByPriority.values();
//                    qDebug() << "此刻的仿真次数: " << totalCollected << "/" << totalTargetCollect;
//                }
                notifyUpstream(block);
            }
        }
//        if(totalTargetCollect >= 6394 && cycleCount <= 7) {
//            qDebug() <<"Ready sources:" << m_readySources.size();
//            qDebug() <<"Ready processors:" << m_readyByPriority.values();
//            qDebug() <<"Ready sinks:" << m_readySinks.size();
//        }



        // 进度报告
        reportProgress(linkKey, m_sinkProcessCount, m_sinkTargetCounts);

        // 增强的死锁检测
        if (!madeProgress) {
            if (++noProgressCount > MAX_NO_PROGRESS) {
                qDebug() << "Deadlock detected! No progress for" << MAX_NO_PROGRESS << "cycles";

                // 打印详细的死锁信息
                qDebug() <<"=== Deadlock Diagnosis ===";
                qDebug() <<"Total collected:" << totalCollected << "/" << totalTargetCollect;
                qDebug() << "Ready queues - Sources:" << m_readySources.size()
                         << ", Sinks:" << m_readySinks.size();

                // 找出所有未完成但不在就绪队列中的 Block
                for (auto& state : m_states) {
                    if (!state.isDone && !state.inReadyQueue) {
                        qDebug() <<"Stuck block [" << QString::fromStdString(state.block->GetName()) << "]";
                        qDebug() <<"  isReady:" << state.isReady
                                 << ", isBackpressured:" << state.isBackpressured
                                 << ", upstreamFinished:" << state.upstreamFinished;

                        // 打印输入端口状态
                        for (size_t i = 0; i < state.block->GetInputPortCount(); i++) {
                            auto* reader = state.block->GetInputPort(state.block->GetInputPortName(i));
                            if (reader && reader->HasValidConnection()) {
                                size_t available = state.cachedInputCount.value(i, 0);
                                bool upstreamDone = reader->IsUpstreamDone();
                                qDebug() << "  Input[" << i << "]: available=" << available
                                         << ", required=" << state.requiredInputCount
                                         << ", upstreamDone=" << upstreamDone;

                                if (upstreamDone && available < static_cast<size_t>(state.requiredInputCount)) {
                                    qDebug() <<"    WARNING: Upstream done but insufficient data!";
                                }
                            }
                        }

                        // 打印输出端口状态
                        for (size_t i = 0; i < state.block->GetOutputPortCount(); i++) {
                            auto* buffer = state.block->GetOutputPort(state.block->GetOutputPortName(i));
                            if (buffer && buffer->GetReaderCount() > 0) {
                                size_t freeSpace = state.cachedOutputSpace.value(i, 0);
                                qDebug() <<"  Output[" << i << "]: freeSpace=" << freeSpace;
                                if (freeSpace == 0) {
                                    qDebug() <<"    WARNING: Output buffer full!";
                                }
                            }
                        }
                    }
                }
                qDebug() <<"=========================";
                break;
            }
        } else {
            noProgressCount = 0;
        }

        // 检查所有 Sink 是否完成
        allSinksDone = true;
        for (auto& pair : m_sinkProcessCount) {
            auto it = m_sinkTargetCounts.find(pair.first);
            if (it != m_sinkTargetCounts.end() && pair.second < it->second) {
                allSinksDone = false;
                break;
            }
        }
    }

    // 8. 清理阶段：强制停止所有上游
//    LOG_INFO("所有 Sink 已完成，停止所有上游 Block...");
    stopAllUpstreamBlocks();

    // 9. 完成处理
    for (auto block : blocks) {
        if (!block->IsDone()) {
            block->SetDone(true);
            block->Stop();
        }
        block->Done();
    }

//    LOG_INFO("[进度]链路：", linkKey.toStdString(), "，调度完成");
    LOG_INFO("调度完成");
    return true;
}

// ========== 初始化函数 ==========

void ReadyQueueScheduler::initBlockStates(QVector<Block*>& blocks) {
    m_states.clear();
    m_executionOrder = blocks;

    for (int i = 0; i < blocks.size(); i++) {
        Block* block = blocks[i];
        m_blockIndex[block] = i;

        BlockRuntimeState state;
        state.block = block;
        state.isDone = false;
        state.inReadyQueue = false;
        state.executedCount = 0;
        state.batchSize = block->GetBatchSize();
        state.requiredInputCount = 1;
        state.isBackpressured = false;
        state.backpressureLevel = 0;
        state.upstreamFinished = false;

        m_states[block] = state;
    }
}

void ReadyQueueScheduler::initReadyQueues(QVector<Block*>& blocks) {
    m_readySources.clear();
    m_readySinks.clear();
    m_readyByPriority.clear();

    for (auto block : blocks) {
        BlockRuntimeState& state = m_states[block];
        updateBlockReadyState(state);

        if (state.isReady && !state.isDone) {
            addToReadyQueue(state);
        }
    }
}

void ReadyQueueScheduler::calculateRequiredInputCounts() {
    for (auto& state : m_states) {
        state.requiredInputCount = state.block->GetMaxRequiredInputCount();

//        if (state.requiredInputCount > 1) {
//            LOG_DEBUG("Block [", state.block->GetName(), "] requires ",
//                      state.requiredInputCount, " inputs");
//        }
    }
}

int ReadyQueueScheduler::calculateSinkTarget(Block* sink) {
    std::string option = sink->getParameter("StartStopOption").Value;
    std::transform(option.begin(), option.end(), option.begin(), ::tolower);

    int count = 0;
    if (option == "auto") {
        count = m_totalSamples;
    } else if (option == "samples") {
        count = std::stoi(sink->getParameter("SampleStop").Value);
    } else if (option == "time") {
        double timeStop = std::stod(sink->getParameter("TimeStop").Value);
        count = static_cast<int>(timeStop / m_timeInterval);
    }

    return count > 0 ? count : m_totalSamples;
}

void ReadyQueueScheduler::calculateSinkTargets(QVector<Block*>& blocks) {
    m_sinkTargetCounts.clear();

    for (auto block : blocks) {
        if (block->GetBlockType() == Block::BlockType::SINK) {
            int target = calculateSinkTarget(block);
            m_sinkTargetCounts[block->GetName()] = target;
        }
    }
}

// ========== 状态管理函数 ==========

void ReadyQueueScheduler::updateBlockReadyState(BlockRuntimeState& state) {
    bool oldReady = state.isReady;
    if (state.isDone) {
        state.isReady = false;
        return;
    }

    updateCachedInputCounts(state);
    updateCachedOutputSpace(state);

    state.inputsReady = checkInputsReady(state);
    state.outputsReady = checkOutputsReady(state);

    bool newReady = state.inputsReady && state.outputsReady;

    if (state.block->GetBlockType() == Block::BlockType::SOURCE) {
        newReady = state.outputsReady && !state.isDone;
    }
    if (state.block->GetBlockType() == Block::BlockType::SINK) {
        newReady = state.inputsReady && !state.isDone;
    }

    state.isReady = newReady;
//    if (oldReady != state.isReady) {
//        LOG_DEBUG("Block ", state.block->GetName(),
//                  " ready changed: ", oldReady, " -> ", state.isReady,
//                  ", inputsReady=", state.inputsReady,
//                  ", outputsReady=", state.outputsReady);
//    }
}

bool ReadyQueueScheduler::checkInputsReady(BlockRuntimeState& state) {
    Block* block = state.block;

    if (block->GetBlockType() == Block::BlockType::SOURCE) {
        return true;
    }

    for (size_t i = 0; i < block->GetInputPortCount(); i++) {
        std::string portName = block->GetInputPortName(i);
        BufferReader* reader = block->GetInputPort(portName);

        if (!reader || !reader->HasValidConnection()) continue;

        int required = state.requiredInputCount;
        size_t available = state.cachedInputCount.value(i, 0);

        if(block->GetBlockType() == Block::BlockType::SINK) {
            if(available == 1024 || available % 100 == 0) {
//                LOG_DEBUG(block->GetName(), " checkInputsReady: port=", i,
//                          ", required=", required,
//                          ", available=", available);
            }
        }


        if (available < static_cast<size_t>(required)) {
            if (reader->IsUpstreamDone()) {
                state.isDone = true;
                return false;
            }
            return false;
        }
    }

    return true;
}

bool ReadyQueueScheduler::checkOutputsReady(BlockRuntimeState& state) {
    Block* block = state.block;

    if (block->GetBlockType() == Block::BlockType::SINK) {
        return true;
    }

    for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
        Buffer* buffer = block->GetOutputPort(block->GetOutputPortName(i));
        if (!buffer) continue;

        if (buffer->GetReaderCount() == 0) continue;

        size_t freeSpace = state.cachedOutputSpace.value(i, 0);
        if(block->GetBlockType() == Block::BlockType::SINK) {
            if(freeSpace == 1024 || freeSpace % 100 == 0) {
//                LOG_DEBUG(block->GetName(), " checkOutputsReady: port=", i,
//                          ", freeSpace=", freeSpace);
            }
        }
        if (freeSpace == 0) {
            return false;
        }
    }

    return true;
}

void ReadyQueueScheduler::updateCachedInputCounts(BlockRuntimeState& state) {
    Block* block = state.block;
    state.cachedInputCount.clear();

    for (size_t i = 0; i < block->GetInputPortCount(); i++) {
        std::string portName = block->GetInputPortName(i);
        BufferReader* reader = block->GetInputPort(portName);

        if (reader && reader->HasValidConnection()) {
            state.cachedInputCount[i] = reader->GetAvailableDataCount();
        } else {
            state.cachedInputCount[i] = 0;
        }
    }
}

void ReadyQueueScheduler::updateCachedOutputSpace(BlockRuntimeState& state) {
    Block* block = state.block;
    state.cachedOutputSpace.clear();

    for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
        std::string portName = block->GetOutputPortName(i);
        Buffer* buffer = block->GetOutputPort(portName);

        if (buffer) {
            if (buffer->GetReaderCount() == 0) {
                state.cachedOutputSpace[i] = SIZE_MAX;
            } else {
                state.cachedOutputSpace[i] = buffer->GetBufferFreeSpace();
            }
        } else {
            state.cachedOutputSpace[i] = 0;
        }
    }
}

// ========== 背压控制 ==========

bool ReadyQueueScheduler::isBackpressured(Block* block) {
    BlockRuntimeState* state = getState(block);
    if (!state) return false;

    if (state->upstreamFinished) {
        return true;
    }

    // 检查输出端口是否有空间
    bool hasOutputSpace = true;
    for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
        Buffer* buffer = block->GetOutputPort(block->GetOutputPortName(i));
        if (buffer && buffer->GetReaderCount() > 0) {
            size_t freeSpace = state->cachedOutputSpace.value(i, 0);
            if (freeSpace == 0) {
                hasOutputSpace = false;
                break;
            }
        }
    }

    if (!hasOutputSpace) {
        state->backpressureLevel = 100;
        return true;
    }

    state->backpressureLevel = 0;
    return false;
}

bool ReadyQueueScheduler::isOutputAllConsumed(BlockRuntimeState& state) {
    Block* block = state.block;

    for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
        Buffer* buffer = block->GetOutputPort(block->GetOutputPortName(i));
        if (buffer && buffer->GetReaderCount() > 0) {
            if (buffer->GetUsedSpace() > 0) {
                return false;
            }
        }
    }
    return true;
}

int ReadyQueueScheduler::calculateDynamicBatch(BlockRuntimeState &state)
{
    Block* block = state.block;
    int originalBatch = state.batchSize;
    float downstreamUsage = block->GetDownstreamBufferUsage();
//    qDebug() << "ReadyQueueScheduler::calculateDynamicBatch --" << QString::fromStdString(block->GetName())
//             << "downstreamUsage: " << downstreamUsage;

    if (downstreamUsage > 80.0f) {
        state.backpressureLevel = 90;
        return 1;
    } else if (downstreamUsage > 60.0f) {
        state.backpressureLevel = 70;
        return std::max(1, originalBatch / 4);
    } else if (downstreamUsage > 40.0f) {
        state.backpressureLevel = 50;
        return std::max(1, originalBatch / 2);
    } else if (downstreamUsage < 20.0f && state.currentDynamicBatch < originalBatch) {
        state.backpressureLevel = 20;
        return std::min(originalBatch, state.currentDynamicBatch * 2);
    }

    state.backpressureLevel = 0;
    return originalBatch;
}

void ReadyQueueScheduler::propagateBackpressureToUpstream(Block *block)
{
    // 避免重复处理
    if (m_backpressuredBlocks.contains(block)) {
        return;
    }
    m_backpressuredBlocks.insert(block);

    BlockRuntimeState* state = getState(block);
    if (state) {
        state->isBackpressured = true;
    }

    // 找到所有输入端口的上游
    for (size_t i = 0; i < block->GetInputPortCount(); i++) {
        BufferReader* reader = block->GetInputPort(block->GetInputPortName(i));
        if (reader) {
            Buffer* buffer = reader->GetConnectedBuffer();
            if (buffer) {
                // 找到拥有这个buffer的Block（上游）
                void* writer = buffer->GetWriter();
                if (writer) {
                    Block* upstream = static_cast<Block*>(writer);
                    if (!m_backpressuredBlocks.contains(upstream)) {
                        propagateBackpressureToUpstream(upstream);
                    }
                }
            }
        }
    }
}

void ReadyQueueScheduler::releaseBackpressure(Block *block)
{
    if (!m_backpressuredBlocks.contains(block)) {
        return;
    }
    m_backpressuredBlocks.remove(block);

    BlockRuntimeState* state = getState(block);
    if (state) {
        state->isBackpressured = false;
        state->consecutiveFailures = 0;

        // 重新检查就绪状态
        updateBlockReadyState(*state);
        if (state->isReady && !state->isDone && !state->inReadyQueue) {
            addToReadyQueue(*state);
//            LOG_DEBUG("Block [", block->GetName(), "] backpressure released");
        }
    }

    // 递归解除下游的背压标记（如果下游也已解除）
    for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
        Buffer* buffer = block->GetOutputPort(block->GetOutputPortName(i));
        if (buffer) {
            for (auto reader : buffer->GetReaders()) {
                // 找到下游Block
                for (auto& pair : m_states) {
                    Block* downstream = pair.block;
                    for (size_t j = 0; j < downstream->GetInputPortCount(); j++) {
                        if (downstream->GetInputPort(downstream->GetInputPortName(j)) == reader) {
                            // 检查下游是否还有背压
                            BlockRuntimeState* dsState = getState(downstream);
                            if (dsState && !dsState->isBackpressured) {
                                releaseBackpressure(downstream);
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}

void ReadyQueueScheduler::setupBackpressureCallbacks(BlockRuntimeState &state)
{
    Block* block = state.block;

    for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
        Buffer* buffer = block->GetOutputPort(block->GetOutputPortName(i));
        if (buffer) {
            // 设置背压回调
            buffer->setBackpressureCallback([this, block](Buffer* buf, bool isBackpressured) {
                std::ignore = buf;
                BlockRuntimeState* bs = getState(block);
                if (bs) {
                    bs->isBackpressured = isBackpressured;
                    if (isBackpressured) {
                        // 传播背压到上游
                        propagateBackpressureToUpstream(block);
//                        LOG_DEBUG("Block [", block->GetName(), "] backpressured");
                    } else {
                        // 背压解除，重新加入就绪队列
                        releaseBackpressure(block);
                    }
                }
            });
        }
    }
}

// ========== 通知机制 ==========

void ReadyQueueScheduler::notifyDownstream(Block* block) {
    for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
        Buffer* buffer = block->GetOutputPort(block->GetOutputPortName(i));
        if (!buffer) continue;

        // 获取所有读者
        auto readers = buffer->GetReaders();
        for (auto reader : readers) {
            // 找到拥有这个 reader 的 block
            for (auto& pair : m_states) {
                Block* candidate = pair.block;
                for (size_t j = 0; j < candidate->GetInputPortCount(); j++) {
                    if (candidate->GetInputPort(candidate->GetInputPortName(j)) == reader) {
                        BlockRuntimeState& downstreamState = m_states[candidate];
                        bool wasReady = downstreamState.isReady;
                        updateBlockReadyState(downstreamState);

                        // 调试日志
//                        LOG_DEBUG("notifyDownstream: upstream=", block->GetName(),
//                                  ", downstream=", downstreamState.block->GetName(),
//                                  ", wasReady=", wasReady,
//                                  ", nowReady=", downstreamState.isReady,
//                                  ", inQueue=", downstreamState.inReadyQueue,
//                                  ", requiredInput=", downstreamState.requiredInputCount,
//                                  ", cachedInputCount=", downstreamState.cachedInputCount.value(0, 0));

                        if (!wasReady && downstreamState.isReady && !downstreamState.isDone) {
                            addToReadyQueue(downstreamState);
//                            LOG_DEBUG("Added ", downstreamState.block->GetName(), " to ready queue");
                        }
                        break;
                    }
                }
            }
        }
    }
}

void ReadyQueueScheduler::notifyUpstream(Block* block) {
    for (size_t i = 0; i < block->GetInputPortCount(); i++) {
        BufferReader* reader = block->GetInputPort(block->GetInputPortName(i));
        if (!reader) continue;

        Buffer* buffer = reader->GetConnectedBuffer();
        if (!buffer) continue;

        // 通知Buffer空间已释放
        buffer->NotifySpaceAvailable();

        // 找到上游Block
        void* writer = buffer->GetWriter();
        if (writer) {
            Block* upstream = static_cast<Block*>(writer);

            // 解除背压
            if (m_backpressuredBlocks.contains(upstream)) {
                releaseBackpressure(upstream);
            }

            BlockRuntimeState& upstreamState = m_states[upstream];
            bool wasReady = upstreamState.isReady;
            updateBlockReadyState(upstreamState);
//            qDebug() << "notifyUpstream --上游块" << QString::fromStdString(upstreamState.block->GetName())
//                     << "inReadyQueue: " << upstreamState.inReadyQueue;
//            qDebug() << "wasReady: " << wasReady;
//            qDebug() << "isReady: " << upstreamState.isReady;
//            qDebug() << "isDone: " << upstreamState.isDone;

            if (!wasReady && upstreamState.isReady && !upstreamState.isDone) {
//                qDebug() << "notifyUpstream --上游块" << QString::fromStdString(upstream->GetName())
//                         << "加入队列前，inReadyQueue: " << upstreamState.inReadyQueue;
                addToReadyQueue(upstreamState);

            }
        }
    }
}

void ReadyQueueScheduler::notifyUpstreamFinished(Block* sink) {
    QSet<Block*> visited;
    QQueue<Block*> toProcess;
    toProcess.enqueue(sink);

    while (!toProcess.isEmpty()) {
        Block* current = toProcess.dequeue();

        if (visited.contains(current)) continue;
        visited.insert(current);

        // 找到所有输入端口的上游
        for (size_t i = 0; i < current->GetInputPortCount(); i++) {
            BufferReader* reader = current->GetInputPort(current->GetInputPortName(i));
            if (reader) {
                Buffer* buffer = reader->GetConnectedBuffer();
                if (buffer) {
                    // 找到拥有这个 buffer 的 block
                    for (auto& pair : m_states) {
                        Block* upstream = pair.block;
                        for (size_t j = 0; j < upstream->GetOutputPortCount(); j++) {
                            if (upstream->GetOutputPort(upstream->GetOutputPortName(j)) == buffer) {
                                BlockRuntimeState& upstreamState = m_states[upstream];
                                upstreamState.upstreamFinished = true;
                                toProcess.enqueue(upstream);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

void ReadyQueueScheduler::stopAllUpstreamBlocks() {
    for (auto& pair : m_states) {
        Block* block = pair.block;
        BlockRuntimeState& state = pair;

        if (!state.isDone && block->GetBlockType() != Block::BlockType::SINK) {
            state.isDone = true;
            block->SetDone(true);
            block->Stop();
//            LOG_INFO("强制停止 Block: ", block->GetName());
        }
    }
}

void ReadyQueueScheduler::addToReadyQueue(BlockRuntimeState& state) {
    if (state.inReadyQueue) return;
    if (state.isDone) return;
    if (!state.isReady) return;

    state.inReadyQueue = true;

    Block::BlockType type = state.block->GetBlockType();

    switch (type) {
    case Block::BlockType::SOURCE:
        m_readySources.enqueue(state.block);
        break;
    case Block::BlockType::PROCESSOR:
        {
//            qDebug() << "addToReadyQueue --current Block: " << QString::fromStdString(state.block->GetName());
            int priority = getBlockPriority(state.block);
            m_readyByPriority[priority].enqueue(state.block);
        }
        break;
    case Block::BlockType::SINK:
        m_readySinks.enqueue(state.block);
        break;
    }
}

void ReadyQueueScheduler::removeFromReadyQueue(BlockRuntimeState& state) {
    state.inReadyQueue = false;
}

// ========== 执行逻辑 ==========

bool ReadyQueueScheduler::executeSource(BlockRuntimeState& state,
                                         int& globalProcessCount,
                                         int maxProcessCount) {
    Block* block = state.block;

    if (globalProcessCount >= maxProcessCount) {
        if (isOutputAllConsumed(state)) {
            state.isDone = true;
            block->SetDone(true);
            block->Stop();
        }
        return false;
    }

    int remaining = maxProcessCount - globalProcessCount;
    int batchSize = std::min(state.batchSize, remaining);

    bool hasSpaceForBatch = true;
    for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
        size_t freeSpace = state.cachedOutputSpace.value(i, 0);
        if (freeSpace < static_cast<size_t>(batchSize)) {
            hasSpaceForBatch = false;
            break;
        }
    }

    if (!hasSpaceForBatch) {
        return false;
    }

    int executed = tryExecuteBatch(state, batchSize);
    if (executed > 0) {
        globalProcessCount += executed;
        state.executedCount += executed;
        return true;
    }

    return false;
}

bool ReadyQueueScheduler::executeSourceWithBackpressure(BlockRuntimeState& state) {
    Block* block = state.block;

    if (state.upstreamFinished) {
        state.isDone = true;
        block->SetDone(true);
        block->Stop();
        return false;
    }

    bool hasOutputSpace = true;
    for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
        size_t freeSpace = state.cachedOutputSpace.value(i, 0);
        if (freeSpace == 0) {
            hasOutputSpace = false;
            break;
        }
    }

    if (!hasOutputSpace) {
        return false;
    }

    int executed = tryExecuteBatch(state, state.batchSize);
    state.executedCount += executed;
    return executed > 0;
}

bool ReadyQueueScheduler::executeProcessor(BlockRuntimeState& state) {
    Block* block = state.block;

    int batchSize = state.batchSize;

    bool hasEnoughInput = true;
    for (size_t i = 0; i < block->GetInputPortCount(); i++) {
        size_t available = state.cachedInputCount.value(i, 0);
        if (available < static_cast<size_t>(state.requiredInputCount * batchSize)) {
            hasEnoughInput = false;
            break;
        }
    }

    if (!hasEnoughInput) {
        return false;
    }

    bool hasOutputSpace = true;
    for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
        size_t freeSpace = state.cachedOutputSpace.value(i, 0);
        if (freeSpace < static_cast<size_t>(batchSize)) {
            hasOutputSpace = false;
            break;
        }
    }

    if (!hasOutputSpace) {
        return false;
    }

    int executed = tryExecuteBatch(state, batchSize);
    return executed > 0;
}

bool ReadyQueueScheduler::executeProcessorWithBackpressure(BlockRuntimeState& state)
{
    Block* block = state.block;

    if (state.isBackpressured) return false;
    if (state.upstreamFinished) {
        state.isDone = true;
        block->SetDone(true);
        block->Stop();
        return false;
    }

    // 设置背压回调
    if (!state.inReadyQueue) {
        setupBackpressureCallbacks(state);
    }

    // ========== 计算动态批量大小 ==========
    int dynamicBatch = calculateDynamicBatch(state);
    state.currentDynamicBatch = dynamicBatch;

    // ========== 检查输入数据是否足够 ==========
    size_t minAvailable = SIZE_MAX;
    for (size_t i = 0; i < block->GetInputPortCount(); i++) {
        size_t available = state.cachedInputCount.value(i, 0);
        minAvailable = std::min(minAvailable, available);
    }

    int requiredPerBatch = state.requiredInputCount;
    int maxPossibleBatch = (int)(minAvailable / requiredPerBatch);

    // ========== 关键修复：兼容剩余数据处理 ==========
    // 如果输入数据不足一个完整批量，但大于0，仍然尝试处理
    if (maxPossibleBatch == 0 && minAvailable > 0) {
        // 剩余数据不足一个完整批量，尝试用剩余数据量作为批量
        // 注意：这需要模型支持处理部分数据
        dynamicBatch = 1;
        maxPossibleBatch = 1;
//        LOG_DEBUG("Block [", block->GetName(), "] processing remaining ",
//                  minAvailable, " samples (less than batch size)");
    }

    int actualBatch = std::min(dynamicBatch, maxPossibleBatch);

    if (actualBatch <= 0) {
        return false;
    }

    // ========== 检查输出空间 ==========
    bool hasOutputSpace = true;
    for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
        size_t freeSpace = state.cachedOutputSpace.value(i, 0);
        // 每个输出块可能产生多个输出，需要模型告知输出比例
        int outputPerBatch = 1;  // 默认1，模型可重写
        if (freeSpace < static_cast<size_t>(actualBatch * outputPerBatch)) {
            hasOutputSpace = false;
            break;
        }
    }

    if (!hasOutputSpace) {
        return false;
    }

    // 执行批量处理
    int executed = tryExecuteBatch(state, actualBatch);

    if (executed > 0) {
        state.consecutiveFailures = 0;
        return true;
    }

    state.consecutiveFailures++;
    return false;
}

int ReadyQueueScheduler::executeSinkWithCount(BlockRuntimeState& state) {
    Block* block = state.block;

    bool hasData = false;
    size_t totalAvailable = 0;
    for (size_t i = 0; i < block->GetInputPortCount(); i++) {
        size_t available = state.cachedInputCount.value(i, 0);
        totalAvailable += available;
        if (available > 0) hasData = true;
    }

    if (!hasData) {
        bool allUpstreamDone = true;
        for (size_t i = 0; i < block->GetInputPortCount(); i++) {
            BufferReader* reader = block->GetInputPort(block->GetInputPortName(i));
            if (reader && reader->HasValidConnection() && !reader->IsUpstreamDone()) {
                allUpstreamDone = false;
                break;
            }
        }

        if (allUpstreamDone) {
            state.isDone = true;
            block->SetDone(true);
            block->Stop();
        }
        return 0;
    }

    auto it = m_sinkTargetCounts.find(block->GetName());
    int targetRemaining = (it != m_sinkTargetCounts.end()) ?
                          (it->second - m_sinkProcessCount[block->GetName()]) :
                          m_totalSamples;

    int batchSize = std::min(static_cast<int>(totalAvailable), targetRemaining);
    batchSize = std::min(batchSize, 100);

    int executed = tryExecuteBatch(state, batchSize);
    state.executedCount += executed;
    return executed;
}

int ReadyQueueScheduler::tryExecuteBatch(BlockRuntimeState& state, int maxBatch) {
    Block* block = state.block;

    int batchSize = block->GetBatchSize();
    bool useBatch = (batchSize > 1 && maxBatch >= batchSize);
    if(block->GetBlockType() == Block::BlockType::PROCESSOR) {
        qDebug() << "ReadyQueueScheduler::tryExecuteBatch --" << QString::fromStdString(block->GetName())
                 << "maxBatch: " << maxBatch;
        qDebug() << "ReadyQueueScheduler::tryExecuteBatch --" << QString::fromStdString(block->GetName())
                 << "batchSize: " << batchSize;
    }


    int executed = 0;

    if (useBatch && block->GetBlockType() == Block::BlockType::PROCESSOR) {
        int requiredPerBatch = state.requiredInputCount;
        size_t availableInput = SIZE_MAX;
        for (size_t i = 0; i < block->GetInputPortCount(); i++) {
            availableInput = std::min(availableInput, state.cachedInputCount.value(i, 0));
        }

        if (availableInput < static_cast<size_t>(requiredPerBatch * batchSize)) {
            useBatch = false;
        }
    }
    if(block->GetBlockType() == Block::BlockType::SINK) {
//        qDebug() << "ReadyQueueScheduler::tryExecuteBatch --" << QString::fromStdString(block->GetName())
//                 << "useBatch: " << (useBatch?"true":"false");
    }

    if (useBatch) {
        int actualBatch = std::min(batchSize, maxBatch);
        if(block->GetBlockType() == Block::BlockType::SINK) {
//            qDebug() << "ReadyQueueScheduler::tryExecuteBatch --" << QString::fromStdString(block->GetName())
//                     << "actualBatch: " << actualBatch;
        }

        executed = block->RunBatch(actualBatch);
        qDebug() << "ReadyQueueScheduler::tryExecuteBatch --" << QString::fromStdString(block->GetName())
                 << "RunBatch ending, executed: " << executed;
    } else {
        for (int i = 0; i < maxBatch; i++) {
            if (!block->Run()) {
                break;
            }
            executed++;

            updateCachedInputCounts(state);
            updateCachedOutputSpace(state);

            if (!checkInputsReady(state) || !checkOutputsReady(state)) {
                break;
            }
        }
        qDebug() << "ReadyQueueScheduler::tryExecuteBatch --" << QString::fromStdString(block->GetName())
                 << "usingbatch RunBatch ending, executed: " << executed;
    }

    return executed;
}

// ========== 辅助方法 ==========

BlockRuntimeState* ReadyQueueScheduler::getState(Block* block) {
    auto it = m_states.find(block);
    if (it != m_states.end()) {
        return &it.value();
    }
    return nullptr;
}

int ReadyQueueScheduler::getBlockPriority(Block* block) {
    int priority = 5;

    switch (block->GetBlockType()) {
    case Block::BlockType::SOURCE:
        priority = 15;
        break;
    case Block::BlockType::PROCESSOR:
        priority = 10;
        break;
    case Block::BlockType::SINK:
        priority = 5;
        break;
    }

    int requiredInput = block->GetMaxRequiredInputCount();
    if (requiredInput > 1) {
        priority += std::min(requiredInput / 10, 10);

        BlockRuntimeState* state = getState(block);
        if (state) {
            size_t totalInput = 0;
            for (size_t i = 0; i < block->GetInputPortCount(); i++) {
                totalInput += state->cachedInputCount.value(i, 0);
            }
            if (totalInput > static_cast<size_t>(requiredInput * 2)) {
                priority += 5;
            }
        }
    }

    return std::max(0, std::min(priority, 35));
}

void ReadyQueueScheduler::reportProgress(const QString& linkKey,
                        const std::map<std::string, int>& sinkCounts,
                        const std::map<std::string, int>& sinkTargets) {
    if (sinkCounts.empty() || sinkTargets.empty()) {
        return;
    }

    // 计算加权平均进度
    double weightedProgress = 0.0;
    double totalWeight = 0.0;

    for (const auto& pair : sinkCounts) {
        const std::string& sinkName = pair.first;
        int collected = pair.second;

        auto it = sinkTargets.find(sinkName);
        if (it != sinkTargets.end() && it->second > 0) {
            double weight = it->second;
            double sinkProgress = static_cast<double>(collected) * 100.0 / it->second;

            weightedProgress += sinkProgress * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight == 0) {
        return;
    }

    double avgProgress = weightedProgress / totalWeight;
    int currentProgress = static_cast<int>(avgProgress);

    // 计算当前进度所在的10%区间
    int currentThreshold = (currentProgress / 10) * 10;

    // 特殊处理：第一次计算进度时，确保打印10%的进度
    static bool firstTime = true;
    if (firstTime && currentThreshold >= 10) {
        // 首次检测到进度达到10%以上，打印10%的进度
        LOG_DEBUG("当前总进度：10%");
        fflush(stdout);
        m_lastProgressPercent = 10;
        firstTime = false;

        // 如果已经超过了10%，继续处理后续进度
        if (currentThreshold > 10) {
            for (int threshold = 20; threshold <= currentThreshold; threshold += 10) {
                if (threshold <= 100) {
                    LOG_DEBUG("当前总进度：", threshold, "%");
                }
            }
            m_lastProgressPercent = currentThreshold;
        }
    }
    else if (!firstTime && currentThreshold > m_lastProgressPercent) {
        // 正常情况：补打被跳过的阈值
        for (int threshold = m_lastProgressPercent + 10; threshold <= currentThreshold; threshold += 10) {
            if (threshold <= 100 && threshold > 0) {
                LOG_DEBUG("当前总进度：", threshold, "%");
            }
        }
        m_lastProgressPercent = currentThreshold;
    }

    // 确保100%时打印一次
    if (currentProgress >= 100 && m_lastProgressPercent < 100) {
        LOG_DEBUG("当前总进度：100%");
        m_lastProgressPercent = 100;
    }
}
