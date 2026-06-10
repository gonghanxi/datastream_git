#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include "modelparser.h"
#include "codegenerator.h"
#include "compiler.h"
#include "fmupacker.h"

/**
 * @brief FmuExportEngine 主入口
 *
 * 用法: FmuExportEngine <FMU名称(不含.fmu)> <输出目录> [原理图JSON]
 *
 * 流程:
 *   1. 解析原理图 JSON (或使用内置复杂默认测试原理图)
 *   2. 在临时编译目录 _tempCompile_<模型名> 中生成纯计算 C++ 代码和 FMI 接口包装
 *   3. 调用本地编译器在该目录下生成动态链接库
 *   4. 生成 modelDescription.xml 并打包为 .fmu
 *   5. 删除临时编译目录，清理所有中间文件
 *
 * 跨平台支持:
 *   Windows: 自动查找 Visual Studio，使用 MSVC 编译 + PowerShell 打包
 *   Linux:   使用 g++ 编译 + zip 打包
 *
 * 所有从命令行传入的路径参数均使用 fromLocal8Bit 转换，以正确支持中文路径。
 */
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    qSetMessagePattern("[%{type}] %{message}");

    // 检查参数数量
    if (argc < 3) {
        qCritical() << "Usage: FmuExportEngine <FMU name (without .fmu)> <output dir> [schematic JSON]";
        return -1;
    }

    // 将命令行参数从本地编码转换为 QString，避免中文路径乱码
    QString fmuNameRaw = QString::fromLocal8Bit(argv[1]);
    QString outputPath = QString::fromLocal8Bit(argv[2]);
    QString jsonStr = (argc >= 4) ? QString::fromLocal8Bit(argv[3]) : QString(DEFAULT_SCHEMATIC_JSON);

    // 如果用户误带了 .fmu 后缀，则去除
    if (fmuNameRaw.endsWith(".fmu", Qt::CaseInsensitive))
        fmuNameRaw.chop(4);

    // 验证 FMI 头文件目录是否存在
    QString fmiHeadersDir = QDir(QCoreApplication::applicationDirPath()).filePath("fmi3");
    if (!QDir(fmiHeadersDir).exists()) {
        qWarning() << "fmi3 headers directory not found:" << fmiHeadersDir;
        qWarning() << "Please place fmi3Functions.h, fmi3FunctionTypes.h, fmi3PlatformTypes.h into that folder.";
        return -1;
    }

    qInfo() << "Starting FMU generation for:" << fmuNameRaw;

    // 步骤1：解析原理图 JSON
    ModelDescriptor modelDesc = parseSchematicJson(jsonStr);
    modelDesc.modelName = fmuNameRaw;   // 使用用户指定的名称

    // 步骤2：创建临时编译目录，所有中间文件均存放于此
    QDir outputDir(outputPath);
    QString tempCompileDir = outputDir.filePath("_tempCompile_" + fmuNameRaw);
    QDir().mkpath(tempCompileDir);

    // 步骤3：在临时目录中生成 C++ 源代码
    ModelCodeGenerator codeGen(modelDesc);
    codeGen.generate(tempCompileDir);

    // 步骤4：编译动态库
    bool isWin = false;
#ifdef Q_OS_WIN
    isWin = true;
#endif
    QString libExt = isWin ? ".dll" : ".so";
    QString libPath = QDir(tempCompileDir).filePath(fmuNameRaw + libExt);

    if (!Compiler::compile(tempCompileDir, libPath, isWin, fmiHeadersDir)) {
        qCritical() << "Compilation failed.";
        return -2;
    }

    // 步骤5：生成 modelDescription.xml 并打包 FMU
    QString modelDescXml = FMUPacker::generateModelDescription(modelDesc);
    QString fmuFilePath = outputDir.filePath(fmuNameRaw + ".fmu");

    bool linuxUseTar = true; // false:linux使用zip打包;  true:linux使用tar打包
    if (!FMUPacker::pack(fmuNameRaw, outputPath, libPath, modelDescXml, isWin, linuxUseTar)) {
        qCritical() << "Packaging failed.";
        return -3;
    }

    // 步骤6：清理整个临时编译目录
    QDir(tempCompileDir).removeRecursively();

    qInfo() << "FMU successfully generated:" << fmuFilePath;
    return 0;
}
