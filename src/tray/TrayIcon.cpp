#include "tray/TrayIcon.h"

#include "app/CommandIds.h"
#include "app/AppResources.h"
#include "tray/TrayMenuModel.h"
#include "tray/TrayNotificationPolicy.h"
#include "utils/LogUtils.h"

#include <cwchar>

namespace mysnip::tray {
namespace {

constexpr UINT kTrayIconId = 1;

void CopyToNotifyBuffer(wchar_t* dst, std::size_t dstCount, std::wstring_view value) {
    const auto count = (std::min)(dstCount - 1, value.size());
    std::wmemcpy(dst, value.data(), count);
    dst[count] = L'\0';
}

HICON LoadAppIcon(HINSTANCE instance, int width, int height) {
    HICON icon = reinterpret_cast<HICON>(LoadImageW(
        instance,
        MAKEINTRESOURCEW(IDI_APP_ICON),
        IMAGE_ICON,
        width,
        height,
        LR_DEFAULTCOLOR | LR_SHARED));
    if (!icon) {
        icon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    }
    if (!icon) {
        icon = LoadIconW(nullptr, IDI_APPLICATION);
    }
    return icon;
}

} // namespace

bool TrayIcon::Add(HWND hwnd, UINT callbackMessage) {
    nid_ = {};
    nid_.cbSize = sizeof(nid_);
    nid_.hWnd = hwnd;
    nid_.uID = kTrayIconId;
    nid_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid_.uCallbackMessage = callbackMessage;
    const auto instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
    nid_.hIcon = LoadAppIcon(instance, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    CopyToNotifyBuffer(nid_.szTip, std::size(nid_.szTip), L"MySnipaste");

    if (!Shell_NotifyIconW(NIM_ADD, &nid_)) {
        utils::LogLastError(L"[Tray] Shell_NotifyIcon NIM_ADD");
        return false;
    }

    nid_.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid_);
    added_ = true;
    return true;
}

void TrayIcon::Remove() {
    if (!added_) {
        return;
    }

    if (!Shell_NotifyIconW(NIM_DELETE, &nid_)) {
        utils::LogLastError(L"[Tray] Shell_NotifyIcon NIM_DELETE");
    }
    added_ = false;
}

void TrayIcon::ShowMenu(HWND hwnd) {
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        utils::LogLastError(L"[Tray] CreatePopupMenu");
        return;
    }

    for (const auto& item : BuildTrayMenuItems()) {
        if (item.separator) {
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        } else {
            AppendMenuW(menu, MF_STRING, item.commandId, item.text.data());
        }
    }

    POINT pt{};
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(menu);
}

void TrayIcon::ShowBalloon(std::wstring_view title, std::wstring_view message) {
    if (!added_) {
        return;
    }

    const auto instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(nid_.hWnd, GWLP_HINSTANCE));
    NOTIFYICONDATAW clear = nid_;
    clear.uFlags = NIF_INFO;
    clear.szInfo[0] = L'\0';
    clear.szInfoTitle[0] = L'\0';
    clear.dwInfoFlags = NIIF_NONE;
    Shell_NotifyIconW(NIM_MODIFY, &clear);

    NOTIFYICONDATAW balloon = nid_;
    balloon.hIcon = LoadAppIcon(instance, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    balloon.hBalloonIcon = LoadAppIcon(instance, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    balloon.uFlags = NIF_INFO | NIF_ICON;
    CopyToNotifyBuffer(balloon.szInfoTitle, std::size(balloon.szInfoTitle), title);
    CopyToNotifyBuffer(balloon.szInfo, std::size(balloon.szInfo), message);
    balloon.dwInfoFlags = BalloonInfoFlags(balloon.hBalloonIcon != nullptr);
    if (!Shell_NotifyIconW(NIM_MODIFY, &balloon)) {
        utils::LogLastError(L"[Tray] Shell_NotifyIcon NIM_MODIFY");
        return;
    }

    nid_.hIcon = balloon.hIcon;
    nid_.hBalloonIcon = balloon.hBalloonIcon;
}

} // namespace mysnip::tray
