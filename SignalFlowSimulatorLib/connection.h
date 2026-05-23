#ifndef CONNECTION_H
#define CONNECTION_H
#include <QString>

class Connection {
public:
    //连接关系类
    Connection() = default;
    Connection(const QString& fromModelId, const QString& fromPort, const QString& toModelId, const QString& toPort)
        : m_fromModelId(fromModelId), m_fromPort(fromPort), m_toModelId(toModelId), m_toPort(toPort) {}

    //源 模型Id
    QString fromModelId() const { return m_fromModelId; }
    //源 端口Id
    QString fromPort() const { return m_fromPort; }
    //目标 模型Id
    QString toModelId() const { return m_toModelId; }
    //目标 端口Id
    QString toPort() const { return m_toPort; }

private:
    QString m_fromModelId;
    QString m_fromPort;
    QString m_toModelId;
    QString m_toPort;
};

#endif // CONNECTION_H
