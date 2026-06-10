#ifndef FMUPACKER_H
#define FMUPACKER_H

#include <QString>
#include "modelparser.h"

/**
 * @brief FMU 打包工具，负责生成符合 FMI 3.0 标准的 .fmu 文件
 */
class FMUPacker {
public:
    /**
     * @brief 打包动态库和模型描述为 .fmu 文件
     * @param fmuName      FMU 名称（不含 .fmu 后缀）
     * @param outputDir    输出目录
     * @param binaryPath   编译好的动态库完整路径
     * @param modelDescXml modelDescription.xml 文本内容
     * @param isWindows    平台标识
     * @param useTar       Linux 下打包方式选择：true 使用 tar，false 使用 zip（默认）
     * @return 成功返回 true，失败返回 false
     */
    static bool pack(const QString &fmuName,
                     const QString &outputDir,
                     const QString &binaryPath,
                     const QString &modelDescXml,
                     bool isWindows,
                     bool useTar = false);

    /**
     * @brief 根据模型描述生成 modelDescription.xml 字符串
     * @param desc 模型描述符
     * @return XML 文本
     */
    static QString generateModelDescription(const ModelDescriptor &desc);
};

#endif // FMUPACKER_H
