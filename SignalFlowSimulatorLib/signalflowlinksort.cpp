#include "signalflowlinksort.h"
#include <algorithm>

SignalFlowLinkSort::SignalFlowLinkSort()
{
}

QVector<Block*> SignalFlowLinkSort::sortProcessorsCrossLayer(
    const QString& mainLinkKey,
    const QMap<QString, QVector<BlockInfo>>& allBlocksInfo,
    const QMap<QString, QVector<Connection>>& allConnections)
{
    // 清空之前的数据
    m_globalBlocks.clear();
    m_processorGlobalIds.clear();
    m_lastTraceInfo.clear();

    qDebug() << "\n=== 开始跨层级处理器拓扑排序 ===";
    qDebug() << "主链路:" << mainLinkKey;

    // 第一步：收集所有原子块
    collectAllAtomicBlocks(mainLinkKey, allBlocksInfo);

    qDebug() << "共收集到" << m_globalBlocks.size() << "个原子块，"
             << "处理器:" << m_processorGlobalIds.size();

    // 第二步：构建跨层级的依赖关系
    buildCrossLayerDependencies(mainLinkKey, allBlocksInfo, allConnections);

    // 第三步：对处理器进行拓扑排序
    QVector<QString> sortedIds = topologicalSortProcessors();

    // 转换为Block指针
    QVector<Block*> sortedProcessors;
    for (const QString& gid : sortedIds) {
        if (m_globalBlocks.contains(gid) && m_globalBlocks[gid].block != nullptr) {
            sortedProcessors.append(m_globalBlocks[gid].block);
        }
    }

    // 调试输出
    qDebug() << "处理器排序结果（共" << sortedProcessors.size() << "个）：";
    for (Block* block : sortedProcessors) {
        qDebug() << "  " << QString::fromStdString(block->GetName());
    }

    return sortedProcessors;
}

void SignalFlowLinkSort::collectAllAtomicBlocks(
    const QString& linkKey,
    const QMap<QString, QVector<BlockInfo>>& allBlocksInfo)
{
    auto blocksInfo = allBlocksInfo.value(linkKey);

    for (const BlockInfo& info : blocksInfo) {
        if (info.isSubSystem && !info.childTopoId.isEmpty()) {
            // 递归处理子系统
            collectAllAtomicBlocks(info.childTopoId, allBlocksInfo);
        } else if (info.block != nullptr) {
            QString globalId = linkKey + ":" + QString::number(info.cmpId);

            CrossLayerBlockNode node(
                globalId,
                linkKey,
                info.cmpId,
                info.instanceName,
                info.block->GetBlockType(),
                info.block
            );

            m_globalBlocks[globalId] = node;

            if (node.type == Block::BlockType::PROCESSOR) {
                m_processorGlobalIds.insert(globalId);
                qDebug() << "处理器:" << info.instanceName << "全局ID:" << globalId;
            }
        }
    }
}

void SignalFlowLinkSort::buildCrossLayerDependencies(
    const QString& linkKey,
    const QMap<QString, QVector<BlockInfo>>& allBlocksInfo,
    const QMap<QString, QVector<Connection>>& allConnections)
{
    auto blocksInfo = allBlocksInfo.value(linkKey);
    auto connections = allConnections.value(linkKey);

    // 处理当前层级的所有连接
    for (const Connection& conn : connections) {
        processConnectionForDependencies(conn, linkKey, allBlocksInfo, allConnections);
    }

    // 递归处理子系统
    for (const BlockInfo& info : blocksInfo) {
        if (info.isSubSystem && !info.childTopoId.isEmpty()) {
            buildCrossLayerDependencies(info.childTopoId, allBlocksInfo, allConnections);
        }
    }
}

void SignalFlowLinkSort::processConnectionForDependencies(
    const Connection& conn,
    const QString& currentLinkKey,
    const QMap<QString, QVector<BlockInfo>>& allBlocksInfo,
    const QMap<QString, QVector<Connection>>& allConnections)
{
    bool bValid = false;
    int srcId = conn.fromModelId().toInt(&bValid);
    int srcPort = conn.fromPort().toInt(&bValid);
    int dstId = conn.toModelId().toInt(&bValid);
    int dstPort = conn.toPort().toInt(&bValid);

    if (!bValid) return;

    auto blocksInfo = allBlocksInfo.value(currentLinkKey);

    // 查找源和目标块信息
    BlockInfo srcInfo, dstInfo;
    PortMsg srcPortMsg, dstPortMsg;

    for (const BlockInfo& block : blocksInfo) {
        if (block.cmpId == srcId && block.portsMsg.contains(srcPort)) {
            srcInfo = block;
            srcPortMsg = block.portsMsg[srcPort];
        }
        if (block.cmpId == dstId && block.portsMsg.contains(dstPort)) {
            dstInfo = block;
            dstPortMsg = block.portsMsg[dstPort];
        }
    }

    if (srcInfo.cmpId == 0 || dstInfo.cmpId == 0) return;

    // 只处理 OUT -> IN 的连接
    if (srcPortMsg.putType != "out" || dstPortMsg.putType != "in") return;

    qDebug() << "\n处理连接:" << currentLinkKey
             << srcInfo.instanceName << ":" << srcPortMsg.name
             << "(isSubSystem:" << srcInfo.isSubSystem << ")"
             << "->"
             << dstInfo.instanceName << ":" << dstPortMsg.name
             << "(isSubSystem:" << dstInfo.isSubSystem << ")";

    // 获取实际的源块和目标块（穿透子系统）
    QVector<QString> actualSrcs, actualDsts;

    // 如果源是子系统，需要穿透找到内部的源
    if (srcInfo.isSubSystem) {
        actualSrcs = traceToAtomicBlocks(srcInfo, srcPortMsg, true,
                                         currentLinkKey, allBlocksInfo, allConnections);
    } else {
        actualSrcs.append(currentLinkKey + ":" + QString::number(srcInfo.cmpId));
    }

    // 如果目标是子系统，需要穿透找到内部的目标
    if (dstInfo.isSubSystem) {
        actualDsts = traceToAtomicBlocks(dstInfo, dstPortMsg, false,
                                         currentLinkKey, allBlocksInfo, allConnections);
    } else {
        actualDsts.append(currentLinkKey + ":" + QString::number(dstInfo.cmpId));
    }

//    qDebug() << "  实际源块:" << actualSrcs;
//    qDebug() << "  实际目标块:" << actualDsts;

    // 记录输入输出关系
    for (const QString& src : actualSrcs) {
        for (const QString& dst : actualDsts) {
            if (m_globalBlocks.contains(src) && m_globalBlocks.contains(dst)) {
                // 确保不添加自依赖
                if (src != dst) {
                    m_globalBlocks[src].outputs.insert(dst);
                    m_globalBlocks[dst].inputs.insert(src);
                    qDebug() << "  建立依赖:" << m_globalBlocks[src].name << "(" << src << ") -> "
                             << m_globalBlocks[dst].name << "(" << dst << ")";
                }
            }
        }
    }
}

QVector<QString> SignalFlowLinkSort::traceToAtomicBlocks(
    const BlockInfo& blockInfo,
    const PortMsg& port,
    bool isSource,
    const QString& currentLinkKey,
    const QMap<QString, QVector<BlockInfo>>& allBlocksInfo,
    const QMap<QString, QVector<Connection>>& allConnections)
{
    QVector<QString> result;

    // 如果当前块不是子系统，直接返回
    if (!blockInfo.isSubSystem) {
        QString globalId = currentLinkKey + ":" + QString::number(blockInfo.cmpId);
        result.append(globalId);
        return result;
    }

    // 处理子系统
    QString subLinkKey = blockInfo.childTopoId;
    auto subConns = allConnections.value(subLinkKey);
    auto subBlocks = allBlocksInfo.value(subLinkKey);

    QString traceInfo = QString("穿透子系统:%1 当前端口:%2 端口ID:%3 isSource:%4")
                        .arg(blockInfo.instanceName).arg(port.name).arg(port.id).arg(isSource);
    m_lastTraceInfo.append(traceInfo);
//    qDebug() << traceInfo;

    // 查找与当前端口ID匹配的出入口模型
    QString matchedPortBlockId;
    QString matchedPortName;
    int matchedPortId = -1;
    BlockInfo matchedPortBlockInfo;

    for (const BlockInfo& subBlock : subBlocks) {
        // 检查这个子块是否是出入口模型（通常类型包含"Port"）
        if (subBlock.cmpType.contains("Port") || subBlock.instanceName.contains("Port")) {
            for (auto it = subBlock.portsMsg.begin(); it != subBlock.portsMsg.end(); ++it) {
                const PortMsg& subPort = it.value();
                // 如果这个端口的topProtId等于当前端口的ID，说明这就是对应的出入口
                if (subPort.topProtId == port.id) {
                    traceInfo = QString("  找到出入口模型:%1 端口:%2 topProtId:%3 匹配端口ID:%4")
                                .arg(subBlock.instanceName).arg(subPort.name).arg(subPort.topProtId).arg(port.id);
                    m_lastTraceInfo.append(traceInfo);
//                    qDebug() << traceInfo;

                    matchedPortBlockId = subLinkKey + ":" + QString::number(subBlock.cmpId);
                    matchedPortName = subBlock.instanceName;
                    matchedPortId = subPort.id;
                    matchedPortBlockInfo = subBlock;
                    break;
                }
            }
        }
        if (!matchedPortBlockId.isEmpty()) break;
    }

    if (isSource) {
        // 作为源端：需要找到从子系统输出的真正源块
        if (!matchedPortBlockId.isEmpty()) {
            // 找到了对应的出入口模型，现在需要找到谁连接到了这个出入口模型
            traceInfo = QString("  查找连接到出入口模型%1的源块").arg(matchedPortName);
            m_lastTraceInfo.append(traceInfo);
//            qDebug() << traceInfo;

            bool foundSource = false;
            for (const Connection& conn : subConns) {
                bool bValid = false;
                int srcId = conn.fromModelId().toInt(&bValid);
                int srcPort = conn.fromPort().toInt(&bValid);
                int dstId = conn.toModelId().toInt(&bValid);
                int dstPort = conn.toPort().toInt(&bValid);

                if (!bValid) continue;

                // 查找连接到出入口模型（作为目标）的连接
                if (dstId == matchedPortBlockInfo.cmpId && dstPort == matchedPortId) {
                    // 找到了源块
                    for (const BlockInfo& sourceBlock : subBlocks) {
                        if (sourceBlock.cmpId == srcId && sourceBlock.portsMsg.contains(srcPort)) {
                            PortMsg sourcePort = sourceBlock.portsMsg[srcPort];
                            traceInfo = QString("    找到源块:%1 端口:%2")
                                        .arg(sourceBlock.instanceName).arg(sourcePort.name);
                            m_lastTraceInfo.append(traceInfo);
//                            qDebug() << traceInfo;

                            // 递归追踪源块
                            QVector<QString> deeper = traceToAtomicBlocks(
                                sourceBlock, sourcePort, true,
                                subLinkKey, allBlocksInfo, allConnections);
                            result.append(deeper);
                            foundSource = true;
                        }
                    }
                }
            }

            // 如果没有找到连接到出入口模型的源，说明出入口模型本身就是源
            if (!foundSource) {
                traceInfo = QString("    出入口模型本身作为源");
                m_lastTraceInfo.append(traceInfo);
                qDebug() << traceInfo;
                result.append(matchedPortBlockId);
            }
        } else {
            // 没有找到出入口模型，尝试查找直接连接
            for (const Connection& conn : subConns) {
                bool bValid = false;
                int srcId = conn.fromModelId().toInt(&bValid);
                int srcPort = conn.fromPort().toInt(&bValid);
                int dstId = conn.toModelId().toInt(&bValid);
                int dstPort = conn.toPort().toInt(&bValid);

                if (!bValid) continue;

                if (srcId == blockInfo.cmpId && srcPort == port.id) {
                    for (const BlockInfo& subBlock : subBlocks) {
                        if (subBlock.cmpId == dstId && subBlock.portsMsg.contains(dstPort)) {
                            PortMsg subPort = subBlock.portsMsg[dstPort];
                            QVector<QString> deeper = traceToAtomicBlocks(
                                subBlock, subPort, false,
                                subLinkKey, allBlocksInfo, allConnections);
                            result.append(deeper);
                        }
                    }
                }
            }
        }
    } else {
        // 作为目标端：需要找到子系统内部接收输出的真正目标块
        if (!matchedPortBlockId.isEmpty()) {
            // 找到了对应的出入口模型，现在需要找到这个出入口模型输出到谁
            traceInfo = QString("  查找出入口模型%1输出的目标块").arg(matchedPortName);
            m_lastTraceInfo.append(traceInfo);
            qDebug() << traceInfo;

            bool foundTarget = false;
            for (const Connection& conn : subConns) {
                bool bValid = false;
                int srcId = conn.fromModelId().toInt(&bValid);
                int srcPort = conn.fromPort().toInt(&bValid);
                int dstId = conn.toModelId().toInt(&bValid);
                int dstPort = conn.toPort().toInt(&bValid);

                if (!bValid) continue;

                // 查找出入口模型（作为源）输出的连接
                if (srcId == matchedPortBlockInfo.cmpId && srcPort == matchedPortId) {
                    // 找到了目标块
                    for (const BlockInfo& targetBlock : subBlocks) {
                        if (targetBlock.cmpId == dstId && targetBlock.portsMsg.contains(dstPort)) {
                            PortMsg targetPort = targetBlock.portsMsg[dstPort];
                            traceInfo = QString("    找到目标块:%1 端口:%2")
                                        .arg(targetBlock.instanceName).arg(targetPort.name);
                            m_lastTraceInfo.append(traceInfo);
                            qDebug() << traceInfo;

                            // 递归追踪目标块
                            QVector<QString> deeper = traceToAtomicBlocks(
                                targetBlock, targetPort, false,
                                subLinkKey, allBlocksInfo, allConnections);
                            result.append(deeper);
                            foundTarget = true;
                        }
                    }
                }
            }

            // 如果没有找到出入口模型输出的目标，说明出入口模型本身就是目标
            if (!foundTarget) {
                traceInfo = QString("    出入口模型本身作为目标");
                m_lastTraceInfo.append(traceInfo);
                qDebug() << traceInfo;
                result.append(matchedPortBlockId);
            }
        } else {
            // 没有找到出入口模型，尝试查找直接连接
            for (const Connection& conn : subConns) {
                bool bValid = false;
                int srcId = conn.fromModelId().toInt(&bValid);
                int srcPort = conn.fromPort().toInt(&bValid);
                int dstId = conn.toModelId().toInt(&bValid);
                int dstPort = conn.toPort().toInt(&bValid);

                if (!bValid) continue;

                if (dstId == blockInfo.cmpId && dstPort == port.id) {
                    for (const BlockInfo& subBlock : subBlocks) {
                        if (subBlock.cmpId == srcId && subBlock.portsMsg.contains(srcPort)) {
                            PortMsg subPort = subBlock.portsMsg[srcPort];
                            QVector<QString> deeper = traceToAtomicBlocks(
                                subBlock, subPort, true,
                                subLinkKey, allBlocksInfo, allConnections);
                            result.append(deeper);
                        }
                    }
                }
            }
        }
    }

    // 如果还是没找到，可能需要继续向上层查找
    if (result.isEmpty()) {
        traceInfo = "  警告: 在子系统中未找到对应的原子块";
        m_lastTraceInfo.append(traceInfo);
        qDebug() << traceInfo;
    }

    return result;
}

QVector<QString> SignalFlowLinkSort::topologicalSortProcessors()
{
    QMap<QString, QSet<QString>> dependencyGraph;
    QMap<QString, int> inDegree;

    // 初始化
    for (const QString& gid : m_processorGlobalIds) {
        dependencyGraph[gid] = QSet<QString>();
        inDegree[gid] = 0;
    }

    // 构建处理器之间的依赖关系
    for (const QString& gid : m_processorGlobalIds) {
        for (const QString& output : m_globalBlocks[gid].outputs) {
            if (m_processorGlobalIds.contains(output)) {
                if (!dependencyGraph[gid].contains(output)) {
                    dependencyGraph[gid].insert(output);
//                    qDebug() << "处理器依赖:" << m_globalBlocks[gid].name
//                             << "(" << gid << ") -> "
//                             << m_globalBlocks[output].name
//                             << "(" << output << ")";
                }
            }
        }
    }

    // 计算入度
    for (const QString& gid : m_processorGlobalIds) {
        for (const QString& dep : dependencyGraph[gid]) {
            inDegree[dep]++;
        }
    }

    // 拓扑排序
    QQueue<QString> queue;
    QSet<QString> visited;
    QVector<QString> sortedIds;

    // 找出所有入度为0的处理器
    for (const QString& gid : m_processorGlobalIds) {
        if (inDegree[gid] == 0) {
            queue.enqueue(gid);
            qDebug() << "起始处理器:" << m_globalBlocks[gid].name << "(" << gid << ")";
        }
    }

    while (!queue.isEmpty()) {
        QString currentGid = queue.dequeue();

        if (visited.contains(currentGid)) continue;

        visited.insert(currentGid);
        sortedIds.append(currentGid);

        for (const QString& neighbor : dependencyGraph[currentGid]) {
            if (!m_processorGlobalIds.contains(neighbor)) continue;

            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0 && !visited.contains(neighbor)) {
                queue.enqueue(neighbor);
            }
        }
    }

    // 添加剩余的处理器（处理环状依赖）
    for (const QString& gid : m_processorGlobalIds) {
        if (!visited.contains(gid)) {
            sortedIds.append(gid);
            qDebug() << "添加剩余处理器:" << m_globalBlocks[gid].name << "(" << gid << ")";
        }
    }

    return sortedIds;
}
