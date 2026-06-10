#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "modelparser.h"
#include <QString>

/**
 * @brief 根据模型描述生成纯计算 C++ 代码和 FMI 接口封装
 *
 * 生成文件包括：
 * - generated_model.h / .cpp : 计算模型类（含自激励正弦波信号）
 * - fmi_wrapper.cpp : FMI 3.0 CoSimulation 接口包装，包含所有必需函数
 */
class ModelCodeGenerator {
public:
    /**
     * @brief 构造函数
     * @param desc 模型描述符（包含模块列表及参数）
     */
    ModelCodeGenerator(const ModelDescriptor &desc);

    /**
     * @brief 在指定目录生成所有代码文件
     * @param outputDir 输出目录（若不存在则自动创建）
     */
    void generate(const QString &outputDir);

private:
    ModelDescriptor m_desc;   ///< 内部保存的模型描述

    /**
     * @brief 生成计算模块头文件和源文件
     * @param dir 输出目录
     */
    void generateComputationalCode(const QString &dir);

    /**
     * @brief 生成 FMI 包装源文件（包含所有 FMI 3.0 必需函数）
     * @param dir 输出目录
     */
    void generateFmiWrapper(const QString &dir);
};

#endif // CODEGENERATOR_H
