// FMUModelInfo.h
#ifndef FMUMODELINFO_H
#define FMUMODELINFO_H

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
    class FMUBlock;
}
// FMU端口信息
struct FMUPortMsg {
    int id = -1;                    // 端口ID
    QString putType;                // 方向: in/out
    QString name;                   // 端口名称
    int valueReference = -1;        // FMU value reference (关键字段)
    QString dataType;               // 数据类型
    bool isOptional = false;        // 是否可选
    int portRate = 0;               // 端口速率
    int topProtId = -1;             // 顶层端口ID（用于子系统穿透）

    FMUPortMsg() = default;

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

// FMU参数信息
struct FMUParameter {
    QString name;                   // 参数名称
    QString value;                  // 参数值（转换后）
    QString originalValue;          // 原始值（表达式）
    int valueReference = -1;        // FMU value reference (关键字段)
    QString dataType;               // 数据类型
    QString unitType;               // 单位类型
    QString unit;                   // 单位
    bool isDefault = false;         // 是否使用默认值
    QString calculateValue;         // 计算值

    FMUParameter() = default;

    // 转换为通用Parameter
    Parameter toParameter() const {
        Parameter param;
        param.Name = name.toStdString();
        param.Value = value.toStdString();
        return param;
    }
};

// FMU模型完整信息
struct FMUModelInfo {
    // 基础信息
    int cmpId = -1;                 // 组件ID
    QString instanceName;           // 实例名称
    QString cmpType;                // 组件类型 (应为 "Fmu")
    QString guid;                   // 组件GUID（全局唯一标识符）
    bool isSubSystem = false;       // 是否子系统
    QString childTopoId;            // 子拓扑ID（如果是子系统）
    QString subsystemPath;          // 子系统路径
    SimuParameter m_Sima;             // 仿真器参数

    // FMU特有字段
    QVector<QString> dllOrSoPaths;  // 动态库路径列表（多个平台）

    // 端口和参数
    QMap<int, FMUPortMsg> fmuPorts;           // FMU端口映射
    QMap<QString, FMUParameter> fmuParameters; // FMU参数映射

    // 运行时实例（由FMU引擎创建）
    void* fmuInstance = nullptr;    // FMU实例指针

    FMUModelInfo() = default;

    // 转换为通用BlockInfo
    BlockInfo toBlockInfo() const {
        BlockInfo blockInfo;
        blockInfo.cmpId = cmpId;
        blockInfo.isSubSystem = isSubSystem;
        blockInfo.cmpType = cmpType;
        blockInfo.instanceName = instanceName;
        blockInfo.guid = guid;
        blockInfo.childTopoId = childTopoId;
        blockInfo.subsystemPath = subsystemPath;
        blockInfo.isFmuModel = true;
        blockInfo.dllOrSoPaths = dllOrSoPaths;
        blockInfo.block = nullptr;  // 由FMU引擎创建

        // 转换端口
        for (auto it = fmuPorts.begin(); it != fmuPorts.end(); ++it) {
            blockInfo.portsMsg[it.key()] = it.value().toPortMsg();
            blockInfo.portValueReferences[it.key()] = it.value().valueReference;
        }

        // 转换参数
        for (auto it = fmuParameters.begin(); it != fmuParameters.end(); ++it) {
            blockInfo.parameters[it.key().toStdString()] = it.value().toParameter();
            blockInfo.paramValueReferences[it.key()] = it.value().valueReference;
        }

        return blockInfo;
    }

    Block* createBlock() const;
//    FMUBlock* createBlock() const;
};

// FMU模型解析器类
class FMUModelParser {
public:
    FMUModelParser() = default;
    ~FMUModelParser() = default;

    // 主解析接口
    bool parseFMUModel(
        const QString& currentLinkKey,
        const QJsonObject& cmpObj,
        const QVector<Variable>& currentVars,
        const SimuParameter& simuPara,
        VariableScopeManager& scopeMgr,
        ExpressionResolver& resolver,
        FMUModelInfo& outModelInfo);

private:
    // 解析 dllORso 路径
    bool parseDllOrSoPaths(const QJsonObject& cmpObj, FMUModelInfo& modelInfo);

    // 解析端口（包含 valueReference）
    bool parsePorts(const QJsonObject& cmpObj, FMUModelInfo& modelInfo);

    // 解析参数（包含 valueReference）
    bool parseParameters(
        const QString& currentLinkKey,
        const QJsonObject& cmpObj,
        const QVector<Variable>& currentVars,
        VariableScopeManager& scopeMgr,
        ExpressionResolver& resolver,
        FMUModelInfo& modelInfo);

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

#endif // FMUMODELINFO_H
