#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QStringList>
#include <QMutex>
#include <QDateTime>

class Logger : public QObject {
    Q_OBJECT
public:
    static Logger& instance();

    void log(const QString& level, const QString& msg);
    void flushToFile(const QString& reason = QString());
    QStringList getRecentLogs(int count = 500) const;

    // 清理旧日志文件（保留最近 hours 小时内的文件，默认24小时）
    static void cleanOldLogs(int hours = 24);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    QStringList m_buffer;
    mutable QMutex m_mutex;
    static const int MAX_BUFFER_SIZE = 5000;
};

#endif // LOGGER_H