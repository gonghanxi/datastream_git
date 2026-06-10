#ifndef MODELPARSER_H
#define MODELPARSER_H

#include <QString>
#include <QVector>
#include <QJsonObject>

/**
 * @brief 单个模块的描述信息
 *
 * type   - 模块类型，如 "Gain"、"Delay"、"FIR"、"Noise"
 * name   - 模块实例名，例如 "gain1"
 * params - 模块参数，以 JSON 对象形式存储
 */
struct ModuleInfo {
    QString type;
    QString name;
    QJsonObject params;
};

/**
 * @brief 整个原理图模型的描述
 *
 * modelName - 模型名称
 * stepSize  - 仿真步长（秒）
 * modules   - 按连接顺序排列的模块列表
 */
struct ModelDescriptor {
    QString modelName;
    double stepSize = 0.001;
    QVector<ModuleInfo> modules;
};

/**
 * @brief 解析原理图 JSON 字符串，生成模型描述符
 * @param jsonStr 符合约定的 JSON 文本
 * @return 解析得到的 ModelDescriptor 结构体
 */
ModelDescriptor parseSchematicJson(const QString &jsonStr);

/** 默认测试用原理图 JSON（复杂版本，测试大部分 FMI 3.0 接口） */
extern const char *DEFAULT_SCHEMATIC_JSON;

#endif // MODELPARSER_H
