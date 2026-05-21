#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QObject>
#include <QHash>
#include <QKeySequence>
#include <windows.h>

// Windows 修饰键宏
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

// 鼠标消息常量（如果未定义）
#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x020B
#endif
#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP   0x020C
#endif

#define XBUTTON1 0x0001
#define XBUTTON2 0x0002

class HotkeyManager : public QObject {
    Q_OBJECT
public:
    static HotkeyManager& instance();
    bool startHook();
    void stopHook();
    bool isRunning() const { return m_running; }

    // 统一的注册/更新/移除接口（自动识别键盘或鼠标热键）
    bool registerHotkey(const QKeySequence& seq, const QString& callbackId);
    bool updateHotkey(const QKeySequence& seq, const QString& callbackId);
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
    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HotkeyManager* s_instance;

    bool m_running = false;
    HHOOK m_hook = nullptr;          // 键盘钩子
    HHOOK m_mouseHook = nullptr;     // 鼠标钩子
    DWORD m_modifierState = 0;       // 当前按下的修饰键 (MOD_* 组合)

    // 键盘热键映射: vkCode -> (callbackId, requiredModMask)
    QHash<DWORD, QPair<QString, DWORD>> m_hotkeys;
    // 鼠标热键映射: (buttonCode, requiredModMask) -> callbackId
    // buttonCode: 0=左键,1=右键,2=中键,3=XBUTTON1(侧键后退),4=XBUTTON2(侧键前进)
    QHash<QPair<int, DWORD>, QPair<QString, DWORD>> m_mouseHotkeys;

    bool parseKeySequence(const QKeySequence& seq, DWORD& vk, DWORD& modMask);
    bool parseMouseHotkey(const QString& hotkeyStr, int& buttonCode, DWORD& modMask);
    static DWORD qtKeyToWindowsVk(int qtKey);
    static DWORD qtModsToWindowsModMask(Qt::KeyboardModifiers mods);
};

#endif // HOTKEYMANAGER_H