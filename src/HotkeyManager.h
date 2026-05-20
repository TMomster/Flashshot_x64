#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QObject>
#include <QHash>
#include <windows.h>

class HotkeyManager : public QObject {
    Q_OBJECT
public:
    static HotkeyManager& instance();
    bool startHook();
    void stopHook();
    bool isRunning() const { return m_running; }

    bool registerHotkey(const QString& hotkeyStr, const QString& callbackId);
    bool updateHotkey(const QString& hotkeyStr, const QString& callbackId);
    void unregisterHotkey(const QString& callbackId);

signals:
    void hotkeyTriggered(const QString& callbackId);

private:
    HotkeyManager();
    ~HotkeyManager();
    HotkeyManager(const HotkeyManager&) = delete;
    HotkeyManager& operator=(const HotkeyManager&) = delete;

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HotkeyManager* s_instance;
    bool m_running = false;
    HHOOK m_hook = nullptr;
    QHash<DWORD, QPair<QString, DWORD>> m_hotkeys; 
    DWORD m_modifierState = 0;

    bool parseHotkey(const QString& str, DWORD& vk, DWORD& modMask);
};

#endif // HOTKEYMANAGER_H