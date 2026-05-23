#ifndef SIGNALFLOWLINKSORT_H
#define SIGNALFLOWLINKSORT_H

#include <QMap>
#include <QVector>
#include <QSet>
#include <QQueue>
#include <QDebug>
#include <memory>
#include <Block.h>
#include "connection.h"
#include "algorithmmanager.h"

using namespace SystemVueModelBuilder;

// 简化的模型信息类，用于拓扑排序
class SortableModel {
public:
    SortableModel() {}
    SortableModel(const QString& id, Block* blockPtr = nullptr)
        : m_id(id), m_block(blockPtr) {}

    QString id() const { return m_id; }
    Block* block() const { return m_block; }

    // 判断模型类型
    bool isSource() const {
        return m_block && m_block->GetBlockType() == Block::BlockType::SOURCE;
    }
    bool isSink() const {
        return m_block && m_block->GetBlockType() == Block::BlockType::SINK;
    }
    bool isProcessor() const {
        return m_block && m_block->GetBlockType() == Block::BlockType::PROCESSOR;
    }

private:
    QString m_id;
    Block* m_block = nullptr;
};

// 跨层级拓扑排序的Block节点结构
struct CrossLayerBlockNode {
    QString globalId;                         // 全局唯一ID (linkKey:localId)
    QString linkId;                            // 所属链路ID
    int localId;                               // 本地ID
    QString name;                              // 实例名称
    Block::BlockType type;                      // 块类型
    Block* block;                               // 块指针
    QSet<QString> inputs;                       // 输入来自哪些块
    QSet<QString> outputs;                      // 输出到哪些块

    CrossLayerBlockNode() : localId(-1), type(Block::BlockType::PROCESSOR), block(nullptr) {}

    CrossLayerBlockNode(const QString& gid, const QString& lid, int lidVal,
                        const QString& n, Block::BlockType t, Block* b)
        : globalId(gid), linkId(lid), localId(lidVal), name(n), type(t), block(b) {}
};

class SignalFlowLinkSort
{
public:
    SignalFlowLinkSort();
    // ==================== 新增跨层级拓扑排序接口 ====================
    /**
     * @brief 执行跨层级的处理器拓扑排序
     * @param mainLinkKey 主链路ID
     * @param allBlocksInfo 所有链路的块信息
     * @param allConnections 所有链路的连接信息
     * @return 排序后的处理器列表
     */
    QVector<Block*> sortProcessorsCrossLayer(
        const QString& mainLinkKey,
        const QMap<QString, QVector<BlockInfo>>& allBlocksInfo,
        const QMap<QString, QVector<Connection>>& allConnections);

private:
    // ==================== 新增跨层级拓扑排序私有方法 ====================
    /**
     * @brief 递归收集所有原子块
     * @param linkKey 当前链路ID
     * @param allBlocksInfo 所有链路的块信息
     */
    void collectAllAtomicBlocks(
        const QString& linkKey,
        const QMap<QString, QVector<BlockInfo>>& allBlocksInfo);

    /**
     * @brief 构建跨层级的依赖关系
     * @param linkKey 当前链路ID
     * @param allBlocksInfo 所有链路的块信息
     * @param allConnections 所有链路的连接信息
     */
    void buildCrossLayerDependencies(
        const QString& linkKey,
        const QMap<QString, QVector<BlockInfo>>& allBlocksInfo,
        const QMap<QString, QVector<Connection>>& allConnections);

    /**
     * @brief 处理单个连接，建立跨层级依赖
     * @param conn 连接信息
     * @param currentLinkKey 当前链路ID
     * @param allBlocksInfo 所有链路的块信息
     * @param allConnections 所有链路的连接信息
     */
    void processConnectionForDependencies(
        const Connection& conn,
        const QString& currentLinkKey,
        const QMap<QString, QVector<BlockInfo>>& allBlocksInfo,
        const QMap<QString, QVector<Connection>>& allConnections);

    /**
     * @brief 追踪到原子块（完全穿透子系统）
     * @param blockInfo 当前块信息
     * @param port 端口信息
     * @param isSource true: 找源端, false: 找目标端
     * @param currentLinkKey 当前链路ID
     * @param allBlocksInfo 所有链路的块信息
     * @param allConnections 所有链路的连接信息
     * @return 原子块的全局ID列表
     */
    QVector<QString> traceToAtomicBlocks(
        const BlockInfo& blockInfo,
        const PortMsg& port,
        bool isSource,
        const QString& currentLinkKey,
        const QMap<QString, QVector<BlockInfo>>& allBlocksInfo,
        const QMap<QString, QVector<Connection>>& allConnections);

    /**
     * @brief 对处理器进行拓扑排序
     * @return 排序后的处理器全局ID列表
     */
    QVector<QString> topologicalSortProcessors();

private:
    // ==================== 新增跨层级拓扑排序成员变量 ====================
    QMap<QString, CrossLayerBlockNode> m_globalBlocks;      // 全局ID -> 块节点
    QSet<QString> m_processorGlobalIds;                      // 处理器的全局ID集合

    // 调试信息
    mutable QVector<QString> m_lastTraceInfo;                // 上次追踪的调试信息
};

#endif // SIGNALFLOWLINKSORT_H
