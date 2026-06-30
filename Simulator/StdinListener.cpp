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

void StdinListener::setPauseControls(QAtomicInt* paused,
                                      QAtomicInt* stopRequested,
                                      QMutex* pauseMutex,
                                      QWaitCondition* pauseCond)
{
    m_paused = paused;
    m_stopRequested = stopRequested;
    m_pauseMutex = pauseMutex;
    m_pauseCond = pauseCond;
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
            // 直接操作原子标志，绕过事件循环
            if (m_paused) {
                *m_paused = 1;
                qDebug() << "[StdinListener] 暂停标志已直接设置";
            }
            // 保留信号槽作为备份
            emit commandReceived(ControlCommand::PAUSE);
        }
        else if (line == "continue" || line == "resume") {
            qDebug() << "[StdinListener] 收到命令: RESUME";
            // 直接操作原子标志，绕过事件循环
            if (m_paused) {
                *m_paused = 0;
                // 获取互斥锁后唤醒等待中的调度线程
                if (m_pauseMutex && m_pauseCond) {
                    QMutexLocker locker(m_pauseMutex);
                    m_pauseCond->wakeAll();
                }
                qDebug() << "[StdinListener] 继续标志已直接设置，调度循环已唤醒";
            }
            // 保留信号槽作为备份
            emit commandReceived(ControlCommand::RESUME);
        }
        else if (line == "stop" || line == "exit" || line == "quit") {
            qDebug() << "[StdinListener] 收到命令: STOP";
            // 直接操作原子标志
            if (m_stopRequested) {
                *m_stopRequested = 1;
            }
            if (m_paused && (*m_paused)) {
                *m_paused = 0;
                if (m_pauseMutex && m_pauseCond) {
                    QMutexLocker locker(m_pauseMutex);
                    m_pauseCond->wakeAll();
                }
            }
            // 保留信号槽作为备份
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
