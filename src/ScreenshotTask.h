#ifndef SCREENSHOTASK_H
#define SCREENSHOTASK_H

#include <QRunnable>
#include <QImage>
#include <QString>

class ScreenshotTask : public QRunnable {
public:
    ScreenshotTask(const QImage& img, const QString& path, int quality);
    void run() override;

private:
    QImage m_image;
    QString m_path;
    int m_quality;
};

class ReplaySaveTask : public QRunnable {
public:
    ReplaySaveTask(const QVector<QImage>& frames, const QString& dir, int quality);
    void run() override;

private:
    QVector<QImage> m_frames;
    QString m_dir;
    int m_quality;
};

#endif // SCREENSHOTASK_H