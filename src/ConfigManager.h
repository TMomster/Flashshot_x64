#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QSettings>
#include <QDir>

class ConfigManager : public QObject {
    Q_OBJECT
public:
    static ConfigManager& instance();

    // 获取数据目录（%USERPROFILE%/MomsterTech/Flashshot_x64）
    static QString dataDir();

    void load();
    void save();

    // Getter
    QString hotkey() const { return m_hotkey; }
    QString replayHotkey() const { return m_replayHotkey; }
    QString saveDir() const { return m_saveDir; }
    int quality() const { return m_quality; }   // 0=低,1=中,2=高
    bool replayEnabled() const { return m_replayEnabled; }
    int replayDuration() const { return m_replayDuration; }
    int replayInterval() const { return m_replayInterval; }
    int replayScale() const { return m_replayScale; }
    bool autostart() const { return m_autostart; }
    bool desktopShortcut() const { return m_desktopShortcut; }
    bool notificationsEnabled() const { return m_notificationsEnabled; }
    bool soundEnabled() const { return m_soundEnabled; }
    int notificationDuration() const { return m_notificationDuration; }

    // Setter (会调用 save 并执行实际动作)
    void setHotkey(const QString& key);
    void setReplayHotkey(const QString& key);
    void setSaveDir(const QString& dir);
    void setQuality(int q);
    void setReplayEnabled(bool en);
    void setReplayDuration(int sec);
    void setReplayInterval(int ms);
    void setReplayScale(int percent);
    void setAutostart(bool en);
    void setDesktopShortcut(bool en);
    void setNotificationsEnabled(bool en);
    void setSoundEnabled(bool en);
    void setNotificationDuration(int ms);

signals:
    void configChanged();

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // 实际执行自启动/快捷方式操作的静态方法
    static void applyAutostart(bool enable);
    static void applyDesktopShortcut(bool enable);

    QSettings* m_settings = nullptr;
    QString m_hotkey = "F12";
    QString m_replayHotkey = "Ctrl+Shift+PageUp";
    QString m_saveDir = QDir::homePath() + "/Pictures/Flashshot_x64";
    int m_quality = 2;
    bool m_replayEnabled = false;
    int m_replayDuration = 10;
    int m_replayInterval = 500;
    int m_replayScale = 50;
    bool m_autostart = false;
    bool m_desktopShortcut = false;
    bool m_notificationsEnabled = true;
    bool m_soundEnabled = true;
    int m_notificationDuration = 1000;
};

#endif // CONFIGMANAGER_H