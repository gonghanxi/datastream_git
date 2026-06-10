#include "InstrumentManager.h"
#include <QDebug>

InstrumentManager::InstrumentManager(const QString &logDir, QObject *parent)
    : QObject(parent), m_recorder(logDir) {}

void InstrumentManager::addInstrument(const QString &id,
                                      InstrumentType type,
                                      IConnection *connection)
{
    if (!connection) return;
    Instrument inst;
    inst.type = type;
    inst.connection = connection;
    inst.listening = false;
    m_instruments.insert(id, inst);

    // 连接数据信号，用于记录
    connect(connection, &IConnection::dataReceived, this,
            [this, id](const QByteArray &data) {
                onConnectionData(id, data);
            });

    // 记录日志：设备注册成功
    m_recorder.recordEvent(id, QString("Instrument registered, type %1")
                                  .arg(static_cast<int>(type)));
}

void InstrumentManager::sendCommand(const QString &id, const QString &command)
{
    if (!m_instruments.contains(id)) {
        qWarning() << "Instrument" << id << "not found";
        return;
    }
    IConnection *conn = m_instruments[id].connection;
    QByteArray data = command.toUtf8();
    // 实际发送
    conn->send(data);
    m_recorder.record(id, "SEND", command); // 记录发送日志
}

QString InstrumentManager::receiveCommand(const QString &id, int timeoutMs)
{
    if (!m_instruments.contains(id)) {
        qWarning() << "Instrument" << id << "not found";
        return {};
    }
    IConnection *conn = m_instruments[id].connection;
    // 离线时拒绝接收并记录
    if (!conn->isOnline()) {
        m_recorder.recordEvent(id, "Receive failed: connection offline");
        return {};
    }
    QByteArray raw = conn->receive(timeoutMs);// 阻塞接收
    QString resp = QString::fromUtf8(raw);
    m_recorder.record(id, "RECEIVE", resp);   // 记录接收日志
    return resp;
}

void InstrumentManager::startListening(const QString &id)
{
    if (!m_instruments.contains(id)) return;
    m_instruments[id].listening = true;
    // 异步接收由 dataReceived 信号驱动，这里无需额外操作
}

void InstrumentManager::stopListening(const QString &id)
{
    if (!m_instruments.contains(id)) return;
    m_instruments[id].listening = false;
}

void InstrumentManager::onConnectionData(const QString &id, const QByteArray &data)
{
    // 如果处于监听状态，则将数据通过信号发出，并记录
    if (m_instruments.value(id).listening) {
        emit instrumentDataReceived(id, data);
        m_recorder.record(id, "ASYNC_RECEIVE", QString::fromUtf8(data));
    }
}
