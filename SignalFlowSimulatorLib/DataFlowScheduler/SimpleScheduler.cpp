#include "SimpleScheduler.h"
#include <QDebug>
#include <queue>
#include <set>
#include <algorithm>
#include <iostream>

SimpleScheduler::SimpleScheduler()
{
}

SimpleScheduler::~SimpleScheduler()
{
}

bool SimpleScheduler::schedule(const QString& linkKey, 
                               QVector<Block*> blocks,
                               std::shared_ptr<DataStreamVerification> verificationSystem,
                               const SimuParameter& simuParams)
{
    return simpleSchedulerImpl(linkKey, blocks, verificationSystem, simuParams);
}

void SimpleScheduler::pause()
{
    if (m_paused && !(*m_paused)) {
        *m_paused = 1;
        qDebug() << "[SimpleScheduler] 暂停标志已设置";
    }
}

void SimpleScheduler::resume()
{
    if (m_paused && (*m_paused)) {
        *m_paused = 0;
        if (m_pauseMutex && m_pauseCond) {
            QMutexLocker locker(m_pauseMutex);
            m_pauseCond->wakeAll();
        }
        qDebug() << "[SimpleScheduler] 继续标志已设置，调度循环已唤醒";
    }
}

void SimpleScheduler::requestStop()
{
    if (m_stopRequested) {
        *m_stopRequested = 1;
        qDebug() << "[SimpleScheduler] 停止请求标志已设置";
        
        // 如果当前处于暂停状态，需要唤醒等待线程
        if (m_paused && (*m_paused)) {
            *m_paused = 0;
            if (m_pauseMutex && m_pauseCond) {
                QMutexLocker locker(m_pauseMutex);
                m_pauseCond->wakeAll();
            }
        }
    }
}

bool SimpleScheduler::isPaused() const
{
    return m_paused && (*m_paused == 1);
}

void SimpleScheduler::setPauseControls(QAtomicInt* paused, 
                                       QAtomicInt* stopRequested,
                                       QMutex* pauseMutex,
                                       QWaitCondition* pauseCond)
{
    m_paused = paused;
    m_stopRequested = stopRequested;
    m_pauseMutex = pauseMutex;
    m_pauseCond = pauseCond;
}

bool SimpleScheduler::simpleSchedulerImpl(const QString& linkKey, 
                                          QVector<Block*> blocks,
                                          std::shared_ptr<DataStreamVerification> verificationSystem,
                                          const SimuParameter& simuParams)
{
    if(!verificationSystem) {
        LOG_ERROR("Verification system not initialized!");
        return false;
    }

    // 1. 创建约束系统
    auto VerificationSystem = Block::GetVerificationSystem();
    if(!VerificationSystem) {
        LOG_ERROR("无法获取验证系统！");
        return false;
    }

    // 2. 注册所有块的约束变量
    for (auto block : blocks) {
        VerificationSystem->registerBlock(block);
    }

    // 3. 检查约束系统可行性
//    if (!verificationSystem->CheckFeasibility()) {
//        LOG_ERROR("数据一致性校验失败!");
//        return false;
//    }
    LOG_INFO("数据一致性校验成功");
    LOG_INFO("数据流可以继续执行");

    // 按类型分组
    QMap<Block::BlockType, QVector<Block*>> blocksByType;
    for (Block* block : blocks) {
        blocksByType[block->GetBlockType()].append(block);
    }

    // 对处理器进行排序
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

    // 构建最终执行顺序
    QVector<Block*> executionOrder;
    executionOrder.append(blocksByType[Block::BlockType::SOURCE]);      // 1. SOURCE
    executionOrder.append(sortedProcessors);                            // 2. 排序后的PROCESSOR
    executionOrder.append(blocksByType[Block::BlockType::SINK]);        // 3. SINK

    // 调试输出
    qDebug() << "最终执行顺序：";
    for (Block* block : executionOrder) {
        qDebug() << "  " << QString::fromStdString(block->GetName());
    }

    // 统计信号源和收集器数量
    int sourceCount = 0, sinkCount = 0;
    int OutputBusCount = 0;
    for (Block* block : blocks) {
        if (block->GetBlockType() == Block::BlockType::SOURCE) {
            sourceCount++;
        }
        else if (block->GetBlockType() == Block::BlockType::SINK) {
            sinkCount++;
        }
        for(size_t i = 0; i < block->GetOutputPortCount();i++) {
            Buffer* buffer = block->GetOutputPort(block->GetOutputPortName(i));
            if(buffer->IsBusType(buffer->GetDataType())) {
                OutputBusCount++;
            }
        }
    }

    if (sourceCount == 0 || sinkCount == 0) {
        LOG_ERROR("链路：", linkKey.toStdString(), "，缺少必要的信号源或收集器");
        return false;
    }

    // 计算最大处理次数
    int maxProcessCount = calculateMaxProcessCount(blocks, linkKey, sourceCount);

    // 初始化状态
    for (Block* block : executionOrder) {
        block->SetDone(false);
    }

    std::map<std::string, int> sinkProcessCount;
    for (Block* block : blocks) {
        if (block->GetBlockType() == Block::BlockType::SINK) {
            sinkProcessCount[block->GetName()] = 0;
        }
    }

    // 调度状态变量
    unsigned int nalive = blocks.size();
    unsigned int blocks_count = nalive;
    bool first_progress = true;  // 第一个步长，必须让SOURCE产生数据
    bool made_progress_last_pass = true;
    bool making_progress = false;
    int processCount = 0;
    int iteration = 0;
    int lastProgress = -1;
    int noProgressCount = 0;
    int FinalProgress = 0;
    const int MAX_NO_PROGRESS_ITERATIONS = 1000;
    std::set<std::string> processedBlocks;  // 追踪已实际执行过的 processor block

    // 主调度循环
    while (nalive > 0 || processCount < maxProcessCount) {

        // 检查停止请求
        if (m_stopRequested && (*m_stopRequested)) {
            LOG_INFO("检测到stdin停止命令，正在终止仿真...");
            qDebug() << "[SimpleScheduler] 停止标志已触发，退出调度循环";
            break;
        }
        
        // 检查暂停状态
        if (m_paused && (*m_paused)) {
            QMutexLocker locker(m_pauseMutex);

            LOG_INFO("仿真已暂停，等待继续或停止命令...");
            qDebug() << "[SimpleScheduler] 进入暂停等待状态";

            // 阻塞等待
            while (m_paused && (*m_paused) && m_stopRequested && !(*m_stopRequested)) {
                m_pauseCond->wait(m_pauseMutex, 1000);
            }

            if (m_stopRequested && (*m_stopRequested)) {
                LOG_INFO("暂停期间收到停止命令，终止仿真...");
                qDebug() << "[SimpleScheduler] 暂停期间检测到停止标志，退出调度循环";
                break;
            }

            LOG_INFO("仿真继续执行");
            qDebug() << "[SimpleScheduler] 已从暂停状态恢复，继续调度";
        }

        iteration++;
        making_progress = false;

        qDebug() << "\n=== Iteration " << iteration << " ===";
        qDebug() << "Active blocks: " << nalive << ", Process count: " << processCount << "/" << maxProcessCount;

        // 按拓扑顺序执行
        for (Block* currentBlock : executionOrder) {
            if (currentBlock->IsDone()) {
                qDebug() << "currentBlock '" << QString::fromStdString(currentBlock->GetName())
                         << "' is done, try to skip this block";
                continue;
            }

            QString blocktype;
            if(currentBlock->GetBlockType() == Block::BlockType::SOURCE) blocktype = "Source";
            else if(currentBlock->GetBlockType() == Block::BlockType::PROCESSOR) blocktype = "Processor";
            else blocktype = "Sink";

            qDebug() << " ";
            qDebug() << "Processing block: " << QString::fromStdString(currentBlock->GetName()) << "[" << blocktype << "]";
            qDebug() << " ";

            Block::BlockType type = currentBlock->GetBlockType();
            bool blockDone = false;
            bool progressMade = false;

            // 死锁检测
            if (blocks_count > 0 && noProgressCount / blocks_count > 500) {
                LOG_ERROR("Too many iterations without progress, forcing exit");
                blockDone = true;
            } else {
                if (type == Block::BlockType::SOURCE) {
                    if (!first_progress && made_progress_last_pass) {
                        continue;
                    }

                    // 检查source是否应该结束
                    size_t AllOutPortsfreeSpace = 0;
                    for(size_t i = 0; i < currentBlock->GetOutputPortCount(); i++) {
                        std::string portName = currentBlock->GetOutputPortName(i);
                        Buffer* buffer = currentBlock->GetOutputPort(currentBlock->GetOutputPortName(i));
                        if(buffer->GetReaderCount() == 0) {
                            AllOutPortsfreeSpace += 1;
                        } else {
                            size_t OutputPortfreeSpace = currentBlock->GetOutputPort(portName)->GetBufferFreeSpace();
                            AllOutPortsfreeSpace += OutputPortfreeSpace;
                        }
                    }
                    qDebug() << "AllOutPortsfreeSpace is 0 or not: " << (AllOutPortsfreeSpace == 0 ? "true" : "false");
                    
                    if ((processCount >= maxProcessCount && AllOutPortsfreeSpace != 0) ||
                            currentBlock->IsDownstreamDone()) {
                        blockDone = true;
                    }

                    // 检查输出空间是否可用
                    bool output_ready = true;
                    for (size_t i = 0; i < currentBlock->GetOutputPortCount(); i++) {
                        std::string portName = currentBlock->GetOutputPortName(i);
                        Buffer* buffer = currentBlock->GetOutputPort(currentBlock->GetOutputPortName(i));
                        if (buffer->GetReaderCount() != 0) {
                            size_t freeSpace = buffer->GetBufferFreeSpace();
                            if (freeSpace == 0) {
                                output_ready = false;
                                break;
                            }
                        }
                    }

                    if (!output_ready) {
                        qDebug() << "Source output not ready, skipping";
                        continue;
                    }
                    qDebug() << "output_ready: " << output_ready;

                    // 执行source块的工作
                    progressMade = processSourceBlock(currentBlock, processCount, maxProcessCount);
                    if (progressMade) {
                        making_progress = true;
                        qDebug() << "Source produced data";
                    }
                }
                else if (type == Block::BlockType::SINK) {
                    // 检查输入数据可用性
                    int max_items_avail = 0;
                    bool input_done = false;
                    bool all_upstream_done = true;

                    if (currentBlock->IsDone()) {
                        blockDone = true;
                        goto process_sink_done;
                    }

                    for (size_t i = 0; i < currentBlock->GetInputPortCount(); i++) {
                        std::string portName = currentBlock->GetInputPortName(i);
                        BufferReader* reader = currentBlock->GetInputPort(portName);
                        size_t available = reader->GetAvailableDataCount();
                        qDebug() << "Sink - 上游可以数据: " << available;

                        if(available < 1) {
                            if (reader->IsUpstreamDone()) {
                                input_done = true;
                            } else {
                                input_done = false;
                            }
                        } else {
                            input_done = false;
                        }

                        if (!reader->IsUpstreamDone()) {
                            all_upstream_done = false;
                        }

                        if(max_items_avail < available) {
                            max_items_avail = available;
                        }
                    }

                    // 检查sink是否应该结束
                    qDebug() << "Sink - 结束标志: "
                             << (input_done? "true":"false") << ","
                             << (all_upstream_done? "true":"false");
                    qDebug() << "Sink - 结束标志 FinalProgress: " << FinalProgress;
                    qDebug() << "Sink - 结束标志 OutputBusCount: " << OutputBusCount;
                    
                    if ((input_done && all_upstream_done) ||
                            ( FinalProgress == 100 && nalive / sinkCount == 1 && OutputBusCount > 0)) {
                        blockDone = true;
                        goto process_sink_done;
                    }
                    else if (max_items_avail < 1) {
                        // 如果没有数据可用，跳过
                        qDebug() << "Sink: no data available";
                        continue;
                    }
                    else {
                        // 执行sink块的工作
                        progressMade = processSinkBlock(currentBlock);
                        if (progressMade) {
                            making_progress = true;
                            BufferReader* reader = currentBlock->GetInputPort(currentBlock->GetInputPortName(0));
                            qDebug() << "sink reader read size: " << reader->GetReadSize();
                            sinkProcessCount[currentBlock->GetName()] += reader->GetReadSize();
                            qDebug() << QString::fromStdString(currentBlock->GetName())
                                     << " Sink collected data, count: " << sinkProcessCount[currentBlock->GetName()];
                        }
                    }

                    process_sink_done:
                    if (blockDone) {
                        currentBlock->SetDone(true);
                        currentBlock->Stop();
                        nalive--;
                        qDebug() << "Block " << QString::fromStdString(currentBlock->GetName()) << " marked as done";
                        sinkCount--;
                        break;
                    }
                }
                else { // PROCESSOR
                    // 检查输入是否就绪
                    bool input_ready = true;
                    bool upstream_done = false;
                    bool hasActiveInputPorts = false;
                    bool hasConnectedPorts = false;       // 是否有已连接的输入端口
                    bool allConnectedUpstreamDone = true;  // 所有已连接端口的上游是否完成

                    for (size_t i = 0; i < currentBlock->GetInputPortCount(); i++) {
                        std::string portName = currentBlock->GetInputPortName(i);
                        qDebug() << "currentBlock Port: " << QString::fromStdString(portName);
                        BufferReader* reader = currentBlock->GetInputPort(portName);
                        bool isConnected = reader->HasValidConnection();
                        bool isBusType = reader->IsBusType(reader->GetDataType());

                        // 非总线类型且无有效连接：完全跳过
                        if(!isConnected && !isBusType) {
                            qDebug() << "Checking currentBlock Port: false";
                            continue;
                        }

                        // 总线类型但无实际连接（未连接的 bus 端口）：跳过
                        if(isBusType && reader->GetBusConnections().empty()) {
                            continue;
                        }

                        hasActiveInputPorts = true;
                        hasConnectedPorts = true;

                        if (!reader->HasDataAvailable()) {
                            input_ready = false;
                            //判断bus类型是否应该退出
                            if(isBusType) {
                                bool all_done = true;
                                for(auto busreader : reader->GetBusConnections()) {
                                    all_done &= busreader.bridgeReader->IsUpstreamDone();
                                    qDebug() << "busreader.bridgeReader: "
                                             << QString::fromStdString(busreader.bridgeReader->GetName());
                                    qDebug() << "all_done: " << (all_done?"true":"false");
                                    qDebug() << "busreader: " << (busreader.bridgeReader->IsUpstreamDone()?"true":"false");
                                }
                                upstream_done = all_done;
                                allConnectedUpstreamDone &= all_done;
                            }
                            else {
                                if (reader->IsUpstreamDone()) {
                                    upstream_done = true;
                                    // allConnectedUpstreamDone &= true (不变)
                                } else {
                                    allConnectedUpstreamDone = false;
                                }
                            }
                        } else {
                            // 有数据可用，但上游可能仍在产生数据
                            if(isBusType) {
                                bool all_done = true;
                                for(auto busreader : reader->GetBusConnections()) {
                                    all_done &= busreader.bridgeReader->IsUpstreamDone();
                                }
                                allConnectedUpstreamDone &= all_done;
                            } else {
                                if (!reader->IsUpstreamDone()) {
                                    allConnectedUpstreamDone = false;
                                }
                            }
                        }
                    }

                    // 如果没有已连接的端口，allConnectedUpstreamDone 不应为 true
                    if (!hasConnectedPorts) {
                        allConnectedUpstreamDone = false;
                    }

                    if (!hasActiveInputPorts) {
                        noProgressCount++;
                        continue;
                    }

                    // 检查输出空间
                    size_t AllOutPortsfreeSpace = 0;
                    for(size_t i = 0; i < currentBlock->GetOutputPortCount(); i++) {
                        std::string portName = currentBlock->GetOutputPortName(i);
                        Buffer* buffer = currentBlock->GetOutputPort(currentBlock->GetOutputPortName(i));
                        if(buffer->GetReaderCount() == 0) {
                            AllOutPortsfreeSpace += 1;
                        } else {
                            size_t OutputPortfreeSpace = currentBlock->GetOutputPort(portName)->GetBufferFreeSpace();
                            AllOutPortsfreeSpace += OutputPortfreeSpace;
                        }
                    }
                    qDebug() << "AllOutPortsfreeSpace: " << AllOutPortsfreeSpace;

                    bool output_ready = true;
                    for (size_t i = 0; i < currentBlock->GetOutputPortCount(); i++) {
                        std::string portName = currentBlock->GetOutputPortName(i);
                        if(currentBlock->GetOutputPort(portName)->GetReaderCount() == 0) {
                            continue;
                        }
                        size_t freeSpace = currentBlock->GetOutputPort(portName)->GetBufferFreeSpace();
                        if (freeSpace <= 0) {
                            output_ready = false;
                            break;
                        }
                    }

                    qDebug() << "upstream_done: " << upstream_done
                             << " allConnectedUpstreamDone: " << allConnectedUpstreamDone;

                    bool hasRunBefore = processedBlocks.count(currentBlock->GetName()) > 0;

                    // 当所有已连接端口上游完成但块从未执行时，强制执行一次
                    // 防止因 input_ready 永远为 false 导致死锁
                    bool forceRun = allConnectedUpstreamDone && !hasRunBefore && output_ready;

                    // 检查processor是否应该结束
                    // 条件：所有已连接端口上游完成 + 有输出空间 + 至少执行过一次
                    if (allConnectedUpstreamDone && AllOutPortsfreeSpace != 0 && hasRunBefore) {
                        blockDone = true;
                    }
                    else if (!forceRun && (!input_ready || !output_ready)) {
                        qDebug() << "Processor not ready - input_ready: " << input_ready
                                 << ", output_ready: " << output_ready;
                        continue;
                    }

                    if (!blockDone) {
                        // 执行processor块的工作
                        progressMade = processProcessorBlock(currentBlock);
                        if (progressMade) {
                            making_progress = true;
                            noProgressCount = 0;
                            processedBlocks.insert(currentBlock->GetName());
                            qDebug() << "Processor processed data";
                        }
                    }
                }
            }

            if (blockDone) {
                currentBlock->SetDone(true);
                currentBlock->Stop();
                nalive--;
                qDebug() << "Block " << QString::fromStdString(currentBlock->GetName()) << " marked as done";
            }

            if (!progressMade && !blockDone) {
                noProgressCount++;
            }
        }

        // 一轮结束后的处理
        made_progress_last_pass = making_progress;
        first_progress = false;
        making_progress = false;

        // 死锁检测
        if (!made_progress_last_pass) {
            if (++noProgressCount > MAX_NO_PROGRESS_ITERATIONS) {
                LOG_ERROR("Deadlock detected, forcing exit");
                break;
            }
        } else {
            noProgressCount = 0;
        }

        // 更新进度
        if (!sinkProcessCount.empty()) {
            double progress = calculateSamplingRateProgress(
                        blocks, sinkProcessCount, maxProcessCount / sourceCount);
            int currentProgress = static_cast<int>(progress);
            if (currentProgress > lastProgress) {
                lastProgress = currentProgress;

                if(currentProgress % 10 == 0) {
                    LOG_DEBUG("当前总进度：", currentProgress, "%");
                    std::cout << "[PROCESS]" << currentProgress << "%" << std::endl;
                    fflush(stdout);
                    
                    lastProgress = currentProgress;
                    if(currentProgress == 100) FinalProgress = currentProgress;
                }
            }
        }
    }

    // 打印最终进度
    double finalRateProgress = calculateSamplingRateProgress(
                blocks, sinkProcessCount, maxProcessCount / sourceCount);
    int finalProgress = static_cast<int>(finalRateProgress);

    if (finalProgress >= 99 || nalive == 0) {
        finalProgress = 100;
    }

    LOG_INFO("最终总进度：", finalProgress, "%");
    std::cout << "[PROCESS]" << finalProgress << "%" << std::endl;

    // 完成处理
    for (Block* block : blocks) {
        if (!block->IsDone()) {
            block->SetDone(true);
            block->Stop();
        }
        block->Done();
    }

    LOG_INFO("调度完成");
    return true;
}

// 以下为辅助函数的实现，与原始SimRunner中的对应函数基本相同
int SimpleScheduler::calculateMaxProcessCount(QVector<Block*> blocks, 
                                             const QString& linkKey, 
                                             int sourceCount)
{
    int maxProcessCount = 0;
    int numSamples = AlgorithmManager::createInstance()->getSimuParameters().value(linkKey).num_Samples;
    int globalMax = numSamples * sourceCount; // 全局上限：源最多产这么多数据

    for (auto block : blocks) {
        if (block->GetBlockType() == Block::BlockType::SINK) {
            std::string option = block->getParameter("StartStopOption").Value;
            std::transform(option.begin(), option.end(), option.begin(), ::tolower);

            int count = 0;
            if (option == "auto") {
                count = globalMax;
            }
            else if (option == "samples") {
                int sampleStart = std::stoi(block->getParameter("SampleStart").Value);
                int sampleStop = std::stoi(block->getParameter("SampleStop").Value);
                count = (sampleStop - sampleStart + 1) * sourceCount;
            }
            else if (option == "time") {
                double timeStart = std::stod(block->getParameter("TimeStart").Value);
                double timeStop = std::stod(block->getParameter("TimeStop").Value);
                double timeInterval = AlgorithmManager::createInstance()->getSimuParameters().value(linkKey).time_Interval;
                count = static_cast<int>((timeStop - timeStart) / timeInterval + 1) * sourceCount;
            }

            if (count > maxProcessCount) {
                maxProcessCount = count;
            }
        }
    }

    // 不超过全局仿真数据量上限
    if (maxProcessCount > globalMax) {
        maxProcessCount = globalMax;
    }

    return maxProcessCount > 0 ? maxProcessCount : 1000;
}

double SimpleScheduler::calculateCumulativeSamplingRate(Block* source, 
                                                        Block* sink, 
                                                        QVector<Block*> blocks)
{
    std::queue<std::pair<Block*, double>> q;
    std::set<std::string> visited;

    q.push({source, 1.0});

    while (!q.empty()) {
        auto [current, currentRatio] = q.front();
        q.pop();

        std::string currentName = current->GetName();

        if (current == sink) {
            return currentRatio;
        }

        if (visited.find(currentName) != visited.end()) {
            continue;
        }
        visited.insert(currentName);

        for (size_t i = 0; i < current->GetOutputPortCount(); i++) {
            std::string portName = current->GetOutputPortName(i);
            Buffer* buffer = current->GetOutputPort(portName);

            if (buffer && buffer->GetReaderCount() > 0) {
                double blockRatio = getBlockProcessingRatio(current);

                std::vector<BufferReader*> readers;
                for (auto block : blocks) {
                    for (size_t i = 0; i < block->GetInputPortCount(); i++) {
                        std::string portName = block->GetInputPortName(i);
                        BufferReader* reader = block->GetInputPort(portName);

                        if (reader && reader->GetConnectedBuffer() == buffer) {
                            readers.push_back(reader);
                        }
                    }
                }

                for (auto reader : readers) {
                    Block* nextBlock = getBlockByReader(reader, blocks);
                    if (nextBlock && visited.find(nextBlock->GetName()) == visited.end()) {
                        q.push({nextBlock, currentRatio * blockRatio});
                    }
                }
            }
        }
    }

    return 1.0;
}

double SimpleScheduler::getBlockProcessingRatio(Block* block)
{
    double ratio = 1.0;

    if(block->GetBlockType() == Block::BlockType::SOURCE) {
        double MaxWriteRatio = 0.0;
        for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
            Buffer* buffer = block->GetOutputPort(block->GetOutputPortName(i));
            double Writeratio = buffer->GetWriteSize();
            if(Writeratio >= MaxWriteRatio) {
                MaxWriteRatio = Writeratio;
            }
        }
        if(MaxWriteRatio != ratio) {
            ratio = MaxWriteRatio;
        }
        return ratio;
    }
    else if(block->GetBlockType() == Block::BlockType::SINK) {
        return ratio;
    }
    else {
        double MaxReadRatio = 0.0;
        for (size_t i = 0; i < block->GetInputPortCount(); i++) {
            BufferReader* reader = block->GetInputPort(block->GetInputPortName(i));
            double Readratio = reader->GetReadSize();
            if(Readratio >= MaxReadRatio) {
                MaxReadRatio = Readratio;
            }
        }

        double MaxWriteRatio = 0.0;
        for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
            Buffer* buffer = block->GetOutputPort(block->GetOutputPortName(i));
            double Writeratio = buffer->GetWriteSize();
            if(Writeratio >= MaxWriteRatio) {
                MaxWriteRatio = Writeratio;
            }
        }

        double CompareRatio = MaxReadRatio / MaxWriteRatio;
        if(ratio != CompareRatio) {
            ratio = CompareRatio;
        }

        return ratio;
    }
    return ratio;
}

Block* SimpleScheduler::getBlockByReader(BufferReader* reader, 
                                         QVector<Block*> blocks)
{
    for (auto block : blocks) {
        for (size_t i = 0; i < block->GetInputPortCount(); i++) {
            std::string portName = block->GetInputPortName(i);
            if (block->GetInputPort(portName) == reader) {
                return block;
            }
        }
    }
    return nullptr;
}

double SimpleScheduler::calculateSamplingRateProgress(QVector<Block*> blocks, 
                                                     const std::map<std::string, int>& sinkProcessCount, 
                                                     int maxProcessCount)
{
    if (sinkProcessCount.empty()) return 0.0;

    std::vector<Block*> sources;
    std::vector<Block*> sinks;

    for (auto block : blocks) {
        if (block->GetBlockType() == Block::BlockType::SOURCE) {
            sources.push_back(block);
        } else if (block->GetBlockType() == Block::BlockType::SINK) {
            sinks.push_back(block);
        }
    }

    if (sources.empty() || sinks.empty()) {
        return 0.0;
    }

    std::map<std::string, double> sinkCumulativeRatios;

    for (auto sink : sinks) {
        double minRatio = std::numeric_limits<double>::max();

        for (auto source : sources) {
            double ratio = calculateCumulativeSamplingRate(source, sink, blocks);
            if (ratio < minRatio) {
                minRatio = ratio;
            }
        }

        sinkCumulativeRatios[sink->GetName()] = minRatio;
        qDebug() << "SINK " << QString::fromStdString(sink->GetName())
                 << " 累积采样率: " << minRatio;
    }

    double totalAdjustedProgress = 0.0;
    int sinkCount = 0;

    for (const auto& pair : sinkProcessCount) {
        std::string sinkName = pair.first;
        int rawCount = pair.second;

        double cumulativeRatio = 1.0;
        if (sinkCumulativeRatios.find(sinkName) != sinkCumulativeRatios.end()) {
            cumulativeRatio = sinkCumulativeRatios[sinkName];
        }

        double adjustedCount = rawCount * cumulativeRatio;
        double progress = (adjustedCount / maxProcessCount) * 100.0;

        totalAdjustedProgress += std::min(progress, 100.0);
        sinkCount++;
    }

    double avgProgress = (sinkCount > 0) ? (totalAdjustedProgress / sinkCount) : 0.0;
    return avgProgress;
}

double SimpleScheduler::calculateSingleSinkProgress(QVector<Block*> blocks, 
                                                   const std::string& sinkName, 
                                                   int sinkCount, 
                                                   int maxProcessCount)
{
    Block* sinkBlock = nullptr;
    for (auto block : blocks) {
        if (block->GetBlockType() == Block::BlockType::SINK &&
                block->GetName() == sinkName) {
            sinkBlock = block;
            break;
        }
    }

    if (!sinkBlock) return 0.0;

    std::vector<Block*> sources;
    for (auto block : blocks) {
        if (block->GetBlockType() == Block::BlockType::SOURCE) {
            sources.push_back(block);
        }
    }

    if (sources.empty()) return 0.0;

    double minCumulativeRatio = std::numeric_limits<double>::max();

    for (auto source : sources) {
        double ratio = calculateCumulativeSamplingRate(source, sinkBlock, blocks);
        if (ratio < minCumulativeRatio) {
            minCumulativeRatio = ratio;
        }
    }

    double adjustedCount = sinkCount * minCumulativeRatio;
    double progress = (adjustedCount / maxProcessCount) * 100.0;

    return std::max(0.0, std::min(progress, 100.0));
}

bool SimpleScheduler::processSourceBlock(Block* block, 
                                         int& processCount, 
                                         int maxProcessCount)
{
    size_t allOutPortsFreeSpace = 0;
    for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
        std::string portName = block->GetOutputPortName(i);
        Buffer* buffer = block->GetOutputPort(portName);

        if (buffer->GetReaderCount() == 0) {
            allOutPortsFreeSpace++;
        } else {
            allOutPortsFreeSpace += buffer->GetBufferFreeSpace();
        }
    }

    if (processCount >= maxProcessCount && allOutPortsFreeSpace != 0) {
        block->SetDone(true);
        block->Stop();
        return false;
    }

    bool outputReady = true;
    for (size_t i = 0; i < block->GetOutputPortCount(); i++) {
        std::string portName = block->GetOutputPortName(i);
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
        return false;
    }

    int result = generalWork(block);
    if (result > 0) {
        processCount += result;
        return true;
    }

    return false;
}

bool SimpleScheduler::processSinkBlock(Block* block)
{
    bool inputDone = false;
    bool allUpstreamDone = true;
    int maxItemsAvail = 0;

    for (size_t i = 0; i < block->GetInputPortCount(); i++) {
        std::string portName = block->GetInputPortName(i);
        BufferReader* reader = block->GetInputPort(portName);

        size_t available = reader->GetAvailableDataCount();

        if (available < 1) {
            if (reader->IsUpstreamDone()) {
                inputDone = true;
            } else {
                inputDone = false;
            }
        } else {
            inputDone = false;
        }

        if (!reader->IsUpstreamDone()) {
            allUpstreamDone = false;
        }

        if (maxItemsAvail < available) {
            maxItemsAvail = available;
        }
    }

    if (inputDone && allUpstreamDone) {
        block->SetDone(true);
        block->Stop();
        return false;
    }

    if (maxItemsAvail < 1) {
        return false;
    }

    if(generalWork(block) < 0) return false;

    return true;
}

bool SimpleScheduler::processProcessorBlock(Block* block)
{
    int result = generalWork(block);
    if (result > 0) {
        return true;
    }

    return false;
}

int SimpleScheduler::generalWork(Block* currentBlock)
{
    if (!currentBlock || currentBlock->IsDone()) {
        return -1;
    }
    
    if(currentBlock->Run()) {
        if(currentBlock->GetBlockType() == Block::BlockType::SOURCE) {
            std::map<std::string, Buffer *> outports = currentBlock->GetOutputPorts();
            int generate_num = 1;
            for( auto it = outports.begin() ; it != outports.end() ; ++it) {
                size_t WriteSize = it->second->GetWriteSize();
                generate_num = std::max(generate_num, static_cast<int>(WriteSize));
            }
            return generate_num;
        }
        return 1;
    }
    else {
        return 0;
    }
    return 0;
}
