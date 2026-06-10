#include <QCoreApplication>

#include "SerialConnection.h"
#include "TcpConnection.h"
#include "InstrumentManager.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // 创建串口连接（频谱仪）
    auto serialConn = new SerialConnection("COM3", 9600);
    serialConn->enableHealthCheck("*IDN?\n", 5000, 3000, 3);

    // 创建 TCP 连接（矢网）
    auto tcpConn = new TcpConnection("192.168.1.100", 5025);
    tcpConn->enableHealthCheck("*IDN?\n");

    // 管理器
    InstrumentManager manager("./data_logs");
    manager.addInstrument("SA1", InstrumentType::SA, serialConn);
    manager.addInstrument("VNA1", InstrumentType::VNA, tcpConn);

    // 打开连接
    serialConn->open();
    tcpConn->open();

    // 发送命令（预留）
    manager.sendCommand("SA1", ":FREQ:CENT 1e9");

    // 查询一次
    QString resp = manager.receiveCommand("SA1");
    qDebug() << "Response:" << resp;

    // 持续监听
    manager.startListening("VNA1");
    // 连接 manager::instrumentDataReceived 信号处理异步数据

    return a.exec();
}
