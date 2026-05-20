#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QObject>
#include <QHash>
#include <QKeySequence>
#include <windows.h>

// Windows 修饰键宏（确保使用正确名称）
#ifndef MOD_ALT
#define MOD_ALT 0x0001
#endif
#ifndef MOD_CONTROL
#define MOD_CONTROL 0x0002
#endif
#ifndef MOD_SHIFT
#define MOD_SHIFT 0x0004
#endif
#ifndef MOD_WIN
#define MOD_WIN 0x0008
#endif

class HotkeyManager : public QObject {
    Q_OBJECT
public:
    static HotkeyManager& instance();
    bool startHook();
    void stopHook();
    bool isRunning() const { return m_running; }

    bool registerHotkey(const QKeySequence& seq, const QString& callbackId);
    bool updateHotkey(const QKeySequence& seq, const QString& callbackId);
    void unregisterHotkey(const QString& callbackId);

    bool registerHotkey(const QString& hotkeyStr, const QString& callbackId);
    bool updateHotkey(const QString& hotkeyStr, const QString& callbackId);

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
    QHash<DWORD, QPair<QString, DWORD>> m_hotkeys; // vkCode -> (callbackId, requiredModMask)
    DWORD m_modifierState = 0;

    bool parseKeySequence(const QKeySequence& seq, DWORD& vk, DWORD& modMask);
    static DWORD qtKeyToWindowsVk(int qtKey);
    static DWORD qtModsToWindowsModMask(Qt::KeyboardModifiers mods);
};

#endif // HOTKEYMANAGER_H