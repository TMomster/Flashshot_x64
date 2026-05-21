#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onHotkeyTriggered(const QString& id);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void toggleReplay();
    void toggleAutostart();                     // 新增：切换开机自启动
    void openSaveDir();
    void openLogDir();
    void runWizard();
    void exportLog();
    void quitApp();
    void showAboutDialog();

private:
    void setupTray();
    void applyConfig();
    void doScreenshot();
    void doReplayCapture();
    void updateTrayTooltip();

    QSystemTrayIcon* m_trayIcon = nullptr;
    QAction* m_replayToggleAction = nullptr;
    QAction* m_autostartToggleAction = nullptr; // 新增：开机自启动菜单项
    bool m_replayEnabled = false;
};

#endif // MAINWINDOW_H