#pragma once

#include <string_view>
#include <windows.h>
#include <shellapi.h>

namespace mysnip::tray {

class TrayIcon {
public:
    bool Add(HWND hwnd, UINT callbackMessage);
    void Remove();
    void ShowMenu(HWND hwnd);
    void ShowBalloon(std::wstring_view title, std::wstring_view message);

private:
    NOTIFYICONDATAW nid_{};
    bool added_ = false;
};

} // namespace mysnip::tray
