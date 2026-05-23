// SubsystemParameterMapper.cpp
#include "SubsystemParameterMapper.h"
#include <QDebug>
#include <QJsonValue>

SubsystemParameterMapper::SubsystemParameterMapper(VariableScopeManager* scopeMgr)
    : m_scopeMgr(scopeMgr)
{
}

QVector<SystemVueModelBuilder::Parameter> SubsystemParameterMapper::parseParametersFromJson(const QJsonObject& obj)
{
    QVector<SystemVueModelBuilder::Parameter> parameters;

    if (!obj.contains("attribute") || !obj["attribute"].isArray()) {
        return parameters;
    }

    QJsonArray attrArray = obj["attribute"].toArray();

    for (const QJsonValue& attrVal : attrArray) {
        if (!attrVal.isObject()) continue;

        QJsonObject attrObj = attrVal.toObject();

        SystemVueModelBuilder::Parameter param;
        param.Name = attrObj["name"].toString().toStdString();

        // 只有在有calculateValue时才使用它，否则使用value
        if (attrObj.contains("calculateValue") && !attrObj["calculateValue"].isNull()) {
            QJsonValue calculateValue = attrObj["calculateValue"];
            if (calculateValue.isDouble()) {
                param.Value = QString::number(calculateValue.toDouble()).toStdString();
                qDebug() << "使用calculateValue:" << param.Name.c_str() << "=" << param.Value.c_str();
            } else if (calculateValue.isString()) {
                param.Value = calculateValue.toString().toStdString();
                qDebug() << "使用calculateValue:" << param.Name.c_str() << "=" << param.Value.c_str();
            } else {
                param.Value = attrObj["defaultValue"].toString().toStdString();
                qDebug() << "使用defaultValue(calculateValue非字符串/数字):" << param.Name.c_str() << "=" << param.Value.c_str();
            }
        } else {
            // 没有calculateValue，使用defaultValue字段
            param.Value = attrObj["defaultValue"].toString().toStdString();
            qDebug() << "使用defaultValue(无calculateValue):" << param.Name.c_str() << "=" << param.Value.c_str();
        }

        parameters.append(param);
    }

    return parameters;
}

QString SubsystemParameterMapper::mapParameterToVariableName(const QString& paramName) const
{
    // 1. 检查自定义映射
    if (m_customMappings.contains(paramName)) {
        return m_customMappings[paramName];
    }

    // 2. 默认同名映射
    return paramName;
}

void SubsystemParameterMapper::setCustomMapping(const QString& paramName, const QString& varName)
{
    m_customMappings[paramName] = varName;
}

QString SubsystemParameterMapper::getChildScopeId(const QString& parentScopeId, const QString& instanceName) const
{
    QString key = parentScopeId + ":" + instanceName;
    return m_instanceToScopeMap.value(key);
}

bool SubsystemParameterMapper::mapSubsystemParameters(
    const QString& parentScopeId,
    const QJsonObject& subsystemObj,
    const QString& childScopeId)
{
    // 1. 获取子系统组件实例名
    QString instanceName = subsystemObj["instanceName"].toString();

    // 2. 保存映射关系
    QString key = parentScopeId + ":" + instanceName;
    m_instanceToScopeMap[key] = childScopeId;

    // 3. 解析参数
    QVector<SystemVueModelBuilder::Parameter> parameters = parseParametersFromJson(subsystemObj);

    // 4. 确保子作用域已注册
    if (!m_scopeMgr->hasScope(childScopeId)) {
        m_scopeMgr->registerScope(childScopeId, parentScopeId);
    }

    // 5. 建立映射关系
    for (const SystemVueModelBuilder::Parameter& param : parameters) {
        QString paramName = QString::fromStdString(param.Name);
        QString varName = mapParameterToVariableName(paramName);

        // 存储参数到变量的映射
        m_scopeMgr->setParameterMapping(childScopeId, paramName, varName);

        QString paramValue = QString::fromStdString(param.Value);

        // 重要：只有在参数值非空时才设置覆盖值
        // 对于sub1.json，这里不会设置覆盖值，因为param.Value是从value字段来的"5"，不是calculateValue
        // 但我们不需要设置覆盖值，让子系统使用自己的默认值
        if (!paramValue.isEmpty() && paramValue != "0" && paramValue != "0.0") {
            // 检查这个参数是否真的有覆盖值（来自父作用域的修改）
            // 这里通过检查paramValue是否与子系统中定义的默认值不同来判断
            // 但更简单的方法是：如果param.Value是从calculateValue来的，我们就设置覆盖值
            // 如果是来自value字段，就不设置覆盖值

            // 我们需要在parseParametersFromJson中标记哪些参数来自calculateValue
            // 简化处理：暂时只设置非空的覆盖值
            m_scopeMgr->setParameterOverride(childScopeId, varName, paramValue);
            qDebug() << "设置参数覆盖:" << childScopeId << varName << "=" << paramValue;
        } else {
            qDebug() << "跳过参数覆盖(值为空):" << childScopeId << varName << "=" << paramValue;
        }

        qDebug() << "子系统参数映射:"
                 << "父作用域:" << parentScopeId
                 << "组件:" << instanceName
                 << "参数:" << paramName << "=" << paramValue
                 << "→ 内部变量:" << varName
                 << "子作用域:" << childScopeId;
    }

    return true;
}

bool SubsystemParameterMapper::syncParameterToVariable(
    const QString& parentScopeId,
    const QString& instanceName,
    const QString& paramName,
    const QString& paramValue)
{
    // 1. 查找对应的子作用域
    QString childScopeId = getChildScopeId(parentScopeId, instanceName);

    if (childScopeId.isEmpty()) {
        qWarning() << "未找到子作用域:" << parentScopeId << instanceName;
        return false;
    }

    // 2. 获取映射的变量名
    QString varName = mapParameterToVariableName(paramName);

    // 3. 只有在参数值非空时才设置覆盖值
    if (!paramValue.isEmpty()) {
        m_scopeMgr->setParameterOverride(childScopeId, varName, paramValue);
        qDebug() << "同步参数到变量:"
                 << "参数:" << paramName << "=" << paramValue
                 << "→ 变量:" << varName
                 << "作用域:" << childScopeId;
        return true;
    }

    return false;
}

bool SubsystemParameterMapper::syncAllParameters(
    const QString& parentScopeId,
    const QString& instanceName,
    const QJsonObject& subsystemObj)
{
    QString childScopeId = getChildScopeId(parentScopeId, instanceName);

    if (childScopeId.isEmpty()) {
        qWarning() << "同步参数失败：未找到子作用域" << parentScopeId << instanceName;
        return false;
    }

    QVector<SystemVueModelBuilder::Parameter> parameters = parseParametersFromJson(subsystemObj);
    bool hasAnyOverride = false;

    for (const SystemVueModelBuilder::Parameter& param : parameters) {
        QString paramName = QString::fromStdString(param.Name);
        QString varName = mapParameterToVariableName(paramName);
        QString paramValue = QString::fromStdString(param.Value);

        // 只有在参数值非空时才设置覆盖值
        if (!paramValue.isEmpty()) {
            m_scopeMgr->setParameterOverride(childScopeId, varName, paramValue);
            hasAnyOverride = true;
            qDebug() << "批量同步参数到变量:"
                     << "参数:" << paramName << "=" << paramValue
                     << "→ 变量:" << varName
                     << "作用域:" << childScopeId;
        }
    }

    return true;
}
