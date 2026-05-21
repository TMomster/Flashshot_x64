#include "ConfigManager.h"
#include "Logger.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QProcess>
#include <QFile>
#include <QSettings>
#include <QDebug>
#include <QProcessEnvironment>
#include <QDir>
#include <QFileInfo>

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
    Logger::instance().log("INFO", QString("applyAutostart 被调用，enable=%1").arg(enable));

    QString appPath = QCoreApplication::applicationFilePath();
    QString taskName = "Flashshot_x64";
    QString workingDir = QFileInfo(appPath).absolutePath();

    if (enable) {
        Logger::instance().log("INFO", "通过 PowerShell 创建计划任务（最高权限）...");
        
        // 使用 PowerShell 的 ScheduledTasks 模块创建任务，确保最高权限
        QString psScript = QString(
            "try { "
            "    $Action = New-ScheduledTaskAction -Execute '%1' -WorkingDirectory '%2'; "
            "    $Trigger = New-ScheduledTaskTrigger -AtLogOn -User '%3'; "
            "    $Settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -MultipleInstances IgnoreNew; "
            "    $Principal = New-ScheduledTaskPrincipal -UserId '%3' -LogonType Interactive -RunLevel Highest; "
            "    Register-ScheduledTask -TaskName '%4' -Action $Action -Trigger $Trigger -Settings $Settings -Principal $Principal -Force; "
            "    Write-Host 'SUCCESS'; "
            "} catch { "
            "    Write-Host $_.Exception.Message; "
            "}"
        ).arg(appPath, workingDir, QProcessEnvironment::systemEnvironment().value("USERNAME"), taskName);
        
        QProcess process;
        process.start("powershell", QStringList() << "-Command" << psScript);
        process.waitForFinished(10000);
        QString output = process.readAllStandardOutput();
        QString error = process.readAllStandardError();
        Logger::instance().log("INFO", "PowerShell 输出: " + output);
        if (!error.isEmpty()) {
            Logger::instance().log("ERROR", "PowerShell 错误: " + error);
        }

        // 检查任务是否创建成功
        QProcess check;
        check.start("schtasks", QStringList() << "/query" << "/tn" << taskName);
        check.waitForFinished(2000);
        if (check.exitCode() == 0) {
            Logger::instance().log("INFO", "计划任务创建成功（最高权限已设置）");
        } else {
            Logger::instance().log("ERROR", "计划任务创建失败");
        }

        // 备选：启动文件夹快捷方式（确保自启动）
        Logger::instance().log("INFO", "创建启动文件夹快捷方式作为备用...");
        QString startupFolder = QProcessEnvironment::systemEnvironment().value("APPDATA")
                                + "/Microsoft/Windows/Start Menu/Programs/Startup";
        QDir().mkpath(startupFolder);
        QString shortcutPath = startupFolder + "/Flashshot_x64.lnk";
        QString psShortcut = QString(
            "$WScriptShell = New-Object -ComObject WScript.Shell;"
            "$Shortcut = $WScriptShell.CreateShortcut('%1');"
            "$Shortcut.TargetPath = '%2';"
            "$Shortcut.WorkingDirectory = '%3';"
            "$Shortcut.Save();"
        ).arg(shortcutPath, appPath, workingDir);
        QProcess::execute("powershell", {"-Command", psShortcut});
        if (QFile::exists(shortcutPath)) {
            Logger::instance().log("INFO", "启动文件夹快捷方式已创建: " + shortcutPath);
        } else {
            Logger::instance().log("WARNING", "创建启动文件夹快捷方式失败");
        }

        // 清理旧注册表项
        QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      QSettings::NativeFormat);
        reg.remove("Flashshot_x64");
        Logger::instance().log("INFO", "开机自启动已启用（任务计划 + 快捷方式）");
    } else {
        // 禁用自启动：删除任务计划和启动文件夹快捷方式
        Logger::instance().log("INFO", "删除计划任务...");
        QProcess process;
        process.start("schtasks", QStringList() << "/delete" << "/tn" << taskName << "/f");
        process.waitForFinished(3000);
        QString output = process.readAllStandardOutput();
        QString error = process.readAllStandardError();
        Logger::instance().log("INFO", "schtasks 删除输出: " + output);
        if (!error.isEmpty()) {
            Logger::instance().log("ERROR", "schtasks 删除错误: " + error);
        }

        QString startupFolder = QProcessEnvironment::systemEnvironment().value("APPDATA")
                                + "/Microsoft/Windows/Start Menu/Programs/Startup";
        QString shortcutPath = startupFolder + "/Flashshot_x64.lnk";
        if (QFile::exists(shortcutPath)) {
            QFile::remove(shortcutPath);
            Logger::instance().log("INFO", "启动文件夹快捷方式已删除");
        }
        Logger::instance().log("INFO", "开机自启动已禁用");
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