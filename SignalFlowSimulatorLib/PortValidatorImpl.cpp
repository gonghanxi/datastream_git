#include "PortValidatorImpl.h"
#include "dataflowcheck.h"

ConnectionValidator::ValidationResult PortValidatorImpl::validatePortDirection()
{
    ConnectionValidator::ValidationResult result(true);
    m_validator->m_invalidConnections.clear();

    for (const Connection& conn : m_validator->m_connections) {
        bool ok1, ok2, ok3, ok4;
        int srcBlockId = conn.fromModelId().toInt(&ok1);
        int srcPortId = conn.fromPort().toInt(&ok2);
        int dstBlockId = conn.toModelId().toInt(&ok3);
        int dstPortId = conn.toPort().toInt(&ok4);

        if (!ok1 || !ok2 || !ok3 || !ok4) {
            result.isValid = false;
            result.errorMessage = QString("连接关系包含无效的ID格式: %1:%2 (cmpId: cp_%1) -> %3:%4 (cmpId: cp_%3)")
                .arg(conn.fromModelId()).arg(conn.fromPort())
                .arg(conn.toModelId()).arg(conn.toPort());
            m_validator->m_invalidConnections.append(conn);
            continue;
        }

        ConnectionValidator::PortInfo* srcPort = m_validator->findPortInfo(srcBlockId, srcPortId);
        ConnectionValidator::PortInfo* dstPort = m_validator->findPortInfo(dstBlockId, dstPortId);

        if (!srcPort || !dstPort) {
            // 获取模型信息以便更好的错误信息
            QString srcModelName = "未知模型";
            QString dstModelName = "未知模型";
            QString srcPortName = "端口" + conn.fromPort();
            QString dstPortName = "端口" + conn.toPort();

            BlockInfo* srcBlock = m_validator->findBlockInfo(srcBlockId);
            BlockInfo* dstBlock = m_validator->findBlockInfo(dstBlockId);

            if (srcBlock) {
                srcModelName = srcBlock->instanceName;
                // 尝试从端口中获取端口名
                if (srcPort) {
                    srcPortName = srcPort->portName;
                }
            }

            if (dstBlock) {
                dstModelName = dstBlock->instanceName;
                // 尝试从端口中获取端口名
                if (dstPort) {
                    dstPortName = dstPort->portName;
                }
            }

            result.isValid = false;
            result.errorMessage = QString("连接关系引用了不存在的端口:\n"
                                        "从 [%1(%2)](cmpId: cp_%5) 到 [%3(%4)](cmpId: cp_%6)")
                .arg(srcModelName).arg(srcPortName)
                .arg(dstModelName).arg(dstPortName)
                .arg(conn.fromModelId())
                .arg(conn.toModelId());
            m_validator->m_invalidConnections.append(conn);
            continue;
        }

        //同向端口报错
        if (!DataFlowCheck::portPutTypeCheck(srcPort->putType, dstPort->putType)) {
            result.isValid = false;
            result.errorMessage = QString("端口方向错误:\n"
                                        "连接: %3(%4) (cmpId: cp_%1) -> %5(%6) (cmpId: cp_%2)\n"
                                        "原因: 同向端口不能连接 (%7-%7)")
                .arg(m_validator->findBlockInfo(srcBlockId)->cmpId)
                .arg(m_validator->findBlockInfo(dstBlockId)->cmpId)
                .arg(srcPort->instanceName).arg(srcPort->portName)
                .arg(dstPort->instanceName).arg(dstPort->portName)
                .arg(srcPort->putType);
            m_validator->m_invalidConnections.append(conn);
            continue;
        }
    }

    return result;
}

ConnectionValidator::ValidationResult PortValidatorImpl::validatePortDataType()
{
    ConnectionValidator::ValidationResult result(true);

    // 清空临时缓存
    m_validator->m_portInfoCache.clear();

    // 获取所有子链路的linkKey（排除主链路）
    QStringList subLinkKeys;
    for (auto it = m_validator->m_allBlocksInfoMap.begin();
         it != m_validator->m_allBlocksInfoMap.end(); ++it) {
        const QString& linkKey = it.key();
        if (linkKey != m_validator->m_linkKey) {  // 排除主链路
            subLinkKeys.append(linkKey);
        }
    }

    qDebug() << "=== 开始校验所有子链路内部端口数据类型 ===";
    qDebug() << "发现" << subLinkKeys.size() << "个子链路需要校验";

    // 对每个子链路进行递归校验
    for (const QString& subLinkKey : subLinkKeys) {
        qDebug() << "\n校验子链路:" << subLinkKey;
        ConnectionValidator::ValidationResult subResult = validateSubLinkPortDataType(subLinkKey);

        if (!subResult.isValid) {
            // 如果子链路校验失败，直接返回错误
            result.isValid = false;
            result.errorMessage = subResult.errorMessage;
            return result;
        }

        // 收集警告信息
        if (!subResult.warnings.isEmpty()) {
            result.warnings.append(subResult.warnings);
        }
    }

    if (subLinkKeys.isEmpty()) {
        qDebug() << "没有子链路需要校验";
    } else {
        qDebug() << "=== 所有子链路内部端口数据类型校验完成 ===";
    }

    return result;
}

ConnectionValidator::ValidationResult PortValidatorImpl::validateInputPortConnections()
{
    ConnectionValidator::ValidationResult result(true);

    qDebug() << "=== 开始输入端口连接完整性校验 ===";

    // 1. 统计每个块的输入端口连接情况
    QMap<QString, QSet<int>> allInputPorts;        // blockId -> 所有输入端口ID
    QMap<QString, QSet<int>> connectedInputPorts;  // blockId -> 已连接的输入端口ID
    QMap<QString, QSet<int>> optionalInputPorts;   // blockId -> 可选输入端口ID
    QMap<QString, QSet<int>> nonOptionalInputPorts; // blockId -> 非可选输入端口ID

    // 初始化映射
    for (const BlockInfo& blockInfo : m_validator->m_blocksInfo) {
        QString blockId = QString::number(blockInfo.cmpId);
        allInputPorts[blockId] = QSet<int>();
        connectedInputPorts[blockId] = QSet<int>();
        optionalInputPorts[blockId] = QSet<int>();
        nonOptionalInputPorts[blockId] = QSet<int>();
    }

    // 2. 遍历所有连接，记录已连接的输入端口
    for (const Connection& conn : m_validator->m_connections) {
        bool ok1, ok2, ok3, ok4;
        int srcBlockId = conn.fromModelId().toInt(&ok1);
        int srcPortId = conn.fromPort().toInt(&ok2);
        int dstBlockId = conn.toModelId().toInt(&ok3);
        int dstPortId = conn.toPort().toInt(&ok4);

        if (!ok1 || !ok2 || !ok3 || !ok4) continue;

        ConnectionValidator::PortInfo* srcPort = m_validator->findPortInfo(srcBlockId, srcPortId);
        ConnectionValidator::PortInfo* dstPort = m_validator->findPortInfo(dstBlockId, dstPortId);

        if (!srcPort || !dstPort) continue;

        // 确定真实的数据流向
        QString realDstBlockId;
        int realDstPortId;

        if (srcPort->putType == "out" && dstPort->putType == "in") {
            // 正常方向：out -> in
            realDstBlockId = conn.toModelId();
            realDstPortId = dstPortId;
        } else if (srcPort->putType == "in" && dstPort->putType == "out") {
            // 反向连接：纠正方向
            realDstBlockId = conn.fromModelId();
            realDstPortId = srcPortId;
        } else {
            // 同向连接，跳过
            continue;
        }

        // 记录已连接的输入端口
        connectedInputPorts[realDstBlockId].insert(realDstPortId);

        qDebug() << "检测到输入连接: 块" << realDstBlockId << "端口" << realDstPortId;
    }

    // 3. 遍历所有块，收集输入端口信息
    for (const BlockInfo& blockInfo : m_validator->m_blocksInfo) {
        QString blockId = QString::number(blockInfo.cmpId);

        // 遍历块的所有端口
        for (auto it = blockInfo.portsMsg.begin(); it != blockInfo.portsMsg.end(); ++it) {
            int portId = it.key();
            const PortMsg& portMsg = it.value();

            // 只处理输入端口（输出端口不检查）
            if (portMsg.putType != "in") continue;

            // 记录所有输入端口
            allInputPorts[blockId].insert(portId);

            // 记录可选/非可选端口
            if (portMsg.isOptional) {
                optionalInputPorts[blockId].insert(portId);
            } else {
                nonOptionalInputPorts[blockId].insert(portId);
            }

            // 调试输出
            qDebug() << "块" << blockId << "(" << blockInfo.instanceName
                     << ") 输入端口" << portId << "(" << portMsg.name
                     << ") isOptional:" << portMsg.isOptional;
        }
    }

    // 4. 检查每个块的输入端口连接完整性
    for (const BlockInfo& blockInfo : m_validator->m_blocksInfo) {
        QString blockId = QString::number(blockInfo.cmpId);
        QString blockName = blockInfo.instanceName;

        QSet<int> allPorts = allInputPorts[blockId];
        QSet<int> connectedPorts = connectedInputPorts[blockId];
        QSet<int> optionalPorts = optionalInputPorts[blockId];
        QSet<int> nonOptionalPorts = nonOptionalInputPorts[blockId];

        // 跳过没有输入端口的块
        if (allPorts.isEmpty()) {
            qDebug() << "块" << blockId << "(" << blockName << "): 没有输入端口，跳过检查";
            continue;
        }

        // 找出未连接的输入端口
        QSet<int> unconnectedPorts = allPorts - connectedPorts;

        // 调试信息
        qDebug() << "块" << blockId << "(" << blockName << "):"
                 << "总输入端口:" << allPorts.size()
                 << "已连接:" << connectedPorts.size()
                 << "未连接:" << unconnectedPorts.size()
                 << "(可选:" << optionalPorts.size() << "非可选:" << nonOptionalPorts.size() << ")";

        // 如果有任何输入端口已连接，则检查未连接的端口
        if (!connectedPorts.isEmpty() && !unconnectedPorts.isEmpty()) {
            QStringList unconnectedPortNames;
            QStringList nonOptionalUnconnectedPortNames;
            bool hasNonOptionalUnconnected = false;

            for (int portId : unconnectedPorts) {
                // 查找端口信息
                ConnectionValidator::PortInfo* portInfo = m_validator->findPortInfo(blockInfo.cmpId, portId);
                if (portInfo) {
                    // 查找原始的PortMsg以获取isOptional信息
                    PortMsg* portMsg = nullptr;
                    if (blockInfo.portsMsg.contains(portId)) {
                        const PortMsg& msg = blockInfo.portsMsg[portId];
                        portMsg = const_cast<PortMsg*>(&msg);
                    }

                    bool isOptional = portMsg ? portMsg->isOptional : false;

                    if (!isOptional) {
                        hasNonOptionalUnconnected = true;
                        nonOptionalUnconnectedPortNames.append(portInfo->portName);
                    }
                    unconnectedPortNames.append(portInfo->portName + (isOptional ? "(可选)" : ""));
                }
            }

            // 如果有非可选端口未连接，报错
            if (hasNonOptionalUnconnected) {
                result.isValid = false;
                result.errorMessage = QString("输入端口连接不完整:\n"
                                            "模型 %1(cmpId: cp_%2) 有已连接的输入端口，但以下非可选输入端口未连接:%3\n")
                    .arg(blockName)
                    .arg(blockId)
                    .arg(nonOptionalUnconnectedPortNames.join(", "));
                qDebug() << "校验失败:" << result.errorMessage;
                return result;
            } else if (!unconnectedPorts.isEmpty()) {
                // 只有可选端口未连接
            }
        }

        // 特殊情况：如果一个模型所有输入端口都是可选的且都未连接，这是允许的
        if (connectedPorts.isEmpty() && optionalPorts.size() == allPorts.size()) {
            qDebug() << "块" << blockId << "(" << blockName << "): 所有输入端口都是可选的且未连接，允许";
        }
    }

    qDebug() << "=== 输入端口连接完整性校验完成 ===";
    return result;
}

ConnectionValidator::ValidationResult PortValidatorImpl::validateSubLinkPortDataType(const QString &linkKey)
{
    ConnectionValidator::ValidationResult result(true);

    // 获取子链路的信息
    QVector<BlockInfo> subBlocks = m_validator->getBlocksInfoByLinkKey(linkKey);
    QVector<Connection> subConnections = m_validator->getConnectionsByLinkKey(linkKey);

    if (subBlocks.isEmpty() || subConnections.isEmpty()) {
        qDebug() << "validateSubLinkPortDataType no result";
        return result;  // 没有内部连接，直接返回
    }

    qDebug() << "=== 开始校验子链路内部端口数据类型: " << linkKey << " ===";

    // 遍历子链路中的所有连接
    for (const Connection& conn : subConnections) {
        bool ok1, ok2, ok3, ok4;
        int srcBlockId = conn.fromModelId().toInt(&ok1);
        int srcPortId = conn.fromPort().toInt(&ok2);
        int dstBlockId = conn.toModelId().toInt(&ok3);
        int dstPortId = conn.toPort().toInt(&ok4);

        if (!ok1 || !ok2 || !ok3 || !ok4) continue;

        // 从子链路中查找端口信息
        ConnectionValidator::PortInfo* srcPort = m_validator->findPortInfoInLink(linkKey, srcBlockId, srcPortId, m_validator->m_portInfoCache);
        ConnectionValidator::PortInfo* dstPort = m_validator->findPortInfoInLink(linkKey, dstBlockId, dstPortId, m_validator->m_portInfoCache);

        if (!srcPort || !dstPort) {
            qDebug() << "子链路" << linkKey << "中找不到端口信息:"
                     << "srcBlockId:" << srcBlockId << "srcPortId:" << srcPortId
                     << "dstBlockId:" << dstBlockId << "dstPortId:" << dstPortId;
            continue;
        }

        // 跳过出入口模型的连接（inPort/outPort之间的连接会在穿透校验中处理）
        if (srcPort->topProtId != -1 || dstPort->topProtId != -1) {
            qDebug() << "跳过出入口模型连接:" << srcPort->instanceName << "->" << dstPort->instanceName;
            continue;
        }

        // 确定正确的数据类型校验顺序
        PortMsg::PortDataType dataTypeStart, dataTypeEnd;

        if (srcPort->putType == "out" && dstPort->putType == "in") {
            // 正常方向：out -> in
            dataTypeStart = srcPort->dataType;
            dataTypeEnd = dstPort->dataType;
            qDebug() << "校验子链路内部连接:" << srcPort->instanceName << "(" << srcPort->portName << ")"
                     << "->" << dstPort->instanceName << "(" << dstPort->portName << ")"
                     << "类型:" << m_validator->dataTypeToString(dataTypeStart) << "->" << m_validator->dataTypeToString(dataTypeEnd);
        } else if (srcPort->putType == "in" && dstPort->putType == "out") {
            // 反向连接：纠正为 out -> in
            dataTypeStart = dstPort->dataType;
            dataTypeEnd = srcPort->dataType;
            qDebug() << "校验子链路内部反向连接:" << dstPort->instanceName << "(" << dstPort->portName << ")"
                     << "->" << srcPort->instanceName << "(" << srcPort->portName << ")"
                     << "类型:" << m_validator->dataTypeToString(dataTypeStart) << "->" << m_validator->dataTypeToString(dataTypeEnd);
        } else {
            // 同向连接，跳过
            qDebug() << "跳过同向连接:" << srcPort->instanceName << "->" << dstPort->instanceName;
            continue;
        }

        // 检查端口数据类型兼容性
        if (!DataFlowCheck::portDataTypeCheck(dataTypeStart, dataTypeEnd)) {
            result.isValid = false;
            result.errorMessage = QString("子链路 %1 内部端口数据类型不兼容: %2(%3:%4) (cmpId: cp_%5)")
//                                        "源类型: %8, 目标类型: %9")
                .arg(linkKey)
//                .arg(srcPort->instanceName).arg(srcPort->portName)
//                .arg(m_validator->dataTypeToString(srcPort->dataType))
                .arg(dstPort->instanceName).arg(dstPort->portName)
                .arg(m_validator->dataTypeToString(dstPort->dataType))
//                .arg(m_validator->dataTypeToString(dataTypeStart))
//                .arg(m_validator->dataTypeToString(dataTypeEnd))
//                .arg(srcBlockId)
                .arg(dstBlockId);
            return result;
        }
    }

    // 递归校验更深层的子链路
    for (const BlockInfo& blockInfo : subBlocks) {
        if (blockInfo.isSubSystem && !blockInfo.childTopoId.isEmpty()) {
            qDebug() << "递归校验更深层子链路:" << blockInfo.childTopoId;
            ConnectionValidator::ValidationResult subResult = validateSubLinkPortDataType(blockInfo.childTopoId);
            if (!subResult.isValid) {
                return subResult;
            }
        }
    }

    return result;
}
