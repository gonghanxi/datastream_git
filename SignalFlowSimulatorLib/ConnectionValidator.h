#ifndef CONNECTIONVALIDATOR_H
#define CONNECTIONVALIDATOR_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QString>
#include <QSet>
#include <QDebug>
#include "algorithmmanager.h"

class PortValidatorImpl;
class ConnectionValidator
{
public:
    // 路径节点结构体
    struct PathNode {
        QString blockId;
        QString portId;
        QString instanceName;
        QString portName;
        QString linkKey;  // 该节点所在的链路key
    };
    // 校验结果结构体
    struct ValidationResult {
        bool isValid;
        QString errorMessage;
        QList<QString> warnings;

        ValidationResult() : isValid(true) {}
        ValidationResult(bool valid, const QString& msg = "")
            : isValid(valid), errorMessage(msg) {}
    };

    //实际系统结构体
    struct ActualConnection {
        QString srcBlockId;      // 实际源块ID
        QString srcPortId;       // 实际源端口ID
        QString dstBlockId;      // 实际目标块ID
        QString dstPortId;       // 实际目标端口ID
        QString srcLinkKey;      // 源块所在的链路key
        QString dstLinkKey;      // 目标块所在的链路key
        QString originalPath;    // 原始连接路径（用于错误信息）

        ActualConnection() : srcLinkKey(""), dstLinkKey("") {}
        ActualConnection(const QString& sbid, const QString& spid,
                        const QString& dbid, const QString& dpid,
                        const QString& sLinkKey, const QString& dLinkKey,
                        const QString& path = "")
            : srcBlockId(sbid), srcPortId(spid),
              dstBlockId(dbid), dstPortId(dpid),
              srcLinkKey(sLinkKey), dstLinkKey(dLinkKey),
              originalPath(path) {}
    };

    // 初始化校验器
    ConnectionValidator(const QVector<BlockInfo>& blocksInfo,
                       const QVector<Connection>& connections,
                       const QString& linkKey = "");

    //析构函数,用于实现类智能指针的自动释放
    //不能直接default
    ~ConnectionValidator();

    // 执行所有校验
    ValidationResult validateAll();

    // 各个校验方法的详细实现
    ValidationResult validatePortDirection();     // 端口方向校验

    ValidationResult validatePortDataType();      // 端口数据类型兼容性

    ValidationResult validateInputPortConnections(); // 检查输入端口连接完整性

    ValidationResult validateBusRequirements();   // Bus要求校验

    ValidationResult validateCyclicDependencies();// 闭环检测

    ValidationResult validateLinkCompleteness();  // 链路完整性
    ValidationResult validateSubLinkCompleteness(const QString& linkKey, QList<QString>& allIsolatedBlocks); //递归校验子链路的完整性

    ValidationResult validatePenetratedDataTypes();  // 穿透校验


    // 获取实际连接（子系统穿透连接）
    QVector<ActualConnection> getActualConnections();

private:
    // 端口数据结构
    struct PortInfo {
        QString instanceName;
        QString portName;
        QString putType;
        PortMsg::PortDataType dataType;
        int portId;
        int topProtId;
        bool isBusPort;
        bool isOptional;

        PortInfo()
            : instanceName()
            , portName()
            , putType()
            , dataType(PortMsg::PortDataType::INT)
            , portId(-1)
            , topProtId(-1)
            , isBusPort(false)
            , isOptional(false)
        {}
    };

    // 成员变量
    QVector<BlockInfo> m_blocksInfo;
    QVector<Connection> m_connections;
    QString m_linkKey;

    // 新增成员变量，存储所有链路的信息
    QMap<QString, QVector<BlockInfo>> m_allBlocksInfo;
    QMap<QString, QVector<Connection>> m_allConnections;

    // 当前链路的 linkKey
    QString m_currentLinkKey;

    QList<PortInfo> m_portInfoCache;  // 用于存储动态创建的 PortInfo

    // 内部查找结构
    QMap<int, BlockInfo*> m_blockIdMap;          // blockId -> BlockInfo*
    QMap<QString, QVector<BlockInfo>> m_linkKeyBlocksMap;  // linkKey -> 该链路的所有BlockInfo
    QMap<int, QString> m_blockIdToLinkKeyMap;              // blockId -> 所属linkKey
    QMap<QString, QMap<int, PortInfo>> m_linkKeyPortsMap;  // linkKey -> (portKey -> PortInfo)

    QMap<QString, QList<PortInfo>> m_blockPortsMap; // instanceName -> PortInfo列表
    QMap<QString, PortInfo*> m_portLookup; // blockId+portId -> PortInfo*

    // 校验结果
    QList<QString> m_topologicalOrder;
    QList<Connection> m_invalidConnections;
    QMap<QString, int> m_inDegree;  // 用于拓扑排序的入度表
    QMap<QString, QList<QString>> m_dependencyGraph;  // 依赖图

    QMap<QString, QVector<BlockInfo>> m_allBlocksInfoMap;
    QMap<QString, QVector<Connection>> m_allConnectionsMap;

    //功能实现类
    std::unique_ptr<PortValidatorImpl> m_porter;
    friend class PortValidatorImpl;

    // 私有方法
    void initializeDataStructures();
    PortInfo* findPortInfo(int blockId, int portId);//查找PortInfo
    BlockInfo* findBlockInfo(int blockId);//BlockInfo
    bool isBusDataType(PortMsg::PortDataType dataType);//是否属于bus类型
    QString dataTypeToString(PortMsg::PortDataType dataType);//数据类型 -> string

    // 构建端口映射表
    QMap<int, ActualConnection> buildPortMapping(
            const QString& childTopoId);
    //构建路径字符串
    QString buildPathString(const QVector<PathNode>& path);
    //通过连接关系，查找目标模型/端口，源模型/端口
    QString findConnectedModel(
        const QVector<Connection>& connections,
        const QString& fromBlockId,
        const QString& fromPortId);

    QString findConnectedPort(
        const QVector<Connection>& connections,
        const QString& fromBlockId,
        const QString& fromPortId);

    QString findSourceModel(
        const QVector<Connection>& connections,
        const QString& toBlockId,
        const QString& toPortId);

    QString findSourcePort(
        const QVector<Connection>& connections,
        const QString& toBlockId,
        const QString& toPortId);
    //通过链路查找BlockInfo Connection
    QVector<BlockInfo> getBlocksInfoByLinkKey(const QString& linkKey);
    QVector<Connection> getConnectionsByLinkKey(const QString& linkKey);
    // 查找inPort/outPort连接的方法
    QString findInPortConnection(const QString& childTopoId, int topProtId);
    QString findOutPortConnection(const QString& childTopoId, int topProtId);

    //递归追踪完整路径
    void traceForwardPath(
        BlockInfo* currentBlock, PortInfo* currentPort,
        BlockInfo* targetBlock, PortInfo* targetPort,
        QVector<PathNode>& path,
        QVector<ActualConnection>& results,
        QSet<QString>& visitedLinks,
        bool isPenetrated);
    //通过链路key 查找
    PortInfo* findPortInfoInLink(const QString& linkKey, int blockId, int portId, QList<PortInfo>& portCache);
    BlockInfo* findBlockInfoInLink(const QString& linkKey, int blockId);


    //通过子系统的端口查找 出入口模型
    QString findInPortBySubsystemPort(const QString& childTopoId, int subsystemPortId);
    QString findOutPortBySubsystemPort(const QString& childTopoId, int subsystemPortId);

    //通过 Block查找所属链路
    QString getLinkKeyByBlock(int blockId);


};





#endif // CONNECTIONVALIDATOR_H
