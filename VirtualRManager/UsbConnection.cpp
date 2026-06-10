#include "UsbConnection.h"
#include <QDebug>

UsbConnection::UsbConnection(const QString &resourceName, QObject *parent)
    : IConnection(parent), m_resourceName(resourceName), m_isOpen(false) {}

UsbConnection::~UsbConnection() { close(); }

bool UsbConnection::open()
{
    // 在此调用 VISA 打开资源
    // ViSession defaultRM, vi;
    // viOpenDefaultRM(&defaultRM);
    // viOpen(defaultRM, m_resourceName.toUtf8().data(), VI_NULL, VI_NULL, &vi);
    // 这里先模拟成功
    // 此处应调用VISA viOpen等函数打开设备，这里占位返回true
    // 实际项目中需替换为真实实现
    m_isOpen = true;
    notifyConnected();
    return true;
}

void UsbConnection::close()
{
    // viClose 关闭设备
    // viClose(vi);
    m_isOpen = false;
    notifyDisconnected();
}

qint64 UsbConnection::send(const QByteArray &data)
{
    // viWrite 发送数据
    // viWrite(vi, (ViBuf)data.data(), data.size(), &retCnt);
    Q_UNUSED(data);
    return -1;
}

QByteArray UsbConnection::receive(int timeoutMs)
{
    // viRead 带超时接收数据
    // viRead(vi, buf, count, &retCnt) with timeout
    Q_UNUSED(timeoutMs);
    return QByteArray();
}

bool UsbConnection::isOpen() const { return m_isOpen; }
