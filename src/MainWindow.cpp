#include "MainWindow.h"
#include "ConfigManager.h"
#include "HotkeyManager.h"
#include "ReplayBuffer.h"
#include "NotificationManager.h"
#include "ScreenshotTask.h"
#include "SetupWizard.h"
#include "Logger.h"
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
#include <QClipboard>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // 窗口完全隐藏
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(1, 1);
    move(-1000, -1000);

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
    QIcon icon;
    QString iconPath = QCoreApplication::applicationDirPath() + "/resources/Flashshot.png";
    if (QFile::exists(iconPath)) {
        icon = QIcon(iconPath);
    } else {
        icon = QIcon::fromTheme("camera-photo");
    }
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip("Flashshot x64 运行中");  // 临时，稍后更新

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

    QAction* exportLogAct = new QAction("导出日志", this);
    connect(exportLogAct, &QAction::triggered, this, &MainWindow::exportLog);
    menu->addAction(exportLogAct);

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

    // 初始化完整的提示文本
    updateTrayTooltip();
}

void MainWindow::updateTrayTooltip() {
    auto& cfg = ConfigManager::instance();
    QString replayStatus = cfg.replayEnabled() ? "开" : "关";
    QString autostartStatus = cfg.autostart() ? "是" : "否";
    QString tooltip = QString("Flashshot x64 运行中\n即时回放：%1\n开机自启：%2")
                          .arg(replayStatus, autostartStatus);
    m_trayIcon->setToolTip(tooltip);
}

void MainWindow::exportLog() {
    Logger::instance().flushToFile("manual");
    NotificationManager::instance().showMessage("日志已导出到 " + ConfigManager::dataDir() + "/Logs");
}

void MainWindow::applyConfig() {
    auto& cfg = ConfigManager::instance();
    qDebug() << "Current save dir:" << cfg.saveDir();

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

    // 更新托盘提示文本
    updateTrayTooltip();
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

    // 复制到剪贴板（静默，无提示）
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setPixmap(pixmap);

    // 保存到文件
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
    QDir absoluteDir(dir);
    if (!absoluteDir.exists()) {
        absoluteDir.mkpath(".");
    }
    QUrl url = QUrl::fromLocalFile(absoluteDir.absolutePath());
    QDesktopServices::openUrl(url);
}

void MainWindow::openLogDir() {
    QString logDir = ConfigManager::dataDir() + "/logs";
    QDir().mkpath(logDir);
    QUrl url = QUrl::fromLocalFile(logDir);
    QDesktopServices::openUrl(url);
}

void MainWindow::runWizard() {
    SetupWizard wizard(this);
    if (wizard.exec() == QDialog::Accepted) {
        applyConfig();  // 重新应用配置（热键、回放等）
        NotificationManager::instance().showMessage("配置已更新");
    }
}

void MainWindow::quitApp() {
    m_trayIcon->hide();
    HotkeyManager::instance().stopHook();
    ReplayBuffer::instance().stop();
    QThreadPool::globalInstance()->waitForDone(1000);
    QApplication::quit();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    event->ignore();
    hide();
    NotificationManager::instance().showMessage("程序已最小化到托盘");
}