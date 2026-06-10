#include "compiler.h"
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QStandardPaths>

/**
 * @brief 通过 vswhere 查找最新 Visual Studio 的 vcvars64.bat
 * @return 完整路径（统一使用反斜杠），若找不到则返回空字符串
 */
static QString findVisualStudioVcvars() {
    // 默认 vswhere 路径
    QString vswhere = "C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe";
    if (!QFile::exists(vswhere)) {
        // 尝试从 PATH 中查找
        QString path = QStandardPaths::findExecutable("vswhere.exe");
        if (!path.isEmpty()) vswhere = path;
        else return QString();
    }

    QProcess proc;
    proc.start(vswhere, {"-latest", "-property", "installationPath"});
    proc.waitForFinished(5000);
    if (proc.exitCode() != 0) return QString();

    QString vsPath = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    if (vsPath.isEmpty()) return QString();

    // 统一使用 Windows 反斜杠
    vsPath = QDir::toNativeSeparators(vsPath);

    // 构建 vcvars64.bat 路径
    QString vcvars = vsPath + QDir::separator() + "VC" + QDir::separator()
                     + "Auxiliary" + QDir::separator() + "Build" + QDir::separator()
                     + "vcvars64.bat";
    if (QFile::exists(vcvars)) return vcvars;

    qWarning() << "vcvars64.bat not found in" << vsPath;
    return QString();
}

/**
 * @brief 编译源代码为动态链接库
 *
 * Windows 下通过临时 .bat 文件调用 vcvars64.bat 设置环境后编译
 * Linux 下直接使用 g++ 编译
 */
bool Compiler::compile(const QString &sourceDir,
                       const QString &outputPath,
                       bool isWindows,
                       const QString &fmiHeadersDir)
{
    QProcess proc;
    QString program;
    QStringList args;

    if (isWindows) {
        // ---- Windows: 自动配置 MSVC 环境 ----
        QString vcvars = findVisualStudioVcvars();
        if (vcvars.isEmpty()) {
            qWarning() << "Cannot find Visual Studio. Please install VS 2017+ or "
                          "run from Developer Command Prompt.";
            return false;
        }

        // 将所有路径转换为原生格式，确保反斜杠
        QString nativeSrcDir = QDir::toNativeSeparators(sourceDir);
        QString nativeOut = QDir::toNativeSeparators(outputPath);
        QString nativeFmi = QDir::toNativeSeparators(fmiHeadersDir);

        // 在源码目录下创建临时编译脚本 _build.bat
        QString batPath = nativeSrcDir + "\\_build.bat";
        QFile batFile(batPath);
        if (!batFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Cannot create build script" << batPath;
            return false;
        }
        QTextStream stream(&batFile);
        stream << "@echo off\r\n";
        stream << "call \"" << vcvars << "\"\r\n";
        stream << "cl /EHsc /LD /Fe:\"" << nativeOut << "\" \""
               << nativeSrcDir << "\\fmi_wrapper.cpp\" \""
               << nativeSrcDir << "\\generated_model.cpp\" -I\""
               << nativeSrcDir << "\" -I\"" << nativeFmi << "\"\r\n";
        batFile.close();

        // 执行该批处理文件
        program = "cmd";
        args << "/c" << batPath;

        qInfo() << "Executing build script:" << batPath;

        proc.start(program, args);
        if (!proc.waitForFinished(60000)) {
            qWarning() << "Compilation timed out.";
            QFile::remove(batPath);
            return false;
        }

        // 读取输出并转码
        QString stdErr = QString::fromLocal8Bit(proc.readAllStandardError());
        QString stdOut = QString::fromLocal8Bit(proc.readAllStandardOutput());

        // 编译完成后删除临时脚本
        QFile::remove(batPath);

        if (proc.exitCode() != 0) {
            qWarning() << "Compilation error:\n" << stdErr;
            if (!stdOut.isEmpty())
                qInfo() << "Compiler output:\n" << stdOut;
            return false;
        }

        if (!stdOut.isEmpty())
            qInfo() << "Compiler output:" << stdOut;
    } else {
        // ---- Linux: 使用 g++ ----
        QProcess test;
        test.start("which", {"g++"});
        test.waitForFinished();
        if (test.exitCode() != 0) {
            qWarning() << "g++ not found. Please install g++ (sudo apt install g++).";
            return false;
        }

        program = "g++";
        // -shared : 生成共享库
        // -fPIC   : 生成位置无关代码（Linux 共享库必需）
        args << "-shared" << "-fPIC" << "-o" << outputPath
             << sourceDir + "/fmi_wrapper.cpp"
             << sourceDir + "/generated_model.cpp"
             << "-I" + sourceDir
             << "-I" + fmiHeadersDir;

        qInfo() << "Compiling:" << program << args.join(' ');

        proc.start(program, args);
        if (!proc.waitForFinished(60000)) {
            qWarning() << "Compilation timed out.";
            return false;
        }

        if (proc.exitCode() != 0) {
            QString err = QString::fromLocal8Bit(proc.readAllStandardError());
            qWarning() << "Compilation error:\n" << err;
            return false;
        }
    }

    qInfo() << "Compilation successful.";
    return true;
}
