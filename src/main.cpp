#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QDir>
#include <QThreadPool>  // 新增
#include <signal.h>
#include "MainWindow.h"
#include "ConfigManager.h"
#include "HotkeyManager.h"
#include "ReplayBuffer.h"
#include "Logger.h"

static const QString APP_ID = "Flashshot_Singleton_2026";

// 信号处理函数（用于捕获崩溃）
void signalHandler(int sig) {
    Logger::instance().log("FATAL", QString("Signal received: %1").arg(sig));
    Logger::instance().flushToFile(QString("signal_%1").arg(sig));
    signal(sig, SIG_DFL);
    raise(sig);
}

// 全局消息处理器（Qt 日志）
void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QString level;
    switch (type) {
        case QtDebugMsg: level = "DEBUG"; break;
        case QtInfoMsg:  level = "INFO"; break;
        case QtWarningMsg: level = "WARNING"; break;
        case QtCriticalMsg: level = "CRITICAL"; break;
        case QtFatalMsg: level = "FATAL"; break;
    }
    Logger::instance().log(level, msg);
    if (type == QtFatalMsg) {
        Logger::instance().flushToFile("fatal");
        abort();
    }
}

bool tryAcquireSingleton() {
    QLocalSocket socket;
    socket.connectToServer(APP_ID);
    if (socket.waitForConnected(500)) {
        socket.close();
        return false;
    }
    QLocalServer *server = new QLocalServer();
    QObject::connect(server, &QLocalServer::newConnection, [server](){
        QLocalSocket *client = server->nextPendingConnection();
        client->write("already_running");
        client->flush();
        client->disconnectFromServer();
        delete client;
    });
    if (!server->listen(APP_ID)) {
        if (server->serverError() == QAbstractSocket::AddressInUseError) {
            QLocalServer::removeServer(APP_ID);
            server->listen(APP_ID);
        } else {
            delete server;
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[]) {
    // 安装日志消息处理器
    qInstallMessageHandler(messageHandler);

    // 安装信号处理器 (用于捕获崩溃)
    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGFPE, signalHandler);
    signal(SIGILL, signalHandler);

    if (!tryAcquireSingleton()) {
        QApplication app(argc, argv);
        QSystemTrayIcon tray;
        QIcon icon = QIcon::fromTheme("camera-photo");
        if (icon.isNull()) icon = QIcon(":/resources/Flashshot.png");
        tray.setIcon(icon);
        tray.show();
        tray.showMessage("Flashshot", "程序已在运行中", QSystemTrayIcon::Information, 1500);
        QTimer::singleShot(2000, &app, &QApplication::quit);
        return app.exec();
    }

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setApplicationName("Flashshot");
    app.setApplicationVersion("2.4.1");
    app.setOrganizationName("MomsterTech");

    // 设置应用程序图标（运行时窗口图标，可选）
    QIcon appIcon(":/resources/Flashshot.ico");
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }

    Logger::instance().log("INFO", "Program started");

    // 创建数据目录
    QString dataDir = ConfigManager::dataDir();
    QDir().mkpath(dataDir);
    QDir().mkpath(dataDir + "/logs");

    // 清理超过24小时的旧日志文件
    Logger::cleanOldLogs(24);

    // 加载配置
    ConfigManager::instance().load();

    // 启动键盘钩子
    if (!HotkeyManager::instance().startHook()) {
        QMessageBox::critical(nullptr, "错误", "无法安装键盘钩子，请以管理员权限运行程序。");
        Logger::instance().log("ERROR", "Failed to install keyboard hook");
        return 1;
    }

    MainWindow w;
    w.setWindowFlag(Qt::Window, false);
    w.setParent(nullptr);
    w.hide();

    QObject::connect(&app, &QApplication::aboutToQuit, [](){
        Logger::instance().log("INFO", "Program exiting");
        Logger::instance().flushToFile("shutdown");
        HotkeyManager::instance().stopHook();
        ReplayBuffer::instance().stop();
        QThreadPool::globalInstance()->waitForDone(1000);
    });

    int ret = app.exec();

    Logger::instance().flushToFile("normal_exit");

    return ret;
}