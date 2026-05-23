#ifndef STDINLISTENER_H
#define STDINLISTENER_H

#include <QThread>
#include <QAtomicInt>

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

protected:
    void run() override;

public slots:
    void stopListening();

signals:
    void commandReceived(ControlCommand cmd);

private:
    QAtomicInt m_running;
};

#endif // STDINLISTENER_H