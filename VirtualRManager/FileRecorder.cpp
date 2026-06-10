#include "FileRecorder.h"
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDebug>

FileRecorder::FileRecorder(const QString &storageDir)
{
    m_dir = QDir(storageDir);
    if (!m_dir.exists()) {
        m_dir.mkpath("."); // 若目录不存在则创建
    }
}

void FileRecorder::record(const QString &instrumentId,
                          const QString &direction,
                          const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString line = QString("[%1] [%2] %3")
                   .arg(timestamp, direction, message);
    writeLine(fileNameForInstrument(instrumentId), line);
}

void FileRecorder::recordEvent(const QString &instrumentId,
                               const QString &event)
{
    record(instrumentId, "EVENT", event);
}

QString FileRecorder::fileNameForInstrument(const QString &instrumentId) const
{
    // 每天一个文件，按仪器ID和日期命名
    QString date = QDateTime::currentDateTime().toString("yyyyMMdd");
    return QString("%1_%2.txt").arg(instrumentId, date);
}

void FileRecorder::writeLine(const QString &fileName, const QString &line)
{
    QMutexLocker locker(&m_mutex); // 保证线程安全
    QFile file(m_dir.filePath(fileName));
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << line << "\n";
        file.close();
    } else {
        qWarning() << "FileRecorder: cannot open file" << fileName;
    }
}
