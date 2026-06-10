#include "ConnectionMonitor.h"
#include "IConnection.h"
#include <QThread>

ConnectionMonitor::ConnectionMonitor(IConnection *conn,
                                     const QString &heartbeatCmd,
                                     int intervalMs, int timeoutMs,
                                     int maxRetries, QObject *parent)
    : QObject(parent),
      m_connection(conn),
      m_heartbeatCmd(heartbeatCmd),
      m_intervalMs(intervalMs),
      m_timeoutMs(timeoutMs),
      m_maxRetries(maxRetries),
      m_failedCount(0),
      m_online(false)
{
    m_heartbeatTimer = new QTimer(this);
    m_responseTimer = new QTimer(this);
    m_responseTimer->setSingleShot(true); // 响应超时仅触发一次

    connect(m_heartbeatTimer, &QTimer::timeout,
            this, &ConnectionMonitor::onHeartbeatTimer);
    connect(m_responseTimer, &QTimer::timeout,
            this, &ConnectionMonitor::onResponseTimeout);
}

ConnectionMonitor::~ConnectionMonitor()
{
    stop();
}

void ConnectionMonitor::start()
{
    m_online = m_connection->isOpen(); // 初始状态跟随物理连接
    m_failedCount = 0;
    m_heartbeatTimer->start(m_intervalMs); // 定期触发心跳
}

void ConnectionMonitor::stop()
{
    m_heartbeatTimer->stop();
    m_responseTimer->stop();
}

bool ConnectionMonitor::isOnline() const { return m_online; }

void ConnectionMonitor::onHeartbeatTimer()
{
    if (!m_connection->isOpen()) {
        m_online = false;
        emit onlineStatusChanged(false);
        tryReconnect();
        return;
    }

    // 发送心跳命令
    m_connection->send(m_heartbeatCmd.toUtf8()); // 发送心跳指令
    m_responseTimer->start(m_timeoutMs); // 开始等待响应
}

void ConnectionMonitor::onResponseTimeout()
{
    m_failedCount++;
    if (m_failedCount >= m_maxRetries) { // 连续超时达到阈值，判定离线
        m_online = false;
        emit onlineStatusChanged(false);
        tryReconnect();
    }
}

void ConnectionMonitor::onHeartbeatResponse()
{
    m_responseTimer->stop(); // 收到任何回复，停止超时计时
    if (m_failedCount > 0) { // 曾出现失败，现在恢复
        m_failedCount = 0;
        if (!m_online) {
            m_online = true;
            emit onlineStatusChanged(true);
            emit reconnected();
        }
    }
}

void ConnectionMonitor::onConnectionError(const QString &)
{
    // 错误发生时，若连接断开会由 onDisconnected 处理
}

void ConnectionMonitor::onConnected()
{
    m_online = true;
    m_failedCount = 0;
    m_heartbeatTimer->start(m_intervalMs); // 重新开始定时心跳
}

void ConnectionMonitor::onDisconnected()
{
    m_online = false;
    emit onlineStatusChanged(false);
    tryReconnect();
}

void ConnectionMonitor::tryReconnect()
{
    m_heartbeatTimer->stop();
    m_responseTimer->stop();

    for (int i = 0; i < m_maxRetries; ++i) {
        emit reconnecting(i + 1);               // 通知UI/管理器正在尝试第几次
        if (m_connection->reconnectAttempt())   // 调用IConnection的重连方法
            return;                             // 成功则内部会调用onConnected
        QThread::msleep(1000);                  // 间隔1秒后重试
    }
    emit reconnectFailed();                     // 全部尝试失败
}
