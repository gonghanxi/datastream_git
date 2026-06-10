#ifndef FILERECORDER_H
#define FILERECORDER_H

#include <QString>
#include <QDir>
#include <QMutex>

class FileRecorder
{
public:
    explicit FileRecorder(const QString &storageDir = "./instrument_logs");

    // 记录一次指令交互（方向为SEND或RECEIVE）
    void record(const QString &instrumentId,
                const QString &direction,
                const QString &message);

    // 记录系统事件（连接、断线、重连等）
    void recordEvent(const QString &instrumentId,
                     const QString &event);

private:
    QDir m_dir;
    QMutex m_mutex; // 保证多线程写入文件安全
    QString fileNameForInstrument(const QString &instrumentId) const;
    void writeLine(const QString &fileName, const QString &line);
};

#endif
