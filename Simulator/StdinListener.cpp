#include "StdinListener.h"
#include <QDebug>
#include <QTextStream>
#include <iostream>

// 注册元类型用于跨线程信号槽
Q_DECLARE_METATYPE(ControlCommand)

StdinListener::StdinListener(QObject *parent)
    : QThread(parent)
    , m_running(1)
{
    // 注册自定义枚举类型
    qRegisterMetaType<ControlCommand>("ControlCommand");
}

void StdinListener::run()
{
    QTextStream in(stdin);

    qDebug() << "[StdinListener] 监听线程已启动，等待命令...";
    qDebug() << "[StdinListener] 支持命令: pause(暂停) continue(继续) stop(停止)";

    while (m_running) {
        // 阻塞读取一行输入
        QString line = in.readLine();

        if (line.isEmpty()) {
            continue;
        }

        line = line.trimmed().toLower();

        if (line == "pause") {
            qDebug() << "[StdinListener] 收到命令: PAUSE";
            emit commandReceived(ControlCommand::PAUSE);
        }
        else if (line == "continue" || line == "resume") {
            qDebug() << "[StdinListener] 收到命令: RESUME";
            emit commandReceived(ControlCommand::RESUME);
        }
        else if (line == "stop" || line == "exit" || line == "quit") {
            qDebug() << "[StdinListener] 收到命令: STOP";
            emit commandReceived(ControlCommand::STOP);
            break;
        }
        else {
            qDebug() << "[StdinListener] 未知命令:" << line
                     << "(支持: pause, continue, stop)";
        }
    }

    qDebug() << "[StdinListener] 监听线程退出";
}

void StdinListener::stopListening()
{
    m_running = 0;
}
