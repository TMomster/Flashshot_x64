#ifndef REPLAYBUFFER_H
#define REPLAYBUFFER_H

#include <QObject>
#include <QTimer>
#include <QImage>
#include <QVector>
#include <QMutex>
#include <QElapsedTimer>

struct FrameData {
    qint64 timestamp;  // ms
    QByteArray compressed; // JPEG 压缩数据
    int width, height;
};

class ReplayBuffer : public QObject {
    Q_OBJECT
public:
    static ReplayBuffer& instance();
    void start();
    void stop();
    void updateConfig(int maxDurationSec, int intervalMs, int scalePercent);
    QVector<QImage> getAllFrames(); // 解压后返回 QImage 列表

private slots:
    void captureFrame();

private:
    ReplayBuffer();
    ~ReplayBuffer();
    QTimer* m_timer = nullptr;
    QVector<FrameData> m_buffer;
    QMutex m_mutex;
    int m_maxDuration = 10000;   // ms
    int m_interval = 500;        // ms
    int m_scalePercent = 50;
    bool m_running = false;
};

#endif // REPLAYBUFFER_H