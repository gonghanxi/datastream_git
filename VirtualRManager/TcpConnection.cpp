#include "TcpConnection.h"

TcpConnection::TcpConnection(const QString &host, quint16 port,
                             QObject *parent)
    : IConnection(parent),
      m_socket(new QTcpSocket(this)),
      m_host(host),
      m_port(port)
{
    connect(m_socket, &QTcpSocket::connected,
            this, &TcpConnection::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &TcpConnection::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &TcpConnection::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &TcpConnection::onErrorOccurred);
}

TcpConnection::~TcpConnection()
{
    close();
}

bool TcpConnection::open()
{
    m_socket->connectToHost(m_host, m_port);
    // 等待连接完成
    if (!m_socket->waitForConnected(3000)) { // 阻塞等待连接成功，超时3秒
        notifyConnectionError(m_socket->errorString());
        return false;
    }
    // 成功时会由 onConnected 回调，这里也直接通知
    return true;
}

void TcpConnection::onConnected()
{
    notifyConnected(); // 通知监控器，开始心跳
}

void TcpConnection::onDisconnected()
{
    notifyDisconnected(); // 触发监控器重连
}

void TcpConnection::close()
{
    if (m_socket->isOpen()) {
        m_socket->disconnectFromHost();
    }
}

qint64 TcpConnection::send(const QByteArray &data)
{
    if (!m_socket->isOpen()) return -1;
    return m_socket->write(data);
}

QByteArray TcpConnection::receive(int timeoutMs)
{
    if (!m_socket->isOpen()) return {};
    if (!m_socket->waitForReadyRead(timeoutMs))
        return {};
    QByteArray data = m_socket->readAll();
    notifyDataReceived(data);
    return data;
}

bool TcpConnection::isOpen() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void TcpConnection::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    notifyDataReceived(data);
}

void TcpConnection::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    notifyConnectionError(m_socket->errorString());
}
