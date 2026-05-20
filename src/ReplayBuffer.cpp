#include "ReplayBuffer.h"
#include <QScreen>
#include <QGuiApplication>
#include <QBuffer>
#include <QImageWriter>
#include <QPixmap>
#include <QDateTime>
#include <QDebug>

ReplayBuffer& ReplayBuffer::instance() {
    static ReplayBuffer buf;
    return buf;
}

ReplayBuffer::ReplayBuffer() {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ReplayBuffer::captureFrame);
}

ReplayBuffer::~ReplayBuffer() {
    stop();
}

void ReplayBuffer::updateConfig(int maxDurationSec, int intervalMs, int scalePercent) {
    QMutexLocker locker(&m_mutex);
    m_maxDuration = maxDurationSec * 1000;
    m_interval = intervalMs;
    m_scalePercent = scalePercent;
    if (m_running) {
        m_timer->setInterval(m_interval);
    }
}

void ReplayBuffer::start() {
    if (m_running) return;
    m_running = true;
    m_timer->start(m_interval);
    qDebug() << "Replay buffer started";
}

void ReplayBuffer::stop() {
    if (!m_running) return;
    m_timer->stop();
    QMutexLocker locker(&m_mutex);
    m_buffer.clear();
    m_running = false;
    qDebug() << "Replay buffer stopped";
}

void ReplayBuffer::captureFrame() {
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    QPixmap pixmap = screen->grabWindow(0);
    if (pixmap.isNull()) return;

    if (m_scalePercent < 100) {
        QSize newSize = pixmap.size() * (m_scalePercent / 100.0);
        pixmap = pixmap.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QImage image = pixmap.toImage();
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG", 75);
    buffer.close();

    FrameData data;
    data.timestamp = QDateTime::currentMSecsSinceEpoch();
    data.compressed = ba;
    data.width = image.width();
    data.height = image.height();

    QMutexLocker locker(&m_mutex);
    m_buffer.append(data);
    qint64 cutoff = data.timestamp - m_maxDuration;
    while (!m_buffer.isEmpty() && m_buffer.first().timestamp < cutoff) {
        m_buffer.removeFirst();
    }
}

QVector<QImage> ReplayBuffer::getAllFrames() {
    QMutexLocker locker(&m_mutex);
    QVector<QImage> frames;
    for (const FrameData& fd : m_buffer) {
        QImage img;
        if (img.loadFromData(fd.compressed, "JPEG")) {
            frames.append(img);
        } else {
            qWarning() << "Failed to decompress frame";
        }
    }
    return frames;
}