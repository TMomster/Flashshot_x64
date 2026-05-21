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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextBrowser>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <windows.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
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
    NotificationManager::instance().showMessage("Flashshot x64 已启动");
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
    m_trayIcon->setToolTip("Flashshot x64 运行中");

    QMenu* menu = new QMenu(this);

    // 回放功能开关
    m_replayToggleAction = new QAction("回放功能: 关闭", this);
    connect(m_replayToggleAction, &QAction::triggered, this, &MainWindow::toggleReplay);
    menu->addAction(m_replayToggleAction);

    // 开机自启动开关
    auto& cfg = ConfigManager::instance();
    bool autostartEnabled = cfg.autostart();
    m_autostartToggleAction = new QAction(autostartEnabled ? "开机自启动: 开" : "开机自启动: 关", this);
    connect(m_autostartToggleAction, &QAction::triggered, this, &MainWindow::toggleAutostart);
    menu->addAction(m_autostartToggleAction);

    menu->addSeparator();

    // 打开截图保存目录
    QAction* openDirAct = new QAction("打开截图保存目录", this);
    connect(openDirAct, &QAction::triggered, this, &MainWindow::openSaveDir);
    menu->addAction(openDirAct);

    // 打开日志目录
    QAction* openLogAct = new QAction("打开日志目录", this);
    connect(openLogAct, &QAction::triggered, this, &MainWindow::openLogDir);
    menu->addAction(openLogAct);

    // 导出日志
    QAction* exportLogAct = new QAction("导出日志", this);
    connect(exportLogAct, &QAction::triggered, this, &MainWindow::exportLog);
    menu->addAction(exportLogAct);

    menu->addSeparator();

    // 重新设置向导
    QAction* wizardAct = new QAction("重新设置向导", this);
    connect(wizardAct, &QAction::triggered, this, &MainWindow::runWizard);
    menu->addAction(wizardAct);

    // 关于
    QAction* aboutAct = new QAction("关于", this);
    connect(aboutAct, &QAction::triggered, this, &MainWindow::showAboutDialog);
    menu->addAction(aboutAct);

    // 退出
    QAction* quitAct = new QAction("退出", this);
    connect(quitAct, &QAction::triggered, this, &MainWindow::quitApp);
    menu->addAction(quitAct);

    m_trayIcon->setContextMenu(menu);
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);

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
    NotificationManager::instance().showMessage("正在导出日志...");
    QFuture<void> future = QtConcurrent::run([](){
        Logger::instance().flushToFile("manual");
    });
    QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher](){
        NotificationManager::instance().showMessage("日志已导出到 " + ConfigManager::dataDir() + "/Logs");
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void MainWindow::applyConfig() {
    auto& cfg = ConfigManager::instance();
    qDebug() << "当前保存目录:" << cfg.saveDir();

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

    // 更新开机自启动菜单项文本（新增）
    bool autostartEnabled = cfg.autostart();
    m_autostartToggleAction->setText(autostartEnabled ? "开机自启动: 开" : "开机自启动: 关");

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

    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setPixmap(pixmap);

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
        applyConfig();
        NotificationManager::instance().showMessage("配置已更新");
    }
}

void MainWindow::quitApp() {
    m_trayIcon->hide();
    ::ExitProcess(0);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    event->ignore();
    hide();
    NotificationManager::instance().showMessage("程序已最小化到托盘");
}

void MainWindow::showAboutDialog() {
    QString version = QCoreApplication::applicationVersion();

    QDialog dialog(this);
    dialog.setWindowTitle(tr("关于 Flashshot x64"));
    dialog.setModal(true);
    dialog.resize(600, 500);

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    QTextBrowser* textBrowser = new QTextBrowser();
    textBrowser->setReadOnly(true);
    textBrowser->setFrameStyle(QFrame::NoFrame);
    textBrowser->setStyleSheet("QTextBrowser { background-color: palette(window); }");
    textBrowser->setOpenExternalLinks(true);

    QString htmlContent = QString(
        "<h3 style='text-align: center;'>Flashshot x64 %1</h3>"
        "<p style='text-align: center;'>版权所有 (C) 2026 Momster</p>"
        "<p style='text-align: center;'>截图工具，支持全局热键、回放缓冲区、自定义通知与剪贴板集成。</p>"
        "<br>"
        "<b>开源许可证</b><br>"
        "本软件（Flashshot64 特有代码）采用 <b>GNU General Public License 版本 3</b> 授权。<br>"
        "您可以在随附的 <tt>LICENSE</tt> 文件中查看完整条款。<br>"
        "<br>"
        "<b>第三方组件</b><br>"
        "本软件使用了 Qt 6 框架，采用 <b>GNU Lesser General Public License 版本 3</b> 授权。<br>"
        "Qt 是 The Qt Company 及其贡献者的注册商标。<br>"
        "Qt 库的源代码可以从以下地址获取：<br>"
        "<a href='https://download.qt.io/official_releases/qt/6.11/6.11.0/single/'>"
        "https://download.qt.io/official_releases/qt/6.11/6.11.0/single/</a><br>"
        "<br>"
        "<b>隐私说明</b><br>"
        "本软件不收集任何个人信息，所有数据仅保存在本地。<br>"
        "详见随附的 <tt>PRIVACY.md</tt> 文件。<br>"
        "<br>"
        "对于商业授权（闭源使用）或问题反馈，请联系作者。<br>"
        "联系方式见源代码文件头部的版权声明。"
    ).arg(version);

    textBrowser->setHtml(htmlContent);
    mainLayout->addWidget(textBrowser);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton(tr("确定"));
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    dialog.move(screenGeometry.center() - dialog.rect().center());

    dialog.exec();
}

void MainWindow::toggleAutostart() {
    bool current = ConfigManager::instance().autostart();
    ConfigManager::instance().setAutostart(!current);
}