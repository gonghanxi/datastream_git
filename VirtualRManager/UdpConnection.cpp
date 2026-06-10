#include "UdpConnection.h"
#include <QDebug>

UdpConnection::UdpConnection(const QString &localIP, quint16 localPort,
                             const QString &remoteIP, quint16 remotePort,
                             QObject *parent)
    : IConnection(parent),
      m_socket(new QUdpSocket(this)),
      m_localAddr(QHostAddress(localIP)),
      m_localPort(localPort),
      m_remoteAddr(QHostAddress(remoteIP)),
      m_remotePort(remotePort),
      m_isOpen(false)
{
    connect(m_socket, &QUdpSocket::readyRead,
            this, &UdpConnection::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QUdpSocket::error),
            this, &UdpConnection::onSocketError);
}

UdpConnection::~UdpConnection()
{
    close();
}

bool UdpConnection::open()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->close();

    if (!m_socket->bind(m_localAddr, m_localPort)) { // UDP绑定本地端口
        notifyConnectionError("UDP bind failed: " + m_socket->errorString());
        return false;
    }
    m_isOpen = true;
    notifyConnected();
    return true;
}

void UdpConnection::close()
{
    m_socket->close();
    m_isOpen = false;
    notifyDisconnected();
}

qint64 UdpConnection::send(const QByteArray &data)
{
    if (!m_isOpen) return -1;
    return m_socket->writeDatagram(data, m_remoteAddr, m_remotePort);
}

QByteArray UdpConnection::receive(int timeoutMs)
{
    if (!m_isOpen) return {};
    if (!m_socket->waitForReadyRead(timeoutMs))
        return {};
    QByteArray datagram;
    datagram.resize(m_socket->pendingDatagramSize());
    QHostAddress sender;
    quint16 senderPort;
    m_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
    notifyDataReceived(datagram); // 通知监控器
    return datagram;
}

bool UdpConnection::isOpen() const { return m_isOpen; }

void UdpConnection::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());
        m_socket->readDatagram(datagram.data(), datagram.size());
        notifyDataReceived(datagram);
    }
}

void UdpConnection::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    notifyConnectionError(m_socket->errorString());
}
