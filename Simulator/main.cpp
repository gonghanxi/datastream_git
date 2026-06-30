#include <QApplication>
#include <QCoreApplication>
#include <QLibrary>
#include <iostream>
#include <QDebug>
#include <cstdlib>
#include <QCommandLineParser>
#include <QtConcurrent>
#include <QTimer>
#include <QFile>
#include <QDateTime>
#include <chrono>
#include "../Common/LogExport.h"
#include "../Common/ISimRunner.h"
#include <QTextCodec>
#include "StdinListener.h"

// 跨平台动态库加载宏
#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
#else
    #include <dlfcn.h>
    #include <locale.h>
#endif

using namespace std;
using namespace std::chrono;

// 跨平台控制台编码设置函数
void setupConsoleEncoding() {
#ifdef _WIN32
    // Windows 下设置控制台编码为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#else
    // Linux 下设置 locale
    setlocale(LC_ALL, "");
    // 或者强制使用 UTF-8
    setlocale(LC_ALL, "C.UTF-8");
#endif
}

int main(int argc, char *argv[])
{
    // 设置控制台编码
    setupConsoleEncoding();

    QCoreApplication app(argc, argv);

    // 设置文本编码
#ifdef _WIN32
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#else
    // Linux 下使用系统 locale
    QTextCodec::setCodecForLocale(QTextCodec::codecForLocale());
#endif

    // 获取当前时间作为日志开始时间
    QDateTime startDateTime = QDateTime::currentDateTime();
    QString logStartTime = startDateTime.toString("yyyyMMdd hh:mm:ss");

    // ========== 参数校验（改为3个参数） ==========
//    if(argc != 3) {
//        LOG_ERROR("参数个数为：", argc, "，参数个数错误！请检查参数。");
//        return 1;
//    }
    if(argc != 4) {
        LOG_ERROR("参数个数为：", argc, "，参数个数错误！请检查参数。");
        return 1;
    }

    // ========== 解析命令行参数 ==========
    QString linkFiles;
    QString outPutPath;

    // 跨平台字符编码处理
#ifdef _WIN32
    QTextCodec* gbkCodec = QTextCodec::codecForName("GBK");
    if (gbkCodec) {
        linkFiles = gbkCodec->toUnicode(argv[1]);
        outPutPath = gbkCodec->toUnicode(argv[2]);
    } else {
        linkFiles = QString::fromLocal8Bit(argv[1]);
        outPutPath = QString::fromLocal8Bit(argv[2]);
    }
#else
    // Linux 下直接使用 UTF-8
    linkFiles = QString::fromUtf8(argv[1]);
    outPutPath = QString::fromUtf8(argv[2]);
#endif

    // ========== 处理链路文件路径 ==========
    if(linkFiles.front() == '[' && linkFiles.back() == ']') {
        linkFiles = linkFiles.mid(1, linkFiles.length() - 2);
    } else {
        LOG_ERROR("链路文件路径格式错误，需要以 [ 开头以 ] 结尾。");
        return 1;
    }

    // 检查文件是否存在
    QStringList filesList = linkFiles.split(",");
    for(auto file : filesList) {
        if(!QFile::exists(file)) {
            LOG_ERROR("链路文件：", file.toStdString(), "，不存在。");
            return 1;
        }
    }

    if(!QFile::exists(outPutPath)) {
        LOG_ERROR("输出文件夹不存在。");
        return 1;
    }

    // ========== 加载动态库 ==========
    QString appPath = QCoreApplication::applicationDirPath();

#ifdef _WIN32
    QString libPath = appPath + "/SignalFlowSimulatorLib.dll";
    QLibrary myLib(libPath);
    if (!myLib.load()) {
        LOG_ERROR("动态库：", libPath.toStdString(), "加载失败。");
        LOG_ERROR("错误信息：", myLib.errorString().toStdString());
        return 1;
    }

    // 获取函数指针
    CreateSimFunction createFunction = (CreateSimFunction)myLib.resolve("createSimRunner");

    if (!createFunction) {
        LOG_ERROR("函数：createSimRunner指针获取失败。");
        LOG_ERROR("错误信息：", myLib.errorString().toStdString());
        return 1;
    }
#else
    // Linux 下使用 QLibrary
    QString libPath = appPath + "/libSignalFlowSimulatorLib.so";
    QLibrary myLib(libPath);

    // 设置加载标志
    myLib.setLoadHints(QLibrary::ResolveAllSymbolsHint);

    if (!myLib.load()) {
        // 尝试备用路径
        libPath = appPath + "/SignalFlowSimulatorLib.so";
        myLib.setFileName(libPath);
        if (!myLib.load()) {
            LOG_ERROR("动态库加载失败。尝试路径：", libPath.toStdString());
            LOG_ERROR("错误信息：", myLib.errorString().toStdString());
            return 1;
        }
    }

    // 获取函数指针
    CreateSimFunction createFunction = (CreateSimFunction)myLib.resolve("createSimRunner");

    if (!createFunction) {
        LOG_ERROR("函数：createSimRunner 解析失败。");
        LOG_ERROR("错误信息：", myLib.errorString().toStdString());
        return 1;
    }
#endif

    // ========== 准备参数 ==========
    QByteArray appPathUtf8 = appPath.toUtf8();
    QByteArray outPutPathUtf8 = outPutPath.toUtf8();

    std::vector<QByteArray> fileBytes;
    std::vector<const char*> filePtrs;

    for (const QString& file : filesList) {
        fileBytes.push_back(file.toUtf8());
        filePtrs.push_back(fileBytes.back().constData());
    }

    // ========== 创建仿真器实例 ==========
    ISimRunner* sim = createFunction(appPathUtf8.constData(),
                                     filePtrs.data(),
                                     static_cast<int>(filePtrs.size()),
                                     outPutPathUtf8.constData());

    if (!sim) {
        LOG_ERROR("仿真器创建失败。");
        return 1;
    }

    // ========== 启动 stdin 监听线程 ==========
    StdinListener stdinListener;

    // 传递暂停控制指针，允许 StdinListener 直接操作原子标志（绕过事件循环）
    stdinListener.setPauseControls(
        sim->getPausedPtr(),
        sim->getStopRequestedPtr(),
        sim->getPauseMutexPtr(),
        sim->getPauseCondPtr()
    );

    // 连接暂停命令
    QObject::connect(&stdinListener, &StdinListener::commandReceived,
        [&](ControlCommand cmd) {
            if (cmd == ControlCommand::PAUSE) {
                qDebug() << "[main] 执行暂停";
                sim->pause();
            }
        });

    // 连接继续命令
    QObject::connect(&stdinListener, &StdinListener::commandReceived,
        [&](ControlCommand cmd) {
            if (cmd == ControlCommand::RESUME) {
                qDebug() << "[main] 执行继续";
                sim->resume();
            }
        });

    // 连接停止命令
    QObject::connect(&stdinListener, &StdinListener::commandReceived,
        [&](ControlCommand cmd) {
            if (cmd == ControlCommand::STOP) {
                qDebug() << "[main] 执行停止";
                sim->requestStop();
            }
        });

    // 启动监听线程
    stdinListener.start();

    qDebug() << "========================================";
    qDebug() << "仿真器已启动，支持以下控制命令:";
    qDebug() << "  pause    - 暂停仿真";
    qDebug() << "  continue - 继续仿真";
    qDebug() << "  stop     - 停止仿真";
    qDebug() << "========================================";

    // ========== 记录仿真开始时间 ==========
    auto simulationStartTime = high_resolution_clock::now();

    // ========== 仿真初始化 ==========
    bool success = sim->start();

    if(success) {
        qDebug() << "链路完成所有准备工作";

        LOG_INFO("当前日志由数据流仿真器创建于", logStartTime.toStdString());
        std::cout.flush();
        LOG_INFO("数据流仿真器已启动...");
        std::cout.flush();
        LOG_INFO("仿真初始化成功，开始执行仿真...");
        std::cout.flush();

        // ========== 超时控制 ==========
        bool timeoutOccurred = false;

        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);

        QObject::connect(&timeoutTimer, &QTimer::timeout, [&]() {
            timeoutOccurred = true;
            LOG_ERROR("仿真运行时间超过10分钟，强制退出！");
            QCoreApplication::quit();
        });

        timeoutTimer.start(600000);  // 10分钟

        // ========== 异步执行仿真 ==========
        QtConcurrent::run([&]() {
            success = sim->run();
            // 仿真完成后退出事件循环
            QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
        });

        // ========== 进入事件循环 ==========
        QCoreApplication::exec();

        // ========== 超时处理 ==========
        if(timeoutOccurred) {
            timeoutTimer.stop();
            LOG_ERROR("仿真因超时被强制终止。");
            stdinListener.stopListening();
            stdinListener.wait(3000);
            delete sim;
            myLib.unload();
            return 1;
        }

        // 停止定时器
        timeoutTimer.stop();

        // ========== 计算耗时 ==========
        auto simulationEndTime = high_resolution_clock::now();
        auto simulationDuration = duration_cast<duration<double>>(simulationEndTime - simulationStartTime);
        double cpuTimeUsed = simulationDuration.count();

        // ========== 打印仿真参数 ==========
        SystemVueModelBuilder::SimuParameter simParams = sim->getSimulationParameters();

        LOG_INFO("仿真起始时间：", simParams.startTime, " s");
        std::cout.flush();
        LOG_INFO("仿真结束时间：", simParams.stopTime, " s");
        std::cout.flush();
        LOG_INFO("CPU仿真耗时：", cpuTimeUsed, " s");
        std::cout.flush();
        LOG_INFO("最小时间步长：", simParams.time_Interval, " s");
        std::cout.flush();
        LOG_INFO("最大时间步长：", simParams.time_Interval, " s");
        std::cout.flush();

    } else {
        LOG_INFO("仿真初始化失败。");
        LOG_INFO("仿真起始时间：", 0, " s");
        LOG_INFO("仿真结束时间：", 0, " s");
        LOG_INFO("CPU仿真耗时：", 0, " s");
        LOG_INFO("最小时间步长：", 0, " s");
        LOG_INFO("最大时间步长：", 0, " s");
    }

    LOG_INFO("数据流程序执行完成。");
    std::cout.flush();

    // ========== 清理资源 ==========
    stdinListener.stopListening();
    if (!stdinListener.wait(3000)) {
        stdinListener.terminate();  // 强制终止阻塞的线程
        stdinListener.wait(2000);
    }

    delete sim;
    myLib.unload();

    return success ? 0 : 1;
}
