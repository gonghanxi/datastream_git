#ifndef FMUPACKAGER_H
#define FMUPACKAGER_H

#pragma execution_character_set("utf-8")

#include <QString>

// FMU打包器：负责FMU文件生成逻辑
class FmuPackager
{
public:
    explicit FmuPackager() = default;

    /**
     * @brief 创建FMU压缩包
     * @param fmuName FMU文件名（默认无后缀，如：MyModel）
     * @param outputPath 生成路径
     * @return 成功返回true，失败返回false
     */
    bool createFmuPackage(const QString& fmuName, const QString& outputPath);

private:
    // 创建FMU内部目录结构
    bool createDirectoryStructure(const QString& basePath);
    // 生成modelDescription.xml文件（预留扩展接口）
    bool generateModelDescription(const QString& filePath);
    // 创建空的so文件
    bool createEmptySoFile(const QString& soFilePath);
    // 压缩为FMU文件
    bool createFmuCrossPlatform(const QString& tempDir, const QString& fmuFilePath);
    // 清理临时文件
    void cleanTempDirectory(const QString& tempDir);

private:
    QString m_fmuName = "";
    QString m_tempRootPath = "";
};

#endif
