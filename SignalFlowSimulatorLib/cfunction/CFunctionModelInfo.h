#ifndef CFUNCTIONMODELINFO_H
#define CFUNCTIONMODELINFO_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "Block.h"
#include "Variable.h"
#include "VariableScopeManager.h"
#include "ExpressionResolver.h"
#include "VarExpressionParse.h"
#include "unitconvert.h"
#include "../Common/LogExport.h"

namespace SystemVueModelBuilder {
    class CFunctionBlock;
}

// CFunction端口信息
struct CFunctionPortMsg {
    int id = -1;
    QString putType;            // in / out
    QString name;
    QString dataType;
    bool isOptional = false;
    int portRate = 1;

    CFunctionPortMsg() = default;

    PortMsg toPortMsg() const {
        PortMsg port;
        port.id = id;
        port.putType = putType;
        port.name = name;
        port.isOptional = isOptional;
        port.dataType = UnitConvert::convertToDataType(dataType);
        port.portRate = portRate;
        port.topProtId = -1;
        return port;
    }
};

// CFunction参数信息（attribute）
struct CFunctionParameter {
    QString name;
    QString value;
    QString originalValue;
    QString dataType;
    QStringList selectOptions;  // 枚举时有效

    CFunctionParameter() = default;

    SystemVueModelBuilder::Parameter toParameter() const {
        SystemVueModelBuilder::Parameter param;
        param.Name = name.toStdString();
        param.Value = value.toStdString();
        return param;
    }
};

// CFunction编译配置（configData）
struct CFunctionConfigData {
    struct FileEntry {
        QString path;
        QString name;
    };

    QString language;                   // "c" 或 "cpp"
    QVector<FileEntry> libFiles;        // 动态库/静态库
    QVector<FileEntry> headerFiles;     // 头文件
    QVector<FileEntry> cFiles;          // 源文件

    CFunctionConfigData() = default;
};

// CFunction模型完整信息
struct CFunctionModelInfo {
    // 基础信息
    int cmpId = -1;
    QString instanceName;
    QString cmpType;                    // "CFunction"

    // CFunction特有字段
    CFunctionConfigData configData;     // 编译配置
    QString equations;                  // 源代码（Equations字段）

    // 端口和参数
    QMap<int, CFunctionPortMsg> ports;
    QMap<QString, CFunctionParameter> parameters;

    // 生成的cfunction.json绝对路径
    QString generatedJsonPath;

    // 仿真参数
    SimuParameter simuParams;

    CFunctionModelInfo() = default;

    // 转换为通用BlockInfo
    BlockInfo toBlockInfo() const {
        BlockInfo blockInfo;
        blockInfo.cmpId = cmpId;
        blockInfo.cmpType = cmpType;
        blockInfo.instanceName = instanceName;
        blockInfo.isSubSystem = false;

        for (auto it = ports.begin(); it != ports.end(); ++it) {
            blockInfo.portsMsg[it.key()] = it.value().toPortMsg();
        }

        for (auto it = parameters.begin(); it != parameters.end(); ++it) {
            blockInfo.parameters[it.key().toStdString()] = it.value().toParameter();
        }

        // 填充CFunction特有字段
        blockInfo.isCFunctionModel = true;
        blockInfo.cfunctionLanguage = configData.language;
        for (const auto& f : configData.libFiles) {
            blockInfo.cfunctionLibFilePaths.append(f.path);
            blockInfo.cfunctionLibFileNames.append(f.name);
        }
        for (const auto& f : configData.headerFiles) {
            blockInfo.cfunctionHeaderFilePaths.append(f.path);
            blockInfo.cfunctionHeaderFileNames.append(f.name);
        }
        for (const auto& f : configData.cFiles) {
            blockInfo.cfunctionCFilePaths.append(f.path);
            blockInfo.cfunctionCFileNames.append(f.name);
        }
        blockInfo.cfunctionEquations = equations;
        blockInfo.cfunctionGeneratedJsonPath = generatedJsonPath;

        return blockInfo;
    }

    Block* createBlock() const;
};

// CFunction模型解析器
class CFunctionModelParser {
public:
    CFunctionModelParser() = default;
    ~CFunctionModelParser() = default;

    // 主解析接口：从链路JSON解析CFunction模型信息
    bool parseCFunctionModel(
        const QString& currentLinkKey,
        const QJsonObject& cmpObj,
        const QVector<Variable>& currentVars,
        const SimuParameter& simuPara,
        VariableScopeManager& scopeMgr,
        ExpressionResolver& resolver,
        CFunctionModelInfo& outModelInfo);

    // 生成cfunction.json文件，返回文件绝对路径
    QString generateCFunctionJson(
        const CFunctionModelInfo& modelInfo,
        const QString& outputDir);

    // 读取cfunction.json的output字段，填充输出数据
    bool readCFunctionOutput(
        const QString& jsonPath,
        QVector<QPair<QString, QVector<double>>>& outputs);

private:
    // 解析configData字段
    bool parseConfigData(const QJsonObject& cmpObj, CFunctionModelInfo& modelInfo);

    // 解析Equations字段
    bool parseEquations(const QJsonObject& cmpObj, CFunctionModelInfo& modelInfo);

    // 解析attribute字段（自定义参数）
    bool parseAttributes(
        const QString& currentLinkKey,
        const QJsonObject& cmpObj,
        const QVector<Variable>& currentVars,
        VariableScopeManager& scopeMgr,
        ExpressionResolver& resolver,
        CFunctionModelInfo& modelInfo);

    // 解析port字段（输入输出端口）
    bool parsePorts(const QJsonObject& cmpObj, CFunctionModelInfo& modelInfo);

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

#endif // CFUNCTIONMODELINFO_H
