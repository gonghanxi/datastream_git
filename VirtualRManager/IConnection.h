#ifndef ICONNECTION_H
#define ICONNECTION_H

#include <QObject>
#include <QByteArray>
#include <QString>

class ConnectionMonitor;

class IConnection : public QObject
{
    Q_OBJECT
public:
    explicit IConnection(QObject *parent = nullptr);
    virtual ~IConnection();

    // 纯虚函数 – 子类必须实现
    virtual bool open() = 0; // 打开连接，成功返回true
    virtual void close() = 0; // 关闭连接，释放资源
    virtual qint64 send(const QByteArray &data) = 0; // 发送原始字节数据
    virtual QByteArray receive(int timeoutMs = 3000) = 0; // 阻塞接收，超时返回空
    virtual bool isOpen() const = 0; // 返回当前连接是否已建立（不含监控状态）

    // 健康检查接口
    void enableHealthCheck(const QString &heartbeatCmd = "*IDN?\n",
                           int intervalMs = 5000,
                           int timeoutMs = 3000,
                           int maxRetries = 3);
    void disableHealthCheck();
    bool isOnline() const; // 综合监控状态与连接状态，供管理器判断

    // 重连尝试（默认关闭再打开，子类可重写）
    virtual bool reconnectAttempt();

signals:
    void dataReceived(QByteArray data);          // 数据到达（异步）
    void connectionError(const QString &error);  // 连接错误描述
    void onlineStatusChanged(bool online);       // 在线状态变化
    void reconnecting(int attempt);              // 正在进行重连，attempt为第几次
    void reconnectFailed();                      // 重连最终失败
    void reconnected();                          // 重连成功

protected:
    // 子类在收到数据后调用，会触发监控器心跳复位
    void notifyDataReceived(const QByteArray &data);
    // 子类在发生错误/断开时调用
    void notifyConnectionError(const QString &error);
    // 连接建立时调用
    void notifyConnected();
    // 连接关闭时调用
    void notifyDisconnected();


    ConnectionMonitor *m_monitor;

private:
    void initMonitorConnections(); // 连接监控器信号与本对象信号
};

#endif // ICONNECTION_H
