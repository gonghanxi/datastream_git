// LinkParser.h
#ifndef LINKPARSER_H
#define LINKPARSER_H

#include <QString>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QVector>

#include "Variable.h"
#include "VariableScopeManager.h"
#include "SubsystemParameterMapper.h"
#include "ExpressionResolver.h"
#include "MathExpressionCalculator.h"
#include "VarExpressionParse.h"
#include "Block.h"
#include "algorithmmanager.h"
#include "unitconvert.h"
#include "../Common/LogExport.h"

// 解析结果结构体
struct ParseResult {
    bool success;
    QString errorMessage;
    QString mainLinkKey;
    QMap<QString, SimuParameter> simuParameters;

    ParseResult() : success(false) {}
};

class LinkParser {
public:
    LinkParser() = default;
    ~LinkParser() = default;

    // 主解析接口
    ParseResult parseLinkFiles(
        const QString& appPath,
        const QStringList& linkFiles,
        const QString& outPutPath);

    // 获取解析后的数据（供SimRunner使用）
    QMap<QString, QVector<BlockInfo>> getBlocksInfo() const;
    QMap<QString, QVector<Connection>> getConnections() const;
    QMap<QString, SimuParameter> getSimuParameters() const;

private:
    // ========== 第一阶段：收集和注册 ==========
    bool collectLinkInfo(
        const QString& linkFile,
        QMap<QString, QJsonObject>& linkObjects,
        QMap<QString, QString>& parentLinkMap,
        QString& mainLinkKey,
        QMap<QString, SimuParameter>& simuParams,
        QMap<QString, QVector<Variable>>& allVariables);

    bool parseSimuParameters(
        const QJsonObject& firstObj,
        const QString& mainLinkKey,
        QMap<QString, SimuParameter>& simuParams);

    bool parseVariables(
        const QJsonObject& jsonObj,
        const QString& linkKey,
        QMap<QString, QVector<Variable>>& allVariables);

    // ========== 第二阶段：设置作用域 ==========
    void setupScopes(
        VariableScopeManager& scopeMgr,
        const QMap<QString, QVector<Variable>>& allVariables);

    // ========== 第三阶段：处理子系统映射 ==========
    bool processSubsystemMapping(
        SubsystemParameterMapper& paramMapper,
        const QMap<QString, QJsonObject>& linkObjects,
        const QMap<QString, QString>& parentLinkMap);

    // ========== 第四阶段：解析模型和参数 ==========
    bool parseModelsAndParameters(
        const QString& appPath,
        const QString& outPutPath,
        const QMap<QString, QJsonObject>& linkObjects,
        const QMap<QString, SimuParameter>& simuParams,
        const QMap<QString, QVector<Variable>>& allVariables,
        VariableScopeManager& scopeMgr,
        ExpressionResolver& resolver,
        QMap<QString, QVector<BlockInfo>>& allBlocksInfo,
        QMap<QString, QVector<Connection>>& allConnections);

    bool parseSingleModel(
        const QString& currentLinkKey,
        const QJsonObject& cmpObj,
        const QString& appPath,
        const QString& outPutPath,
        const SimuParameter& simuPara,
        const QVector<Variable>& currentVars,
        VarExpressionParse& varParser,
        VariableScopeManager& scopeMgr,
        ExpressionResolver& resolver,
        QVector<BlockInfo>& blocksInfo);

    bool parsePorts(
        const QJsonObject& cmpObj,
        BlockInfo& blockInfo,
        bool& topProtIdExist);

    bool parseParameters(
        const QString& currentLinkKey,
        const QJsonObject& cmpObj,
        BlockInfo& blockInfo,
        const QVector<Variable>& currentVars,
        VarExpressionParse& varParser,
        VariableScopeManager& scopeMgr,
        ExpressionResolver& resolver);

    // ========== 第五阶段：解析连接关系 ==========
    bool parseConnections(
        const QJsonObject& jsonObj,
        const QString& currentLinkKey,
        QMap<QString, QVector<Connection>>& allConnections);

    // ========== 辅助方法 ==========
    QJsonDocument readJsonFile(const QString& filePath);
    QString extractId(const QString& id, const QString& prefix);
    bool dataCollectionCheck(const QMap<QString, QVector<BlockInfo>>& allBlocksInfo);
    SimuParameter getSimuParameterByLinkKey(
        const QString& linkKey,
        const QMap<QString, SimuParameter>& simuParams,
        const QString& mainLinkKey);

private:
    // 存储解析结果
    QMap<QString, QVector<BlockInfo>> m_blocksInfo;
    QMap<QString, QVector<Connection>> m_connections;
    QMap<QString, SimuParameter> m_simuParameters;
    QStringList m_subsystemPathStack;  // 子系统路径栈
};

#endif // LINKPARSER_H
