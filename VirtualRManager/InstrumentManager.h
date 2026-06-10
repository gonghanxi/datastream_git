#ifndef INSTRUMENTMANAGER_H
#define INSTRUMENTMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include "IConnection.h"
#include "FileRecorder.h"

enum class InstrumentType {
    VNA,         // 矢量网络分析仪
    SA,          // 频谱分析仪
    SG,          // 信号发生器
    Other
};

class InstrumentManager : public QObject
{
    Q_OBJECT
public:
    explicit InstrumentManager(const QString &logDir = "./logs",
                               QObject *parent = nullptr);

    // 注册一个仪器到管理器，后续可进行收发和监听
    void addInstrument(const QString &id,
                       InstrumentType type,
                       IConnection *connection);

    // 预留发送指令接口（目前不执行业务，仅记录日志并发送）
    void sendCommand(const QString &id, const QString &command);

    // 同步接收一次指令（阻塞），返回接收到的字符串
    QString receiveCommand(const QString &id, int timeoutMs = 3000);

    // 开启/停止持续监听模式（异步数据通过 instrumentDataReceived 信号发出）
    void startListening(const QString &id);
    void stopListening(const QString &id);

    // 获取文件记录器，供外部额外使用
    FileRecorder *recorder() { return &m_recorder; }

signals:
    // 异步数据到达（仅当开启监听时发射）
    void instrumentDataReceived(const QString &id, const QByteArray &data);

private:
    struct Instrument {
        InstrumentType type;
        IConnection *connection;
        bool listening;
    };
    QMap<QString, Instrument> m_instruments;
    FileRecorder m_recorder;

    // 处理来自IConnection的异步数据，仅在监听模式下转发并记录
    void onConnectionData(const QString &id, const QByteArray &data);
};

#endif
