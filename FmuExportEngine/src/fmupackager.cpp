#include "fmupackager.h"
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QProcess>
#include <QCoreApplication>

bool FmuPackager::createFmuPackage(const QString &fmuName, const QString &outputPath)
{
    m_fmuName = fmuName;

    if(fmuName.contains(".fmu",Qt::CaseInsensitive))
    {
        // 防止参数自带.fmu
        m_fmuName = m_fmuName.replace(".fmu","",Qt::CaseInsensitive);
    }

    QDir outputDir(outputPath);

    if (!outputDir.exists() && !outputDir.mkpath(".")) {
        qCritical() << "错误：无法创建输出路径" << outputPath;
        return false;
    }

    m_tempRootPath = outputDir.filePath(m_fmuName + "_temp");
    QDir tempDir(m_tempRootPath);

    if (tempDir.exists()) {
        cleanTempDirectory(m_tempRootPath);
    }

    bool ret = createDirectoryStructure(m_tempRootPath)
            && generateModelDescription(tempDir.filePath("modelDescription.xml"))
            && createEmptySoFile(tempDir.filePath("binaries/linux64/" + m_fmuName + ".so"))
            && createFmuCrossPlatform(m_tempRootPath, outputDir.filePath(m_fmuName + ".fmu"));

    cleanTempDirectory(m_tempRootPath);
    return ret;
}

bool FmuPackager::createDirectoryStructure(const QString &basePath)
{
    QDir baseDir(basePath);
    QString linux64Path = baseDir.filePath("binaries/linux64");
    if (baseDir.mkpath(linux64Path)) {
        return true;
    } else {
        qCritical() << "错误：创建目录失败" << linux64Path;
        return false;
    }
}

bool FmuPackager::generateModelDescription(const QString& filePath)
{
    QFile xmlFile(filePath);
    if (!xmlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "错误：创建XML文件失败";
        return false;
    }

    // 后续扩展文件内部结构
    QTextStream out(&xmlFile);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<fmiModelDescription modelName=\"" << m_fmuName << "\"/>\n";
    xmlFile.close();
    return true;
}

bool FmuPackager::createEmptySoFile(const QString& soFilePath)
{
    QFile soFile(soFilePath);
    if (soFile.open(QIODevice::WriteOnly)) {
        soFile.close();
        return true;
    }
    qCritical() << "错误：创建SO文件失败";
    return false;
}

// ===================== 跨平台 FMU 打包（Windows + Linux） =====================
bool FmuPackager::createFmuCrossPlatform(const QString& tempDir, const QString& fmuFilePath)
{
#ifdef Q_OS_WIN
    // ===================== Windows 方案 =====================
    QString zipFilePath = fmuFilePath + ".zip";

    QProcess process;
    process.setWorkingDirectory(tempDir);

    QStringList args;
    args << "-Command"
         << "Compress-Archive -Path * -DestinationPath \"" + zipFilePath + "\" -Force";

    process.start("powershell", args);
    process.waitForFinished(30000);

    if (process.exitCode() != 0) {
        qCritical() << "Windows 压缩失败：" << process.readAllStandardError();
        return false;
    }

    // 重命名为 .fmu
    QFile::remove(fmuFilePath);
    bool renameOk = QFile::rename(zipFilePath, fmuFilePath);

    if (!renameOk || !QFile::exists(fmuFilePath)) {
        qCritical() << "重命名为FMU失败";
        return false;
    }

#elif defined(Q_OS_LINUX)
// ===================== Linux 方案（改用 tar，兼容 Docker）=====================
    QProcess process;
    process.setWorkingDirectory(tempDir);  // 必须在打包目录内

    QStringList args;
    // -c 创建  -a 自动压缩格式  -f 指定输出文件  -C . 确保当前目录
    args << "-c"
         << "-a"
         << "-f" << fmuFilePath
         << ".";

    // tar是系统自带命令，Docker最小化环境也有
    process.start("/usr/bin/tar", args);
    process.waitForFinished(30000);

    // 输出调试信息
    qInfo() << "tar 标准输出：" << process.readAllStandardOutput();
    if (process.exitCode() != 0 || !QFile::exists(fmuFilePath)) {
        qCritical() << "tar 错误输出：" << process.readAllStandardError();
        qCritical() << "Linux tar 压缩失败，退出码：" << process.exitCode();
        return false;
    }
#else
    qCritical() << "不支持的操作系统";
    return false;
#endif

    qInfo() << "成功生成 FMU：" << fmuFilePath;
    return true;
}

void FmuPackager::cleanTempDirectory(const QString& tempDir)
{
    QDir dir(tempDir);
    if (dir.exists()) {
        dir.removeRecursively();
    }
}
