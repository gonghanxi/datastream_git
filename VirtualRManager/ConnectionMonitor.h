#ifndef CONNECTIONMONITOR_H
#define CONNECTIONMONITOR_H

#include <QObject>
#include <QTimer>
#include <QString>

class IConnection;

class ConnectionMonitor : public QObject
{
    Q_OBJECT
public:
    explicit ConnectionMonitor(IConnection *conn,
                               const QString &heartbeatCmd,
                               int intervalMs, int timeoutMs,
                               int maxRetries,
                               QObject *parent = nullptr);
    ~ConnectionMonitor();

    void start();            // 开始心跳检测
    void stop();             // 停止心跳检测
    bool isOnline() const;   // 当前是否判定在线

public slots:
    void onHeartbeatResponse();                 // 当接收到任何数据/心跳回复时调用
    void onConnectionError(const QString &error);
    void onConnected();                         // 物理连接建立时调用
    void onDisconnected();                      // 物理连接断开时调用

signals:
    void onlineStatusChanged(bool online);
    void reconnecting(int attempt);
    void reconnectFailed();
    void reconnected();

private slots:
    void onHeartbeatTimer();     // 定时发送心跳
    void onResponseTimeout();    // 心跳回复超时

private:
    void tryReconnect();         // 执行重连序列

    IConnection *m_connection;
    QString m_heartbeatCmd;      // 心跳命令，通常为*IDN?\n
    int m_intervalMs;            // 心跳间隔
    int m_timeoutMs;             // 等待回复超时
    int m_maxRetries;            // 超时多少次后判定断线

    QTimer *m_heartbeatTimer;
    QTimer *m_responseTimer;     // 单次触发，用于测量心跳响应超时
    int m_failedCount;           // 连续失败次数
    bool m_online;               // 当前逻辑在线状态
};

#endif
