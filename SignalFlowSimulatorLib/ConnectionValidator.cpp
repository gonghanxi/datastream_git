#include "ConnectionValidator.h"
#include "dataflowcheck.h"
#include "PortValidatorImpl.h"
#include <stack>

ConnectionValidator::ConnectionValidator(const QVector<BlockInfo> &blocksInfo, const QVector<Connection> &connections, const QString &linkKey)
    : m_blocksInfo(blocksInfo)
    , m_connections(connections)
    , m_linkKey(linkKey)
{
    AlgorithmManager* algoMgr = AlgorithmManager::createInstance();
    if (algoMgr) {
        m_allBlocksInfoMap = algoMgr->getBlocksInfo();
        m_allConnectionsMap = algoMgr->getConnection();

        // 预先加载所有链路的信息到本地存储
        for (auto it = m_allBlocksInfoMap.begin(); it != m_allBlocksInfoMap.end(); ++it) {
            const QString& currentLinkKey = it.key();
            const QVector<BlockInfo>& blocksInfo = it.value();

            // 存储BlockInfo副本
            m_linkKeyBlocksMap[currentLinkKey] = blocksInfo;

            // 建立blockId到linkKey的映射
            for (const BlockInfo& blockInfo : blocksInfo) {
                m_blockIdToLinkKeyMap[blockInfo.cmpId] = currentLinkKey;
            }

            // 构建端口映射
            QMap<int, PortInfo> portsMap;
            for (const BlockInfo& blockInfo : blocksInfo) {
                for (auto portIt = blockInfo.portsMsg.begin(); portIt != blockInfo.portsMsg.end(); ++portIt) {
                    int portId = portIt.key();
                    const PortMsg& portMsg = portIt.value();

                    PortInfo portInfo;
                    portInfo.portId = portId;
                    portInfo.instanceName = blockInfo.instanceName;
                    portInfo.portName = portMsg.name;
                    portInfo.putType = portMsg.putType;
                    portInfo.dataType = portMsg.dataType;
                    portInfo.topProtId = portMsg.topProtId;
                    portInfo.isBusPort = isBusDataType(portMsg.dataType);
                    portInfo.isOptional = portMsg.isOptional;

                    int compositeKey = blockInfo.cmpId * 1000 + portId;
                    portsMap[compositeKey] = portInfo;
                }
            }
            m_linkKeyPortsMap[currentLinkKey] = portsMap;
        }
    }
    qDebug() << "m_allBlocksInfoMap: " << m_allBlocksInfoMap.size();
    qDebug() << "m_allConnectionsMap: " << m_allConnectionsMap.size();

    initializeDataStructures();

    //实现类指针初始化
    m_porter = std::make_unique<PortValidatorImpl>(this);
}

ConnectionValidator::~ConnectionValidator()
{

}

ConnectionValidator::ValidationResult ConnectionValidator::validateAll()
{
    ValidationResult result;

    // 按顺序执行所有校验
    //端口方向校验
    ValidationResult dirResult = validatePortDirection();
    if (!dirResult.isValid) {
        return dirResult;
    }

    //子系统端口穿透校验
    ValidationResult penetratedResult = validatePenetratedDataTypes();
    if (!penetratedResult.isValid) {
        return penetratedResult;
    }

    //端口数据类型校验
    ValidationResult dataTypeResult = validatePortDataType();
    if (!dataTypeResult.isValid) {
        return dataTypeResult;
    }
    LOG_INFO("端口数据类型校验成功");

    //输入端口连接完整性校验
    ValidationResult inputConnResult = validateInputPortConnections();
    if (!inputConnResult.isValid) {
        return inputConnResult;
    }

    // 添加输入端口校验的警告信息
    if (!inputConnResult.warnings.isEmpty()) {
        result.warnings.append(inputConnResult.warnings);
    }

    //Bus关联校验
    ValidationResult busResult = validateBusRequirements();
    if (!busResult.isValid) {
        return busResult;
    }

    //闭环连接校验
    ValidationResult cycleResult = validateCyclicDependencies();
    if (!cycleResult.isValid) {
        return cycleResult;
    }
    LOG_INFO("端口连接匹配性校验成功");

    // 链路完整性校验
    ValidationResult completenessResult = validateLinkCompleteness();
    if (!completenessResult.isValid) {
        return completenessResult;
    }
    LOG_INFO("链路完整性校验成功");

    // 添加链路完整性校验的警告信息
    if (!completenessResult.warnings.isEmpty()) {
        result.warnings.append(completenessResult.warnings);
    }

    return result;
}

ConnectionValidator::ValidationResult ConnectionValidator::validatePortDirection()
{
    return m_porter->validatePortDirection();
}

ConnectionValidator::ValidationResult ConnectionValidator::validatePortDataType()
{
    return m_porter->validatePortDataType();
}

ConnectionValidator::ValidationResult ConnectionValidator::validateBusRequirements()
{
    ValidationResult result(true);
    QList<QString> warnings;

    // 统计每个输入端口的连接数
    QMap<QString, int> inputConnectionCount; // key: blockId_portId

    for (const Connection& conn : m_connections) {
        bool ok3, ok4;
        int dstBlockId = conn.toModelId().toInt(&ok3);
        int dstPortId = conn.toPort().toInt(&ok4);

        if (!ok3 || !ok4) continue;

        PortInfo* dstPort = findPortInfo(dstBlockId, dstPortId);
        if (!dstPort) continue;

        // 只统计输入端口
        if (dstPort->putType == "in") {
            QString key = QString("%1_%2").arg(dstBlockId).arg(dstPortId);
            inputConnectionCount[key] = inputConnectionCount.value(key, 0) + 1;
        }
    }

    // 检查Bus要求
    for (auto it = inputConnectionCount.begin(); it != inputConnectionCount.end(); ++it) {
        QStringList parts = it.key().split('_');
        if (parts.size() != 2) continue;

        int blockId = parts[0].toInt();
        int portId = parts[1].toInt();

        PortInfo* port = findPortInfo(blockId, portId);
        if (!port) continue;

        int connectionCount = it.value();

        // 规则：非Bus端口只能有一个连接
        if (!port->isBusPort && connectionCount > 1) {
            BlockInfo* block = findBlockInfo(blockId);
            QString blockName = block ? block->instanceName : QString::number(blockId);

            result.isValid = false;
            result.errorMessage = QString("非Bus端口有多个连接: %1(cmpId: cp_%2)的端口 %3 有 %4 个连接。非Bus端口只能有一个输入连接")
                    .arg(blockName).arg(block->cmpId).arg(port->portName).arg(connectionCount);
        }

        // 规则：Bus端口应该有多个连接（警告）
        if (port->isBusPort && connectionCount <= 1) {
            BlockInfo* block = findBlockInfo(blockId);
            QString blockName = block ? block->instanceName : QString::number(blockId);

            warnings.append(QString("Bus端口连接数较少: %1(%2) 只有 %3 个连接。Bus端口通常用于多路信号")
                            .arg(blockName).arg(port->portName).arg(connectionCount));
        }
    }

    // 添加警告信息
    if (!warnings.isEmpty()) {
        result.warnings.append(warnings);
    }

    return result;
}

ConnectionValidator::ValidationResult ConnectionValidator::validateCyclicDependencies()
{
    ValidationResult result(true);

    // 使用Kahn算法进行拓扑排序并检测环
    QMap<QString, int> inDegree = m_inDegree;
    QList<QString> queue;

    // 找到所有入度为0的节点
    for (auto it = inDegree.begin(); it != inDegree.end(); ++it) {
        if (it.value() == 0) {
            queue.append(it.key());
        }
    }

    QList<QString> topologicalOrder;
    int visitedCount = 0;

    while (!queue.isEmpty()) {
        QString nodeId = queue.takeFirst();
        topologicalOrder.append(nodeId);

        if (m_dependencyGraph.contains(nodeId)) {
            for (const QString& neighbor : m_dependencyGraph[nodeId]) {
                inDegree[neighbor]--;
                if (inDegree[neighbor] == 0) {
                    queue.append(neighbor);
                }
            }
        }
        visitedCount++;
    }

    // 检查是否有环
    if (visitedCount != m_dependencyGraph.size()) {
        // 存在环，找到环中的节点
        QSet<QString> cycleNodes;
        for (auto it = inDegree.begin(); it != inDegree.end(); ++it) {
            if (it.value() > 0) {
                cycleNodes.insert(it.key());
            }
        }

        // 构建环的路径
        QStringList cyclePath;
        if (!cycleNodes.isEmpty()) {
            QString startNode = *cycleNodes.begin();
            QSet<QString> visited;
            QSet<QString> recursionStack;

            // 使用DFS找到环
            std::function<bool(const QString&, QList<QString>&)> findCycle =
                    [&](const QString& currentNode, QList<QString>& path) -> bool {
                if (!m_dependencyGraph.contains(currentNode)) return false;

                visited.insert(currentNode);
                path.append(currentNode);

                for (const QString& neighbor : m_dependencyGraph[currentNode]) {
                    if (cycleNodes.contains(neighbor)) {
                        if (!visited.contains(neighbor)) {
                            if (findCycle(neighbor, path)) {
                                return true;
                            }
                        } else if (path.contains(neighbor)) {
                            // 找到环
                            int startIdx = path.indexOf(neighbor);
                            QList<QString> cycle = path.mid(startIdx);
                            cyclePath = QStringList(cycle);
                            return true;
                        }
                    }
                }

                path.removeLast();
                return false;
            };

            QList<QString> path;
            findCycle(startNode, path);
        }

        result.isValid = false;
        if (!cyclePath.isEmpty()) {
            result.errorMessage = QString("发现闭环连接: %1 -> ... -> %1")
                    .arg(cyclePath.join(" -> "));
        } else {
            result.errorMessage = QString("存在闭环依赖关系，无法进行拓扑排序");
        }
    } else {
        // 保存拓扑排序结果
        m_topologicalOrder = topologicalOrder;

        // 检查模型是否连接到自身（直接自环）
        for (const Connection& conn : m_connections) {
            if (conn.fromModelId() == conn.toModelId()) {
                result.isValid = false;
                result.errorMessage = QString("模型 %1 (cmpId: cp_%2)连接到自身，形成自环")
                        .arg(conn.fromModelId()).arg(conn.fromModelId());
                break;
            }
        }
    }

    return result;
}

ConnectionValidator::ValidationResult ConnectionValidator::validateLinkCompleteness()
{
    ValidationResult result(true);
    QList<QString> warnings;
    QList<QString> errors;  // 存储错误信息

    qDebug() << "=== 开始链路完整性校验 ===";

    // 定义数据收集器类型
    QSet<QString> dataCollectionTypes = {"Sink", "SinkCx", "SinkEnv"};

    // 1. 构建真实的数据流向图（纠正反向连接）
    QMap<QString, QList<QString>> realFlowGraph; // 真实数据流向：src -> [dsts]
    QMap<QString, QList<QString>> reverseFlowGraph; // 反向图：dst -> [srcs]

    // 初始化
    for (const BlockInfo& blockInfo : m_blocksInfo) {
        QString blockId = QString::number(blockInfo.cmpId);
        realFlowGraph[blockId] = QList<QString>();
        reverseFlowGraph[blockId] = QList<QString>();
    }

    // 构建真实的数据流向
    for (const Connection& conn : m_connections) {
        bool ok1, ok2, ok3, ok4;
        int blockId1 = conn.fromModelId().toInt(&ok1);
        int portId1 = conn.fromPort().toInt(&ok2);
        int blockId2 = conn.toModelId().toInt(&ok3);
        int portId2 = conn.toPort().toInt(&ok4);

        if (!ok1 || !ok2 || !ok3 || !ok4) continue;

        PortInfo* port1 = findPortInfo(blockId1, portId1);
        PortInfo* port2 = findPortInfo(blockId2, portId2);

        if (!port1 || !port2) continue;

        QString realSrcId, realDstId;

        // 确定真实的数据流向
        if (port1->putType == "out" && port2->putType == "in") {
            // 正常方向
            realSrcId = conn.fromModelId();
            realDstId = conn.toModelId();
        } else if (port1->putType == "in" && port2->putType == "out") {
            // 反向连接
            realSrcId = conn.toModelId();
            realDstId = conn.fromModelId();
        } else {
            // 同向连接，跳过
            continue;
        }

        realFlowGraph[realSrcId].append(realDstId);
        reverseFlowGraph[realDstId].append(realSrcId);

        qDebug() << "真实数据流向:" << realSrcId << "->" << realDstId
                 << "[原始连接:" << conn.fromModelId() << "->" << conn.toModelId() << "]";
    }

    // 2. 识别起始节点（没有输入连接的节点）作为支路起点
    QList<QString> startNodes;
    for (const BlockInfo& blockInfo : m_blocksInfo) {
        QString blockId = QString::number(blockInfo.cmpId);

        // 没有输入连接或者所有输入连接来自自身（自环）
        bool hasInput = false;
        if (reverseFlowGraph.contains(blockId)) {
            for (const QString& srcId : reverseFlowGraph[blockId]) {
                if (srcId != blockId) { // 排除自环
                    hasInput = true;
                    break;
                }
            }
        }

        if (!hasInput) {
            startNodes.append(blockId);
            qDebug() << "真实起点:" << blockId << "("
                     << blockInfo.instanceName << ")";
        }
    }

    // 3. 从每个起始节点开始，识别支路（使用深度优先搜索）
    QList<QSet<QString>> branches; // 存储每条支路的节点集合
    QSet<QString> allVisited;
    QList<QString> isolatedBlocks; // 存储孤立块

    for (const QString& startNode : startNodes) {
        if (allVisited.contains(startNode)) {
            continue;
        }

        // 使用深度优先搜索（DFS）找出所有从起始节点出发的完整路径
        QList<QList<QString>> allPaths;
        QList<QString> currentPath;

        std::function<void(const QString&)> dfs = [&](const QString& currentNode) {
            currentPath.append(currentNode);

            // 如果当前节点是终点（没有下游），保存这条路径
            if (!realFlowGraph.contains(currentNode) || realFlowGraph[currentNode].isEmpty()) {
                allPaths.append(currentPath);
            } else {
                // 继续向下游搜索
                for (const QString& neighbor : realFlowGraph[currentNode]) {
                    dfs(neighbor);
                }
            }

            currentPath.removeLast();
        };

        dfs(startNode);

        // 将每条路径作为独立的支路
        for (const QList<QString>& path : allPaths) {
            qDebug() << "path:" << path;
            QSet<QString> branch;
            for (const QString& nodeId : path) {
                branch.insert(nodeId);
                allVisited.insert(nodeId);
            }

            if (!branch.isEmpty()) {
                branches.append(branch);

                // 调试输出
                QStringList branchNames;
                for (const QString& blockId : branch) {
                    BlockInfo* block = findBlockInfo(blockId.toInt());
                    if (block) {
                        branchNames.append(block->instanceName);
                    }
                }
                qDebug() << "识别到支路:" << branchNames.join(" → ");
                // 如果这条支路只有一个节点，且不是子系统，则视为孤立块
                if (branch.size() == 1) {
                    QString blockId = *branch.begin();
                    BlockInfo* block = findBlockInfo(blockId.toInt());
                    if (block && !block->isSubSystem) {
                        isolatedBlocks.append(block->instanceName);
                        qDebug() << "识别到孤立块:" << block->instanceName;
                    }
                }
            }
            qDebug() << "branches: " << branches.size();
        }
    }

    // 4. 处理剩余的孤立节点（没有连接关系的节点）

    for (const BlockInfo& blockInfo : m_blocksInfo) {
        QString blockId = QString::number(blockInfo.cmpId);
        if (!allVisited.contains(blockId) && !blockInfo.isSubSystem) {
            QSet<QString> isolatedBranch;
            isolatedBranch.insert(blockId);
            branches.append(isolatedBranch);
            allVisited.insert(blockId);
            isolatedBlocks.append(blockInfo.instanceName); // 记录孤立块名称

            qDebug() << "孤立节点支路:" << blockId << "("
                     << blockInfo.instanceName << ")";
        }
    }

    // 5. 分析每条支路的完整性
    int completeBranches = 0;
    int incompleteBranches = 0;

    qDebug() << "=== 支路完整性分析 ===";
    qDebug() << "共识别到" << branches.size() << "条支路";

    errors.clear();
    warnings.clear();

    for (int i = 0; i < branches.size(); i++) {
        const QSet<QString>& branch = branches[i];

        bool hasSource = false;
        bool hasSink = false;
        QStringList sourceNames;
        QStringList sinkNames;
        QStringList branchNames;
        BlockInfo* questionblockInfo = nullptr;

        for (const QString& blockId : branch) {
            BlockInfo* blockInfo = findBlockInfo(blockId.toInt());

            if (!blockInfo) continue;

            branchNames.append(blockInfo->instanceName);

            // 检查是否是数据源
            bool isSource = false;

            // 1. 普通数据源块
            if (blockInfo->block && blockInfo->block->GetBlockType() == Block::BlockType::SOURCE) {
                isSource = true;
            }
            // 2. 子系统，检查其内部是否有数据源
            else if (blockInfo->isSubSystem) {
                QString childTopoId = blockInfo->childTopoId;
                if (!childTopoId.isEmpty()) {
                    QVector<BlockInfo> subBlocks = getBlocksInfoByLinkKey(childTopoId);
                    for (const BlockInfo& subBlock : subBlocks) {
                        // 跳过inPort/outPort
                        if (subBlock.cmpType == "inPort" || subBlock.cmpType == "outPort") {
                            continue;
                        }

                        // 内部块是数据源
                        if (subBlock.block && subBlock.block->GetBlockType() == Block::BlockType::SOURCE) {
                            isSource = true;
                            break;
                        }
                    }
                }

                if (isSource) {
                    qDebug() << "识别子系统" << blockInfo->instanceName << "包含数据源";
                }
            }

            // 检查是否是数据收集器
            bool isSink = false;

            // 1. 普通数据收集器块
            if (blockInfo->block && blockInfo->block->GetBlockType() == Block::BlockType::SINK) {
                isSink = true;
            }
            // 2. 特定类型的收集器
            if (dataCollectionTypes.contains(blockInfo->cmpType)) {
                isSink = true;
            }
            // 3. 子系统，检查其内部是否有数据收集器
            else if (blockInfo->isSubSystem) {
                QString childTopoId = blockInfo->childTopoId;
                if (!childTopoId.isEmpty()) {
                    QVector<BlockInfo> subBlocks = getBlocksInfoByLinkKey(childTopoId);
                    for (const BlockInfo& subBlock : subBlocks) {
                        // 跳过inPort/outPort
                        if (subBlock.cmpType == "inPort" || subBlock.cmpType == "outPort") {
                            continue;
                        }

                        // 内部块是数据收集器
                        if (subBlock.block && subBlock.block->GetBlockType() == Block::BlockType::SINK) {
                            isSink = true;
                            break;
                        }
                        if (dataCollectionTypes.contains(subBlock.cmpType)) {
                            isSink = true;
                            break;
                        }
                    }
                }

                if (isSink) {
                    qDebug() << "识别子系统" << blockInfo->instanceName << "包含数据收集器";
                }
            }

            // 记录结果
            if (isSource) {
                hasSource = true;
                sourceNames.append(blockInfo->instanceName);
            }
            if (isSink) {
                hasSink = true;
                sinkNames.append(blockInfo->instanceName);
            }
            if(branch.size() <= 1) {
                questionblockInfo = blockInfo;
            }
        }

        qDebug() << "支路" << i + 1 << "[" << branch.size() << "个节点]:"
                 << branchNames.join(" → ");
        qDebug() << "  数据源:" << (hasSource ? sourceNames.join(", ") : "无");
        qDebug() << "  收集器:" << (hasSink ? sinkNames.join(", ") : "无");

        if (hasSource && hasSink) {
            qDebug() << "完整支路";
            completeBranches++;
        } else if (!hasSource && hasSink) {
            //信号源为孤立块，也报错
            if(branch.size() == 1) {
                QString blockName = branchNames.isEmpty() ? "未知" : branchNames.first();
                QString errorMsg;
                if(questionblockInfo) {
                    errorMsg = QString(" '%1' (cmpId: cp_%2)")
                            .arg(blockName).arg(questionblockInfo->cmpId);
                }
                errors.append(errorMsg);
                qDebug() << "错误:" << errorMsg;
            }
            QString warningMsg = QString("支路 %1 有数据收集器(%2)但没有数据源")
                    .arg(i + 1).arg(sinkNames.join(", "));
            warnings.append(warningMsg);
            incompleteBranches++;
            qDebug() << warningMsg;
        } else if (hasSource && !hasSink) {
            //收集器为孤立块，也报错
            if(branch.size() == 1) {
                QString blockName = branchNames.isEmpty() ? "未知" : branchNames.first();
                QString errorMsg;
                if(questionblockInfo) {
                    errorMsg = QString(" '%1' (cmpId: cp_%2)")
                            .arg(blockName).arg(questionblockInfo->cmpId);
                }
                errors.append(errorMsg);
                qDebug() << "错误:" << errorMsg;
            }
            QString warningMsg = QString("支路 %1 有数据源(%2)但没有数据收集器")
                    .arg(i + 1).arg(sourceNames.join(", "));
            warnings.append(warningMsg);
            incompleteBranches++;
            qDebug() << warningMsg;
        } else {
            // 既没有数据源也没有收集器
            if (branch.size() > 1) {
                QString warningMsg = QString("支路 %1 既没有数据源也没有数据收集器")
                        .arg(i + 1);
                warnings.append(warningMsg);
                incompleteBranches++;
                qDebug() << warningMsg;
            } else {
                // 单节点，既不是数据源也不是收集器
                QString blockName = branchNames.isEmpty() ? "未知" : branchNames.first();
                QString errorMsg;
                if(questionblockInfo) {
                    errorMsg = QString("孤立块 '%1' (cmpId: cp_%2) 既不是数据源也不是数据收集器")
                            .arg(blockName).arg(questionblockInfo->cmpId);
                }
                else {
                    errorMsg = QString("孤立块 '%1' 既不是数据源也不是数据收集器")
                            .arg(blockName);
                }
                errors.append(errorMsg);
                incompleteBranches++;
                qDebug() << "错误:" << errorMsg;
            }
        }
    }
    // 6. 检查数据源是否有输出连接（使用真实数据流图）
    for (const BlockInfo& blockInfo : m_blocksInfo) {
        if (blockInfo.block && blockInfo.block->GetBlockType() == Block::BlockType::SOURCE) {
            QString blockId = QString::number(blockInfo.cmpId);

            // 检查这个数据源在真实数据流图中是否有下游
            if (!realFlowGraph.contains(blockId) || realFlowGraph[blockId].isEmpty()) {
                QString warningMsg = QString("数据源 '%1' 的输出端口没有连接到任何地方").arg(blockInfo.instanceName);
                warnings.append(warningMsg);
                qDebug() << "警告:" << warningMsg;
            } else {
                //                qDebug() << "数据源" << blockInfo.instanceName << "有输出连接到:" << realFlowGraph[blockId];
            }
        }
    }

    // 7. 检查数据收集器是否有输入连接（使用反向图）
    for (const BlockInfo& blockInfo : m_blocksInfo) {
        bool isSink = false;

        // 检查是否是数据收集器
        if (blockInfo.block && blockInfo.block->GetBlockType() == Block::BlockType::SINK) {
            isSink = true;
        }
        if (dataCollectionTypes.contains(blockInfo.cmpType)) {
            isSink = true;
        }

        if (isSink) {
            QString blockId = QString::number(blockInfo.cmpId);

            // 检查这个数据收集器在反向图中是否有上游
            if (!reverseFlowGraph.contains(blockId) || reverseFlowGraph[blockId].isEmpty()) {
                QString warningMsg = QString("数据收集器 '%1' 没有输入连接").arg(blockInfo.instanceName);
                warnings.append(warningMsg);
                qDebug() << "警告:" << warningMsg;
            } else {
                //                qDebug() << "数据收集器" << blockInfo.instanceName << "有输入来自:" << reverseFlowGraph[blockId];
            }
        }
    }

    // 8. 递归校验所有子链路的完整性
    QList<QString> subLinkErrors;
    bool hasSubLinkErrors = false;
    for (const BlockInfo& blockInfo : m_blocksInfo) {
        if (blockInfo.isSubSystem && !blockInfo.childTopoId.isEmpty()) {
            qDebug() << "递归校验子链路:" << blockInfo.childTopoId;
            ValidationResult subResult = validateSubLinkCompleteness(blockInfo.childTopoId, subLinkErrors);
            if (!subResult.isValid) {
                hasSubLinkErrors = true;
                result.isValid = false;  // 主链路结果也设为无效

                // 将子链路的错误信息添加到 errors 列表
                if (!subLinkErrors.isEmpty()) {
                    errors.append(subLinkErrors);
                }
            }
        }
    }

    // 9. 汇总结果
    QString branchSummary = QString("链路共检测到 %1 条支路，其中 %2 条完整，%3 条不完整")
            .arg(branches.size())
            .arg(completeBranches)
            .arg(incompleteBranches);

    qDebug() << "=== 支路分析总结 ===";
    qDebug() << branchSummary;

    // 构建详细的支路信息
    QStringList branchDetails;

    // 首先统计完整支路
    if (completeBranches > 0) {
        branchDetails.append("完整支路:");
        int completeIndex = 1;
        for (int i = 0; i < branches.size(); i++) {
            const QSet<QString>& branch = branches[i];

            bool hasSource = false;
            bool hasSink = false;
            QStringList branchNames;

            for (const QString& blockId : branch) {
                BlockInfo* blockInfo = findBlockInfo(blockId.toInt());
                if (blockInfo) {
                    branchNames.append(blockInfo->instanceName);

                    // 检查是否是数据源
                    if (blockInfo->block && blockInfo->block->GetBlockType() == Block::BlockType::SOURCE) {
                        hasSource = true;
                    }
                    // 检查是否是数据收集器
                    bool isSink = false;
                    if (blockInfo->block && blockInfo->block->GetBlockType() == Block::BlockType::SINK) {
                        isSink = true;
                    }
                    if (dataCollectionTypes.contains(blockInfo->cmpType)) {
                        isSink = true;
                    }
                    if (isSink) {
                        hasSink = true;
                    }
                }
            }

            // 如果是完整支路，添加到详细信息
            if (hasSource && hasSink) {
                QString sourceNames, sinkNames;
                for (const QString& blockId : branch) {
                    BlockInfo* blockInfo = findBlockInfo(blockId.toInt());
                    if (blockInfo) {
                        if (blockInfo->block && blockInfo->block->GetBlockType() == Block::BlockType::SOURCE) {
                            if (!sourceNames.isEmpty()) sourceNames += ", ";
                            sourceNames += blockInfo->instanceName;
                        }
                        bool isSink = false;
                        if (blockInfo->block && blockInfo->block->GetBlockType() == Block::BlockType::SINK) {
                            isSink = true;
                        }
                        if (dataCollectionTypes.contains(blockInfo->cmpType)) {
                            isSink = true;
                        }
                        if (isSink) {
                            if (!sinkNames.isEmpty()) sinkNames += ", ";
                            sinkNames += blockInfo->instanceName;
                        }
                    }
                }

                branchDetails.append(QString("  支路%1: \"%2\" (数据源: %3, 收集器: %4)")
                                     .arg(completeIndex++)
                                     .arg(branchNames.join(" → "))
                                     .arg(sourceNames.isEmpty() ? "无" : sourceNames)
                                     .arg(sinkNames.isEmpty() ? "无" : sinkNames));
            }
        }
    }

    // 然后统计不完整支路
    if (incompleteBranches > 0) {
        branchDetails.append("不完整支路:");
        int incompleteIndex = 1;
        for (int i = 0; i < branches.size(); i++) {
            const QSet<QString>& branch = branches[i];

            bool hasSource = false;
            bool hasSink = false;
            QStringList sourceNames;
            QStringList sinkNames;
            QStringList branchNames;

            for (const QString& blockId : branch) {
                BlockInfo* blockInfo = findBlockInfo(blockId.toInt());
                if (blockInfo) {
                    branchNames.append(blockInfo->instanceName);

                    // 检查是否是数据源
                    bool isSource = false;
                    //            const QString& blockId = QString::number(blockInfo->cmpId);

                    // 1. 普通数据源块
                    if (blockInfo->block && blockInfo->block->GetBlockType() == Block::BlockType::SOURCE) {
                        isSource = true;
                    }
                    // 2. 子系统，检查其内部是否有数据源
                    else if (blockInfo->isSubSystem) {
                        QString childTopoId = blockInfo->childTopoId;
                        if (!childTopoId.isEmpty()) {
                            QVector<BlockInfo> subBlocks = getBlocksInfoByLinkKey(childTopoId);
                            for (const BlockInfo& subBlock : subBlocks) {
                                // 跳过inPort/outPort
                                if (subBlock.cmpType == "inPort" || subBlock.cmpType == "outPort") {
                                    continue;
                                }

                                // 内部块是数据源
                                if (subBlock.block && subBlock.block->GetBlockType() == Block::BlockType::SOURCE) {
                                    isSource = true;
                                    break;
                                }
                            }
                        }

                        if (isSource) {
                            qDebug() << "识别子系统" << blockInfo->instanceName << "包含数据源";
                        }
                    }

                    // 检查是否是数据收集器
                    bool isSink = false;

                    // 1. 普通数据收集器块
                    if (blockInfo->block && blockInfo->block->GetBlockType() == Block::BlockType::SINK) {
                        isSink = true;
                    }
                    // 2. 特定类型的收集器
                    if (dataCollectionTypes.contains(blockInfo->cmpType)) {
                        isSink = true;
                    }
                    // 3. 子系统，检查其内部是否有数据收集器
                    else if (blockInfo->isSubSystem) {
                        QString childTopoId = blockInfo->childTopoId;
                        if (!childTopoId.isEmpty()) {
                            QVector<BlockInfo> subBlocks = getBlocksInfoByLinkKey(childTopoId);
                            for (const BlockInfo& subBlock : subBlocks) {
                                // 跳过inPort/outPort
                                if (subBlock.cmpType == "inPort" || subBlock.cmpType == "outPort") {
                                    continue;
                                }

                                // 内部块是数据收集器
                                if (subBlock.block && subBlock.block->GetBlockType() == Block::BlockType::SINK) {
                                    isSink = true;
                                    break;
                                }
                                if (dataCollectionTypes.contains(subBlock.cmpType)) {
                                    isSink = true;
                                    break;
                                }
                            }
                        }

                        if (isSink) {
                            qDebug() << "识别子系统" << blockInfo->instanceName << "包含数据收集器";
                        }
                    }

                    // 记录结果
                    if (isSource) {
                        hasSource = true;
                        sourceNames.append(blockInfo->instanceName);
                    }
                    if (isSink) {
                        hasSink = true;
                        sinkNames.append(blockInfo->instanceName);
                    }
                }
            }

            // 如果是不完整支路，添加到详细信息
            if (!hasSource || !hasSink) {
                QString reason;
                if (!hasSource && hasSink) {
                    reason = QString("有数据收集器(%1)但没有数据源").arg(sinkNames.join(", "));
                } else if (hasSource && !hasSink) {
                    reason = QString("有数据源(%1)但没有数据收集器").arg(sourceNames.join(", "));
                } else {
                    reason = "既没有数据源也没有数据收集器";
                }

                branchDetails.append(QString("  支路%1: \"%2\" (%3)")
                                     .arg(incompleteIndex++)
                                     .arg(branchNames.join(" → "))
                                     .arg(reason));
            }
        }
    }

    qDebug() << "completeBranches: " << completeBranches;

    // 关键修改：根据需求调整校验逻辑
    if (completeBranches == 0) {
        // 情况1：完全没有完整支路，整个链路无效
        qDebug() << "情况1：完全没有完整支路，整个链路无效";
        result.isValid = false;

        // 构建详细的错误信息
        QStringList errorDetails;
        errorDetails.append("链路完整性校验失败：整个链路没有完整的数据流通路");
        result.errorMessage = errorDetails.join("\n");
    }
    else if (!isolatedBlocks.isEmpty() || hasSubLinkErrors) {
        // 情况3：有完整支路，但存在孤立节点，报错
        qDebug() << "情况3：有完整支路，但存在孤立节点, 报错";
        result.isValid = false;

        // 构建详细的错误信息
        QStringList errorDetails;
        errorDetails.append(QString("链路完整性校验失败：存在孤立块:%1").arg(errors.last()));
        result.errorMessage = errorDetails.join("\n");
    }
    else if (!errors.isEmpty() || !warnings.isEmpty()) {
        // 情况2：有完整支路，但有不完整支路或警告
        // 设置结果为有效（因为至少有一条完整支路）
        qDebug() << "情况2：有完整支路，但有不完整支路, 警告";
        result.isValid = true;

        // 构建警告信息
        QStringList warningDetails;
        warningDetails.append(QString("链路完整性警告: %1").arg(branchSummary));
        result.warnings.append(warningDetails.join("\n"));
    }

    qDebug() << "=== 链路完整性校验完成 ===";

    return result;
}

ConnectionValidator::ValidationResult ConnectionValidator::validateSubLinkCompleteness(const QString &linkKey, QList<QString> &allIsolatedBlocks)
{
    ValidationResult result(true);

    // 获取子链路的信息
    QVector<BlockInfo> subBlocks = getBlocksInfoByLinkKey(linkKey);
    QVector<Connection> subConnections = getConnectionsByLinkKey(linkKey);

    if (subBlocks.isEmpty()) {
        return result;
    }

    qDebug() << "=== 开始校验子链路完整性: " << linkKey << " ===";

    // 1. 构建子链路的真实数据流向图（包括inPort/outPort）
    QMap<QString, QList<QString>> realFlowGraph;
    QMap<QString, QList<QString>> reverseFlowGraph;
    QMap<QString, bool> isSourcePort;  // 标记是否是数据源（inPort视为数据源）
    QMap<QString, bool> isSinkPort;     // 标记是否是数据收集器（outPort视为数据收集器）
    QMap<QString, QString> blockNames;  // 块名称映射

    // 初始化
    for (const BlockInfo& blockInfo : subBlocks) {
        QString blockId = QString::number(blockInfo.cmpId);
        realFlowGraph[blockId] = QList<QString>();
        reverseFlowGraph[blockId] = QList<QString>();
        blockNames[blockId] = blockInfo.instanceName;

        // 初始化标记
        isSourcePort[blockId] = false;
        isSinkPort[blockId] = false;

        // 检查是否是inPort（视为数据源）
        if (blockInfo.cmpType == "inPort") {
            isSourcePort[blockId] = true;
        }

        // 检查是否是outPort（视为数据收集器）
        if (blockInfo.cmpType == "outPort") {
            isSinkPort[blockId] = true;
        }
    }

    // 2. 构建子链路的数据流向（包括inPort/outPort）
    for (const Connection& conn : subConnections) {
        bool ok1, ok2, ok3, ok4;
        int blockId1 = conn.fromModelId().toInt(&ok1);
        int portId1 = conn.fromPort().toInt(&ok2);
        int blockId2 = conn.toModelId().toInt(&ok3);
        int portId2 = conn.toPort().toInt(&ok4);

        if (!ok1 || !ok2 || !ok3 || !ok4) continue;

        // 从子链路中查找端口信息
        PortInfo* port1 = findPortInfoInLink(linkKey, blockId1, portId1, m_portInfoCache);
        PortInfo* port2 = findPortInfoInLink(linkKey, blockId2, portId2, m_portInfoCache);

        if (!port1 || !port2) continue;

        QString realSrcId, realDstId;

        if (port1->putType == "out" && port2->putType == "in") {
            realSrcId = conn.fromModelId();
            realDstId = conn.toModelId();
        } else if (port1->putType == "in" && port2->putType == "out") {
            realSrcId = conn.toModelId();
            realDstId = conn.fromModelId();
        } else {
            continue;
        }

        realFlowGraph[realSrcId].append(realDstId);
        reverseFlowGraph[realDstId].append(realSrcId);

        qDebug() << "子链路数据流向:" << blockNames[realSrcId] << "->" << blockNames[realDstId];
    }

    // 3. 识别起始节点（没有输入连接的节点）
    QList<QString> startNodes;
    for (const BlockInfo& blockInfo : subBlocks) {
        QString blockId = QString::number(blockInfo.cmpId);

        bool hasInput = false;
        if (reverseFlowGraph.contains(blockId)) {
            for (const QString& srcId : reverseFlowGraph[blockId]) {
                if (srcId != blockId) {
                    hasInput = true;
                    break;
                }
            }
        }

        if (!hasInput) {
            startNodes.append(blockId);
            qDebug() << "子链路起始节点:" << blockNames[blockId];
        }
    }

    // 4. 使用并查集或DFS找出所有连通分量
    QSet<QString> visited;
    QList<QSet<QString>> branches;

    std::function<void(const QString&, QSet<QString>&)> dfsCollectAll =
            [&](const QString& currentNode, QSet<QString>& branchNodes) {
        if (branchNodes.contains(currentNode)) {
            return;
        }
        branchNodes.insert(currentNode);

        // 遍历正向图
        if (realFlowGraph.contains(currentNode)) {
            for (const QString& neighbor : realFlowGraph[currentNode]) {
                dfsCollectAll(neighbor, branchNodes);
            }
        }

        // 遍历反向图（重要：这样才能找到所有连接的节点）
        if (reverseFlowGraph.contains(currentNode)) {
            for (const QString& predecessor : reverseFlowGraph[currentNode]) {
                dfsCollectAll(predecessor, branchNodes);
            }
        }
    };

    // 遍历所有节点，找出所有连通分量
    for (const BlockInfo& blockInfo : subBlocks) {
        QString blockId = QString::number(blockInfo.cmpId);
        if (!visited.contains(blockId)) {
            QSet<QString> branchNodes;
            dfsCollectAll(blockId, branchNodes);

            // 将当前连通分量的所有节点标记为已访问
            visited.unite(branchNodes);
            branches.append(branchNodes);

            // 调试输出支路信息
            QStringList branchNames;
            for (const QString& nodeId : branchNodes) {
                branchNames.append(blockNames[nodeId]);
            }
            qDebug() << "识别到连通分量:" << branchNames.join(" → ");
        }
    }
    // 4. 使用DFS识别子链路的支路
//    QSet<QString> visited;
//    QList<QSet<QString>> branches;

//    std::function<void(const QString&, QSet<QString>&)> dfs =
//            [&](const QString& currentNode, QSet<QString>& branchNodes) {
//        branchNodes.insert(currentNode);
//        visited.insert(currentNode);
//        qDebug() << "currentNode" << currentNode;

//        if (realFlowGraph.contains(currentNode)) {
//            for (const QString& neighbor : realFlowGraph[currentNode]) {
//                qDebug() << "neighbor" << neighbor;
//                if (!visited.contains(neighbor)) {
//                    dfs(neighbor, branchNodes);
//                }
//            }
//        }
//    };

//    // 从每个起始节点开始DFS
//    for (const QString& startNode : startNodes) {
//        if (!visited.contains(startNode)) {
//            QSet<QString> branchNodes;
//            dfs(startNode, branchNodes);
//            branches.append(branchNodes);

//            qDebug() << "visited: " << visited.values();
//            qDebug() << "branchNodes: " << branchNodes.values();
//            qDebug() << "branches size: " << branches.size();
//            for(int i = 0; i < branches.size();i++) {
//                qDebug() << "branches: " << branches.value(i);
//            }

//            // 调试输出支路信息
//            QStringList branchNames;
//            for (const QString& blockId : branchNodes) {
//                branchNames.append(blockNames[blockId]);
//            }
//            qDebug() << "识别到子链路支路:" << branchNames.join(" → ");
//        }
//    }

    // 5. 处理未被访问的节点（孤立节点）
    for (const BlockInfo& blockInfo : subBlocks) {
        QString blockId = QString::number(blockInfo.cmpId);
        if (!visited.contains(blockId)) {
            QSet<QString> isolatedBranch;
            isolatedBranch.insert(blockId);
            branches.append(isolatedBranch);
            visited.insert(blockId);

            qDebug() << "子链路孤立节点:" << blockNames[blockId];
        }
    }

    // 6. 分析每条支路的完整性
    qDebug() << "=== 子链路支路完整性分析 ===";

    for (int i = 0; i < branches.size(); i++) {
        const QSet<QString>& branch = branches[i];

        bool hasSource = false;
        bool hasSink = false;
        QStringList sourceNames;
        QStringList sinkNames;
        QStringList branchNames;
        BlockInfo* questionblockInfo = nullptr;

        for (const QString& blockId : branch) {
            BlockInfo* blockInfo = findBlockInfoInLink(linkKey, blockId.toInt());
            if (!blockInfo) {
                branchNames.append(blockNames[blockId]);
                continue;
            }

            branchNames.append(blockInfo->instanceName);

            // 检查是否是数据源（普通数据源 或 inPort）
            bool isSource = isSourcePort.value(blockId, false);  // 检查是否被标记为inPort

            if (blockInfo->block && blockInfo->block->GetBlockType() == Block::BlockType::SOURCE) {
                isSource = true;
            }
            // 子系统内部可能包含数据源
            else if (blockInfo->isSubSystem) {
                QString childTopoId = blockInfo->childTopoId;
                if (!childTopoId.isEmpty()) {
                    QVector<BlockInfo> subBlocks = getBlocksInfoByLinkKey(childTopoId);
                    for (const BlockInfo& subBlock : subBlocks) {
                        if (subBlock.cmpType == "inPort" || subBlock.cmpType == "outPort") {
                            continue;
                        }
                        if (subBlock.block && subBlock.block->GetBlockType() == Block::BlockType::SOURCE) {
                            isSource = true;
                            break;
                        }
                    }
                }
            }

            // 检查是否是数据收集器（普通收集器 或 outPort）
            bool isSink = isSinkPort.value(blockId, false);  // 检查是否被标记为outPort

            if (blockInfo->block && blockInfo->block->GetBlockType() == Block::BlockType::SINK) {
                isSink = true;
            }
            QSet<QString> sinkTypes = {"Sink", "SinkCx", "SinkEnv"};
            if (sinkTypes.contains(blockInfo->cmpType)) {
                isSink = true;
            }
            // 子系统内部可能包含数据收集器
            else if (blockInfo->isSubSystem) {
                QString childTopoId = blockInfo->childTopoId;
                if (!childTopoId.isEmpty()) {
                    QVector<BlockInfo> subBlocks = getBlocksInfoByLinkKey(childTopoId);
                    for (const BlockInfo& subBlock : subBlocks) {
                        if (subBlock.cmpType == "inPort" || subBlock.cmpType == "outPort") {
                            continue;
                        }
                        if (subBlock.block && subBlock.block->GetBlockType() == Block::BlockType::SINK) {
                            isSink = true;
                            break;
                        }
                        if (sinkTypes.contains(subBlock.cmpType)) {
                            isSink = true;
                            break;
                        }
                    }
                }
            }

            if (isSource) {
                hasSource = true;
                sourceNames.append(blockInfo->instanceName);
            }
            if (isSink) {
                hasSink = true;
                sinkNames.append(blockInfo->instanceName);
            }
            if(branch.size() <= 1) {
                questionblockInfo = blockInfo;
            }
        }

        qDebug() << "子链路支路" << i + 1 << ":" << branchNames.join(" → ");
        qDebug() << "  数据源:" << (hasSource ? sourceNames.join(", ") : "无");
        qDebug() << "  收集器:" << (hasSink ? sinkNames.join(", ") : "无");

        // 判断支路完整性
        if (hasSource && hasSink) {
            qDebug() << "  完整支路";
        } else {
            QString errorMsg;
            if (!hasSource && hasSink) {
                if(branch.size() == 1) {
                    QString blockName = branchNames.isEmpty() ? "未知" : branchNames.first();
                    if(questionblockInfo) {
                        errorMsg = QString("子系统 %1 中存在孤立块 '%2' (cmpId: cp_%3)")
                                .arg(linkKey).arg(branchNames.first()).arg(questionblockInfo->cmpId);
                    }
                }
                errorMsg = QString("子系统 %1 中的支路 '%2' 有数据收集器(%3)但没有数据源")
                        .arg(linkKey).arg(branchNames.join(" → ")).arg(sinkNames.join(", "));
            } else if (hasSource && !hasSink) {
                if(branch.size() == 1) {
                    QString blockName = branchNames.isEmpty() ? "未知" : branchNames.first();
                    if(questionblockInfo) {
                        errorMsg = QString("子系统 %1 中存在孤立块 '%2' (cmpId: cp_%3)")
                                .arg(linkKey).arg(branchNames.first()).arg(questionblockInfo->cmpId);
                    }
                }
                errorMsg = QString("子系统 %1 中的支路 '%2' 有数据源(%3)但没有数据收集器")
                        .arg(linkKey).arg(branchNames.join(" → ")).arg(sourceNames.join(", "));
            } else {
                if (branch.size() == 1) {
                    // 单节点支路，直接称为孤立块
                    if(questionblockInfo) {
                        errorMsg = QString("子系统 %1 中存在孤立块 '%2' (cmpId: cp_%3)")
                                .arg(linkKey).arg(branchNames.first()).arg(questionblockInfo->cmpId);
                    }
                    else {
                        errorMsg = QString("子系统 %1 中存在孤立块 '%2'")
                                .arg(linkKey).arg(branchNames.first());
                    }

                } else {
                    errorMsg = QString("子系统 %1 中的支路 '%2' 既没有数据源也没有数据收集器")
                            .arg(linkKey).arg(branchNames.join(" → "));
                }
            }

            allIsolatedBlocks.append(errorMsg);
            result.isValid = false;
            result.errorMessage.append(errorMsg);
            qDebug() << "  错误:" << errorMsg;
        }
    }

    // 7. 递归校验更深层的子链路
    for (const BlockInfo& blockInfo : subBlocks) {
        if (blockInfo.isSubSystem && !blockInfo.childTopoId.isEmpty()) {
            qDebug() << "递归校验更深层子链路:" << blockInfo.childTopoId;
            ValidationResult subResult = validateSubLinkCompleteness(blockInfo.childTopoId, allIsolatedBlocks);
            if (!subResult.isValid) {
                result.isValid = false;
            }
        }
    }

    qDebug() << "=== 校验子链路完整性完成 ===";

    return result;
}

ConnectionValidator::ValidationResult ConnectionValidator::validatePenetratedDataTypes()
{
    ValidationResult result(true);

    qDebug() << "=== 开始穿透连接数据类型校验 ===";

    // 获取所有实际连接（穿透子系统）
    QVector<ActualConnection> actualCons = getActualConnections();

    if (actualCons.isEmpty()) {
        qDebug() << "没有发现实际连接";
        return result;
    }

    qDebug() << "共发现" << actualCons.size() << "个实际连接需要校验";

    for (int i = 0; i < actualCons.size(); i++) {
        const ActualConnection& ac = actualCons[i];

        bool ok1, ok2, ok3, ok4;
        int srcBlockId = ac.srcBlockId.toInt(&ok1);
        int srcPortId = ac.srcPortId.toInt(&ok2);
        int dstBlockId = ac.dstBlockId.toInt(&ok3);
        int dstPortId = ac.dstPortId.toInt(&ok4);

        if (!ok1 || !ok2 || !ok3 || !ok4) {
            qDebug() << "连接" << i + 1 << ": 无效的端口ID格式";
            continue;
        }

        // 根据存储的linkKey查找端口信息
        PortInfo* srcPort = nullptr;
        PortInfo* dstPort = nullptr;

        // 优先使用ActualConnection中存储的linkKey
        if (!ac.srcLinkKey.isEmpty()) {
            srcPort = findPortInfoInLink(ac.srcLinkKey, srcBlockId, srcPortId, m_portInfoCache);
        } else {
            // 兼容旧的连接，尝试从当前链路查找
            srcPort = findPortInfo(srcBlockId, srcPortId);
        }

        if (!ac.dstLinkKey.isEmpty()) {
            dstPort = findPortInfoInLink(ac.dstLinkKey, dstBlockId, dstPortId, m_portInfoCache);
        } else {
            dstPort = findPortInfo(dstBlockId, dstPortId);
        }

        if (!srcPort || !dstPort) {
            qDebug() << "连接" << i + 1 << ": 无法找到端口信息";
            qDebug() << "  srcBlockId:" << srcBlockId << "srcPortId:" << srcPortId
                     << "srcLinkKey:" << ac.srcLinkKey << "found:" << (srcPort != nullptr);
            qDebug() << "  dstBlockId:" << dstBlockId << "dstPortId:" << dstPortId
                     << "dstLinkKey:" << ac.dstLinkKey << "found:" << (dstPort != nullptr);
            continue;
        }

        qDebug() << "校验连接" << i + 1 << ":";
        qDebug() << "  实际源:" << srcPort->instanceName
                 << "端口:" << srcPort->portName
                 << "方向:" << srcPort->putType
                 << "类型:" << dataTypeToString(srcPort->dataType);
        qDebug() << "  实际目标:" << dstPort->instanceName
                 << "端口:" << dstPort->portName
                 << "方向:" << dstPort->putType
                 << "类型:" << dataTypeToString(dstPort->dataType);
        qDebug() << "  路径:" << ac.originalPath;

        // 执行数据类型校验
        if (!DataFlowCheck::portDataTypeCheck(srcPort->dataType, dstPort->dataType)) {
            result.isValid = false;
            result.errorMessage = QString("连接端口数据类型不兼容: %1(cmpId: cp_%2)")
                    .arg(dstPort->instanceName).arg(findBlockInfo(dstBlockId)->cmpId);

//                                          "实际数据流: %2(%3:%4) -> %5(%6:%7)\n"
//                                          "路径: %8")
//                    .arg(findBlockInfo(srcBlockId)->cmpId)
//                    .arg(findBlockInfo(dstBlockId)->cmpId)
//                    .arg(srcPort->instanceName)
//                    .arg(srcPort->portName)
//                    .arg(dataTypeToString(srcPort->dataType))
//                    .arg(dstPort->instanceName)
//                    .arg(dstPort->portName)
//                    .arg(dataTypeToString(dstPort->dataType))
//                    .arg(ac.originalPath);

            qDebug() << "校验失败:" << result.errorMessage;
            return result;
        }

        qDebug() << "校验通过";
    }

    qDebug() << "=== 穿透连接校验完成 ===";
    return result;
}

ConnectionValidator::ValidationResult ConnectionValidator::validateInputPortConnections()
{
    return m_porter->validateInputPortConnections();
}

QVector<ConnectionValidator::ActualConnection> ConnectionValidator::getActualConnections()
{
    QVector<ActualConnection> actualCons;
    QSet<QString> visitedLinks; //用于防止递归循环

    // 步骤1：构建所有子系统的端口映射
    QMap<QString, QMap<int, ActualConnection>> subSystemPortMaps;
    for (const BlockInfo& blockInfo : m_blocksInfo) {
        if (blockInfo.isSubSystem && !blockInfo.childTopoId.isEmpty()) {
            QString childTopoId = blockInfo.childTopoId;
            QMap<int, ActualConnection> portMap = buildPortMapping(childTopoId);
            if (!portMap.isEmpty()) {
                subSystemPortMaps[childTopoId] = portMap;
            }
        }
    }

    // 步骤2：处理所有连接，穿透子系统
    for (const Connection& conn : m_connections) {
        bool ok1, ok2, ok3, ok4;
        int srcBlockId = conn.fromModelId().toInt(&ok1);
        int srcPortId = conn.fromPort().toInt(&ok2);
        int dstBlockId = conn.toModelId().toInt(&ok3);
        int dstPortId = conn.toPort().toInt(&ok4);

        if (!ok1 || !ok2 || !ok3 || !ok4) continue;


        BlockInfo* srcBlock = findBlockInfo(srcBlockId);
        BlockInfo* dstBlock = findBlockInfo(dstBlockId);
        PortInfo* srcPort = findPortInfo(srcBlockId, srcPortId);
        PortInfo* dstPort = findPortInfo(dstBlockId, dstPortId);


        if (!srcBlock || !dstBlock || !srcPort || !dstPort) continue;

        // 初始化路径追踪
        PathNode node;
        QVector<PathNode> currentPath;
        // 然后赋值
        node.blockId = QString::number(srcBlockId);
        node.portId = QString::number(srcPortId);
        node.instanceName = srcBlock->instanceName;
        node.portName = srcPort->portName;
        node.linkKey = m_linkKey;
        // 添加到 QVector
        currentPath.append(node);

        // 递归穿透，找到实际的数据源和目标
        QVector<ActualConnection> foundConnections;

        // 根据连接方向决定如何追踪
        if (srcPort->putType == "out" && dstPort->putType == "in") {
            // 正向连接：从源追踪到目标，可能穿透子系统
            traceForwardPath(srcBlock, srcPort, dstBlock, dstPort,
                             currentPath, foundConnections, visitedLinks, false);
        } else if (srcPort->putType == "in" && dstPort->putType == "out") {
            // 反向连接：纠正方向后追踪
            traceForwardPath(dstBlock, dstPort, srcBlock, srcPort,
                             currentPath, foundConnections, visitedLinks, false);
        }

        for (const ActualConnection& ac : foundConnections) {
            actualCons.append(ac);
        }
    }
    return actualCons;
}

// ========== 递归追踪完整路径 ==========
void ConnectionValidator::traceForwardPath(BlockInfo* currentBlock, PortInfo* currentPort,
                                           BlockInfo* targetBlock, PortInfo* targetPort,
                                           QVector<PathNode>& path,
                                           QVector<ActualConnection>& results,
                                           QSet<QString>& visitedLinks, bool isPenetrated)
{
    QString nodeKey = QString("%1_%2").arg(currentBlock->cmpId).arg(currentPort->portId);
    if (!isPenetrated && visitedLinks.contains(nodeKey)) {
        qDebug() << "traceForwardPath: visitedLinks has contains " << currentBlock->cmpId;
        return;
    }
    visitedLinks.insert(nodeKey);

    // 情况1：当前块就是目标块 → 找到一条完整路径
    if (!isPenetrated && currentBlock->cmpId == targetBlock->cmpId &&
            currentPort->portId == targetPort->portId) {
        qDebug() << "情况1：当前块就是目标块";
        if (path.size() >= 2) {
            PathNode& first = path.first();
            PathNode& last = path.last();
            // 获取源和目标所在的链路key
            //            QString srcLinkKey = getLinkKeyByBlock(first.blockId.toInt());
            //            QString dstLinkKey = getLinkKeyByBlock(last.blockId.toInt());

            ActualConnection ac(
                        first.blockId, first.portId,
                        last.blockId, last.portId,
                        first.linkKey, last.linkKey,
                        buildPathString(path)
                        );
            results.append(ac);
            qDebug() << "找到实际连接:" << ac.originalPath;
        }
        return;
    }

    // 情况2：当前块是普通块，目标块是子系统（外部块 -> 子系统输入端口）
    if (!currentBlock->isSubSystem && targetBlock->isSubSystem) {
        qDebug() << "情况2：普通块 -> 子系统输入端口";
        // 目标端口是子系统的输入端口（如 a 的 Port1）
        // 需要在子系统内部找对应的 inPort 模型
        QString innerConnection = findInPortBySubsystemPort(
                    targetBlock->childTopoId,
                    targetPort->portId  // 传入子系统端口ID，如 Port1 的ID p_2
                    );

        if (!innerConnection.isEmpty()) {
            QStringList parts = innerConnection.split(":");
            if (parts.size() >= 5) {
                int innerBlockId = parts[0].toInt();
                int innerPortId = parts[1].toInt();
                QString innerInstanceName = parts[2];
                QString innerPortName = parts[3];
                QString childLinkKey = parts[4];

                BlockInfo* innerBlock = findBlockInfoInLink(childLinkKey, innerBlockId);
                PortInfo* innerPort = findPortInfoInLink(childLinkKey, innerBlockId,innerPortId, m_portInfoCache);
                if (innerBlock && innerPort) {
                    path.append(PathNode{
                                    QString::number(innerBlockId),
                                    QString::number(innerPortId),
                                    innerInstanceName,
                                    innerPortName,
                                    childLinkKey
                                });

                    // 将内部块作为新的目标，继续追踪
                    traceForwardPath(currentBlock, currentPort,
                                     innerBlock, innerPort,
                                     path, results, visitedLinks, true);
                    path.removeLast();
                }
            }
        }
        return;
    }

    // 情况3：当前块是子系统，目标块是普通块（子系统输出端口 -> 外部块）
    if (currentBlock->isSubSystem && !targetBlock->isSubSystem) {
        qDebug() << "情况3：子系统输出端口 -> 普通块";
        //        qDebug() << "currentBlock: " << currentBlock->instanceName;
        //        qDebug() << "currentBlock childTopoId: " << currentBlock->childTopoId;
        //        qDebug() << "currentBlock isSubsystem: " << currentBlock->isSubSystem;

        //        qDebug() << "currentPort: " << currentPort->portName;
        //        qDebug() << "currentPort topProtId: " << currentPort->topProtId;
        //        qDebug() << "currentPort portId: " << currentPort->portId;


        //        qDebug() << "targetBlock: " << targetBlock->instanceName;
        //        qDebug() << "targetBlock childTopoId: " << targetBlock->childTopoId;
        //        qDebug() << "targetBlock isSubsystem: " << targetBlock->isSubSystem;

        //        qDebug() << "targetPort: " << targetPort->portName;
        //        qDebug() << "targetPort topProtId: " << targetPort->topProtId;
        //        qDebug() << "targetPort portId: " << targetPort->portId;
        // 当前端口是子系统的输出端口（如 a 的 Port2）
        // 需要在子系统内部找对应的 outPort 模型的上游
        QString innerConnection = findOutPortBySubsystemPort(
                    currentBlock->childTopoId,
                    currentPort->portId  // 传入子系统端口ID，如 Port2 的ID p_3
                    );

        if (!innerConnection.isEmpty()) {
            QStringList parts = innerConnection.split(":");
            if (parts.size() >= 5) {
                int innerBlockId = parts[0].toInt();
                int innerPortId = parts[1].toInt();
                QString innerInstanceName = parts[2];
                QString innerPortName = parts[3];
                QString childLinkKey = parts[4];

                BlockInfo* innerBlock = findBlockInfoInLink(childLinkKey, innerBlockId);
                PortInfo* innerPort = findPortInfoInLink(childLinkKey, innerBlockId,innerPortId, m_portInfoCache);

                if (innerBlock && innerPort) {
                    //                    qDebug() << "找到内部 outPort 上游:" << innerBlock->instanceName
                    //                             << "端口:" << innerPort->portName;

                    // 删除当前的子系统块
                    if (!path.isEmpty() && path.last().instanceName == currentBlock->instanceName) {
                        path.removeLast();
                    }

                    // 添加内部块到路径
                    path.append(PathNode{
                                    QString::number(innerBlockId),
                                    QString::number(innerPortId),
                                    innerInstanceName,
                                    innerPortName,
                                    childLinkKey
                                });

                    // 添加目标块到路径
                    path.append(PathNode{
                                    QString::number(targetBlock->cmpId),
                                    QString::number(targetPort->portId),
                                    targetBlock->instanceName,
                                    targetPort->portName,
                                    m_linkKey
                                });

                    // 以内部块作为新的当前块，继续追踪到目标
                    traceForwardPath(innerBlock, innerPort,
                                     targetBlock, targetPort,
                                     path, results, visitedLinks, true);
                    // 恢复路径（按添加顺序反向移除）
                    path.removeLast(); // 移除目标块
                    path.removeLast(); // 移除内部块
                }
            }
        }
        return;
    }

    // 情况4：当前块是普通块，目标块也是普通块 → 遍历下游连接
    // isPenetrated为 false时不属于穿透校验
    if (!isPenetrated && !currentBlock->isSubSystem && !targetBlock->isSubSystem) {
        qDebug() << "情况4：普通块->普通块";

        for (const Connection& conn : m_connections) {
            bool ok1, ok2, ok3, ok4;
            int srcId = conn.fromModelId().toInt(&ok1);
            int srcPort = conn.fromPort().toInt(&ok2);
            int dstId = conn.toModelId().toInt(&ok3);
            int dstPort = conn.toPort().toInt(&ok4);

            if (!ok1 || !ok2 || !ok3 || !ok4) continue;

            if (srcId == currentBlock->cmpId &&
                    srcPort == currentPort->portId) {

                BlockInfo* nextBlock = findBlockInfo(dstId);
                PortInfo* nextPort = findPortInfo(dstId, dstPort);

                if (nextBlock && nextPort) {
                    path.append(PathNode{
                                    QString::number(dstId),
                                    QString::number(dstPort),
                                    nextBlock->instanceName,
                                    nextPort->portName,
                                    m_linkKey
//                                    getLinkKeyByBlock(dstId)
                                });

                    traceForwardPath(nextBlock, nextPort,
                                     targetBlock, targetPort,
                                     path, results, visitedLinks,isPenetrated);

                    path.removeLast();
                }
            }
        }
    }
    // 情况5：穿透后的块（如E2、C2）继续追踪到目标
    // isPenetrated为 true时属于穿透校验
    if (isPenetrated && !currentBlock->isSubSystem && !targetBlock->isSubSystem) {
        qDebug() << "情况5：穿透完成，生成实际连接";

        // 获取路径的起点和当前点
        if (path.size() >= 2) {
            PathNode& first = path.first();
            PathNode& last = path.last();

            ActualConnection ac(
                        first.blockId, first.portId,
                        last.blockId, last.portId,
                        first.linkKey, last.linkKey,
                        buildPathString(path)
                        );
            results.append(ac);
            qDebug() << "穿透完成，找到实际连接: " << ac.originalPath;
        }
    }
    // 情况6：子系统 -> 子系统
    if (currentBlock->isSubSystem && targetBlock->isSubSystem) {
        qDebug() << "情况6：子系统 -> 子系统";

        // 1. 从当前子系统的输出端口找到内部上游块
        QString sourceInnerConnection = findOutPortBySubsystemPort(
                    currentBlock->childTopoId,
                    currentPort->portId
                    );

        // 2. 从目标子系统的输入端口找到内部下游块
        QString targetInnerConnection = findInPortBySubsystemPort(
                    targetBlock->childTopoId,
                    targetPort->portId
                    );

        if (!sourceInnerConnection.isEmpty() && !targetInnerConnection.isEmpty()) {
            QStringList sourceParts = sourceInnerConnection.split(":");
            QStringList targetParts = targetInnerConnection.split(":");

            if (sourceParts.size() >= 5 && targetParts.size() >= 5) {
                // 解析源内部块
                int sourceInnerBlockId = sourceParts[0].toInt();
                int sourceInnerPortId = sourceParts[1].toInt();
                QString sourceInnerInstanceName = sourceParts[2];
                QString sourceInnerPortName = sourceParts[3];
                QString sourceChildLinkKey = sourceParts[4];

                // 解析目标内部块
                int targetInnerBlockId = targetParts[0].toInt();
                int targetInnerPortId = targetParts[1].toInt();
                QString targetInnerInstanceName = targetParts[2];
                QString targetInnerPortName = targetParts[3];
                QString targetChildLinkKey = targetParts[4];

                // 查找内部块信息
                BlockInfo* sourceInnerBlock = findBlockInfoInLink(sourceChildLinkKey, sourceInnerBlockId);
                PortInfo* sourceInnerPort = findPortInfoInLink(sourceChildLinkKey, sourceInnerBlockId,
                                                               sourceInnerPortId, m_portInfoCache);

                BlockInfo* targetInnerBlock = findBlockInfoInLink(targetChildLinkKey, targetInnerBlockId);
                PortInfo* targetInnerPort = findPortInfoInLink(targetChildLinkKey, targetInnerBlockId,
                                                               targetInnerPortId, m_portInfoCache);

                if (sourceInnerBlock && sourceInnerPort && targetInnerBlock && targetInnerPort) {
                    qDebug() << "找到源内部块:" << sourceInnerBlock->instanceName
                             << "端口:" << sourceInnerPort->portName;
                    qDebug() << "找到目标内部块:" << targetInnerBlock->instanceName
                             << "端口:" << targetInnerPort->portName;

                    // 删除当前的子系统块
                    if (!path.isEmpty() && path.last().instanceName == currentBlock->instanceName) {
                        path.removeLast();
                    }

                    // 添加源内部块到路径
                    path.append(PathNode{
                                    QString::number(sourceInnerBlockId),
                                    QString::number(sourceInnerPortId),
                                    sourceInnerInstanceName,
                                    sourceInnerPortName,
                                    sourceChildLinkKey
                                });

                    // 添加目标内部块到路径
                    path.append(PathNode{
                                    QString::number(targetInnerBlockId),
                                    QString::number(targetInnerPortId),
                                    targetInnerInstanceName,
                                    targetInnerPortName,
                                    targetChildLinkKey
                                });

                    // 以源内部块作为当前块，目标内部块作为目标，继续追踪
                    traceForwardPath(sourceInnerBlock, sourceInnerPort,
                                     targetInnerBlock, targetInnerPort,
                                     path, results, visitedLinks, true);

                    path.removeLast(); // 移除目标内部块
                    path.removeLast(); // 移除源内部块
                }
            }
        }
        return;
    }
}

ConnectionValidator::PortInfo *ConnectionValidator::findPortInfoInLink(const QString &linkKey, int blockId, int portId, QList<ConnectionValidator::PortInfo> &portCache)
{
    //    qDebug() << "findPortInfoInLink - linkKey:" << linkKey << "blockId:" << blockId << "portId:" << portId;

    BlockInfo* blockInfo = findBlockInfoInLink(linkKey, blockId);
    if (!blockInfo) {
        qDebug() << "  blockInfo not found for blockId:" << blockId;
        return nullptr;
    }
    //    qDebug() << "  found block:" << blockInfo->instanceName << "type:" << blockInfo->cmpType;
    //    qDebug() << "  block has" << blockInfo->portsMsg.size() << "ports";

    auto it = blockInfo->portsMsg.find(portId);
    if (it == blockInfo->portsMsg.end()) {
        qDebug() << "  portId" << portId << "not found in block's ports";
        // 打印所有可用的端口ID供调试
        QStringList availablePorts;
        for (auto portIt = blockInfo->portsMsg.begin(); portIt != blockInfo->portsMsg.end(); ++portIt) {
            availablePorts << QString::number(portIt.key()) + "(" + portIt.value().name + ")";
        }
        //        qDebug() << "  available ports:" << availablePorts.join(", ");
        return nullptr;
    }
    //    qDebug() << "  found port:" << it.value().name << "type:" << dataTypeToString(it.value().dataType);

    // 创建 PortInfo 对象并添加到缓存
    PortInfo portInfo;
    portInfo.portId = portId;
    portInfo.instanceName = blockInfo->instanceName;
    auto port = it.value();
    portInfo.portName = port.name;
    portInfo.putType = port.putType;
    portInfo.dataType = port.dataType;
    portInfo.topProtId = port.topProtId;
    portInfo.isBusPort = isBusDataType(port.dataType);
    portInfo.isOptional = port.isOptional;

    portCache.append(portInfo);
    return &portCache.last();
}

BlockInfo *ConnectionValidator::findBlockInfoInLink(const QString &linkKey, int blockId)
{
    //    QVector<BlockInfo> blocksInfo = getBlocksInfoByLinkKey(linkKey);
    //    for (BlockInfo& blockInfo : blocksInfo) {
    //        if (blockInfo.cmpId == blockId) {
    //            return &blockInfo;
    //        }
    //    }
    //    return nullptr;
    // 从成员变量缓存中查找
    if (m_linkKeyBlocksMap.contains(linkKey)) {
        QVector<BlockInfo>& blocksInfo = m_linkKeyBlocksMap[linkKey];  // 引用成员变量
        for (BlockInfo& blockInfo : blocksInfo) {
            if (blockInfo.cmpId == blockId) {
                return &blockInfo;  // 返回指向成员变量的指针，生命周期长
            }
        }
    }

    qDebug() << "  blockInfo not found for blockId:" << blockId;
    return nullptr;
}

QString ConnectionValidator::findInPortBySubsystemPort(const QString& childTopoId, int subsystemPortId)
{
    if (childTopoId.isEmpty() || subsystemPortId == -1) {
        return "";
    }

    QVector<BlockInfo> subBlocks = getBlocksInfoByLinkKey(childTopoId);
    QVector<Connection> subConnections = getConnectionsByLinkKey(childTopoId);

    if (subBlocks.isEmpty() || subConnections.isEmpty()) {
        return "";
    }

    // 查找 inPort 模型，其端口的 topProtId 等于 subsystemPortId
    for (const BlockInfo& block : subBlocks) {
        if (block.cmpType == "inPort") {
            for (auto it = block.portsMsg.begin(); it != block.portsMsg.end(); ++it) {
                const PortMsg& portMsg = it.value();
                // inPort 模型的端口方向是 out，topProtId 指向子系统端口
                if (portMsg.putType == "out" && portMsg.topProtId == subsystemPortId) {
                    QString inPortBlockId = QString::number(block.cmpId);
                    QString inPortPortId = QString::number(portMsg.id);

                    // 查找这个 inPort 连接到哪个内部模型
                    for (const Connection& conn : subConnections) {
                        if (conn.fromModelId() == inPortBlockId &&
                                conn.fromPort() == inPortPortId) {

                            QString targetBlockId = conn.toModelId();
                            QString targetPortId = conn.toPort();

                            int targetCmpId = targetBlockId.toInt();
                            int targetPortIdInt = targetPortId.toInt();

                            for (const BlockInfo& targetBlock : subBlocks) {
                                if (targetBlock.cmpId == targetCmpId &&
                                        targetBlock.portsMsg.contains(targetPortIdInt)) {
                                    const PortMsg& targetPort = targetBlock.portsMsg[targetPortIdInt];

                                    return QString("%1:%2:%3:%4:%5")
                                            .arg(targetBlockId)
                                            .arg(targetPortId)
                                            .arg(targetBlock.instanceName)
                                            .arg(targetPort.name)
                                            .arg(childTopoId);  // 添加 linkKey 信息
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return "";
}

QString ConnectionValidator::findOutPortBySubsystemPort(const QString& childTopoId, int subsystemPortId)
{
    if (childTopoId.isEmpty() || subsystemPortId == -1) {
        return "";
    }

    QVector<BlockInfo> subBlocks = getBlocksInfoByLinkKey(childTopoId);
    QVector<Connection> subConnections = getConnectionsByLinkKey(childTopoId);

    if (subBlocks.isEmpty() || subConnections.isEmpty()) {
        return "";
    }

    // 查找 outPort 模型，其端口的 topProtId 等于 subsystemPortId
    for (const BlockInfo& block : subBlocks) {
        if (block.cmpType == "outPort") {
            for (auto it = block.portsMsg.begin(); it != block.portsMsg.end(); ++it) {
                const PortMsg& portMsg = it.value();
                // outPort 模型的端口方向是 in，topProtId 指向子系统端口
                if (portMsg.putType == "in" && portMsg.topProtId == subsystemPortId) {
                    QString outPortBlockId = QString::number(block.cmpId);
                    QString outPortPortId = QString::number(portMsg.id);

                    // 查找哪个内部模型连接到这个 outPort
                    for (const Connection& conn : subConnections) {
                        if (conn.toModelId() == outPortBlockId &&
                                conn.toPort() == outPortPortId) {

                            QString sourceBlockId = conn.fromModelId();
                            QString sourcePortId = conn.fromPort();

                            int sourceCmpId = sourceBlockId.toInt();
                            int sourcePortIdInt = sourcePortId.toInt();

                            for (const BlockInfo& sourceBlock : subBlocks) {
                                if (sourceBlock.cmpId == sourceCmpId &&
                                        sourceBlock.portsMsg.contains(sourcePortIdInt)) {
                                    const PortMsg& sourcePort = sourceBlock.portsMsg[sourcePortIdInt];

                                    return QString("%1:%2:%3:%4:%5")
                                            .arg(sourceBlockId)
                                            .arg(sourcePortId)
                                            .arg(sourceBlock.instanceName)
                                            .arg(sourcePort.name)
                                            .arg(childTopoId);  // 添加 linkKey 信息
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return "";
}

QString ConnectionValidator::getLinkKeyByBlock(int blockId)
{
    // 遍历所有链路，查找包含该块ID的链路
    for (auto it = m_allBlocksInfoMap.begin(); it != m_allBlocksInfoMap.end(); ++it) {
        const QString& linkKey = it.key();
        const QVector<BlockInfo>& blocksInfo = it.value();

        for (const BlockInfo& blockInfo : blocksInfo) {
            if (blockInfo.cmpId == blockId) {
                return linkKey;
            }
        }
    }
    return m_linkKey;  // 默认返回当前链路
}

// ========== 构建路径字符串 ==========
QString ConnectionValidator::buildPathString(const QVector<PathNode>& path)
{
    QStringList parts;
    for (const PathNode& node : path) {
        parts.append(QString("%1(%2)").arg(node.instanceName).arg(node.portName));
    }
    return parts.join(" → ");
}

void ConnectionValidator::initializeDataStructures()
{
    // 清空现有数据
    m_blockIdMap.clear();
    m_blockPortsMap.clear();
    m_portLookup.clear();
    m_dependencyGraph.clear();
    m_inDegree.clear();
    m_portInfoCache.clear();

    // 只获取当前链路的信息进行初始化
    QVector<BlockInfo> currentBlocksInfo = getBlocksInfoByLinkKey(m_linkKey);

    // 存储当前链路的 BlockInfo 副本
    m_linkKeyBlocksMap[m_linkKey] = currentBlocksInfo;

    // 构建当前链路的端口映射
    QMap<int, PortInfo> currentLinkPortsMap;

    // 构建当前链路的映射
    for (const BlockInfo& blockInfo : currentBlocksInfo) {
        //        m_blockIdMap[blockInfo.cmpId] = const_cast<BlockInfo*>(&blockInfo);
        m_blockIdToLinkKeyMap[blockInfo.cmpId] = m_linkKey;

        QString instanceName = blockInfo.instanceName;
        QList<PortInfo> portList;

        for (auto it = blockInfo.portsMsg.begin(); it != blockInfo.portsMsg.end(); ++it) {
            int portId = it.key();
            const PortMsg& portMsg = it.value();

            PortInfo portInfo;
            portInfo.portId = portId;
            portInfo.instanceName = instanceName;
            portInfo.portName = portMsg.name;
            portInfo.putType = portMsg.putType;
            portInfo.dataType = portMsg.dataType;
            portInfo.topProtId = portMsg.topProtId;
            portInfo.isBusPort = isBusDataType(portMsg.dataType);
            portInfo.isOptional = portMsg.isOptional;

            portList.append(portInfo);

            currentLinkPortsMap[blockInfo.cmpId * 1000 + portId] = portInfo;  // 用组合key
            QString lookupKey = QString("%1_%2").arg(blockInfo.cmpId).arg(portId);

            //            qDebug() << "initializeDataStructures lookupKey: " << lookupKey;
            m_portLookup[lookupKey] = &portList.last();
        }

        m_blockPortsMap[instanceName] = portList;

        QString blockIdStr = QString::number(blockInfo.cmpId);
        m_dependencyGraph[blockIdStr] = QList<QString>();
        m_inDegree[blockIdStr] = 0;
    }
    m_linkKeyPortsMap[m_linkKey] = currentLinkPortsMap;

    // 构建依赖图
    for (const Connection& conn : m_connections) {
        bool ok1, ok2, ok3, ok4;
        int srcBlockId = conn.fromModelId().toInt(&ok1);
        int srcPortId = conn.fromPort().toInt(&ok2);
        int dstBlockId = conn.toModelId().toInt(&ok3);
        int dstPortId = conn.toPort().toInt(&ok4);

        if (!ok1 || !ok2 || !ok3 || !ok4) continue;

        PortInfo* srcPort = findPortInfo(srcBlockId, srcPortId);
        PortInfo* dstPort = findPortInfo(dstBlockId, dstPortId);

        if (!srcPort || !dstPort) continue;

        qDebug() << srcPort->instanceName << "srcPort: " << srcPort->portName << ", putType: " << srcPort->putType;
        qDebug() << dstPort->instanceName << "dstPort: " << dstPort->portName << ", putType: " << dstPort->putType;

        // 关键：根据端口方向确定真正的数据流向
        QString realSrcBlockId, realDstBlockId;

        if (srcPort->putType == "out" && dstPort->putType == "in") {
            // 正常方向：out -> in
            qDebug() << "正向连接";
            realSrcBlockId = conn.fromModelId();
            realDstBlockId = conn.toModelId();
        } else if (srcPort->putType == "in" && dstPort->putType == "out") {
            // 反向连接：in -> out，需要纠正
            qDebug() << "反向连接";
            realSrcBlockId = conn.toModelId();  // 原目标变成源
            realDstBlockId = conn.fromModelId(); // 原源变成目标
        } else {
            // 同向连接，跳过
            continue;
        }

        // 添加到依赖图
        if (!m_dependencyGraph.contains(realSrcBlockId)) {
            m_dependencyGraph[realSrcBlockId] = QList<QString>();
        }
        if (!m_dependencyGraph.contains(realDstBlockId)) {
            m_dependencyGraph[realDstBlockId] = QList<QString>();
        }

        // 添加依赖边
        m_dependencyGraph[realSrcBlockId].append(realDstBlockId);

        // 更新入度
        if (!m_inDegree.contains(realDstBlockId)) {
            m_inDegree[realDstBlockId] = 0;
        }
        m_inDegree[realDstBlockId]++;

        // 确保所有节点都有入度记录
        if (!m_inDegree.contains(realSrcBlockId)) {
            m_inDegree[realSrcBlockId] = 0;
        }
    }
}

ConnectionValidator::PortInfo *ConnectionValidator::findPortInfo(int blockId, int portId)
{
    //    QString lookupKey = QString("%1_%2").arg(blockId).arg(portId);
    //    if (m_portLookup.contains(lookupKey)) {
    //        return m_portLookup[lookupKey];
    //    }
    //    return nullptr;
    // 先找到这个 blockId 属于哪个链路
    if (!m_blockIdToLinkKeyMap.contains(blockId)) {
        return nullptr;
    }

    QString linkKey = m_blockIdToLinkKeyMap[blockId];

    // 从该链路的端口映射中查找
    if (m_linkKeyPortsMap.contains(linkKey)) {
        QMap<int, PortInfo>& portsMap = m_linkKeyPortsMap[linkKey];
        int compositeKey = blockId * 1000 + portId;  // 假设portId < 1000
        if (portsMap.contains(compositeKey)) {
            return &portsMap[compositeKey];
        }
    }

    return nullptr;
}

BlockInfo *ConnectionValidator::findBlockInfo(int blockId)
{
    //    if (m_blockIdMap.contains(blockId)) {
    //        return m_blockIdMap[blockId];
    //    }
    //    return nullptr;
    // 先找到这个 blockId 属于哪个链路
    if (!m_blockIdToLinkKeyMap.contains(blockId)) {
        qDebug() << "findBlockInfo: blockId" << blockId << "不在映射中";
        return nullptr;
    }

    QString linkKey = m_blockIdToLinkKeyMap[blockId];

    // 从该链路的 BlockInfo 集合中查找
    if (m_linkKeyBlocksMap.contains(linkKey)) {
        QVector<BlockInfo>& blocksInfo = m_linkKeyBlocksMap[linkKey];
        for (BlockInfo& blockInfo : blocksInfo) {
            if (blockInfo.cmpId == blockId) {
                return &blockInfo;  // 返回指向 m_linkKeyBlocksMap 中对象的指针
            }
        }
    }

    return nullptr;
}

bool ConnectionValidator::isBusDataType(PortMsg::PortDataType dataType)
{
    // 检查是否为Bus类型（多端口类型）
    switch (dataType) {
    case PortMsg::MULTIPLE_INT:
    case PortMsg::MULTIPLE_COMPLEX:
    case PortMsg::MULTIPLE_ANYTYPE:
    case PortMsg::MULTIPLE_ENVELOPE:
    case PortMsg::MULTIPLE_REAL:
    case PortMsg::MULTIPLE_FIXEDPOINT:
    case PortMsg::MULTIPLE_VARIANT:
    case PortMsg::MULTIPLE_INT_MATRIX:
    case PortMsg::MULTIPLE_COMPLEX_MATRIX:
    case PortMsg::MULTIPLE_ANYTYPE_MATRIX:
    case PortMsg::MULTIPLE_ENVELOPE_MATRIX:
    case PortMsg::MULTIPLE_REAL_MATRIX:
    case PortMsg::MULTIPLE_FIXEDPOINT_MATRIX:
    case PortMsg::MULTIPLE_VARIANT_MATRIX:
        return true;
    default:
        return false;
    }
}

QString ConnectionValidator::dataTypeToString(PortMsg::PortDataType dataType)
{
    // 将数据类型枚举转换为字符串
    static QMap<PortMsg::PortDataType, QString> typeMap = {
        {PortMsg::INT, "INT"},
        {PortMsg::COMPLEX, "COMPLEX"},
        {PortMsg::ANYTYPE, "ANYTYPE"},
        {PortMsg::ENVELOPE, "ENVELOPE"},
        {PortMsg::REAL, "REAL"},
        {PortMsg::FIXEDPOINT, "FIXEDPOINT"},
        {PortMsg::VARIANT, "VARIANT"},
        {PortMsg::MULTIPLE_INT, "MULTIPLE_INT"},
        {PortMsg::MULTIPLE_COMPLEX, "MULTIPLE_COMPLEX"},
        {PortMsg::MULTIPLE_ANYTYPE, "MULTIPLE_ANYTYPE"},
        {PortMsg::MULTIPLE_ENVELOPE, "MULTIPLE_ENVELOPE"},
        {PortMsg::MULTIPLE_REAL, "MULTIPLE_REAL"},
        {PortMsg::MULTIPLE_FIXEDPOINT, "MULTIPLE_FIXEDPOINT"},
        {PortMsg::MULTIPLE_VARIANT, "MULTIPLE_VARIANT"},
        {PortMsg::INT_MATRIX, "INT_MATRIX"},
        {PortMsg::COMPLEX_MATRIX, "COMPLEX_MATRIX"},
        {PortMsg::ANYTYPE_MATRIX, "ANYTYPE_MATRIX"},
        {PortMsg::ENVELOPE_MATRIX, "ENVELOPE_MATRIX"},
        {PortMsg::REAL_MATRIX, "REAL_MATRIX"},
        {PortMsg::FIXEDPOINT_MATRIX, "FIXEDPOINT_MATRIX"},
        {PortMsg::VARIANT_MATRIX, "VARIANT_MATRIX"},
        {PortMsg::MULTIPLE_INT_MATRIX, "MULTIPLE_INT_MATRIX"},
        {PortMsg::MULTIPLE_COMPLEX_MATRIX, "MULTIPLE_COMPLEX_MATRIX"},
        {PortMsg::MULTIPLE_ANYTYPE_MATRIX, "MULTIPLE_ANYTYPE_MATRIX"},
        {PortMsg::MULTIPLE_ENVELOPE_MATRIX, "MULTIPLE_ENVELOPE_MATRIX"},
        {PortMsg::MULTIPLE_REAL_MATRIX, "MULTIPLE_REAL_MATRIX"},
        {PortMsg::MULTIPLE_FIXEDPOINT_MATRIX, "MULTIPLE_FIXEDPOINT_MATRIX"},
        {PortMsg::MULTIPLE_VARIANT_MATRIX, "MULTIPLE_VARIANT_MATRIX"}
    };

    return typeMap.value(dataType, "UNKNOWN");
}

QMap<int, ConnectionValidator::ActualConnection> ConnectionValidator::buildPortMapping(const QString &childTopoId)
{
    QMap<int, ActualConnection> portMap;

    if (childTopoId.isEmpty()) {
        return portMap;
    }

    //    qDebug() << "构建端口映射: childTopoId=" << childTopoId;

    // 获取子链路信息
    QVector<BlockInfo> subBlocks = getBlocksInfoByLinkKey(childTopoId);
    QVector<Connection> subConnections = getConnectionsByLinkKey(childTopoId);

    if (subBlocks.isEmpty() || subConnections.isEmpty()) {
        qDebug() << "子链路信息为空";
        return portMap;
    }

    // 构建inPort映射（子系统的输入端口）
    for (const BlockInfo& block : subBlocks) {
        if (block.cmpType == "inPort") {
            for (auto it = block.portsMsg.begin(); it != block.portsMsg.end(); ++it) {
                const PortMsg& portMsg = it.value();
                if (portMsg.putType == "out" && portMsg.topProtId != -1) {
                    // 查找这个inPort连接到哪个内部模型
                    QString connectionInfo = findInPortConnection(childTopoId, portMsg.topProtId);
                    if (!connectionInfo.isEmpty()) {
                        QStringList parts = connectionInfo.split(":");
                        if (parts.size() >= 2) {
                            ActualConnection ac;
                            ac.srcBlockId = parts[0];  // 内部源块ID
                            ac.srcPortId = parts[1];   // 内部源端口ID
                            // 对于inPort，目标为空（表示这是输入端）
                            ac.originalPath = QString("inPort[%1] -> %2(%3)")
                                    .arg(portMsg.topProtId)
                                    .arg(parts.size() > 2 ? parts[2] : "未知")
                                    .arg(parts.size() > 3 ? parts[3] : "未知");
                            portMap[portMsg.topProtId] = ac;
                        }
                    }
                }
            }
        }
    }

    // 构建outPort映射（子系统的输出端口）
    for (const BlockInfo& block : subBlocks) {
        if (block.cmpType == "outPort") {
            for (auto it = block.portsMsg.begin(); it != block.portsMsg.end(); ++it) {
                const PortMsg& portMsg = it.value();
                if (portMsg.putType == "in" && portMsg.topProtId != -1) {
                    // 查找哪个内部模型连接到这个outPort
                    QString connectionInfo = findOutPortConnection(childTopoId, portMsg.topProtId);
                    if (!connectionInfo.isEmpty()) {
                        QStringList parts = connectionInfo.split(":");
                        if (parts.size() >= 2) {
                            ActualConnection ac;
                            // 对于outPort，源为空（表示这是输出端）
                            ac.dstBlockId = parts[0];  // 内部目标块ID
                            ac.dstPortId = parts[1];   // 内部目标端口ID
                            ac.originalPath = QString("%1(%2) -> outPort[%3]")
                                    .arg(parts.size() > 2 ? parts[2] : "未知")
                                    .arg(parts.size() > 3 ? parts[3] : "未知")
                                    .arg(portMsg.topProtId);
                            portMap[portMsg.topProtId] = ac;
                        }
                    }
                }
            }
        }
    }

    return portMap;
}

QString ConnectionValidator::ConnectionValidator::findConnectedModel(const QVector<Connection> &connections, const QString &fromBlockId, const QString &fromPortId)
{
    for (const Connection& conn : connections) {
        if (conn.fromModelId() == fromBlockId && conn.fromPort() == fromPortId) {
            return conn.toModelId();
        }
    }
    return "";
}

QString ConnectionValidator::findConnectedPort(const QVector<Connection> &connections, const QString &fromBlockId, const QString &fromPortId)
{
    for (const Connection& conn : connections) {
        if (conn.fromModelId() == fromBlockId && conn.fromPort() == fromPortId) {
            return conn.toPort();
        }
    }
    return "";
}

QString ConnectionValidator::findSourceModel(const QVector<Connection> &connections, const QString &toBlockId, const QString &toPortId)
{
    for (const Connection& conn : connections) {
        if (conn.toModelId() == toBlockId && conn.toPort() == toPortId) {
            return conn.fromModelId();
        }
    }
    return "";
}

QString ConnectionValidator::findSourcePort(const QVector<Connection> &connections, const QString &toBlockId, const QString &toPortId)
{
    for (const Connection& conn : connections) {
        if (conn.toModelId() == toBlockId && conn.toPort() == toPortId) {
            return conn.fromPort();
        }
    }
    return "";
}

QVector<BlockInfo> ConnectionValidator::getBlocksInfoByLinkKey(const QString &linkKey)
{
    if (linkKey.isEmpty()) {
        return QVector<BlockInfo>();
    }

    // 从本地查找
    if (m_allBlocksInfoMap.contains(linkKey)) {
        return m_allBlocksInfoMap[linkKey];
    }
    return QVector<BlockInfo>();
}

QVector<Connection> ConnectionValidator::getConnectionsByLinkKey(const QString &linkKey)
{
    if (linkKey.isEmpty()) {
        return QVector<Connection>();
    }

    // 从本地查找
    if (m_allConnectionsMap.contains(linkKey)) {
        return m_allConnectionsMap[linkKey];
    }
    return QVector<Connection>();
}

QString ConnectionValidator::findInPortConnection(const QString &childTopoId, int topProtId)
{
    if (childTopoId.isEmpty() || topProtId == -1) {
        return "";
    }

    //    qDebug() << "查找inPort连接: childTopoId=" << childTopoId << ", topProtId=" << topProtId;

    // 获取子链路信息
    QVector<BlockInfo> subBlocks = getBlocksInfoByLinkKey(childTopoId);
    QVector<Connection> subConnections = getConnectionsByLinkKey(childTopoId);

    if (subBlocks.isEmpty() || subConnections.isEmpty()) {
        qDebug() << "子链路信息为空";
        return "";
    }

    // 1. 找到对应topProtId的inPort
    BlockInfo* inPortBlock = nullptr;
    PortMsg* inPortMsg = nullptr;

    for (BlockInfo& block : subBlocks) {
        if (block.cmpType == "inPort") {
            for (auto it = block.portsMsg.begin(); it != block.portsMsg.end(); ++it) {
                if (it.value().topProtId == topProtId && it.value().putType == "out") {
                    inPortBlock = &block;
                    inPortMsg = &(it.value());
                    break;
                }
            }
            if (inPortBlock) break;
        }
    }

    if (!inPortBlock || !inPortMsg) {
        qDebug() << "找不到对应topProtId的inPort";
        return "";
    }

    // 2. 查找inPort连接到哪个内部模型
    QString inPortBlockId = QString::number(inPortBlock->cmpId);
    QString inPortPortId = QString::number(inPortMsg->id);

    for (const Connection& conn : subConnections) {
        if (conn.fromModelId() == inPortBlockId && conn.fromPort() == inPortPortId) {
            // 找到连接的目标
            QString targetBlockId = conn.toModelId();
            QString targetPortId = conn.toPort();

            // 获取目标块和端口信息
            bool ok1, ok2;
            int targetCmpId = targetBlockId.toInt(&ok1);
            int targetPortIdInt = targetPortId.toInt(&ok2);

            if (ok1 && ok2) {
                for (BlockInfo& block : subBlocks) {
                    if (block.cmpId == targetCmpId && block.portsMsg.contains(targetPortIdInt)) {
                        const PortMsg& targetPort = block.portsMsg[targetPortIdInt];

                        // 返回格式：blockId:portId:instanceName:portName
                        return QString("%1:%2:%3:%4")
                                .arg(targetBlockId)
                                .arg(targetPortId)
                                .arg(block.instanceName)
                                .arg(targetPort.name);
                    }
                }
            }
        }
    }

    qDebug() << "inPort没有连接到任何内部模型";
    return "";
}

QString ConnectionValidator::findOutPortConnection(const QString &childTopoId, int topProtId)
{
    if (childTopoId.isEmpty() || topProtId == -1) {
        return "";
    }

    //    qDebug() << "查找outPort连接: childTopoId=" << childTopoId << ", topProtId=" << topProtId;

    // 获取子链路信息
    QVector<BlockInfo> subBlocks = getBlocksInfoByLinkKey(childTopoId);
    QVector<Connection> subConnections = getConnectionsByLinkKey(childTopoId);

    if (subBlocks.isEmpty() || subConnections.isEmpty()) {
        qDebug() << "子链路信息为空";
        return "";
    }

    // 1. 找到对应topProtId的outPort
    BlockInfo* outPortBlock = nullptr;
    PortMsg* outPortMsg = nullptr;

    for (BlockInfo& block : subBlocks) {
        if (block.cmpType == "outPort") {
            for (auto it = block.portsMsg.begin(); it != block.portsMsg.end(); ++it) {
                if (it.value().topProtId == topProtId && it.value().putType == "in") {
                    outPortBlock = &block;
                    outPortMsg = &(it.value());
                    break;
                }
            }
            if (outPortBlock) break;
        }
    }

    if (!outPortBlock || !outPortMsg) {
        qDebug() << "找不到对应topProtId的outPort";
        return "";
    }

    // 2. 查找哪个内部模型连接到这个outPort
    QString outPortBlockId = QString::number(outPortBlock->cmpId);
    QString outPortPortId = QString::number(outPortMsg->id);

    for (const Connection& conn : subConnections) {
        if (conn.toModelId() == outPortBlockId && conn.toPort() == outPortPortId) {
            // 找到连接的源
            QString sourceBlockId = conn.fromModelId();
            QString sourcePortId = conn.fromPort();

            // 获取源块和端口信息
            bool ok1, ok2;
            int sourceCmpId = sourceBlockId.toInt(&ok1);
            int sourcePortIdInt = sourcePortId.toInt(&ok2);

            if (ok1 && ok2) {
                for (BlockInfo& block : subBlocks) {
                    if (block.cmpId == sourceCmpId && block.portsMsg.contains(sourcePortIdInt)) {
                        const PortMsg& sourcePort = block.portsMsg[sourcePortIdInt];

                        // 返回格式：blockId:portId:instanceName:portName
                        return QString("%1:%2:%3:%4")
                                .arg(sourceBlockId)
                                .arg(sourcePortId)
                                .arg(block.instanceName)
                                .arg(sourcePort.name);
                    }
                }
            }
        }
    }

    qDebug() << "没有内部模型连接到outPort";
    return "";
}
