#include "ScreenshotTask.h"
#include <QDir>
#include <QImageWriter>
#include <QDebug>

ScreenshotTask::ScreenshotTask(const QImage& img, const QString& path, int quality)
    : m_image(img), m_path(path), m_quality(quality) {}

void ScreenshotTask::run() {
    QImageWriter writer(m_path);
    writer.setQuality(m_quality);
    if (!writer.write(m_image)) {
        qWarning() << "Failed to save screenshot:" << m_path << writer.errorString();
    } else {
        qDebug() << "Saved screenshot:" << m_path;
    }
}

ReplaySaveTask::ReplaySaveTask(const QVector<QImage>& frames, const QString& dir, int quality)
    : m_frames(frames), m_dir(dir), m_quality(quality) {}

void ReplaySaveTask::run() {
    QDir().mkpath(m_dir);
    int idx = 1;
    for (const QImage& img : m_frames) {
        QString path = QString("%1/Backshot_%2.jpg").arg(m_dir).arg(idx++, 3, 10, QChar('0'));
        QImageWriter writer(path);
        writer.setQuality(m_quality);
        if (!writer.write(img)) {
            qWarning() << "Failed to save replay frame:" << path << writer.errorString();
        }
    }
    qDebug() << "Saved replay with" << m_frames.size() << "frames to" << m_dir;
}