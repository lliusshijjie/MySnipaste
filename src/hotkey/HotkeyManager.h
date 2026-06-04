#pragma once

#include <windows.h>

namespace mysnip::hotkey {

class HotkeyManager {
public:
    bool Register(HWND hwnd, int id, UINT modifiers, UINT vk);
    void Unregister(HWND hwnd, int id);

private:
    bool registered_ = false;
    int id_ = 0;
};

} // namespace mysnip::hotkey
