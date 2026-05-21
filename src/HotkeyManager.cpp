#include "HotkeyManager.h"
#include <QDebug>
#include <QMetaObject>
#include <QCoreApplication>

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

DWORD HotkeyManager::qtKeyToWindowsVk(int qtKey) {
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) return qtKey;
    if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9) return qtKey;
    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F24) return 0x70 + (qtKey - Qt::Key_F1);
    static QHash<int, DWORD> specialMap = {
        {Qt::Key_Space, VK_SPACE}, {Qt::Key_Return, VK_RETURN}, {Qt::Key_Enter, VK_RETURN},
        {Qt::Key_Tab, VK_TAB}, {Qt::Key_Escape, VK_ESCAPE}, {Qt::Key_Backspace, VK_BACK},
        {Qt::Key_Delete, VK_DELETE}, {Qt::Key_Insert, VK_INSERT},
        {Qt::Key_Home, VK_HOME}, {Qt::Key_End, VK_END},
        {Qt::Key_PageUp, VK_PRIOR}, {Qt::Key_PageDown, VK_NEXT},
        {Qt::Key_Up, VK_UP}, {Qt::Key_Down, VK_DOWN}, {Qt::Key_Left, VK_LEFT}, {Qt::Key_Right, VK_RIGHT},
        {Qt::Key_Print, VK_SNAPSHOT}, {Qt::Key_ScrollLock, VK_SCROLL}, {Qt::Key_Pause, VK_PAUSE},
        {Qt::Key_CapsLock, VK_CAPITAL}, {Qt::Key_NumLock, VK_NUMLOCK},
        {Qt::Key_Menu, VK_APPS}, {Qt::Key_Help, VK_HELP},
        {Qt::Key_AsciiTilde, 0xC0}, {Qt::Key_Minus, 0xBD}, {Qt::Key_Equal, 0xBB},
        {Qt::Key_BracketLeft, 0xDB}, {Qt::Key_BracketRight, 0xDD}, {Qt::Key_Backslash, 0xDC},
        {Qt::Key_Semicolon, 0xBA}, {Qt::Key_Apostrophe, 0xDE}, {Qt::Key_Comma, 0xBC},
        {Qt::Key_Period, 0xBE}, {Qt::Key_Slash, 0xBF}
    };
    if (specialMap.contains(qtKey)) return specialMap[qtKey];
    return 0;
}

DWORD HotkeyManager::qtModsToWindowsModMask(Qt::KeyboardModifiers mods) {
    DWORD mask = 0;
    if (mods & Qt::ControlModifier) mask |= MOD_CONTROL;
    if (mods & Qt::AltModifier) mask |= MOD_ALT;
    if (mods & Qt::ShiftModifier) mask |= MOD_SHIFT;
    if (mods & Qt::MetaModifier) mask |= MOD_WIN;
    return mask;
}

bool HotkeyManager::parseKeySequence(const QKeySequence& seq, DWORD& vk, DWORD& modMask) {
    if (seq.isEmpty()) return false;
    int keyCombination = seq[0];
    int qtKey = keyCombination & ~Qt::KeyboardModifierMask;
    Qt::KeyboardModifiers mods = static_cast<Qt::KeyboardModifiers>(keyCombination & Qt::KeyboardModifierMask);
    vk = qtKeyToWindowsVk(qtKey);
    if (vk == 0) return false;
    modMask = qtModsToWindowsModMask(mods);
    return true;
}

bool HotkeyManager::parseMouseHotkey(const QString& hotkeyStr, int& buttonCode, DWORD& modMask) {
    QStringList parts = hotkeyStr.toLower().remove(' ').split('+');
    modMask = 0;
    QString mainKey;
    for (const QString& p : parts) {
        if (p == "ctrl") modMask |= MOD_CONTROL;
        else if (p == "alt") modMask |= MOD_ALT;
        else if (p == "shift") modMask |= MOD_SHIFT;
        else if (p == "win") modMask |= MOD_WIN;
        else mainKey = p;
    }
    if (mainKey.isEmpty()) return false;
    if (mainKey == "mouseleft") buttonCode = 0;
    else if (mainKey == "mouseright") buttonCode = 1;
    else if (mainKey == "mousemiddle") buttonCode = 2;
    else if (mainKey == "mousex1") buttonCode = 3;
    else if (mainKey == "mousex2") buttonCode = 4;
    else return false;
    return true;
}

bool HotkeyManager::registerHotkey(const QKeySequence& seq, const QString& callbackId) {
    DWORD vk, modMask;
    if (!parseKeySequence(seq, vk, modMask)) {
        qWarning() << "Failed to parse key sequence:" << seq.toString();
        return false;
    }
    if (m_hotkeys.contains(vk)) {
        qWarning() << "Hotkey already registered for vk:" << vk;
        return false;
    }
    m_hotkeys[vk] = qMakePair(callbackId, modMask);
    qDebug() << "Registered keyboard hotkey:" << seq.toString() << "->" << callbackId;
    return true;
}

bool HotkeyManager::registerHotkey(const QString& hotkeyStr, const QString& callbackId) {
    if (hotkeyStr.contains("Mouse", Qt::CaseInsensitive)) {
        int buttonCode;
        DWORD modMask;
        if (!parseMouseHotkey(hotkeyStr, buttonCode, modMask)) {
            qWarning() << "Failed to parse mouse hotkey:" << hotkeyStr;
            return false;
        }
        QPair<int, DWORD> key(buttonCode, modMask);
        if (m_mouseHotkeys.contains(key)) {
            qWarning() << "Mouse hotkey already registered:" << hotkeyStr;
            return false;
        }
        m_mouseHotkeys[key] = qMakePair(callbackId, modMask);
        qDebug() << "Registered mouse hotkey:" << hotkeyStr << "->" << callbackId;
        return true;
    } else {
        return registerHotkey(QKeySequence(hotkeyStr), callbackId);
    }
}

bool HotkeyManager::updateHotkey(const QKeySequence& seq, const QString& callbackId) {
    unregisterHotkey(callbackId);
    return registerHotkey(seq, callbackId);
}

bool HotkeyManager::updateHotkey(const QString& hotkeyStr, const QString& callbackId) {
    unregisterHotkey(callbackId);
    return registerHotkey(hotkeyStr, callbackId);
}

void HotkeyManager::unregisterHotkey(const QString& callbackId) {
    // 移除键盘热键
    for (auto it = m_hotkeys.begin(); it != m_hotkeys.end(); ++it) {
        if (it->first == callbackId) {
            m_hotkeys.erase(it);
            break;
        }
    }
    // 移除鼠标热键
    for (auto it = m_mouseHotkeys.begin(); it != m_mouseHotkeys.end(); ++it) {
        if (it->first == callbackId) {
            m_mouseHotkeys.erase(it);
            break;
        }
    }
    qDebug() << "Unregistered hotkey callback:" << callbackId;
}

LRESULT CALLBACK HotkeyManager::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && s_instance) {
        KBDLLHOOKSTRUCT* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        DWORD vk = kb->vkCode;
        bool keyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

        if (vk == VK_CONTROL)
            s_instance->m_modifierState = keyDown ? (s_instance->m_modifierState | MOD_CONTROL) : (s_instance->m_modifierState & ~MOD_CONTROL);
        else if (vk == VK_MENU)
            s_instance->m_modifierState = keyDown ? (s_instance->m_modifierState | MOD_ALT) : (s_instance->m_modifierState & ~MOD_ALT);
        else if (vk == VK_SHIFT)
            s_instance->m_modifierState = keyDown ? (s_instance->m_modifierState | MOD_SHIFT) : (s_instance->m_modifierState & ~MOD_SHIFT);
        else if (vk == VK_LWIN || vk == VK_RWIN)
            s_instance->m_modifierState = keyDown ? (s_instance->m_modifierState | MOD_WIN) : (s_instance->m_modifierState & ~MOD_WIN);

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

LRESULT CALLBACK HotkeyManager::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && s_instance) {
        MSLLHOOKSTRUCT* ms = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        int buttonCode = -1;
        // 仅处理按下事件
        if (wParam == WM_LBUTTONDOWN) buttonCode = 0;
        else if (wParam == WM_RBUTTONDOWN) buttonCode = 1;
        else if (wParam == WM_MBUTTONDOWN) buttonCode = 2;
        else if (wParam == WM_XBUTTONDOWN) {
            int xButton = HIWORD(ms->mouseData);
            if (xButton == XBUTTON1) buttonCode = 3;
            else if (xButton == XBUTTON2) buttonCode = 4;
        }
        if (buttonCode != -1) {
            DWORD curMod = s_instance->m_modifierState;
            QPair<int, DWORD> key(buttonCode, curMod);
            if (s_instance->m_mouseHotkeys.contains(key)) {
                auto& pair = s_instance->m_mouseHotkeys[key];
                DWORD requiredMod = pair.second;
                if ((curMod & requiredMod) == requiredMod) {
                    QMetaObject::invokeMethod(s_instance, [pair](){
                        emit s_instance->hotkeyTriggered(pair.first);
                    }, Qt::QueuedConnection);
                }
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool HotkeyManager::startHook() {
    if (m_running) return true;
    m_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(nullptr), 0);
    if (!m_hook) {
        qCritical() << "Failed to install keyboard hook.";
        return false;
    }
    m_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(nullptr), 0);
    if (!m_mouseHook) {
        qCritical() << "Failed to install mouse hook, but keyboard hook works.";
        // 鼠标钩子失败不影响主要功能，继续
    }
    m_running = true;
    qDebug() << "Keyboard and mouse hooks installed.";
    return true;
}

void HotkeyManager::stopHook() {
    if (!m_running) return;
    if (m_hook) {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
    }
    if (m_mouseHook) {
        UnhookWindowsHookEx(m_mouseHook);
        m_mouseHook = nullptr;
    }
    m_running = false;
    m_hotkeys.clear();
    m_mouseHotkeys.clear();
    qDebug() << "Hooks stopped.";
}