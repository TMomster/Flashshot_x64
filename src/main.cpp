#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QDir>
#include "MainWindow.h"
#include "ConfigManager.h"
#include "HotkeyManager.h"
#include "ReplayBuffer.h"

// 单例检测：使用 QLocalServer
static const QString APP_ID = "Flashshot_Singleton_2026";

bool tryAcquireSingleton() {
    QLocalSocket socket;
    socket.connectToServer(APP_ID);
    if (socket.waitForConnected(500)) {
        // 已有实例运行
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

    QString dataDir = ConfigManager::dataDir();
    QDir().mkpath(dataDir);
    QDir().mkpath(dataDir + "/logs");

    ConfigManager::instance().load();

    MainWindow w;
    w.showMinimized();

    QObject::connect(&app, &QApplication::aboutToQuit, [](){
        HotkeyManager::instance().stopHook();
        ReplayBuffer::instance().stop();
    });

    return app.exec();
}