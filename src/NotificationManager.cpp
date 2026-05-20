#include "NotificationManager.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>
#include <QIcon>
#include <QApplication>
#include <QStyle>
#include <QMediaDevices>
#include <QAudioOutput>
#include <QCoreApplication>
#include <QFile>
#include <QUrl>
#include <windows.h>

NotificationManager& NotificationManager::instance() {
    static NotificationManager mgr;
    return mgr;
}

NotificationManager::NotificationManager() {
    m_window = new QWidget(nullptr, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    m_window->setAttribute(Qt::WA_TranslucentBackground);
    m_window->setStyleSheet(
        "QWidget { background-color: #000000; border-radius: 12px; border: 1px solid #333333; }"
        "QLabel { color: white; background: transparent; }"
    );

    QHBoxLayout* layout = new QHBoxLayout(m_window);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(10);

    // 左侧图标：从 exe 同级目录下的 resources/Flashshot.png 加载
    QLabel* iconLabel = new QLabel();
    iconLabel->setFixedSize(32, 32);
    QString iconPath = QCoreApplication::applicationDirPath() + "/resources/Flashshot.png";
    QPixmap iconPixmap(iconPath);
    if (!iconPixmap.isNull()) {
        iconLabel->setPixmap(iconPixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        iconLabel->hide();
    }

    // 右侧文字区域
    QVBoxLayout* textLayout = new QVBoxLayout();
    QLabel* title = new QLabel("Flashshot");
    title->setStyleSheet("font-weight: bold; font-size: 13px; color: white;");
    QLabel* content = new QLabel();
    content->setWordWrap(true);
    content->setStyleSheet("font-size: 11px; color: #cccccc;");
    textLayout->addWidget(title);
    textLayout->addWidget(content);
    layout->addWidget(iconLabel);
    layout->addLayout(textLayout, 1);

    m_window->setLayout(layout);
    m_window->adjustSize();

    // 初始化音效：从 exe 同级目录下的 resources/Flashshot_noti.wav 加载
    m_player = new QMediaPlayer(this);
    m_player->setAudioOutput(new QAudioOutput);
    m_player->audioOutput()->setVolume(0.8);
    QString soundPath = QCoreApplication::applicationDirPath() + "/resources/Flashshot_noti.wav";
    if (QFile::exists(soundPath)) {
        m_player->setSource(QUrl::fromLocalFile(soundPath));
        m_soundAvailable = true;
    } else {
        m_soundAvailable = false;
    }

    connect(&m_hideTimer, &QTimer::timeout, m_window, &QWidget::hide);
}

NotificationManager::~NotificationManager() {
    delete m_window;
    delete m_player;
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
        playSound();
    }
}

void NotificationManager::playSound() {
    if (m_soundAvailable && m_player->source().isValid()) {
        m_player->play();
    } else {
        MessageBeep(MB_ICONINFORMATION);
    }
}