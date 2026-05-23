// ShortOpenProcessor.cpp
#include "ShortOpenProcessor.h"
#include "algorithmmanager.h"
#include <QDebug>
#include <optional>
#include "../Common/LogExport.h"

bool ShortOpenProcessor::processAllLinks(
    QMap<QString, QVector<BlockInfo>>& blocksInfoMap,
    QMap<QString, QVector<Connection>>& connectionsMap)
{
    qDebug() << "========== 开始处理短路/开路模型 ==========";
    m_shortCircuitedSourcesAndSinks.clear();

    // 遍历所有链路
    for (auto it = blocksInfoMap.begin(); it != blocksInfoMap.end(); ++it) {
        const QString& linkKey = it.key();
        QVector<BlockInfo>& blocksInfo = it.value();
        QVector<Connection>& connections = connectionsMap[linkKey];

        qDebug() << "处理链路:" << linkKey
                 << "模型数:" << blocksInfo.size()
                 << "连接数:" << connections.size();

        if (!processSingleLink(linkKey, blocksInfo, connections)) {
            LOG_ERROR("链路:", linkKey.toStdString(), "短路/开路处理失败");
            return false;
        }
    }

    qDebug() << "========== 短路/开路处理完成 ==========";
    qDebug() << "短路模型（信号源/数据收集器）数量:"
             << m_shortCircuitedSourcesAndSinks.size();
    return true;
}

bool ShortOpenProcessor::processSingleLink(
    const QString& linkKey,
    QVector<BlockInfo>& blocksInfo,
    QVector<Connection>& connections)
{
    // 收集需要处理的模型信息（不立即修改 blocksInfo）
    struct ProcessItem {
        int blockId;
        QString instanceName;
        QString cmpCategory;
        bool isShort;
        bool isOpen;
        BlockInfo blockInfo;  // 保存完整的 BlockInfo 副本
    };
    QVector<ProcessItem> itemsToProcess;

    // 第一遍：收集所有需要处理的模型信息
    for (int i = 0; i < blocksInfo.size(); ++i) {
        const BlockInfo& blockInfo = blocksInfo[i];
        bool isShort = false;
        bool isOpen = false;

        if (checkCondition(blockInfo, isShort, isOpen)) {
            ProcessItem item;
            item.blockId = blockInfo.cmpId;
            item.instanceName = blockInfo.instanceName;
            item.cmpCategory = blockInfo.cmpCategory;
            item.isShort = isShort;
            item.isOpen = isOpen;
            item.blockInfo = blockInfo;  // 拷贝一份
            itemsToProcess.append(item);

            qDebug() << "发现需要处理的模型:"
                     << blockInfo.instanceName
                     << "ID:" << blockInfo.cmpId
                     << "短路:" << isShort
                     << "开路:" << isOpen
                     << "类别:" << blockInfo.cmpCategory;
        }
    }

    if (itemsToProcess.isEmpty()) {
        return true;
    }

    // 第二遍：处理短路（使用保存的 BlockInfo 副本）
    for (const ProcessItem& item : itemsToProcess) {
        if (item.isShort) {
            qDebug() << "处理短路模型:" << item.instanceName;

            // 检查是否为信号源或数据收集器，保存到容器
            if (item.cmpCategory == "Sources" || item.cmpCategory == "Sinks") {
                ShortCircuitedModel shortModel;
                shortModel.linkKey = linkKey;
                shortModel.blockInfo = std::make_shared<BlockInfo>(item.blockInfo);
                // 或者直接拷贝：shortModel.blockInfo = item.blockInfo;
                m_shortCircuitedSourcesAndSinks.append(shortModel);
                qDebug() << "  短路模型是信号源/数据收集器，已保存待校验";
            }

            // 处理短路（修改 connections）
            processShort(item.blockInfo, connections);
        }
    }

    // 第三遍：处理开路
    for (const ProcessItem& item : itemsToProcess) {
        if (item.isOpen) {
            qDebug() << "处理开路模型:" << item.instanceName;
            processOpen(item.blockInfo, connections);
        }
    }

    // 第四遍：统一删除所有需要删除的模型
    QSet<int> idsToRemove;
    for (const ProcessItem& item : itemsToProcess) {
        idsToRemove.insert(item.blockId);
    }

    for (int i = blocksInfo.size() - 1; i >= 0; --i) {
        if (idsToRemove.contains(blocksInfo[i].cmpId)) {
            qDebug() << "删除模型:" << blocksInfo[i].instanceName;
            blocksInfo.removeAt(i);
        }
    }

    return true;
}

bool ShortOpenProcessor::checkCondition(const BlockInfo& blockInfo, bool& isShort, bool& isOpen)
{
    // 查找 cmpCondition 字段
    QString condition = blockInfo.cmpCondition;


    if (condition.isEmpty()) {
        return false;
    }

    condition = condition.trimmed().toLower();

    if (condition == "short") {
        isShort = true;
        isOpen = false;
        return true;
    } else if (condition == "open") {
        isShort = false;
        isOpen = true;
        return true;
    } else if (condition == "active") {
        //正常模型
        isShort = false;
        isOpen = false;
        return false;
    }

    // 未知条件，警告但继续
    LOG_WARN("未知的cmpCondition值:",
             condition.toStdString(),
             "，模型:",
             blockInfo.instanceName.toStdString());
    return false;
}

void ShortOpenProcessor::processOpen(const BlockInfo& blockInfo,
    QVector<Connection>& connections)
{
    qDebug() << "  开路处理: 删除模型" << blockInfo.instanceName
             << "(ID:" << blockInfo.cmpId << ")及其所有连接";

    // 1. 删除与该模型相关的所有连接
    removeConnectionsForBlock(blockInfo.cmpId, connections);

    // 2. 从模型列表中删除该模型
//    removeBlockFromList(blockInfo.cmpId, blocksInfo);
}

void ShortOpenProcessor::processShort(const BlockInfo& blockInfo,
    QVector<Connection>& connections)
{
//    Q_UNUSED(blocksInfo);

    qDebug() << "  短路处理: 模型" << blockInfo.instanceName
             << "(ID:" << blockInfo.cmpId << ")";

    // 1. 获取所有上游连接（连接到本模型输入端口的连接）
    QVector<Connection> incomingConns = getAllIncomingConnections(blockInfo, connections);

    // 2. 获取所有下游连接（从本模型输出端口出发的连接）
    QVector<Connection> outgoingConns = getAllOutgoingConnections(blockInfo, connections);

    qDebug() << "  上游连接数:" << incomingConns.size()
             << "下游连接数:" << outgoingConns.size();

    // 3. 如果没有上游或下游连接，短路没有意义，只删除模型
    if (incomingConns.isEmpty() || outgoingConns.isEmpty()) {
        qDebug() << "短路模型没有完整的上游/下游连接:" << blockInfo.instanceName;
        // 仍然删除模型及其连接
        removeConnectionsForBlock(blockInfo.cmpId, connections);
//        removeBlockFromList(blockInfo.cmpId, blocksInfo);
        // 注意：这里不能直接删除 blockInfo，因为 blockInfo 是 const 引用
        // 实际删除需要在调用处进行
        return;
    }

    // 4. 收集所有上游源端（上游模型的输出端口）
    struct SourceEndpoint {
        QString fromModelId;  // 上游模型ID
        QString fromPortId;   // 上游模型端口ID
    };
    QVector<SourceEndpoint> sources;

    for (const Connection& conn : incomingConns) {
        SourceEndpoint src;
        src.fromModelId = conn.fromModelId();
        src.fromPortId = conn.fromPort();
        sources.append(src);
        qDebug() << "    上游源:" << src.fromModelId << ":" << src.fromPortId;
    }

    // 5. 收集所有下游目标端（下游模型的输入端口）
    struct TargetEndpoint {
        QString toModelId;    // 下游模型ID
        QString toPortId;     // 下游模型端口ID
    };
    QVector<TargetEndpoint> targets;

    for (const Connection& conn : outgoingConns) {
        TargetEndpoint tgt;
        tgt.toModelId = conn.toModelId();
        tgt.toPortId = conn.toPort();
        targets.append(tgt);
        qDebug() << "    下游目标:" << tgt.toModelId << ":" << tgt.toPortId;
    }

    // 6. 删除原模型的所有连接
    removeConnectionsForBlock(blockInfo.cmpId, connections);

    // 7. 建立全连接：每个上游连接到每个下游
    int newConnCount = 0;
    for (const SourceEndpoint& src : sources) {
        for (const TargetEndpoint& tgt : targets) {
            // 避免自连接（如果上游和下游是同一个模型）
            if (src.fromModelId == tgt.toModelId) {
                qDebug() << "    跳过自连接:" << src.fromModelId << "->" << tgt.toModelId;
                continue;
            }

            addConnectionIfNotExists(
                src.fromModelId, src.fromPortId,
                tgt.toModelId, tgt.toPortId,
                connections);
            newConnCount++;
        }
    }

    qDebug() << "  短路处理完成: 删除了" << (incomingConns.size() + outgoingConns.size())
             << "条旧连接，添加了" << newConnCount << "条新连接";

    // 8. 短路模型本身需要从模型列表中删除
    // 注意：这个删除需要在调用处进行，因为这里 blockInfo 是 const 引用
//    removeBlockFromList(blockInfo.cmpId, blocksInfo);
}

// ========== 辅助方法实现 ==========

QSet<int> ShortOpenProcessor::getInputPortIds(const BlockInfo& blockInfo) const
{
    QSet<int> inputPortIds;
    for (auto it = blockInfo.portsMsg.begin(); it != blockInfo.portsMsg.end(); ++it) {
        const PortMsg& port = it.value();
        if (port.putType == "in") {
            inputPortIds.insert(port.id);
        }
    }
    return inputPortIds;
}

QSet<int> ShortOpenProcessor::getOutputPortIds(const BlockInfo& blockInfo) const
{
    QSet<int> outputPortIds;
    for (auto it = blockInfo.portsMsg.begin(); it != blockInfo.portsMsg.end(); ++it) {
        const PortMsg& port = it.value();
        if (port.putType == "out") {
            outputPortIds.insert(port.id);
        }
    }
    return outputPortIds;
}

QVector<Connection> ShortOpenProcessor::getIncomingConnections(
    const BlockInfo& blockInfo,
    int portId,
    const QVector<Connection>& connections) const
{
    QVector<Connection> result;
    QString targetModelId = QString::number(blockInfo.cmpId);
    QString targetPortId = QString::number(portId);

    for (const Connection& conn : connections) {
        if (conn.toModelId() == targetModelId && conn.toPort() == targetPortId) {
            result.append(conn);
        }
    }
    return result;
}

QVector<Connection> ShortOpenProcessor::getOutgoingConnections(
    const BlockInfo& blockInfo,
    int portId,
    const QVector<Connection>& connections) const
{
    QVector<Connection> result;
    QString sourceModelId = QString::number(blockInfo.cmpId);
    QString sourcePortId = QString::number(portId);

    for (const Connection& conn : connections) {
        if (conn.fromModelId() == sourceModelId && conn.fromPort() == sourcePortId) {
            result.append(conn);
        }
    }
    return result;
}

QVector<Connection> ShortOpenProcessor::getAllIncomingConnections(
    const BlockInfo& blockInfo,
    const QVector<Connection>& connections) const
{
    QVector<Connection> result;
    QString targetModelId = QString::number(blockInfo.cmpId);
    QSet<int> inputPortIds = getInputPortIds(blockInfo);

    for (const Connection& conn : connections) {
        if (conn.toModelId() == targetModelId) {
            // 检查端口是否属于输入端口
            int portId = conn.toPort().toInt();
            if (inputPortIds.contains(portId)) {
                result.append(conn);
            }
        }
    }
    return result;
}

QVector<Connection> ShortOpenProcessor::getAllOutgoingConnections(
    const BlockInfo& blockInfo,
    const QVector<Connection>& connections) const
{
    QVector<Connection> result;
    QString sourceModelId = QString::number(blockInfo.cmpId);
    QSet<int> outputPortIds = getOutputPortIds(blockInfo);

    for (const Connection& conn : connections) {
        if (conn.fromModelId() == sourceModelId) {
            int portId = conn.fromPort().toInt();
            if (outputPortIds.contains(portId)) {
                result.append(conn);
            }
        }
    }
    return result;
}

void ShortOpenProcessor::removeConnectionsForBlock(
    int blockId,
    QVector<Connection>& connections)
{
    QString blockIdStr = QString::number(blockId);

    // 从后往前删除，避免索引问题
    for (int i = connections.size() - 1; i >= 0; --i) {
        const Connection& conn = connections[i];
        if (conn.fromModelId() == blockIdStr || conn.toModelId() == blockIdStr) {
            connections.removeAt(i);
        }
    }
}

void ShortOpenProcessor::removeBlockFromList(
    int blockId,
    QVector<BlockInfo>& blocksInfo)
{
    for (int i = 0; i < blocksInfo.size(); ++i) {
        if (blocksInfo[i].cmpId == blockId) {
            blocksInfo.removeAt(i);
            qDebug() << "  已从模型列表中删除 ID:" << blockId;
            return;
        }
    }
}

void ShortOpenProcessor::addConnectionIfNotExists(
    const QString& fromModelId,
    const QString& fromPortId,
    const QString& toModelId,
    const QString& toPortId,
    QVector<Connection>& connections)
{
    Connection newConn(fromModelId, fromPortId, toModelId, toPortId);

    // 检查是否已存在相同连接
    for (const Connection& conn : connections) {
        if (isSameConnection(conn, newConn)) {
            qDebug() << "    连接已存在，跳过:"
                     << fromModelId << ":" << fromPortId
                     << "->" << toModelId << ":" << toPortId;
            return;
        }
    }

    connections.append(newConn);
    qDebug() << "    添加新连接:"
             << fromModelId << ":" << fromPortId
             << "->" << toModelId << ":" << toPortId;
}

bool ShortOpenProcessor::isSameConnection(
    const Connection& conn1,
    const Connection& conn2) const
{
    return conn1.fromModelId() == conn2.fromModelId() &&
           conn1.fromPort() == conn2.fromPort() &&
           conn1.toModelId() == conn2.toModelId() &&
           conn1.toPort() == conn2.toPort();
}
