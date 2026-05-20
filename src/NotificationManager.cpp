#include "NotificationManager.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QStyle>
#include <QMediaDevices>
#include <QAudioOutput>
#include <QCoreApplication>
#include <QFile>
#include <windows.h>

NotificationManager& NotificationManager::instance() {
    static NotificationManager mgr;
    return mgr;
}

NotificationManager::NotificationManager() {
    m_window = new QWidget(nullptr, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    m_window->setAttribute(Qt::WA_TranslucentBackground);
    m_window->setStyleSheet(
        "QWidget { background-color: #1a1a1a; border-radius: 12px; border: 1px solid #3a3a3a; }"
        "QLabel { color: white; background: transparent; }"
    );
    QHBoxLayout* layout = new QHBoxLayout(m_window);
    layout->setContentsMargins(12, 10, 12, 10);
    QLabel* iconLabel = new QLabel();
    iconLabel->setFixedSize(32, 32);
    // 尝试加载图标
    QIcon icon = QIcon(":/resources/Flashshot.png");
    if (!icon.isNull()) iconLabel->setPixmap(icon.pixmap(32,32));
    QVBoxLayout* textLayout = new QVBoxLayout();
    QLabel* title = new QLabel("Flashshot");
    title->setStyleSheet("font-weight: bold; font-size: 13px;");
    QLabel* content = new QLabel();
    content->setWordWrap(true);
    content->setStyleSheet("font-size: 11px; color: #cccccc;");
    textLayout->addWidget(title);
    textLayout->addWidget(content);
    layout->addWidget(iconLabel);
    layout->addLayout(textLayout, 1);
    m_window->setLayout(layout);
    m_window->adjustSize();

    m_player = new QMediaPlayer(this);
    m_player->setAudioOutput(new QAudioOutput);
    m_player->audioOutput()->setVolume(0.5);
    // 尝试加载音效文件（资源文件或相对路径）
    QUrl soundUrl = QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/notify.wav");
    if (QFile::exists(soundUrl.toLocalFile())) {
        m_player->setSource(soundUrl);
    } else {
        // 使用系统默认提示音（可通过 MessageBoxBeep 实现）
    }

    connect(&m_hideTimer, &QTimer::timeout, m_window, &QWidget::hide);
}

NotificationManager::~NotificationManager() {
    delete m_window;
}

void NotificationManager::configure(bool enabled, int durationMs, bool soundEnabled) {
    m_enabled = enabled;
    m_duration = durationMs;
    m_sound = soundEnabled;
}

void NotificationManager::showMessage(const QString& text) {
    if (!m_enabled) return;
    // 更新内容
    QLabel* content = m_window->findChild<QLabel*>();
    if (content) content->setText(text);
    m_window->adjustSize();
    // 定位到右下角
    QRect screenGeo = QGuiApplication::primaryScreen()->availableGeometry();
    QPoint pos(screenGeo.right() - m_window->width() - 20,
               screenGeo.bottom() - m_window->height() - 20);
    m_window->move(pos);
    m_window->show();
    m_hideTimer.start(m_duration);

    if (m_sound) {
        if (m_player->source().isValid() && m_player->mediaStatus() != QMediaPlayer::NoMedia) {
            m_player->play();
        } else {
            MessageBeep(MB_ICONINFORMATION);
        }
    }
}