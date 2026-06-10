#ifndef UDPCONNECTION_H
#define UDPCONNECTION_H

#include "IConnection.h"
#include <QUdpSocket>
#include <QHostAddress>

class UdpConnection : public IConnection
{
    Q_OBJECT
public:
    explicit UdpConnection(const QString &localIP,
                           quint16 localPort,
                           const QString &remoteIP,
                           quint16 remotePort,
                           QObject *parent = nullptr);
    ~UdpConnection();

    bool open() override;
    void close() override;
    qint64 send(const QByteArray &data) override;
    QByteArray receive(int timeoutMs = 3000) override;
    bool isOpen() const override;

private slots:
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    QUdpSocket *m_socket;
    QHostAddress m_localAddr;
    quint16 m_localPort;
    QHostAddress m_remoteAddr;
    quint16 m_remotePort;
    bool m_isOpen; // UDP无连接，用标志位模拟
};

#endif
