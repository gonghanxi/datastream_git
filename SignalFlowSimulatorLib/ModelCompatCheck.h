#ifndef MODELCOMPATCHECK_H
#define MODELCOMPATCHECK_H

#include <QString>
#include <QJsonObject>
#include <QStringList>
#include <QSet>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include "JsonLinkDefine.h"

// 兼容性检查结果
struct CompatibilityResult {
    bool isCompatible = true;
    QStringList warnings;
    QStringList errors;

    void addWarning(const QString& warning) {
        warnings.append(warning);
    }

    void addError(const QString& error) {
        errors.append(error);
        isCompatible = false;
    }
};

class ModelCompatCheck
{
public:
    ModelCompatCheck();
    ~ModelCompatCheck();

    // 设置是否严格检查（默认只警告，不报错）
    void setStrictMode(bool strict) { m_strictMode = strict; }

    // 检查单个JSON对象的兼容性
    CompatibilityResult checkCompatibility(const QJsonObject& jsonObj,
                                          const QString& context = QString());

    // 检查链路文件（包含多个JSON对象）
    CompatibilityResult checkLinkFile(const QJsonDocument& jsonDoc,
                                     const QString& filePath = QString());

private:
    // 初始化白名单（所有支持的字段）
    void initializeWhitelist();

    // 初始化组件类型白名单
    void initializeComponentTypeWhitelist();

    // 检查单个对象的所有字段
    void checkObjectFields(const QJsonObject& obj,
                          const QString& parentPath,
                          CompatibilityResult& result);

    // 检查数组中的所有对象
    void checkArrayFields(const QJsonArray& array,
                         const QString& parentPath,
                         CompatibilityResult& result);

    // 获取对象的类型（用于上下文信息）
    QString getObjectType(const QJsonObject& obj) const;

    // 检查字段是否需要版本检查
    bool shouldCheckField(const QString& fieldName, const QString& objType) const;

    // 检查特定对象类型的必填字段
    void checkRequiredFields(const QJsonObject& obj,
                            const QString& objType,
                            const QString& context,
                            CompatibilityResult& result);

    // 检查组件类型的有效性
    void checkComponentType(const QString& cmpType,
                           const QString& context,
                           CompatibilityResult& result);

    // 白名单（所有支持的字段）
    QSet<QString> m_whitelist;

    // 组件类型白名单
    QSet<QString> m_componentTypeWhitelist;

    // 需要忽略的字段（如前端专用字段）
    QSet<QString> m_ignoredFields;

    // 连接对象忽略字段
    QSet<QString> m_connectIgnoreFields;

    // 组件对象忽略字段
    QSet<QString> m_cmpIgnoreFields;

    // 不同对象类型的必填字段
    QMap<QString, QSet<QString>> m_requiredFields;

    // 严格模式：未知字段是否报错（默认只警告）
    bool m_strictMode = false;

    // 用于避免重复警告
    QSet<QString> m_reportedUnknownFields;
    QSet<QString> m_reportedComponentTypes;
};

#endif // MODELCOMPATCHECK_H
