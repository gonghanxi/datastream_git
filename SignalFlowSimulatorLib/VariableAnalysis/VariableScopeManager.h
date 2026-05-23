// VariableScopeManager.h
#ifndef VARIABLESCOPEMANAGER_H
#define VARIABLESCOPEMANAGER_H

#include <QString>
#include <QMap>
#include <QVector>
#include <QStack>
#include "Variable.h"

//变量作用域管理器
class VariableScopeManager {
public:
    VariableScopeManager();

    // 作用域节点
    struct ScopeNode {
        QString scopeId;                    // 链路ID
        QString parentScopeId;             // 父链路ID
        QMap<QString, Variable> variables; // 本层定义的变量
        QMap<QString, QString> parameterOverrides; // 参数覆盖值
        QMap<QString, QString> paramToVarMapping;  // 参数名 → 变量名

        // 缓存解析后的值
        QMap<QString, QString> resolvedValueCache;
        bool cacheValid;

        ScopeNode() : cacheValid(false) {}
    };

public:
    // 作用域管理
    void registerScope(const QString& scopeId, const QString& parentScopeId = "");
    bool hasScope(const QString& scopeId) const;

    // 变量管理
    void setScopeVariables(const QString& scopeId, const QVector<Variable>& vars);
    void addScopeVariable(const QString& scopeId, const Variable& var);

    // 参数覆盖管理
    void setParameterOverride(const QString& scopeId,
                             const QString& varName,
                             const QString& value);
    bool hasParameterOverride(const QString& scopeId, const QString& varName) const;
    QString getParameterOverride(const QString& scopeId, const QString& varName) const;

    // 参数映射管理
    void setParameterMapping(const QString& scopeId,
                            const QString& paramName,
                            const QString& varName);
    QString getVariableNameByParameter(const QString& scopeId,
                                      const QString& paramName) const;

    // 变量值解析（核心方法）
    QString resolveVariableValue(const QString& scopeId,
                                const QString& varName,
                                bool* found = nullptr);

    // 批量解析
    QMap<QString, QString> resolveAllVariables(const QString& scopeId);

    // 缓存管理
    void invalidateCache(const QString& scopeId);
    void invalidateAllCache();

    // 调试
    void dumpScope(const QString& scopeId) const;

private:
    // 向上查找变量定义
    bool findVariableUpward(const QString& scopeId,
                           const QString& varName,
                           Variable& outVar,
                           QString& outScopeId);

    // 检查变量引用
    bool containsVariableReference(const QString& value) const;
    QStringList extractVariableNames(const QString& value) const;

    // 递归解析（处理依赖）
    QString resolveWithDependencies(const QString& scopeId,
                                   const QString& varName,
                                   QSet<QString>& resolving);

private:
    QMap<QString, ScopeNode> m_scopes;
};

#endif // VARIABLESCOPEMANAGER_H
