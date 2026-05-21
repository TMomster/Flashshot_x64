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
    // 移除透明背景，确保背景纯黑
    // m_window->setAttribute(Qt::WA_TranslucentBackground);
    m_window->setAutoFillBackground(true);
    m_window->setStyleSheet(
        "QWidget { background-color: #1e1e1e; border-radius: 12px; border: 1px solid #333333; }"
        "QLabel { color: white; background: transparent; border: none; text-shadow: none; }"
    );

    // 主布局：水平布局，左侧图标，右侧文本区域
    QHBoxLayout* mainLayout = new QHBoxLayout(m_window);
    mainLayout->setContentsMargins(12, 10, 12, 10);
    mainLayout->setSpacing(10);

    // 左侧图标：支持 PNG 和 ICO 格式
    QLabel* iconLabel = new QLabel();
    iconLabel->setFixedSize(32, 32);
    QPixmap iconPixmap;
    QStringList possiblePaths = {
        QCoreApplication::applicationDirPath() + "/resources/Flashshot.png",
        QCoreApplication::applicationDirPath() + "/resources/Flashshot.ico",
        QCoreApplication::applicationDirPath() + "/Flashshot.png",
        QCoreApplication::applicationDirPath() + "/Flashshot.ico"
    };
    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            if (iconPixmap.load(path)) {
                break;
            }
        }
    }
    if (!iconPixmap.isNull()) {
        iconLabel->setPixmap(iconPixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        // 占位灰色图标
        QPixmap placeholder(32, 32);
        placeholder.fill(QColor(80, 80, 80));
        iconLabel->setPixmap(placeholder);
    }
    mainLayout->addWidget(iconLabel);

    // 右侧文本区域：垂直布局，包含标题和内容
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(4);
    textLayout->setContentsMargins(0, 0, 0, 0);

    // 标题：加粗，字体稍大
    QLabel* titleLabel = new QLabel("Flashshot");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: #ffffff; border: none; text-shadow: none;");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    textLayout->addWidget(titleLabel);

    // 内容：普通字体，支持自动换行
    QLabel* contentLabel = new QLabel();
    contentLabel->setWordWrap(true);
    contentLabel->setStyleSheet("font-size: 11px; color: #cccccc; border: none; text-shadow: none;");
    contentLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    textLayout->addWidget(contentLabel);

    mainLayout->addLayout(textLayout, 1);

    m_window->setLayout(mainLayout);
    m_window->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    m_window->adjustSize();

    // 保存内容标签指针
    m_contentLabel = contentLabel;

    // 初始化音效
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
    if (m_contentLabel) {
        m_contentLabel->setText(text);
    }
    // 强制更新布局
    m_window->layout()->activate();
    m_window->adjustSize();
    m_window->adjustSize(); // 二次确保

    QRect screenGeo = QGuiApplication::primaryScreen()->availableGeometry();
    QPoint pos(screenGeo.right() - m_window->width() - 20,
               screenGeo.bottom() - m_window->height() - 20);
    m_window->move(pos);
    m_window->show();
    m_hideTimer.start(m_duration);

    if (m_sound) {
        if (m_soundAvailable && m_player->source().isValid()) {
            m_player->play();
        } else {
            MessageBeep(MB_ICONINFORMATION);
        }
    }
}

void NotificationManager::playSound() {
    if (m_soundAvailable && m_player->source().isValid()) {
        m_player->play();
    } else {
        MessageBeep(MB_ICONINFORMATION);
    }
}