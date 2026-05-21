#include "ConfigManager.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QProcess>
#include <QFile>
#include <QSettings>
#include <QDebug>
#include <QProcessEnvironment>
#include <QDir> 

ConfigManager& ConfigManager::instance() {
    static ConfigManager mgr;
    return mgr;
}

QString ConfigManager::dataDir() {
    return QDir::homePath() + "/MomsterTech/Flashshot_x64";
}

void ConfigManager::load() {
    QDir dir(dataDir());
    if (!dir.exists()) dir.mkpath(".");

    QString configPath = dataDir() + "/config.ini";
    m_settings = new QSettings(configPath, QSettings::IniFormat, this);

    m_hotkey = m_settings->value("hotkey", "F12").toString();
    m_replayHotkey = m_settings->value("replay_hotkey", "Ctrl+Shift+PageUp").toString();
    m_saveDir = m_settings->value("save_dir", QDir::homePath() + "/Pictures/Flashshot_x64").toString();
    m_quality = m_settings->value("quality", 2).toInt();
    m_replayEnabled = m_settings->value("replay_enabled", false).toBool();
    m_replayDuration = m_settings->value("replay_duration", 10).toInt();
    m_replayInterval = m_settings->value("replay_interval", 500).toInt();
    m_replayScale = m_settings->value("replay_scale", 50).toInt();
    m_autostart = m_settings->value("autostart", false).toBool();
    m_desktopShortcut = m_settings->value("desktop_shortcut", false).toBool();
    m_notificationsEnabled = m_settings->value("notifications_enabled", true).toBool();
    m_soundEnabled = m_settings->value("sound_enabled", true).toBool();
    m_notificationDuration = m_settings->value("notification_duration", 1000).toInt();

    QDir().mkpath(m_saveDir);
}

void ConfigManager::save() {
    if (!m_settings) return;
    m_settings->setValue("hotkey", m_hotkey);
    m_settings->setValue("replay_hotkey", m_replayHotkey);
    m_settings->setValue("save_dir", m_saveDir);
    m_settings->setValue("quality", m_quality);
    m_settings->setValue("replay_enabled", m_replayEnabled);
    m_settings->setValue("replay_duration", m_replayDuration);
    m_settings->setValue("replay_interval", m_replayInterval);
    m_settings->setValue("replay_scale", m_replayScale);
    m_settings->setValue("autostart", m_autostart);
    m_settings->setValue("desktop_shortcut", m_desktopShortcut);
    m_settings->setValue("notifications_enabled", m_notificationsEnabled);
    m_settings->setValue("sound_enabled", m_soundEnabled);
    m_settings->setValue("notification_duration", m_notificationDuration);
    m_settings->sync();
    emit configChanged();
}

void ConfigManager::applyAutostart(bool enable) {
    // 获取当前可执行文件的完整路径（自动适应任何安装位置）
    QString appPath = QCoreApplication::applicationFilePath();
    QString quotedPath = QString("\"%1\"").arg(appPath);
    const QString regKeyName = "Flashshot_x64";        // 注册表项名称
    const QString shortcutName = "Flashshot_x64.lnk";  // 快捷方式文件名

    // 方法1: 当前用户注册表 (HKCU)
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                  QSettings::NativeFormat);
    if (enable) {
        reg.setValue(regKeyName, quotedPath);
        // 验证是否写入成功
        if (reg.value(regKeyName).toString() != quotedPath) {
            qWarning() << "Failed to write autostart registry, trying startup folder";
            // 方法2: 启动文件夹 (备选)
            QString startupFolder = QProcessEnvironment::systemEnvironment().value("APPDATA")
                                    + "/Microsoft/Windows/Start Menu/Programs/Startup";
            QDir dir(startupFolder);
            if (!dir.exists()) dir.mkpath(".");
            QString shortcutPath = startupFolder + "/" + shortcutName;
            QFile::remove(shortcutPath);  // 先删除旧链接
            // 使用 PowerShell 创建快捷方式
            QString psScript = QString(
                "$WScriptShell = New-Object -ComObject WScript.Shell;"
                "$Shortcut = $WScriptShell.CreateShortcut('%1');"
                "$Shortcut.TargetPath = '%2';"
                "$Shortcut.Save();"
            ).arg(shortcutPath, appPath);
            QProcess::execute("powershell", {"-Command", psScript});
            if (QFile::exists(shortcutPath)) {
                qDebug() << "Autostart shortcut created in startup folder:" << shortcutPath;
            } else {
                qWarning() << "Failed to create autostart shortcut";
            }
        } else {
            qDebug() << "Autostart registry entry created successfully";
        }
    } else {
        // 禁用自启动：删除注册表项
        reg.remove(regKeyName);
        // 同时删除启动文件夹中的快捷方式
        QString startupFolder = QProcessEnvironment::systemEnvironment().value("APPDATA")
                                + "/Microsoft/Windows/Start Menu/Programs/Startup";
        QString shortcutPath = startupFolder + "/" + shortcutName;
        if (QFile::exists(shortcutPath)) {
            QFile::remove(shortcutPath);
        }
        qDebug() << "Autostart disabled";
    }
}

void ConfigManager::applyDesktopShortcut(bool enable) {
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString shortcutPath = desktopPath + "/Flashshot.lnk";
    if (enable) {
        QString target = QCoreApplication::applicationFilePath();
        QString psScript = QString(
            "$WScriptShell = New-Object -ComObject WScript.Shell;"
            "$Shortcut = $WScriptShell.CreateShortcut('%1');"
            "$Shortcut.TargetPath = '%2';"
            "$Shortcut.Save();"
        ).arg(shortcutPath, target);
        QProcess::execute("powershell", {"-Command", psScript});
    } else {
        QFile::remove(shortcutPath);
    }
}

void ConfigManager::setHotkey(const QString& key) {
    if (m_hotkey != key) { m_hotkey = key; save(); }
}
void ConfigManager::setReplayHotkey(const QString& key) {
    if (m_replayHotkey != key) { m_replayHotkey = key; save(); }
}
void ConfigManager::setSaveDir(const QString& dir) {
    if (m_saveDir != dir) { m_saveDir = dir; QDir().mkpath(dir); save(); }
}
void ConfigManager::setQuality(int q) {
    if (m_quality != q) { m_quality = q; save(); }
}
void ConfigManager::setReplayEnabled(bool en) {
    if (m_replayEnabled != en) { m_replayEnabled = en; save(); }
}
void ConfigManager::setReplayDuration(int sec) {
    if (m_replayDuration != sec) { m_replayDuration = sec; save(); }
}
void ConfigManager::setReplayInterval(int ms) {
    if (m_replayInterval != ms) { m_replayInterval = ms; save(); }
}
void ConfigManager::setReplayScale(int percent) {
    if (m_replayScale != percent) { m_replayScale = percent; save(); }
}
void ConfigManager::setAutostart(bool en) {
    if (m_autostart != en) {
        m_autostart = en;
        applyAutostart(en);
        save();
    }
}
void ConfigManager::setDesktopShortcut(bool en) {
    if (m_desktopShortcut != en) {
        m_desktopShortcut = en;
        applyDesktopShortcut(en);
        save();
    }
}
void ConfigManager::setNotificationsEnabled(bool en) {
    if (m_notificationsEnabled != en) { m_notificationsEnabled = en; save(); }
}
void ConfigManager::setSoundEnabled(bool en) {
    if (m_soundEnabled != en) { m_soundEnabled = en; save(); }
}
void ConfigManager::setNotificationDuration(int ms) {
    if (m_notificationDuration != ms) { m_notificationDuration = ms; save(); }
}