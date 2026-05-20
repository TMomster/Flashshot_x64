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

// ½« Qt ¼üÖµ×ª»»Îª Windows ÐéÄâ¼üÂë£¨¼ò»¯°æ£¬ÒÆ³ýÐ¡¼üÅÌÓ³Éä£©
DWORD HotkeyManager::qtKeyToWindowsVk(int qtKey) {
    // ×ÖÄ¸ A-Z
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z)
        return qtKey; // 65-90
    // Êý×Ö 0-9£¨Ö÷¼üÅÌ£©
    if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
        return qtKey; // 48-57
    // ¹¦ÄÜ¼ü F1-F24
    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F24)
        return 0x70 + (qtKey - Qt::Key_F1);
    // µ¼º½¼ü¡¢±à¼­¼üµÈ
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
        // ·ûºÅ¼ü
        {Qt::Key_AsciiTilde, 0xC0}, {Qt::Key_Minus, 0xBD}, {Qt::Key_Equal, 0xBB},
        {Qt::Key_BracketLeft, 0xDB}, {Qt::Key_BracketRight, 0xDD}, {Qt::Key_Backslash, 0xDC},
        {Qt::Key_Semicolon, 0xBA}, {Qt::Key_Apostrophe, 0xDE}, {Qt::Key_Comma, 0xBC},
        {Qt::Key_Period, 0xBE}, {Qt::Key_Slash, 0xBF}
    };
    if (specialMap.contains(qtKey))
        return specialMap[qtKey];
    // ÆäËû¼ü£¨°üÀ¨Ð¡¼üÅÌµÈ£©·µ»Ø 0£¬²»Ö§³Ö
    return 0;
}

// ½« Qt ÐÞÊÎ¼ü×ª»»Îª Windows ÐÞÊÎÑÚÂë
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

bool HotkeyManager::registerHotkey(const QString& hotkeyStr, const QString& callbackId) {
    QKeySequence seq(hotkeyStr);
    return registerHotkey(seq, callbackId);
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
    qDebug() << "Registered hotkey:" << seq.toString() << "->" << callbackId;
    return true;
}

bool HotkeyManager::updateHotkey(const QKeySequence& seq, const QString& callbackId) {
    unregisterHotkey(callbackId);
    return registerHotkey(seq, callbackId);
}

bool HotkeyManager::updateHotkey(const QString& hotkeyStr, const QString& callbackId) {
    return updateHotkey(QKeySequence(hotkeyStr), callbackId);
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