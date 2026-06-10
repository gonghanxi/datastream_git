#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "IConnection.h"
#include <QTcpSocket>

class TcpConnection : public IConnection
{
    Q_OBJECT
public:
    explicit TcpConnection(const QString &host, quint16 port,
                           QObject *parent = nullptr);
    ~TcpConnection();

    bool open() override;
    void close() override;
    qint64 send(const QByteArray &data) override;
    QByteArray receive(int timeoutMs = 3000) override;
    bool isOpen() const override;

private slots:
    void onConnected();                                    // socket连接成功
    void onDisconnected();                                 // socket断开
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    QTcpSocket *m_socket;
    QString m_host;
    quint16 m_port;
};

#endif
