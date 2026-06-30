#ifndef STDINLISTENER_H
#define STDINLISTENER_H

#include <QThread>
#include <QAtomicInt>
#include <QMutex>
#include <QWaitCondition>

// 控制命令枚举
enum class ControlCommand {
    NONE,
    PAUSE,
    RESUME,
    STOP
};

class StdinListener : public QThread
{
    Q_OBJECT

public:
    explicit StdinListener(QObject *parent = nullptr);

    // 设置暂停控制指针，允许直接操作原子标志（绕过事件循环）
    void setPauseControls(QAtomicInt* paused,
                          QAtomicInt* stopRequested,
                          QMutex* pauseMutex,
                          QWaitCondition* pauseCond);

protected:
    void run() override;

public slots:
    void stopListening();

signals:
    void commandReceived(ControlCommand cmd);

private:
    QAtomicInt m_running;

    // 直接操作的暂停控制指针
    QAtomicInt* m_paused = nullptr;
    QAtomicInt* m_stopRequested = nullptr;
    QMutex* m_pauseMutex = nullptr;
    QWaitCondition* m_pauseCond = nullptr;
};

#endif // STDINLISTENER_H