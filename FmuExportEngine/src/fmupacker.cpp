#include "fmupacker.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QDebug>
#include <QUuid>

/**
 * @brief 打包过程：
 * 1. 创建临时目录，模拟 FMU 内部结构（binaries/x86_64-windows 或 x86_64-linux）
 * 2. 拷贝动态库到平台子目录，并重命名为 <fmuName>.dll/.so
 * 3. 写入 modelDescription.xml
 * 4. 使用跨平台兼容方式打包为 .fmu
 *    - Windows: 使用 .NET ZipArchive 强制正斜杠路径
 *    - Linux zip: 使用 zip 命令（需预装）
 *    - Linux tar:  使用 tar 命令（系统自带，Docker 兼容）
 * 5. 清理临时文件
 *
 * @param useTar 仅 Linux 下有效，true 使用 tar 打包，false 使用 zip 打包
 */
bool FMUPacker::pack(const QString &fmuName,
                     const QString &outputDir,
                     const QString &binaryPath,
                     const QString &modelDescXml,
                     bool isWindows,
                     bool useTar)
{
    QDir outDir(outputDir);

    // 1. 创建临时目录结构
    // FMI 3.0 规范要求使用标准平台元组名称
    QString tempRoot = outDir.filePath("_fmu_temp_" + fmuName);
    QString platformDir = isWindows ? "x86_64-windows" : "x86_64-linux";
    QString binariesDir = QDir(tempRoot).filePath("binaries/" + platformDir);
    QDir().mkpath(binariesDir);

    // 2. 拷贝动态库并重命名为 <fmuName>.dll/.so
    QString libExt = isWindows ? ".dll" : ".so";
    QString targetLib = QDir(binariesDir).filePath(fmuName + libExt);
    if (!QFile::copy(binaryPath, targetLib)) {
        qWarning() << "Failed to copy library from" << binaryPath << "to" << targetLib;
        return false;
    }

    // 3. 写入 modelDescription.xml
    QString xmlPath = QDir(tempRoot).filePath("modelDescription.xml");
    QFile xmlFile(xmlPath);
    if (!xmlFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot write modelDescription.xml to" << xmlPath;
        return false;
    }
    xmlFile.write(modelDescXml.toUtf8());
    xmlFile.close();

    // 4. 跨平台打包为 .fmu（ZIP 格式）
    QString fmuFile = outDir.filePath(fmuName + ".fmu");
    QProcess pack;

    if (isWindows) {
        // ---- Windows: 使用 .NET ZipArchive 手动创建 ZIP，强制正斜杠路径 ----
        // Compress-Archive 会保留系统反斜杠，导致 FMPy 校验失败
        // 使用 .NET 的 ZipFile 类逐文件添加，在添加时显式替换路径分隔符
        QString psScript = QString(
            "Add-Type -AssemblyName System.IO.Compression.FileSystem\n"
            "Add-Type -AssemblyName System.IO.Compression\n"
            "$fmuPath = '%1'\n"
            "$sourceDir = '%2'\n"
            "if (Test-Path $fmuPath) { Remove-Item $fmuPath -Force }\n"
            "try {\n"
            "    $zip = [System.IO.Compression.ZipFile]::Open($fmuPath, [System.IO.Compression.ZipArchiveMode]::Create)\n"
            "    $files = Get-ChildItem -Path $sourceDir -Recurse -File\n"
            "    foreach ($file in $files) {\n"
            "        $relativePath = $file.FullName.Substring($sourceDir.Length)\n"
            "        if ($relativePath.StartsWith('\\') -or $relativePath.StartsWith('/')) {\n"
            "            $relativePath = $relativePath.Substring(1)\n"
            "        }\n"
            "        # 关键：将反斜杠替换为正斜杠，符合 ZIP 标准\n"
            "        $entryName = $relativePath -replace '\\\\', '/'\n"
            "        $entry = $zip.CreateEntry($entryName, [System.IO.Compression.CompressionLevel]::Optimal)\n"
            "        $fileBytes = [System.IO.File]::ReadAllBytes($file.FullName)\n"
            "        $entryStream = $entry.Open()\n"
            "        $entryStream.Write($fileBytes, 0, $fileBytes.Length)\n"
            "        $entryStream.Close()\n"
            "    }\n"
            "    $zip.Dispose()\n"
            "} catch { Write-Error $_.Exception.Message; exit 1 }\n"
        ).arg(fmuFile, QDir::toNativeSeparators(tempRoot));

        // 写入带 UTF-8 BOM 的 PowerShell 脚本，确保中文路径正确解析
        QString psScriptPath = QDir(tempRoot).filePath("_pack.ps1");
        QFile psFile(psScriptPath);
        if (!psFile.open(QIODevice::WriteOnly)) {
            qWarning() << "Cannot create PowerShell script";
            return false;
        }
        const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
        psFile.write(reinterpret_cast<const char*>(bom), sizeof(bom));
        psFile.write(psScript.toUtf8());
        psFile.close();

        pack.start("powershell", {"-ExecutionPolicy", "Bypass", "-File",
                    QDir::toNativeSeparators(psScriptPath)});

        if (!pack.waitForFinished(60000) || pack.exitCode() != 0) {
            QString err = QString::fromLocal8Bit(pack.readAllStandardError());
            qWarning() << "Windows packaging failed:" << err;
            QDir(tempRoot).removeRecursively();
            return false;
        }
    } else {
        // ---- Linux: 支持 zip 和 tar 两种打包方式，通过 useTar 参数控制 ----
        if (useTar) {
            // tar 方式：系统自带命令，Docker 最小化环境也支持
            // -c : 创建归档
            // -a : 根据后缀自动选择压缩格式
            // -f : 指定输出文件
            pack.setWorkingDirectory(tempRoot);
            QStringList args;
            args << "-c" << "-a" << "-f" << fmuFile << ".";
            pack.start("tar", args);

            qInfo() << "Packaging with tar...";
        } else {
            // zip 方式：标准 ZIP 格式，FMI 规范推荐
            // -r : 递归包含子目录
            // -q : 静默模式
            pack.setWorkingDirectory(tempRoot);
            QStringList args;
            args << "-r" << "-q" << fmuFile << ".";
            pack.start("zip", args);

            qInfo() << "Packaging with zip...";
        }

        if (!pack.waitForFinished(30000)) {
            qWarning() << "Linux packaging timed out.";
            QDir(tempRoot).removeRecursively();
            return false;
        }

        if (pack.exitCode() != 0) {
            QString err = QString::fromLocal8Bit(pack.readAllStandardError());
            qWarning() << "Linux packaging failed:" << err;
            QDir(tempRoot).removeRecursively();
            return false;
        }
    }

    // 5. 验证生成结果
    if (!QFile::exists(fmuFile)) {
        qWarning() << "FMU file not found:" << fmuFile;
        QDir(tempRoot).removeRecursively();
        return false;
    }

    qint64 fileSize = QFileInfo(fmuFile).size();
    qInfo() << "FMU created:" << fmuFile << "- Size:" << fileSize << "bytes";

    // 6. 清理临时目录
    QDir(tempRoot).removeRecursively();
    return true;
}

/**
 * @brief 生成 FMI 3.0 CoSimulation 模型描述 XML
 *
 * FMI 3.0 规范要求：
 * - fmiVersion="3.0"
 * - modelName
 * - instantiationToken（使用 UUID 生成）
 *
 * 变量定义（共3个）：
 * - time   (vr=0) : 独立变量（时间），causality="independent"
 * - input  (vr=1) : 浮点64位输入，causality="input"
 * - output (vr=2) : 浮点64位输出，causality="output"
 *
 * ModelStructure：
 * - Output: 输出变量 output (vr=2) 依赖 input (vr=1)
 * - InitialUnknown: 初始化阶段需要计算的未知变量
 */
QString FMUPacker::generateModelDescription(const ModelDescriptor &desc) {
    // 生成唯一的 instantiationToken（使用 UUID 确保唯一性）
    QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QString xml;
    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

    // fmiModelDescription 根元素
    xml += "<fmiModelDescription"
           " fmiVersion=\"3.0\""
           " modelName=\"" + desc.modelName + "\""
           " instantiationToken=\"" + token + "\""
           " generationTool=\"FmuExportEngine\">\n";

    // CoSimulation 元素：声明该 FMU 为 Co-Simulation 类型
    xml += "  <CoSimulation modelIdentifier=\"" + desc.modelName + "\""
           " canHandleVariableCommunicationStepSize=\"true\"/>\n";

    // ModelVariables 元素：列出所有模型变量
    xml += "  <ModelVariables>\n";
    // 独立变量（时间）：vr=0，causality="independent"
    xml += "    <Float64 name=\"time\" valueReference=\"0\" causality=\"independent\" variability=\"continuous\"/>\n";
    // 输入变量：vr=1，causality="input"，初始值 0.0
    xml += "    <Float64 name=\"input\" valueReference=\"1\" causality=\"input\" variability=\"continuous\" start=\"0.0\"/>\n";
    // 输出变量：vr=2，causality="output"
    xml += "    <Float64 name=\"output\" valueReference=\"2\" causality=\"output\" variability=\"continuous\" initial=\"calculated\"/>\n";
    xml += "  </ModelVariables>\n";

    // ModelStructure 元素：描述变量间的依赖关系和初始化计算顺序
    xml += "  <ModelStructure>\n";
    // Output 元素：输出变量 output (vr=2) 依赖于输入变量 input (vr=1)
    xml += "    <Output valueReference=\"2\" dependencies=\"1\"/>\n";
    // InitialUnknown 元素：初始化阶段需要计算的未知变量
    xml += "    <InitialUnknown valueReference=\"2\"/>\n";
    xml += "  </ModelStructure>\n";

    xml += "</fmiModelDescription>";
    return xml;
}
