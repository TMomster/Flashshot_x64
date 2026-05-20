#include "HotkeyManager.h"
#include <QDebug>
#include <QMetaObject>
#include <QCoreApplication>

#define MOD_ALT     0x0001
#define MOD_CTRL    0x0002
#define MOD_SHIFT   0x0004
#define MOD_WIN     0x0008

HotkeyManager* HotkeyManager::s_instance = nullptr;

HotkeyManager::HotkeyManager() {
    s_instance = this;
}

HotkeyManager::~HotkeyManager() {
    stopHook();
}

HotkeyManager& HotkeyManager::instance() {
    static HotkeyManager mgr;
    return mgr;
}

bool HotkeyManager::parseHotkey(const QString& str, DWORD& vk, DWORD& modMask) {
    QStringList parts = str.toLower().remove(' ').split('+');
    modMask = 0;
    QString mainKey;
    for (const QString& p : parts) {
        if (p == "ctrl" || p == "control") modMask |= MOD_CTRL;
        else if (p == "alt") modMask |= MOD_ALT;
        else if (p == "shift") modMask |= MOD_SHIFT;
        else if (p == "win" || p == "windows") modMask |= MOD_WIN;
        else mainKey = p;
    }
    if (mainKey.isEmpty()) return false;

    if (mainKey.length() == 1 && mainKey[0].isLetter()) {
        vk = toupper(mainKey[0].toLatin1());
        return true;
    }
    if (mainKey.length() == 1 && mainKey[0].isDigit()) {
        vk = mainKey[0].toLatin1();
        return true;
    }
    if (mainKey.startsWith('f') && mainKey.size() > 1) {
        int num = mainKey.mid(1).toInt();
        if (num >= 1 && num <= 24) {
            vk = 0x70 + num - 1;
            return true;
        }
    }
    static QHash<QString, DWORD> spec = {
        {"space", VK_SPACE}, {"return", VK_RETURN}, {"enter", VK_RETURN},
        {"tab", VK_TAB}, {"escape", VK_ESCAPE}, {"esc", VK_ESCAPE},
        {"backspace", VK_BACK}, {"delete", VK_DELETE}, {"insert", VK_INSERT},
        {"home", VK_HOME}, {"end", VK_END}, {"pageup", VK_PRIOR}, {"pagedown", VK_NEXT},
        {"up", VK_UP}, {"down", VK_DOWN}, {"left", VK_LEFT}, {"right", VK_RIGHT},
        {"prtsc", VK_SNAPSHOT}, {"printscreen", VK_SNAPSHOT}
    };
    if (spec.contains(mainKey)) {
        vk = spec[mainKey];
        return true;
    }
    return false;
}

bool HotkeyManager::registerHotkey(const QString& hotkeyStr, const QString& callbackId) {
    DWORD vk, modMask;
    if (!parseHotkey(hotkeyStr, vk, modMask)) {
        qWarning() << "Failed to parse hotkey:" << hotkeyStr;
        return false;
    }
    if (m_hotkeys.contains(vk)) {
        qWarning() << "Hotkey already registered for vk:" << vk;
        return false;
    }
    m_hotkeys[vk] = qMakePair(callbackId, modMask);
    qDebug() << "Registered hotkey:" << hotkeyStr << "->" << callbackId;
    return true;
}

bool HotkeyManager::updateHotkey(const QString& hotkeyStr, const QString& callbackId) {
    for (auto it = m_hotkeys.begin(); it != m_hotkeys.end(); ++it) {
        if (it->first == callbackId) {
            m_hotkeys.erase(it);
            break;
        }
    }
    return registerHotkey(hotkeyStr, callbackId);
}

void HotkeyManager::unregisterHotkey(const QString& callbackId) {
    for (auto it = m_hotkeys.begin(); it != m_hotkeys.end(); ++it) {
        if (it->first == callbackId) {
            m_hotkeys.erase(it);
            qDebug() << "Unregistered hotkey:" << callbackId;
            break;
        }
    }
}

LRESULT CALLBACK HotkeyManager::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && s_instance) {
        KBDLLHOOKSTRUCT* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        DWORD vk = kb->vkCode;
        bool keyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

        if (vk == VK_CONTROL) s_instance->m_modifierState = keyDown ? (s_instance->m_modifierState | MOD_CTRL) : (s_instance->m_modifierState & ~MOD_CTRL);
        else if (vk == VK_MENU) s_instance->m_modifierState = keyDown ? (s_instance->m_modifierState | MOD_ALT) : (s_instance->m_modifierState & ~MOD_ALT);
        else if (vk == VK_SHIFT) s_instance->m_modifierState = keyDown ? (s_instance->m_modifierState | MOD_SHIFT) : (s_instance->m_modifierState & ~MOD_SHIFT);
        else if (vk == VK_LWIN || vk == VK_RWIN) s_instance->m_modifierState = keyDown ? (s_instance->m_modifierState | MOD_WIN) : (s_instance->m_modifierState & ~MOD_WIN);

        if (keyDown && s_instance->m_hotkeys.contains(vk)) {
            auto& pair = s_instance->m_hotkeys[vk];
            DWORD requiredMod = pair.second;
            if ((s_instance->m_modifierState & requiredMod) == requiredMod) {
                QMetaObject::invokeMethod(s_instance, [pair](){
                    emit s_instance->hotkeyTriggered(pair.first);
                }, Qt::QueuedConnection);
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool HotkeyManager::startHook() {
    if (m_running) return true;
    m_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(nullptr), 0);
    if (!m_hook) {
        qCritical() << "Failed to install keyboard hook. Run as administrator?";
        return false;
    }
    m_running = true;
    qDebug() << "Keyboard hook installed.";
    return true;
}

void HotkeyManager::stopHook() {
    if (!m_running) return;
    if (m_hook) {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
    }
    m_running = false;
    qDebug() << "Keyboard hook stopped.";
}