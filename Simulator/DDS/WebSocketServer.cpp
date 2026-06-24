#include "WebSocketServer.h"
#include <QDebug>
#include <QSslCertificate>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QSslKey>
#include <iostream>

const QString PORT_FILE_PATH = "/tmp/ws_port.conf";

WebSocketServer::WebSocketServer(quint16 port, bool sslMode, QObject *parent)
    : QObject(parent)
    , m_server(nullptr)
    , m_heartbeatTimer(nullptr)
    , m_heartbeatInterval(30000)   // 30秒
    , m_heartbeatTimeout(60000)     // 60秒
    , m_port(port)
    , m_sslMode(sslMode)
    , m_isRunning(false)
{
}

WebSocketServer::~WebSocketServer()
{
    stop();
}

bool WebSocketServer::start()
{
    if (m_isRunning)
        return true;

    // 根据是否加密创建服务器
    QWebSocketServer::SslMode mode = m_sslMode ? QWebSocketServer::SecureMode : QWebSocketServer::NonSecureMode;
    m_server = new QWebSocketServer(QStringLiteral("WebSocket Server"), mode, this);

    // 如果需要SSL，配置证书（示例）
    if (m_sslMode) {
        QSslConfiguration sslConfig;
        // 这里需要加载你的证书和私钥，示例使用自签名
        // QSslCertificate cert = QSslCertificate::fromPath("server.crt").first();
        // QSslKey key = QSslKey::fromPath("server.key", QSsl::Rsa);
        // sslConfig.setLocalCertificate(cert);
        // sslConfig.setPrivateKey(key);
        // m_server->setSslConfiguration(sslConfig);
    }

    if (m_server->listen(QHostAddress::Any, m_port)) {
        quint16 actualPort = m_server->serverPort();
        std::cout << "server open success, actual port: " << actualPort << std::endl;
        writePortToFile(actualPort);
    } else {
        qWarning() << "Server failed to listen on port" << m_port << ":" << m_server->errorString();
        delete m_server;
        m_server = nullptr;
        return false;
    }

    connect(m_server, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
    m_isRunning = true;
//    setupHeartbeat();
    emit serverStarted();
    std::cout << "WebSocket server started on port" << m_port << (m_sslMode ? "(SSL)" : "") << std::endl;
    return true;
}

void WebSocketServer::stop()
{
    if (!m_isRunning)
        return;

    m_isRunning = false;
    if (m_heartbeatTimer) {
        m_heartbeatTimer->stop();
        delete m_heartbeatTimer;
        m_heartbeatTimer = nullptr;
    }

    // 关闭所有客户端连接
    for (QWebSocket *client : m_clients) {
        client->close();
        client->deleteLater();
    }
    m_clients.clear();
    m_lastPongTime.clear();

    if (m_server) {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }
    emit serverStopped();
    qDebug() << "WebSocket server stopped";
}

void WebSocketServer::sendToClient(QWebSocket *client, const QString &message)
{
    if (client && client->state() == QAbstractSocket::ConnectedState) {
        client->sendTextMessage(message);
    }
}

void WebSocketServer::broadcastMessage(const QString &message)
{
    for (QWebSocket *client : m_clients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            client->sendTextMessage(message);
        }
    }
}

void WebSocketServer::sendToAllExcept(QWebSocket *exclude, const QString &message)
{
    for (QWebSocket *client : m_clients) {
        if (client != exclude && client->state() == QAbstractSocket::ConnectedState) {
            client->sendTextMessage(message);
        }
    }
}

void WebSocketServer::setHeartbeatInterval(int ms)
{
    m_heartbeatInterval = ms;
    if (m_heartbeatTimer && m_isRunning) {
        m_heartbeatTimer->setInterval(ms);
    }
}

void WebSocketServer::setHeartbeatTimeout(int ms)
{
    m_heartbeatTimeout = ms;
}

void WebSocketServer::onNewConnection()
{
    QWebSocket *client = m_server->nextPendingConnection();
    if (!client)
        return;

    // 记录客户端信息
    m_clients.append(client);
    m_lastPongTime[client].start();   // 开始记录pong时间

    // 连接信号
    connect(client, &QWebSocket::textMessageReceived, this, &WebSocketServer::onTextMessageReceived);
    connect(client, &QWebSocket::disconnected, this, &WebSocketServer::onSocketDisconnected);
    connect(client, &QWebSocket::pong, this, &WebSocketServer::onClientPong);

    qDebug() << "Client connected from" << client->peerAddress().toString() << client->peerPort();
    emit clientConnected(client);
}

void WebSocketServer::onTextMessageReceived(const QString &message)
{
    QWebSocket *senderClient = qobject_cast<QWebSocket*>(sender());
    if (!senderClient)
        return;

    qDebug() << "Message from" << senderClient->peerAddress().toString() << ":" << message;
    emit messageReceived(senderClient, message);
}

void WebSocketServer::onSocketDisconnected()
{
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (!client)
        return;

    cleanupClient(client);
    emit clientDisconnected(client);
    qDebug() << "Client disconnected:" << client->peerAddress().toString();
}

void WebSocketServer::onClientPong(quint64 elapsedTime, const QByteArray &payload)
{
    Q_UNUSED(elapsedTime);
    Q_UNUSED(payload);
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (client && m_lastPongTime.contains(client)) {
        m_lastPongTime[client].restart();   // 重置超时计时
        qDebug() << "Received pong from" << client->peerAddress().toString();
    }
}

void WebSocketServer::onServerHeartbeat()
{
    // 向所有客户端发送ping，并检查超时
    QMutableHashIterator<QWebSocket*, QElapsedTimer> it(m_lastPongTime);
    while (it.hasNext()) {
        it.next();
        QWebSocket *client = it.key();
        if (!client || client->state() != QAbstractSocket::ConnectedState) {
            it.remove();
            continue;
        }

        // 发送ping
        client->ping();

        // 检查是否超时
        if (it.value().isValid() && it.value().elapsed() > m_heartbeatTimeout) {
            qWarning() << "Client" << client->peerAddress().toString() << "heartbeat timeout, closing";
            emit clientHeartbeatTimeout(client);
            client->close();   // 关闭连接会触发disconnected信号，进而清理
        }
    }
}

void WebSocketServer::setupHeartbeat()
{
    if (m_heartbeatTimer) {
        m_heartbeatTimer->stop();
        delete m_heartbeatTimer;
    }
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(m_heartbeatInterval);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &WebSocketServer::onServerHeartbeat);
    m_heartbeatTimer->start();
    qDebug() << "Heartbeat enabled: interval=" << m_heartbeatInterval << "ms, timeout=" << m_heartbeatTimeout << "ms";
}

void WebSocketServer::cleanupClient(QWebSocket *client)
{
    if (!client)
        return;

    m_clients.removeAll(client);
    m_lastPongTime.remove(client);
    client->deleteLater();
}

void WebSocketServer::writePortToFile(quint16 port)
{
    QFile file(PORT_FILE_PATH);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << port;
        file.close();
        qDebug() << "port information has written to: " << PORT_FILE_PATH;
    }
    else {
        qWarning() << "Can not write to port file";
    }
}
