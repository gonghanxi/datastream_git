// SubsystemParameterMapper.h
#ifndef SUBSYSTEMPARAMETERMAPPER_H
#define SUBSYSTEMPARAMETERMAPPER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include "VariableScopeManager.h"
#include "Variable.h"
#include "Block.h"

//子系统参数映射器
class SubsystemParameterMapper {
public:
    explicit SubsystemParameterMapper(VariableScopeManager* scopeMgr);

    // 处理子系统组件参数映射
    bool mapSubsystemParameters(const QString& parentScopeId,
                               const QJsonObject& subsystemObj,
                               const QString& childScopeId);

    // 同步参数值到内部变量
    bool syncParameterToVariable(const QString& parentScopeId,
                                const QString& instanceName,
                                const QString& paramName,
                                const QString& paramValue);

    // 批量同步所有参数
    bool syncAllParameters(const QString& parentScopeId,
                          const QString& instanceName,
                          const QJsonObject& subsystemObj);

    // 获取映射规则
    QString mapParameterToVariableName(const QString& paramName) const;

    // 设置自定义映射规则
    void setCustomMapping(const QString& paramName, const QString& varName);

    // 获取子作用域ID
    QString getChildScopeId(const QString& parentScopeId, const QString& instanceName) const;

private:
    // 从JSON解析参数
    QVector<SystemVueModelBuilder::Parameter> parseParametersFromJson(const QJsonObject& obj);

    // 验证参数与变量的兼容性
    bool validateParameterVariableCompatibility(const SystemVueModelBuilder::Parameter& param,
                                               const Variable& var);

private:
    VariableScopeManager* m_scopeMgr;
    QMap<QString, QString> m_customMappings;  // 自定义映射规则

    // 父作用域+实例名 -> 子作用域ID
    QMap<QString, QString> m_instanceToScopeMap;
};

#endif // SUBSYSTEMPARAMETERMAPPER_H
