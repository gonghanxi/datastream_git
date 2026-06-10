#include "IConnection.h"
#include "ConnectionMonitor.h"
#include <QDebug>

IConnection::IConnection(QObject *parent)
    : QObject(parent), m_monitor(nullptr) {}

IConnection::~IConnection()
{
    disableHealthCheck();
}

void IConnection::enableHealthCheck(const QString &heartbeatCmd,
                                    int intervalMs, int timeoutMs,
                                    int maxRetries)
{
    if (m_monitor) return;
    // 创建监控器并绑定当前连接
    m_monitor = new ConnectionMonitor(this, heartbeatCmd,
                                      intervalMs, timeoutMs, maxRetries);
    initMonitorConnections();
    m_monitor->start(); // 启动心跳定时器
}

void IConnection::disableHealthCheck()
{
    if (m_monitor) {
        m_monitor->stop();
        delete m_monitor;
        m_monitor = nullptr;
    }
}

bool IConnection::isOnline() const
{
    // 有监控器时以监控器判断为准，否则仅检查物理连接状态
    if (m_monitor)
        return m_monitor->isOnline();
    return isOpen(); // 无监控时直接依靠连接状态
}

void IConnection::notifyDataReceived(const QByteArray &data)
{
    emit dataReceived(data);
    if (m_monitor)
        m_monitor->onHeartbeatResponse(); // 通知监控器收到数据（心跳重置）
}

void IConnection::notifyConnectionError(const QString &error)
{
    emit connectionError(error);
    if (m_monitor)
        m_monitor->onConnectionError(error);
}

void IConnection::notifyConnected()
{
    if (m_monitor)
        m_monitor->onConnected(); // 重置监控器失败计数
}

void IConnection::notifyDisconnected()
{
    if (m_monitor)
        m_monitor->onDisconnected(); // 触发监控器重连流程
}

bool IConnection::reconnectAttempt()
{
    // 默认重连：关闭后重新打开
    close();
    bool ok = open();
    if (ok) {
        emit reconnected();
        notifyConnected();
    }
    return ok;
}

void IConnection::initMonitorConnections()
{
    if (!m_monitor) return;
    // 将监控器信号透传为IConnection信号，供上层使用
    connect(m_monitor, &ConnectionMonitor::onlineStatusChanged,
            this, &IConnection::onlineStatusChanged);
    connect(m_monitor, &ConnectionMonitor::reconnecting,
            this, &IConnection::reconnecting);
    connect(m_monitor, &ConnectionMonitor::reconnectFailed,
            this, &IConnection::reconnectFailed);
    connect(m_monitor, &ConnectionMonitor::reconnected,
            this, &IConnection::reconnected);
}
