#include "hotkey/HotkeyManager.h"

#include "utils/LogUtils.h"

namespace mysnip::hotkey {

bool HotkeyManager::Register(HWND hwnd, int id, UINT modifiers, UINT vk) {
    Unregister(hwnd, id);
    if (!RegisterHotKey(hwnd, id, modifiers, vk)) {
        utils::LogLastError(L"[Hotkey] RegisterHotKey");
        return false;
    }

    registered_ = true;
    id_ = id;
    return true;
}

void HotkeyManager::Unregister(HWND hwnd, int id) {
    if (!registered_) {
        return;
    }

    if (!UnregisterHotKey(hwnd, id_ == 0 ? id : id_)) {
        utils::LogLastError(L"[Hotkey] UnregisterHotKey");
    }

    registered_ = false;
    id_ = 0;
}

} // namespace mysnip::hotkey
