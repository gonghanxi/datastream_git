#ifndef USBCONNECTION_H
#define USBCONNECTION_H

#include "IConnection.h"

class UsbConnection : public IConnection
{
    Q_OBJECT
public:
    explicit UsbConnection(const QString &resourceName,
                           QObject *parent = nullptr); // VISA资源名
    ~UsbConnection();

    bool open() override;
    void close() override;
    qint64 send(const QByteArray &data) override;
    QByteArray receive(int timeoutMs = 3000) override;
    bool isOpen() const override;

private:
    QString m_resourceName; // VISA 资源名，如 "USB0::0x1234::0x5678::..."
    bool m_isOpen;
    // 实际设备句柄（如 ViSession）
    // 实际项目中应持有 ViSession 或 libusb 句柄
};

#endif
