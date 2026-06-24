#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QTimer>
#include <QHash>
#include <QElapsedTimer>

class WebSocketServer : public QObject
{
    Q_OBJECT
public:
    // 客户端连接状态
    enum ClientState {
        ClientConnected,
        ClientDisconnected
    };

    explicit WebSocketServer(quint16 port, bool sslMode = false, QObject *parent = nullptr);
    ~WebSocketServer();

    // 启动/停止服务
    bool start();
    void stop();

    // 发送消息
    void sendToClient(QWebSocket *client, const QString &message);
    void broadcastMessage(const QString &message);
    void sendToAllExcept(QWebSocket *exclude, const QString &message);

    // 客户端管理
    QList<QWebSocket*> getClients() const { return m_clients; }
    int clientCount() const { return m_clients.size(); }

    // 心跳配置
    void setHeartbeatInterval(int ms);   // 默认30秒
    void setHeartbeatTimeout(int ms);    // 默认60秒

signals:
    void serverStarted();
    void serverStopped();
    void clientConnected(QWebSocket *client);
    void clientDisconnected(QWebSocket *client);
    void messageReceived(QWebSocket *sender, const QString &message);
    void clientHeartbeatTimeout(QWebSocket *client);  // 心跳超时，即将断开

private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString &message);
    void onSocketDisconnected();
    void onClientPong(quint64 elapsedTime, const QByteArray &payload);
    void onServerHeartbeat();  // 定时发送ping并检查超时

private:
    void setupHeartbeat();
    void cleanupClient(QWebSocket *client);
    void writePortToFile(quint16 port);

    QWebSocketServer *m_server;
    QList<QWebSocket*> m_clients;
    QHash<QWebSocket*, QElapsedTimer> m_lastPongTime;  // 记录每个客户端最后pong时间
    QTimer *m_heartbeatTimer;
    int m_heartbeatInterval;   // 毫秒
    int m_heartbeatTimeout;     // 毫秒
    quint16 m_port;
    bool m_sslMode;
    bool m_isRunning;
};

#endif // WEBSOCKETSERVER_H
