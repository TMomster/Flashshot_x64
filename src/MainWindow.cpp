#include "MainWindow.h"
#include "ConfigManager.h"
#include "HotkeyManager.h"
#include "ReplayBuffer.h"
#include "NotificationManager.h"
#include "ScreenshotTask.h"
#include "SetupWizard.h"
#include <QMenu>
#include <QAction>
#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>
#include <QThreadPool>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDebug>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupTray();
    applyConfig();
    connect(&HotkeyManager::instance(), &HotkeyManager::hotkeyTriggered,
            this, &MainWindow::onHotkeyTriggered);
    connect(&ConfigManager::instance(), &ConfigManager::configChanged,
            this, &MainWindow::applyConfig);
    NotificationManager::instance().showMessage("Flashshot 已启动");
}

MainWindow::~MainWindow() {
    HotkeyManager::instance().stopHook();
    ReplayBuffer::instance().stop();
}

void MainWindow::setupTray() {
    m_trayIcon = new QSystemTrayIcon(this);
    QIcon icon(":/resources/Flashshot.png");
    if (icon.isNull()) icon = QIcon::fromTheme("camera-photo");
    m_trayIcon->setIcon(icon);
    QMenu* menu = new QMenu(this);
    m_replayToggleAction = new QAction("回放功能: 关闭", this);
    connect(m_replayToggleAction, &QAction::triggered, this, &MainWindow::toggleReplay);
    menu->addAction(m_replayToggleAction);
    menu->addSeparator();
    QAction* openDirAct = new QAction("打开截图保存目录", this);
    connect(openDirAct, &QAction::triggered, this, &MainWindow::openSaveDir);
    menu->addAction(openDirAct);
    QAction* openLogAct = new QAction("打开日志目录", this);
    connect(openLogAct, &QAction::triggered, this, &MainWindow::openLogDir);
    menu->addAction(openLogAct);
    menu->addSeparator();
    QAction* wizardAct = new QAction("重新设置向导", this);
    connect(wizardAct, &QAction::triggered, this, &MainWindow::runWizard);
    menu->addAction(wizardAct);
    QAction* quitAct = new QAction("退出", this);
    connect(quitAct, &QAction::triggered, this, &MainWindow::quitApp);
    menu->addAction(quitAct);
    m_trayIcon->setContextMenu(menu);
    m_trayIcon->show();
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
}

void MainWindow::applyConfig() {
    auto& cfg = ConfigManager::instance();
    HotkeyManager::instance().updateHotkey(cfg.hotkey(), "screenshot");
    m_replayEnabled = cfg.replayEnabled();
    m_replayToggleAction->setText(m_replayEnabled ? "回放功能: 开启" : "回放功能: 关闭");
    if (m_replayEnabled) {
        ReplayBuffer::instance().updateConfig(cfg.replayDuration(), cfg.replayInterval(), cfg.replayScale());
        ReplayBuffer::instance().start();
        HotkeyManager::instance().updateHotkey(cfg.replayHotkey(), "replay");
    } else {
        ReplayBuffer::instance().stop();
        HotkeyManager::instance().unregisterHotkey("replay");
    }
    NotificationManager::instance().configure(cfg.notificationsEnabled(), cfg.notificationDuration(), cfg.soundEnabled());
}

void MainWindow::onHotkeyTriggered(const QString& id) {
    if (id == "screenshot") doScreenshot();
    else if (id == "replay") doReplayCapture();
}

void MainWindow::doScreenshot() {
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    QPixmap pixmap = screen->grabWindow(0);
    if (pixmap.isNull()) return;
    QImage image = pixmap.toImage();
    QString dir = ConfigManager::instance().saveDir();
    QDir().mkpath(dir);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    static QString lastTimestamp;
    static int seq = 1;
    if (timestamp == lastTimestamp) seq++;
    else { lastTimestamp = timestamp; seq = 1; }
    QString filename = QString("Flashshot_%1_%2.jpg").arg(timestamp).arg(seq);
    QString fullPath = dir + "/" + filename;
    int quality = 95;
    switch (ConfigManager::instance().quality()) {
        case 0: quality = 50; break;
        case 1: quality = 75; break;
        case 2: quality = 95; break;
    }
    ScreenshotTask* task = new ScreenshotTask(image, fullPath, quality);
    QThreadPool::globalInstance()->start(task);
    NotificationManager::instance().showMessage("截图已保存");
}

void MainWindow::doReplayCapture() {
    if (!m_replayEnabled) {
        NotificationManager::instance().showMessage("回放功能未启用");
        return;
    }
    QVector<QImage> frames = ReplayBuffer::instance().getAllFrames();
    if (frames.isEmpty()) {
        NotificationManager::instance().showMessage("回放缓冲区为空");
        return;
    }
    QString baseDir = ConfigManager::instance().saveDir();
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    int seq = 1;
    QString dir;
    do {
        dir = QString("%1/Backshot_%2_%3").arg(baseDir).arg(timestamp).arg(seq);
        seq++;
    } while (QDir(dir).exists());
    int quality = 95;
    switch (ConfigManager::instance().quality()) {
        case 0: quality = 50; break;
        case 1: quality = 75; break;
        case 2: quality = 95; break;
    }
    ReplaySaveTask* task = new ReplaySaveTask(frames, dir, quality);
    QThreadPool::globalInstance()->start(task);
    NotificationManager::instance().showMessage(QString("回放已保存 (%1 帧)").arg(frames.size()));
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick) {
        doScreenshot();
    }
}

void MainWindow::toggleReplay() {
    bool newState = !m_replayEnabled;
    ConfigManager::instance().setReplayEnabled(newState);
}

void MainWindow::openSaveDir() {
    QString dir = ConfigManager::instance().saveDir();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void MainWindow::openLogDir() {
    QString logDir = ConfigManager::dataDir() + "/logs";
    QDir().mkpath(logDir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(logDir));
}

void MainWindow::runWizard() {
    SetupWizard wizard(this);
    if (wizard.exec() == QDialog::Accepted) {
        applyConfig();  // 重新应用所有配置（热键、回放等）
        NotificationManager::instance().showMessage("配置已更新");
    }
}

void MainWindow::quitApp() {
    m_trayIcon->hide();
    qApp->quit();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    event->ignore();
    hide();
    NotificationManager::instance().showMessage("程序已最小化到托盘");
}