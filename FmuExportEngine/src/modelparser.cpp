#include "modelparser.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

/**
 * @brief 解析原理图 JSON 字符串
 *
 * 预期格式：
 * {
 *   "modelName": "...",
 *   "stepSize": 0.01,
 *   "modules": [
 *     { "type": "Gain", "name": "...", "params": { ... } },
 *     ...
 *   ]
 * }
 */
ModelDescriptor parseSchematicJson(const QString &jsonStr) {
    // 将 JSON 文本转换为文档对象
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    QJsonObject root = doc.object();

    ModelDescriptor desc;
    // 读取模型名称，默认 "TestModel"
    desc.modelName = root["modelName"].toString("TestModel");
    // 读取仿真步长，默认 0.001
    desc.stepSize = root["stepSize"].toDouble(0.001);

    // 遍历 modules 数组，逐个解析模块信息
    QJsonArray mods = root["modules"].toArray();
    for (const auto &m : mods) {
        QJsonObject mo = m.toObject();
        ModuleInfo mi;
        mi.type   = mo["type"].toString();
        mi.name   = mo["name"].toString();
        mi.params = mo["params"].toObject();
        desc.modules.append(mi);
    }

    return desc;
}

/**
 * @brief 复杂默认原理图，用于测试 FMI 3.0 大多数接口
 *
 * 信号链路：正弦波自激励 → Gain(×2.5) → FIR(5阶低通) → Delay(4步) →
 *           Noise(σ=0.1) → Gain(×0.5) → Output
 *
 * 模块说明：
 * - gain1  : 增益 2.5 倍，将输入信号放大
 * - fir1   : 5 阶低通 FIR 滤波器，平滑信号
 * - delay1 : 4 步延迟，模拟信号传播延迟
 * - noise1 : 高斯噪声（均值 0，标准差 0.1），模拟测量噪声
 * - gain2  : 增益 0.5 倍，衰减信号
 *
 * 此设计可测试：
 * - 多个同类模块的实例化（2 个 Gain）
 * - 不同参数类型的正确解析（标量 + 数组）
 * - FIR 滤波器系数的数组处理
 * - 延迟模块的缓冲操作
 * - 噪声生成器的随机数种子
 * - 级联模块的信号传播
 */
const char *DEFAULT_SCHEMATIC_JSON = R"({
    "modelName": "ComplexTestModel",
    "stepSize": 0.005,
    "modules": [
        { "type": "Gain", "name": "gain1", "params": { "gain": 2.5 } },
        { "type": "FIR", "name": "fir1", "params": { "coefficients": [0.1, 0.2, 0.4, 0.2, 0.1] } },
        { "type": "Delay", "name": "delay1", "params": { "delay": 4 } },
        { "type": "Noise", "name": "noise1", "params": { "mean": 0.0, "stddev": 0.1 } },
        { "type": "Gain", "name": "gain2", "params": { "gain": 0.5 } }
    ]
})";
