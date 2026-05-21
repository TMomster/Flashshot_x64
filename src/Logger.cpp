#include "Logger.h"
#include "ConfigManager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QTextStream>
#include <QCoreApplication>
#include <QDebug>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() {
    m_buffer.reserve(MAX_BUFFER_SIZE);
}

Logger::~Logger() {
    QMutexLocker locker(&m_mutex);
    m_buffer.clear();
}

void Logger::log(const QString& level, const QString& msg) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString formatted = QString("[%1] [%2] %3").arg(timestamp, level, msg);

    QMutexLocker locker(&m_mutex);
    m_buffer.append(formatted);
    while (m_buffer.size() > MAX_BUFFER_SIZE) {
        m_buffer.removeFirst();
    }
}

void Logger::flushToFile(const QString& reason) {
    QString logDir = ConfigManager::dataDir() + "/logs";
    QDir().mkpath(logDir);

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString fileName = QString("flashshot_log_%1.txt").arg(timestamp);
    if (!reason.isEmpty()) {
        fileName = QString("flashshot_log_%1_%2.txt").arg(timestamp, reason);
    }
    QString filePath = logDir + "/" + fileName;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open log file:" << filePath;
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "=== Flashshot Log ===\n";
    out << "Version: " << QCoreApplication::applicationVersion() << "\n";
    out << "Time: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
    if (!reason.isEmpty()) {
        out << "Reason: " << reason << "\n";
    }
    out << "\n";

    QMutexLocker locker(&m_mutex);
    for (const QString& line : m_buffer) {
        out << line << "\n";
    }
    out << "\n=== End of Log ===\n";
    file.close();

    qDebug() << "Log saved to:" << filePath;
}

QStringList Logger::getRecentLogs(int count) const {
    QMutexLocker locker(&m_mutex);
    if (count <= 0 || count >= m_buffer.size()) {
        return m_buffer;
    }
    return m_buffer.mid(m_buffer.size() - count);
}

void Logger::cleanOldLogs(int hours) {
    QString logDir = ConfigManager::dataDir() + "/logs";
    QDir dir(logDir);
    if (!dir.exists()) return;

    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-hours * 3600);
    QStringList filters;
    filters << "flashshot_log_*.txt";
    QFileInfoList entries = dir.entryInfoList(filters, QDir::Files);
    int removedCount = 0;
    for (const QFileInfo& fi : entries) {
        if (fi.lastModified() < cutoff) {
            if (QFile::remove(fi.absoluteFilePath())) {
                removedCount++;
                qDebug() << "Removed old log:" << fi.fileName();
            } else {
                qWarning() << "Failed to remove old log:" << fi.absoluteFilePath();
            }
        }
    }
    if (removedCount > 0) {
        qDebug() << "Cleaned up" << removedCount << "old log file(s) older than" << hours << "hours";
    }
}