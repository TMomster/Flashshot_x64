#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QDir>
#include <signal.h>
#include <QThreadPool>
#include "MainWindow.h"
#include "ConfigManager.h"
#include "HotkeyManager.h"
#include "ReplayBuffer.h"
#include "Logger.h"
#include "NotificationManager.h"

static const QString APP_ID = "Flashshot_Singleton_2026";
static QLocalServer* g_server = nullptr;

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

// 检测是否已有实例在运行
bool isAlreadyRunning() {
    QLocalSocket socket;
    socket.connectToServer(APP_ID);
    return socket.waitForConnected(500);
}

// 向已运行的实例发送命令
void sendCommandToRunningInstance(const QString& cmd) {
    QLocalSocket socket;
    socket.connectToServer(APP_ID);
    if (socket.waitForConnected(500)) {
        socket.write(cmd.toUtf8());
        socket.flush();
        socket.waitForBytesWritten(500);
        socket.disconnectFromServer();
    }
}

// 创建本地服务器，用于接收后续启动实例的命令
void createServer() {
    g_server = new QLocalServer();
    QObject::connect(g_server, &QLocalServer::newConnection, [](){
        QLocalSocket* client = g_server->nextPendingConnection();
        if (client->waitForReadyRead(500)) {
            QByteArray data = client->readAll();
            if (data.startsWith("notify:")) {
                QString msg = QString::fromUtf8(data.mid(7));
                // 确保在主线程中调用通知
                QMetaObject::invokeMethod(qApp, [msg]() {
                    NotificationManager::instance().showMessage(msg);
                }, Qt::QueuedConnection);
            }
        }
        client->disconnectFromServer();
        delete client;
    });
    if (!g_server->listen(APP_ID)) {
        if (g_server->serverError() == QAbstractSocket::AddressInUseError) {
            QLocalServer::removeServer(APP_ID);
            g_server->listen(APP_ID);
        }
    }
}

int main(int argc, char *argv[]) {
    // 安装日志和信号处理器
    qInstallMessageHandler(messageHandler);
    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGFPE, signalHandler);
    signal(SIGILL, signalHandler);

    // 检测是否已有实例运行
    if (isAlreadyRunning()) {
        // 已有实例，发送通知命令并退出
        sendCommandToRunningInstance("notify:程序已在运行中");
        return 0;
    }

    // 正常启动程序
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setApplicationName("Flashshot");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("MomsterTech");

    Logger::instance().log("INFO", "Program started");

    // 创建数据目录
    QString dataDir = ConfigManager::dataDir();
    QDir().mkpath(dataDir);
    QDir().mkpath(dataDir + "/logs");
    Logger::cleanOldLogs(24);

    // 加载配置
    ConfigManager::instance().load();

    // 启动键盘钩子
    if (!HotkeyManager::instance().startHook()) {
        QMessageBox::critical(nullptr, "错误", "无法安装键盘钩子，请以管理员权限运行程序。");
        Logger::instance().log("ERROR", "Failed to install keyboard hook");
        return 1;
    }

    // 创建本地服务器，用于接收后续启动的命令
    createServer();

    // 创建主窗口（隐藏）
    MainWindow w;
    w.setWindowFlag(Qt::Window, false);
    w.setParent(nullptr);
    w.hide();

    // 退出时清理资源
    QObject::connect(&app, &QApplication::aboutToQuit, [](){
        Logger::instance().log("INFO", "Program exiting");
        Logger::instance().flushToFile("shutdown");
        HotkeyManager::instance().stopHook();
        ReplayBuffer::instance().stop();
        if (g_server) {
            g_server->close();
            delete g_server;
            g_server = nullptr;
        }
        QThreadPool::globalInstance()->waitForDone(1000);
    });

    int ret = app.exec();
    Logger::instance().flushToFile("normal_exit");
    return ret;
}