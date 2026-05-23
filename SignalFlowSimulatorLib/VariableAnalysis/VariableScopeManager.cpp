// VariableScopeManager.cpp
#include "VariableScopeManager.h"
#include "MathExpressionCalculator.h"
#include <QDebug>
#include <QRegularExpression>

VariableScopeManager::VariableScopeManager()
{
}

void VariableScopeManager::registerScope(const QString& scopeId, const QString& parentScopeId)
{
    if (!m_scopes.contains(scopeId)) {
        ScopeNode node;
        node.scopeId = scopeId;
        node.parentScopeId = parentScopeId;
        m_scopes[scopeId] = node;

        qDebug() << "注册作用域:" << scopeId << "父作用域:" << parentScopeId;
    }
}

bool VariableScopeManager::hasScope(const QString& scopeId) const
{
    return m_scopes.contains(scopeId);
}

void VariableScopeManager::setScopeVariables(const QString& scopeId, const QVector<Variable>& vars)
{
    if (!m_scopes.contains(scopeId)) {
        registerScope(scopeId);
    }

    ScopeNode& node = m_scopes[scopeId];
    node.variables.clear();

    for (const Variable& var : vars) {
        node.variables[var.name] = var;
    }

    node.cacheValid = false;

    qDebug() << "设置作用域变量:" << scopeId << "变量数:" << vars.size();
}

void VariableScopeManager::addScopeVariable(const QString& scopeId, const Variable& var)
{
    if (!m_scopes.contains(scopeId)) {
        registerScope(scopeId);
    }

    m_scopes[scopeId].variables[var.name] = var;
    m_scopes[scopeId].cacheValid = false;
}

void VariableScopeManager::setParameterOverride(const QString& scopeId,
                                               const QString& varName,
                                               const QString& value)
{
    if (!m_scopes.contains(scopeId)) {
        qWarning() << "作用域不存在:" << scopeId;
        return;
    }

    m_scopes[scopeId].parameterOverrides[varName] = value;
    m_scopes[scopeId].cacheValid = false;

    qDebug() << "设置参数覆盖:" << scopeId << varName << "=" << value;
}

bool VariableScopeManager::hasParameterOverride(const QString& scopeId, const QString& varName) const
{
    if (!m_scopes.contains(scopeId)) return false;
    return m_scopes[scopeId].parameterOverrides.contains(varName);
}

QString VariableScopeManager::getParameterOverride(const QString& scopeId, const QString& varName) const
{
    if (!m_scopes.contains(scopeId)) return QString();
    return m_scopes[scopeId].parameterOverrides.value(varName);
}

void VariableScopeManager::setParameterMapping(const QString& scopeId,
                                              const QString& paramName,
                                              const QString& varName)
{
    if (!m_scopes.contains(scopeId)) {
        registerScope(scopeId);
    }

    m_scopes[scopeId].paramToVarMapping[paramName] = varName;
}

QString VariableScopeManager::getVariableNameByParameter(const QString& scopeId,
                                                        const QString& paramName) const
{
    if (!m_scopes.contains(scopeId)) return paramName; // 默认同名
    return m_scopes[scopeId].paramToVarMapping.value(paramName, paramName);
}

bool VariableScopeManager::findVariableUpward(const QString& scopeId,
                                             const QString& varName,
                                             Variable& outVar,
                                             QString& outScopeId)
{
    if (!m_scopes.contains(scopeId)) {
        return false;
    }

    const ScopeNode& node = m_scopes[scopeId];

    // 在当前作用域查找
    if (node.variables.contains(varName)) {
        outVar = node.variables[varName];
        outScopeId = scopeId;
        return true;
    }

    // 向上查找父作用域
    if (!node.parentScopeId.isEmpty()) {
        return findVariableUpward(node.parentScopeId, varName, outVar, outScopeId);
    }

    return false;
}

bool VariableScopeManager::containsVariableReference(const QString& value) const
{
    static QRegularExpression varRegex("\\b[a-zA-Z_][a-zA-Z0-9_]*\\b");
    return varRegex.match(value).hasMatch();
}

QStringList VariableScopeManager::extractVariableNames(const QString& value) const
{
    QStringList varNames;
    static QRegularExpression varRegex("\\b([a-zA-Z_][a-zA-Z0-9_]*)\\b(?![(])");

    QRegularExpressionMatchIterator it = varRegex.globalMatch(value);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString varName = match.captured(1);

        // 排除数学函数和常量
        if (MathExpressionCalculator::isBuiltInFunction(varName) ||
            MathExpressionCalculator::isBuiltInConstant(varName)) {
            continue;
        }

        // 排除复数单位
        if (varName.toLower() == "i" || varName.toLower() == "j") {
            continue;
        }

        if (!varNames.contains(varName)) {
            varNames.append(varName);
        }
    }

    return varNames;
}

QString VariableScopeManager::resolveWithDependencies(const QString& scopeId,
                                                     const QString& varName,
                                                     QSet<QString>& resolving)
{
    // 检测循环依赖
    if (resolving.contains(varName)) {
        qWarning() << "检测到循环变量依赖:" << varName;
        return QString();
    }

    // 检查缓存
    if (m_scopes.contains(scopeId) && m_scopes[scopeId].cacheValid) {
        if (m_scopes[scopeId].resolvedValueCache.contains(varName)) {
            return m_scopes[scopeId].resolvedValueCache[varName];
        }
    }

    // 1. 优先使用参数覆盖值（这是从父作用域传入的值）
    if (hasParameterOverride(scopeId, varName)) {
        QString overrideValue = getParameterOverride(scopeId, varName);
        qDebug() << "使用参数覆盖值:" << scopeId << varName << "=" << overrideValue;

        resolving.insert(varName);

        // 覆盖值可能还包含变量引用
        QString resolved = overrideValue;
        QStringList refs = extractVariableNames(overrideValue);
        for (const QString& ref : refs) {
            QString refValue = resolveWithDependencies(scopeId, ref, resolving);
            if (!refValue.isNull()) {
                resolved.replace(QRegularExpression("\\b" + ref + "\\b"), refValue);
            }
        }

        resolving.remove(varName);

        // 更新缓存
        if (m_scopes.contains(scopeId)) {
            m_scopes[scopeId].resolvedValueCache[varName] = resolved;
        }

        return resolved;
    }

    // 2. 查找变量定义
    Variable varDef;
    QString defScopeId;
    if (findVariableUpward(scopeId, varName, varDef, defScopeId)) {
        resolving.insert(varName);

        QString value = varDef.defaultValue;
        QStringList refs = extractVariableNames(value);

        // 递归解析依赖的变量
        for (const QString& ref : refs) {
            QString refValue = resolveWithDependencies(defScopeId, ref, resolving);
            if (!refValue.isNull()) {
                value.replace(QRegularExpression("\\b" + ref + "\\b"), refValue);
            }
        }

        resolving.remove(varName);

        // 更新缓存
        if (m_scopes.contains(scopeId)) {
            m_scopes[scopeId].resolvedValueCache[varName] = value;
        }

        return value;
    }

    return QString(); // 未找到
}

QString VariableScopeManager::resolveVariableValue(const QString& scopeId,
                                                  const QString& varName,
                                                  bool* found)
{
    QSet<QString> resolving;
    QString result = resolveWithDependencies(scopeId, varName, resolving);

    if (found) {
        *found = !result.isNull();
    }

    return result;
}

QMap<QString, QString> VariableScopeManager::resolveAllVariables(const QString& scopeId)
{
    QMap<QString, QString> results;

    if (!m_scopes.contains(scopeId)) {
        return results;
    }

    const ScopeNode& node = m_scopes[scopeId];

    // 解析所有变量
    for (auto it = node.variables.begin(); it != node.variables.end(); ++it) {
        QString value = resolveVariableValue(scopeId, it.key());
        if (!value.isNull()) {
            results[it.key()] = value;
        }
    }

    return results;
}

void VariableScopeManager::invalidateCache(const QString& scopeId)
{
    if (m_scopes.contains(scopeId)) {
        m_scopes[scopeId].resolvedValueCache.clear();
        m_scopes[scopeId].cacheValid = false;
    }
}

void VariableScopeManager::invalidateAllCache()
{
    for (auto& node : m_scopes) {
        node.resolvedValueCache.clear();
        node.cacheValid = false;
    }
}

void VariableScopeManager::dumpScope(const QString& scopeId) const
{
    if (!m_scopes.contains(scopeId)) {
        qDebug() << "作用域不存在:" << scopeId;
        return;
    }

    const ScopeNode& node = m_scopes[scopeId];
    qDebug() << "=== 作用域:" << scopeId << "===";
    qDebug() << "父作用域:" << node.parentScopeId;

    qDebug() << "变量定义:";
    for (auto it = node.variables.begin(); it != node.variables.end(); ++it) {
        qDebug() << "  " << it.key() << "=" << it.value().defaultValue;
    }

    qDebug() << "参数覆盖:";
    for (auto it = node.parameterOverrides.begin(); it != node.parameterOverrides.end(); ++it) {
        qDebug() << "  " << it.key() << "=" << it.value();
    }

    qDebug() << "参数映射:";
    for (auto it = node.paramToVarMapping.begin(); it != node.paramToVarMapping.end(); ++it) {
        qDebug() << "  " << it.key() << "→" << it.value();
    }
}
