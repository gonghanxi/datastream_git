#ifndef SERIALCONNECTION_H
#define SERIALCONNECTION_H

#include "IConnection.h"
#include <QSerialPort>

class SerialConnection : public IConnection
{
    Q_OBJECT
public:
    explicit SerialConnection(const QString &portName,
                              int baudRate = 115200,
                              QObject *parent = nullptr);
    ~SerialConnection();

    bool open() override;
    void close() override;
    qint64 send(const QByteArray &data) override;
    QByteArray receive(int timeoutMs = 3000) override;
    bool isOpen() const override;

private slots:
    void onReadyRead(); // 串口有数据可读
    void onErrorOccurred(QSerialPort::SerialPortError error);

private:
    QSerialPort *m_serial;
    QString m_portName;
    int m_baudRate;
};

#endif
