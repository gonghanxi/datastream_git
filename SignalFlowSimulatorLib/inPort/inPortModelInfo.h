#ifndef INPORTMODELINFO_H
#define INPORTMODELINFO_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include "Block.h"
#include "Variable.h"
#include "VariableScopeManager.h"
#include "ExpressionResolver.h"
#include "VarExpressionParse.h"
#include "unitconvert.h"
#include "../Common/LogExport.h"

namespace SystemVueModelBuilder {
    class inPortBlock;
}
// inPort端口信息
struct inPortPortMsg {
    int id = -1;                    // 端口ID
    QString putType;                // 方向: in/out
    QString name;                   // 端口名称
    QString dataType;               // 数据类型
    bool isOptional = false;        // 是否可选
    int portRate = 0;               // 端口速率
    int topProtId = -1;             // 顶层端口ID（用于子系统穿透）

    inPortPortMsg() = default;

    // 转换为通用PortMsg
    PortMsg toPortMsg() const {
        PortMsg port;
        port.id = id;
        port.putType = putType;
        port.name = name;
        port.isOptional = isOptional;
        port.dataType = UnitConvert::convertToDataType(dataType);
        port.portRate = portRate;
        port.topProtId = topProtId;
        return port;
    }
};

// inPort参数信息
struct inPortParameter {
    QString name;                   // 参数名称
    QString value;                  // 参数值（转换后）
    QString originalValue;          // 原始值（表达式）
    QString dataType;               // 数据类型
    QString unitType;               // 单位类型
    QString unit;                   // 单位
    bool isDefault = false;         // 是否使用默认值
    QString calculateValue;         // 计算值

    inPortParameter() = default;

    // 转换为通用Parameter
    Parameter toParameter() const {
        Parameter param;
        param.Name = name.toStdString();
        param.Value = value.toStdString();
        return param;
    }
};

// inPort模型完整信息
struct inPortModelInfo {
    // 基础信息
    int cmpId = -1;                 // 组件ID
    QString instanceName;           // 实例名称
    QString cmpType;                // 组件类型
    bool isSubSystem = false;       // 是否子系统
    QString childTopoId;            // 子拓扑ID（如果是子系统）
    QString subsystemPath;          // 子系统路径
    SimuParameter m_Sima;             // 仿真器参数

    // 端口和参数
    QMap<int, inPortPortMsg> inPorts;           // inPort端口映射
    QMap<QString, inPortParameter> inPortParameters; // inPort参数映射

    // 运行时实例（由FMU引擎创建）
    void* inPortInstance = nullptr;    // inPort实例指针

    inPortModelInfo() = default;

    // 转换为通用BlockInfo
    BlockInfo toBlockInfo() const {
        BlockInfo blockInfo;
        blockInfo.cmpId = cmpId;
        blockInfo.isSubSystem = isSubSystem;
        blockInfo.cmpType = cmpType;
        blockInfo.instanceName = instanceName;
        blockInfo.childTopoId = childTopoId;
        blockInfo.isinPort = true;
        blockInfo.subsystemPath = subsystemPath;
        blockInfo.block = nullptr;  // 由inPort引擎创建

        // 转换端口
        for (auto it = inPorts.begin(); it != inPorts.end(); ++it) {
            blockInfo.portsMsg[it.key()] = it.value().toPortMsg();
        }

        // 转换参数
        for (auto it = inPortParameters.begin(); it != inPortParameters.end(); ++it) {
            blockInfo.parameters[it.key().toStdString()] = it.value().toParameter();
        }

        return blockInfo;
    }

    Block* createBlock() const;
//    FMUBlock* createBlock() const;
};

// inPort模型解析器类
class inPortModelParser {
public:
    inPortModelParser() = default;
    ~inPortModelParser() = default;

    // 主解析接口
    bool parseinPortModel(
        const QString& currentLinkKey,
        const QJsonObject& cmpObj,
        const QVector<Variable>& currentVars,
        const SimuParameter& simuPara,
        VariableScopeManager& scopeMgr,
        ExpressionResolver& resolver,
        inPortModelInfo& outModelInfo);

private:
    // 解析端口（包含 valueReference）
    bool parsePorts(const QJsonObject& cmpObj, inPortModelInfo& modelInfo);

    // 解析参数（包含 valueReference）
    bool parseParameters(
        const QString& currentLinkKey,
        const QJsonObject& cmpObj,
        const QVector<Variable>& currentVars,
        VariableScopeManager& scopeMgr,
        ExpressionResolver& resolver,
        inPortModelInfo& modelInfo);

    // 解析表达式值
    QString resolveParameterValue(
        const QString& currentLinkKey,
        const QString& paramName,
        const QString& paramVal,
        const QString& calculateValueStr,
        const QVector<Variable>& currentVars,
        VariableScopeManager& scopeMgr,
        ExpressionResolver& resolver);

    // 辅助方法
    QString extractId(const QString& id, const QString& prefix);
};

#endif // INPORTMODELINFO_H
