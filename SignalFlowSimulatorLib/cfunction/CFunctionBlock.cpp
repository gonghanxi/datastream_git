#include "CFunctionBlock.h"
#include "CFunctionModelInfo.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QProcessEnvironment>

namespace SystemVueModelBuilder{

static bool g_updateError = true;
static bool g_invokeEngineError = true;
static bool g_rWError = true;

CFunctionBlock::CFunctionBlock(const std::string& name)
    : Block(name)
{
}

CFunctionBlock::~CFunctionBlock()
{
}

void CFunctionBlock::setCFunctionConfig(const QString& instanceName, int cmpId)
{
    m_instanceName = instanceName;
    m_cmpId = cmpId;
}

void CFunctionBlock::setConfigData(const CFunctionConfigData& configData)
{
    m_configData = configData;
}

void CFunctionBlock::setEquations(const QString& equations)
{
    m_equations = equations;
}

void CFunctionBlock::setGeneratedJsonPath(const QString& path)
{
    m_generatedJsonPath = path;
}

void CFunctionBlock::setSimuParams(const SimuParameter& params)
{
    m_simuParams = params;
}

void CFunctionBlock::addPortInfo(const PortMsg& port)
{
    PortInfo info;
    info.name = port.name;
    info.putType = port.putType;
    info.dataType = port.dataType;
    info.id = port.id;
    m_portInfos.push_back(info);
}

void CFunctionBlock::addParameterInfo(const QString& name, const QString& value)
{
    m_parameterValues[name.toStdString()] = value;
}

bool CFunctionBlock::Initialize()
{
    qDebug() << "[CFunctionBlock] Initialize:" << m_instanceName;

    // 根据端口设置块类型（默认SOURCE，有输入端口时设为PROCESSOR）
    bool hasInput = false, hasOutput = false;
    for (const auto& portInfo : m_portInfos) {
        if (portInfo.putType == "in") hasInput = true;
        if (portInfo.putType == "out") hasOutput = true;
    }
    if (hasInput && hasOutput)
        SetBlockType(BlockType::PROCESSOR);
    else if (hasInput)
        SetBlockType(BlockType::SINK);
    else
        SetBlockType(BlockType::SOURCE);

    // 注册端口（参照FMUBlock模式，根据PortDataType创建对应缓冲区类型）
    for (const auto& portInfo : m_portInfos) {
        std::string name = portInfo.name.toStdString();

        if (portInfo.putType == "in") {
            switch (portInfo.dataType) {
            case PortMsg::PortDataType::REAL: {
                auto* buffer = new DoubleCircularBuffer();
                AddInputPort(name, *buffer, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
                break;
            }
            case PortMsg::PortDataType::INT: {
                auto* buffer = new IntCircularBuffer();
                AddInputPort(name, *buffer, 1, DataType::CIRCULAR_BUFFER_INT);
                break;
            }
            case PortMsg::PortDataType::COMPLEX: {
                auto* buffer = new DComplexCircularBuffer();
                AddInputPort(name, *buffer, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
                break;
            }
            case PortMsg::PortDataType::REAL_MATRIX: {
                auto* buffer = new DoubleMatrixCircularBuffer();
                AddInputPort(name, *buffer, 1, DataType::MATRIX_DOUBLE);
                break;
            }
            case PortMsg::PortDataType::INT_MATRIX: {
                auto* buffer = new IntMatrixCircularBuffer();
                AddInputPort(name, *buffer, 1, DataType::MATRIX_INT);
                break;
            }
            case PortMsg::PortDataType::COMPLEX_MATRIX: {
                auto* buffer = new DComplexMatrixCircularBuffer();
                AddInputPort(name, *buffer, 1, DataType::MATRIX_DCOMPLEX);
                break;
            }
            default: {
                auto* buffer = new DoubleCircularBuffer();
                AddInputPort(name, *buffer, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
                break;
            }
            }
        } else if (portInfo.putType == "out") {
            switch (portInfo.dataType) {
            case PortMsg::PortDataType::REAL: {
                auto* buffer = new DoubleCircularBuffer();
                AddOutputPort(name, *buffer, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
                break;
            }
            case PortMsg::PortDataType::INT: {
                auto* buffer = new IntCircularBuffer();
                AddOutputPort(name, *buffer, 1, DataType::CIRCULAR_BUFFER_INT);
                break;
            }
            case PortMsg::PortDataType::COMPLEX: {
                auto* buffer = new DComplexCircularBuffer();
                AddOutputPort(name, *buffer, 1, DataType::CIRCULAR_BUFFER_DCOMPLEX);
                break;
            }
            case PortMsg::PortDataType::REAL_MATRIX: {
                auto* buffer = new DoubleMatrixCircularBuffer();
                AddOutputPort(name, *buffer, 1, DataType::MATRIX_DOUBLE);
                break;
            }
            case PortMsg::PortDataType::INT_MATRIX: {
                auto* buffer = new IntMatrixCircularBuffer();
                AddOutputPort(name, *buffer, 1, DataType::MATRIX_INT);
                break;
            }
            case PortMsg::PortDataType::COMPLEX_MATRIX: {
                auto* buffer = new DComplexMatrixCircularBuffer();
                AddOutputPort(name, *buffer, 1, DataType::MATRIX_DCOMPLEX);
                break;
            }
            default: {
                auto* buffer = new DoubleCircularBuffer();
                AddOutputPort(name, *buffer, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
                break;
            }
            }
        }
    }

    // 设置参数
    for (const auto& kv : m_parameterValues) {
        Parameter param;
        param.Name = kv.first;
        param.Value = kv.second.toStdString();
        setParameter(param);
    }

    return true;
}

bool CFunctionBlock::Setup()
{
    Block::Setup();
    qDebug() << "[CFunctionBlock] Setup:" << m_instanceName;
    return true;
}

bool CFunctionBlock::Run()
{
    return executeCFunction();
}

bool CFunctionBlock::Stop()
{
    qDebug() << "[CFunctionBlock] Stop:" << m_instanceName;
    return Block::Stop();
}

bool CFunctionBlock::Done()
{
    qDebug() << "[CFunctionBlock] Done:" << m_instanceName;
    return Block::Done();
}

bool CFunctionBlock::Flush()
{
    return true;
}

bool CFunctionBlock::executeCFunction()
{
    // 步骤1：更新cfunction.json的input字段
    if (!updateJsonInput()) {
        if(g_updateError) {
            LOG_ERROR("[CFunctionBlock] Failed to update JSON input:",
                      m_instanceName.toStdString());
            g_updateError = false;
            return false;
        }
    }

    // 步骤2：调用外部小引擎
    if (!invokeEngine(m_generatedJsonPath)) {
        if(g_invokeEngineError) {
            LOG_ERROR("[CFunctionBlock] Engine invocation failed:",
                      m_instanceName.toStdString());
            g_invokeEngineError = false;
            return false;
        }
    }

    // 步骤3：读取output并写入输出端口
    if (!readAndWriteOutput()) {
        if(g_rWError) {
            LOG_ERROR("[CFunctionBlock] Failed to read/write output:",
                      m_instanceName.toStdString());
            g_rWError = false;
            return false;
        }
    }

    return true;
}

bool CFunctionBlock::updateJsonInput()
{
    if (m_generatedJsonPath.isEmpty()) {
        LOG_ERROR("[CFunctionBlock] No generated JSON path set");
        return false;
    }

    QFile file(m_generatedJsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR("[CFunctionBlock] Cannot open JSON:", m_generatedJsonPath.toStdString());
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject root = doc.object();
    QJsonArray inputArray = root["input"].toArray();

    // 更新每个输入端口的value
    for (int i = 0; i < inputArray.size(); ++i) {
        QJsonObject inputObj = inputArray[i].toObject();
        QString portName = inputObj["name"].toString();

        // 从Block的输入端口读取数据
        std::string stdPortName = portName.toStdString();
        BufferReader* reader = GetInputPort(stdPortName);
        QString dataType = inputObj["datatype"].toString().toLower();
        if (!reader || !reader->HasDataAvailable()) {
            // 无数据可用时，写入类型对应的默认值，确保JSON中不出现空字符串
            QString defVal;
            if (dataType == "bool") defVal = "false";
            else if (dataType.contains("complex")) defVal = "(0,0)";
            else defVal = "0";
            inputObj["value"] = defVal;
            inputArray[i] = inputObj;
            continue;
        }

        // 根据数据类型读取
        QString valueStr;

        if (dataType == "double" || dataType == "real") {
            std::vector<double> data;
            reader->ReadData(data);
            if (!data.empty()) {
                valueStr = QString::number(data[0], 'g', 15);
            }
        } else if (dataType == "int" || dataType == "integer") {
            std::vector<int> data;
            reader->ReadData(data);
            if (!data.empty()) {
                valueStr = QString::number(data[0]);
            }
        } else if (dataType == "bool") {
            std::vector<bool> data;
            reader->ReadData(data);
            if (!data.empty()) {
                valueStr = data[0] ? "true" : "false";
            }
        } else if (dataType == "complex") {
            std::vector<std::complex<double>> data;
            reader->ReadData(data);
            if (!data.empty()) {
                valueStr = QString("(%1,%2)").arg(data[0].real(), 0, 'g', 15)
                                             .arg(data[0].imag(), 0, 'g', 15);
            }
        } else if (dataType.contains("multiple complex")) {
            // 复数总线: [(r,i),(r,i),...]
            std::vector<std::complex<double>> data;
            reader->ReadData(data);
            if (!data.empty()) {
                QStringList elems;
                for (const auto& c : data) {
                    elems << QString("(%1,%2)").arg(c.real(), 0, 'g', 15)
                                               .arg(c.imag(), 0, 'g', 15);
                }
                valueStr = "[" + elems.join(",") + "]";
            }
        } else if (dataType.startsWith("matrix")) {
            std::vector<double> data;
            reader->ReadData(data);
            if (!data.empty()) {
                // 矩阵格式: [v1,v2,v3;v4,v5,v6]
                // 简化为一维数组输出
                QStringList parts;
                for (double v : data) parts.append(QString::number(v, 'g', 15));
                valueStr = "[" + parts.join(",") + "]";
            }
        }

        if (!valueStr.isEmpty()) {
            inputObj["value"] = valueStr;
            inputArray[i] = inputObj;
        }
    }

    root["input"] = inputArray;

    // 写回文件
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

bool CFunctionBlock::invokeEngine(const QString& jsonPath)
{
    QFileInfo jsonInfo(jsonPath);
    QString jsonDir = jsonInfo.absolutePath();
    QString appDir = QCoreApplication::applicationDirPath();

    // 可执行文件路径：与JSON同目录，名为{instanceName}
    QString exeExt;
#ifdef Q_OS_WIN
    exeExt = ".exe";
#endif
    QString exePath = jsonDir + "/" + m_instanceName + exeExt;

    // 查找引擎脚本（仅在需要编译时使用）
    QString engineScript;
    QStringList engineSearchDirs = { jsonDir, appDir };
    bool engineFound = false;

    for (const QString& dir : engineSearchDirs) {
        QString pyPath = dir + "/engine/cfunction_engine.py";
        if (QFile::exists(pyPath)) {
            engineScript = pyPath;
            engineFound = true;
            break;
        }
    }

    // 步骤1：如果可执行文件不存在，调用引擎生成
    if (!QFile::exists(exePath)) {
        // 已经尝试过编译且失败，不再重复调用编译器，不再打印任何错误
        if (m_buildAttempted) {
            return false;
        }

        if (!engineFound) {
            LOG_ERROR("[CFunctionBlock] Engine script not found. Searched:",
                      (jsonDir + ", " + appDir).toStdString());
            return false;
        }

        qDebug() << "[CFunctionBlock] Executable not found, building:" << exePath;
        m_buildAttempted = true;  // 标记已尝试编译

        // 查找Python解释器：优先使用deploy_package中自带的Python
        QString pythonExe;
#ifdef Q_OS_WIN
        QString bundledPython = appDir + "/deploy_package/python/python.exe";
        if (QFile::exists(bundledPython)) {
            pythonExe = bundledPython;
        } else {
            pythonExe = "python";
        }
#else
        pythonExe = "python3";
#endif

        QProcess buildProcess;
        // 设置环境变量，确保deploy_package中的Python和GCC在PATH中
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString path = env.value("PATH", env.value("Path"));
#ifdef Q_OS_WIN
        QString pathSep = ";";
#else
        QString pathSep = ":";
#endif
        QString bundledPythonDir = appDir + "/deploy_package/python";
        QString bundledGccDir = appDir + "/deploy_package/gcc/bin";
        if (QDir(bundledPythonDir).exists()) path = bundledPythonDir + pathSep + path;
        if (QDir(bundledGccDir).exists()) path = bundledGccDir + pathSep + path;
        env.insert("PATH", path);
        buildProcess.setProcessEnvironment(env);

        QStringList buildArgs;
        buildProcess.start(pythonExe, QStringList()
            << engineScript << jsonPath
            << "--build-only"
            << "--output-dir" << jsonDir
            << "--exe-name" << m_instanceName);

        if (!buildProcess.waitForFinished(300000)) {
            // FailedToStart: python3不存在时waitForFinished也返回false，需优先检查
            if (buildProcess.error() == QProcess::FailedToStart) {
                LOG_ERROR("[CFunctionBlock] Python not found. Tried:", pythonExe.toStdString());
                LOG_ERROR("[CFunctionBlock] Please install python3 or ensure it is in PATH.");
                return false;
            }
            // 真正的超时
            QString partialStderr = buildProcess.readAllStandardError();
            QString partialStdout = buildProcess.readAllStandardOutput();
            buildProcess.kill();
            LOG_ERROR("[CFunctionBlock] Engine build timeout:", m_instanceName.toStdString());
            if (!partialStderr.isEmpty()) {
                LOG_ERROR("[CFunctionBlock] Engine partial stderr:", partialStderr.toStdString());
            }
            if (!partialStdout.isEmpty()) {
                qDebug() << "[CFunctionBlock] Engine partial stdout:" << partialStdout;
            }
            return false;
        }

        if (buildProcess.exitCode() != 0) {
            QString stderrOutput = buildProcess.readAllStandardError();
            QString stdoutOutput = buildProcess.readAllStandardOutput();
            LOG_ERROR("[CFunctionBlock] Engine build error:", stderrOutput.toStdString());
            if (!stdoutOutput.isEmpty()) {
                qDebug() << "[CFunctionBlock] Engine stdout:" << stdoutOutput;
            }
            return false;
        }

        qDebug() << "[CFunctionBlock] Executable built successfully:" << exePath;
    }

    // 验证可执行文件存在
    if (!QFile::exists(exePath)) {
        LOG_ERROR("[CFunctionBlock] Executable not found after build:", exePath.toStdString());
        return false;
    }

    // 步骤2：运行可执行文件，参数为JSON文件路径
    QProcess runProcess;
    runProcess.start(exePath, QStringList() << jsonPath);

    if (!runProcess.waitForFinished(60000)) {
        runProcess.kill();
        LOG_ERROR("[CFunctionBlock] Executable timeout:", m_instanceName.toStdString());
        return false;
    }

    if (runProcess.exitCode() != 0) {
        QString stderrOutput = runProcess.readAllStandardError();
        QString stdoutOutput = runProcess.readAllStandardOutput();
        LOG_ERROR("[CFunctionBlock] Executable error:", stderrOutput.toStdString());
        if (!stdoutOutput.isEmpty()) {
            LOG_ERROR("[CFunctionBlock] Executable stdout:", stdoutOutput.toStdString());
        }
        return false;
    }

    // 即使exitCode==0，也检查stderr以发现隐藏错误
    QString stderrOutput = runProcess.readAllStandardError();
    if (!stderrOutput.isEmpty()) {
        LOG_ERROR("[CFunctionBlock] Executable stderr:", stderrOutput.toStdString());
    }

    // 检查JSON文件状态
    QFileInfo jsonCheck(jsonPath);
    if (!jsonCheck.exists()) {
        LOG_ERROR("[CFunctionBlock] JSON file missing after exec:", jsonPath.toStdString());
        return false;
    }
    if (jsonCheck.size() == 0) {
        LOG_ERROR("[CFunctionBlock] JSON file is EMPTY after exec:", jsonPath.toStdString());
        return false;
    }

    return true;
}

bool CFunctionBlock::readAndWriteOutput()
{
    CFunctionModelParser parser;
    QVector<QPair<QString, QVector<double>>> outputs;

    if (!parser.readCFunctionOutput(m_generatedJsonPath, outputs)) {
        return false;
    }

    for (const auto& output : outputs) {
        std::string portName = output.first.toStdString();
        const QVector<double>& values = output.second;

        qDebug() << "[CFunctionBlock] readAndWriteOutput:"
                 << output.first << "values:" << values.size()
                 << "data:" << values;

        // 获取输出端口的数据类型
        Buffer* buffer = GetOutputPort(portName);
        if (!buffer) {
            qDebug() << "[CFunctionBlock] Output port not found:" << output.first;
            continue;
        }

        if (values.isEmpty()) {
            qDebug() << "[CFunctionBlock] Output empty:" << output.first;
            continue;
        }

        DataType dt = buffer->GetDataType();

        switch (dt) {
        case DataType::DOUBLE:
        case DataType::CIRCULAR_BUFFER_DOUBLE: {
            std::vector<double> data(values.begin(), values.end());
            WriteOutputData(portName, data);
            break;
        }
        case DataType::INT:
        case DataType::CIRCULAR_BUFFER_INT: {
            std::vector<int> data;
            for (double v : values) data.push_back(static_cast<int>(v));
            WriteOutputData(portName, data);
            break;
        }
        case DataType::BOOL:
        case DataType::CIRCULAR_BUFFER_BOOL: {
            std::vector<bool> data;
            for (double v : values) data.push_back(v != 0.0);
            WriteOutputData(portName, data);
            break;
        }
        case DataType::COMPLEX_DOUBLE:
        case DataType::CIRCULAR_BUFFER_DCOMPLEX: {
            std::vector<std::complex<double>> data;
            // 复数格式：每两个值为一组 (real, imag)
            for (int i = 0; i + 1 < values.size(); i += 2) {
                data.emplace_back(values[i], values[i + 1]);
            }
            WriteOutputData(portName, data);
            break;
        }
        case DataType::MATRIX_DOUBLE: {
            std::vector<double> data(values.begin(), values.end());
            WriteOutputData(portName, data);
            break;
        }
        case DataType::MATRIX_INT: {
            std::vector<int> data;
            for (double v : values) data.push_back(static_cast<int>(v));
            WriteOutputData(portName, data);
            break;
        }
        case DataType::MATRIX_DCOMPLEX: {
            std::vector<std::complex<double>> data;
            for (int i = 0; i + 1 < values.size(); i += 2) {
                data.emplace_back(values[i], values[i + 1]);
            }
            WriteOutputData(portName, data);
            break;
        }
        default: {
            // 其他类型：作为double数组写入
            std::vector<double> data(values.begin(), values.end());
            WriteOutputData(portName, data);
            break;
        }
        }

        qDebug() << "[CFunctionBlock] Wrote" << values.size()
                 << "values to port" << output.first;
    }

    return true;
}

}
