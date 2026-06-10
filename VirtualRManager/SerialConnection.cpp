#include "SerialConnection.h"
#include <QDebug>

SerialConnection::SerialConnection(const QString &portName,
                                   int baudRate, QObject *parent)
    : IConnection(parent),
      m_serial(new QSerialPort(this)),
      m_portName(portName),
      m_baudRate(baudRate)
{
    connect(m_serial, &QSerialPort::readyRead,
            this, &SerialConnection::onReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred,
            this, &SerialConnection::onErrorOccurred);
}

SerialConnection::~SerialConnection()
{
    close();
}

bool SerialConnection::open()
{
    m_serial->setPortName(m_portName);
    m_serial->setBaudRate(m_baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadWrite)) {
        notifyConnected(); // 通知基类和监控器
        return true;
    } else {
        notifyConnectionError(m_serial->errorString());
        return false;
    }
}

void SerialConnection::close()
{
    if (m_serial->isOpen()) {
        m_serial->close();
        notifyDisconnected();
    }
}

qint64 SerialConnection::send(const QByteArray &data)
{
    if (!m_serial->isOpen()) return -1;
    return m_serial->write(data);
}

QByteArray SerialConnection::receive(int timeoutMs)
{
    if (!m_serial->isOpen()) return {};
    if (!m_serial->waitForReadyRead(timeoutMs)) // 阻塞等待数据
        return {};
    QByteArray data = m_serial->readAll();
    notifyDataReceived(data); // 收到数据后通知监控器（重置心跳超时）
    return data;
}

bool SerialConnection::isOpen() const
{
    return m_serial->isOpen();
}

void SerialConnection::onReadyRead()
{
    QByteArray data = m_serial->readAll();
    notifyDataReceived(data); // 异步数据到达，同样通知监控器
}

void SerialConnection::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) { // 串口设备被移除等严重错误
        notifyConnectionError(m_serial->errorString());
        notifyDisconnected();
    }
}
