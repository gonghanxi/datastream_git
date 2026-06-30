// ShortOpenProcessor.h
#ifndef SHORTOPENPROCESSOR_H
#define SHORTOPENPROCESSOR_H

#include <QMap>
#include <QVector>
#include <QString>
#include <QSet>
#include <optional>
#include <memory>

#include "connection.h"

// BlockInfo 的前置声明（实际定义在其他地方）
struct BlockInfo;

// 短路模型信息结构体
struct ShortCircuitedModel {
    QString linkKey;           // 所属链路
    std::shared_ptr<BlockInfo> blockInfo;        // 模型信息
};

class ShortOpenProcessor {
public:
    ShortOpenProcessor() = default;
    ~ShortOpenProcessor() = default;

    /**
     * @brief 处理所有链路中的短路和开路模型
     * @param blocksInfoMap 所有链路的模型信息（会被修改）
     * @param connectionsMap 所有链路的连接关系（会被修改）
     * @return 处理是否成功
     */
    bool processAllLinks(
        QMap<QString, QVector<BlockInfo>>& blocksInfoMap,
        QMap<QString, QVector<Connection>>& connectionsMap);

    /**
     * @brief 获取短路模型容器（信号源和数据收集器）
     */
    QVector<ShortCircuitedModel> getShortCircuitedSourcesAndSinks() const {
        return m_shortCircuitedSourcesAndSinks;
    }

private:
    /**
     * @brief 处理单个链路的短路和开路
     */
    bool processSingleLink(
        const QString& linkKey,
        QVector<BlockInfo>& blocksInfo,
        QVector<Connection>& connections);

    /**
     * @brief 检查模型是否有短路或开路条件
     * @param blockInfo 模型信息
     * @param isShort 输出：是否为短路
     * @param isOpen 输出：是否为开路
     * @return 是否需要处理
     */
    bool checkCondition(const BlockInfo& blockInfo, bool& isShort, bool& isOpen);

    /**
     * @brief 处理开路：删除模型及其相关连接
     */
    void processOpen(
        const BlockInfo& blockInfo,
//        QVector<BlockInfo>& blocksInfo,
        QVector<Connection>& connections);

    /**
     * @brief 处理短路：将所有上游端口连接到所有下游端口
     * @param blockInfo 短路模型信息
     * @param connections 连接关系列表（会被修改）
     * @param blocksInfo 当前链路的所有模型信息（用于查找对端端口方向）
     */
    void processShort(
        const BlockInfo& blockInfo,
        QVector<Connection>& connections,
        const QVector<BlockInfo>& blocksInfo);

    /**
     * @brief 获取模型的所有输入端口（下游）ID
     */
    QSet<int> getInputPortIds(const BlockInfo& blockInfo) const;

    /**
     * @brief 获取模型的所有输出端口（上游）ID
     */
    QSet<int> getOutputPortIds(const BlockInfo& blockInfo) const;

    /**
     * @brief 获取连接到指定模型指定端口的源连接（上游）
     */
    QVector<Connection> getIncomingConnections(
        const BlockInfo& blockInfo,
        int portId,
        const QVector<Connection>& connections) const;

    /**
     * @brief 获取从指定模型指定端口出发的目标连接（下游）
     */
    QVector<Connection> getOutgoingConnections(
        const BlockInfo& blockInfo,
        int portId,
        const QVector<Connection>& connections) const;

    /**
     * @brief 获取连接到此模型输入端口的所有上游连接（支持反向连接识别）
     * @param blockInfo 目标模型信息
     * @param connections 连接关系列表
     * @param blocksInfo 当前链路的所有模型信息（用于查找对端端口方向）
     */
    QVector<Connection> getAllIncomingConnections(
        const BlockInfo& blockInfo,
        const QVector<Connection>& connections,
        const QVector<BlockInfo>& blocksInfo) const;

    /**
     * @brief 获取从此模型输出端口出发的所有下游连接（支持反向连接识别）
     * @param blockInfo 目标模型信息
     * @param connections 连接关系列表
     * @param blocksInfo 当前链路的所有模型信息（用于查找对端端口方向）
     */
    QVector<Connection> getAllOutgoingConnections(
        const BlockInfo& blockInfo,
        const QVector<Connection>& connections,
        const QVector<BlockInfo>& blocksInfo) const;

    /**
     * @brief 从连接列表中删除与指定模型相关的所有连接
     */
    void removeConnectionsForBlock(
        int blockId,
        QVector<Connection>& connections);

    /**
     * @brief 从模型列表中删除指定模型
     */
    void removeBlockFromList(
        int blockId,
        QVector<BlockInfo>& blocksInfo);

    /**
     * @brief 添加新的连接（避免重复）
     */
    void addConnectionIfNotExists(
        const QString& fromModelId,
        const QString& fromPortId,
        const QString& toModelId,
        const QString& toPortId,
        QVector<Connection>& connections);

    /**
     * @brief 判断两个连接是否相同
     */
    bool isSameConnection(
        const Connection& conn1,
        const Connection& conn2) const;

    /**
     * @brief 解析 cmpCondition 字符串
     * @param condition 条件字符串，如 "short" 或 "open"
     * @return true=短路, false=开路, 空值表示无处理
     */
    std::optional<bool> parseCondition(const QString& condition) const;

    /**
     * @brief 查找指定块的指定端口的 putType
     * @param blockId 模型ID
     * @param portId 端口ID
     * @param blocksInfo 模型信息列表
     * @return "in"/"out"，找不到返回空字符串
     */
    QString findPortPutType(int blockId, int portId,
                            const QVector<BlockInfo>& blocksInfo) const;

    // 存储短路且为信号源或数据收集器的模型
    QVector<ShortCircuitedModel> m_shortCircuitedSourcesAndSinks;
};

#endif // SHORTOPENPROCESSOR_H
