#include "ocr/OcrTextDialog.h"

#include "ocr/OcrClipboardText.h"
#include "ocr/OcrDialogLayout.h"
#include "utils/LogUtils.h"

#include <algorithm>
#include <string>
#include <windowsx.h>

namespace mysnip::ocr {
namespace {

constexpr wchar_t kOcrDialogClass[] = L"MySnipasteOcrTextDialog";
constexpr int kCommandCopy = 1;
constexpr int kCommandClose = 2;
constexpr DWORD kDialogStyle =
    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX;

int ScaleForDpi(int value, UINT dpi) {
    return MulDiv(value, static_cast<int>(dpi), 96);
}

RECT CenteredWindowRect(HWND owner, SIZE clientSize, UINT dpi) {
    RECT windowRect{0, 0, clientSize.cx, clientSize.cy};
    AdjustWindowRectExForDpi(
        &windowRect,
        kDialogStyle,
        FALSE,
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        dpi);

    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfoW(MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST), &monitorInfo);

    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    const int x = monitorInfo.rcWork.left +
        ((monitorInfo.rcWork.right - monitorInfo.rcWork.left) - width) / 2;
    const int y = monitorInfo.rcWork.top +
        ((monitorInfo.rcWork.bottom - monitorInfo.rcWork.top) - height) / 2;
    return RECT{x, y, x + width, y + height};
}

} // namespace

void OcrTextDialog::ShowModal(HWND owner, const std::wstring& initialText) {
    owner_ = owner;
    done_ = false;
    const auto instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(owner, GWLP_HINSTANCE));

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &OcrTextDialog::WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kOcrDialogClass;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);

    const UINT dpi = GetDpiForWindow(owner);
    const RECT windowRect = CenteredWindowRect(
        owner,
        SIZE{ScaleForDpi(680, dpi), ScaleForDpi(460, dpi)},
        dpi);
    hwnd_ = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        kOcrDialogClass,
        L"MySnipaste OCR",
        kDialogStyle,
        windowRect.left,
        windowRect.top,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        owner,
        nullptr,
        instance,
        this);
    if (!hwnd_) {
        utils::LogLastError(L"[OCR] Create dialog");
        return;
    }

    edit_ = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        initialText.c_str(),
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL,
        0,
        0,
        0,
        0,
        hwnd_,
        nullptr,
        instance,
        nullptr);
    copyButton_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Copy",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0,
        0,
        0,
        0,
        hwnd_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCommandCopy)),
        instance,
        nullptr);
    closeButton_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Close",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0,
        0,
        0,
        0,
        hwnd_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCommandClose)),
        instance,
        nullptr);

    const HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    for (HWND control : {edit_, copyButton_, closeButton_}) {
        if (control) {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }
    }
    LayoutControls();

    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);
    SetFocus(edit_);

    MSG msg{};
    while (!done_ && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(hwnd_, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void OcrTextDialog::CopyText() {
    if (!edit_) {
        return;
    }
    const int length = GetWindowTextLengthW(edit_);
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    GetWindowTextW(edit_, text.data(), length + 1);
    text.resize(static_cast<std::size_t>(length));
    CopyUnicodeText(hwnd_, text);
}

void OcrTextDialog::LayoutControls() {
    if (!hwnd_ || !edit_ || !copyButton_ || !closeButton_) {
        return;
    }

    RECT clientRect{};
    GetClientRect(hwnd_, &clientRect);
    const auto layout = ComputeOcrDialogLayout(clientRect, GetDpiForWindow(hwnd_));
    const auto moveControl = [](HWND control, const RECT& bounds) {
        MoveWindow(
            control,
            bounds.left,
            bounds.top,
            bounds.right - bounds.left,
            bounds.bottom - bounds.top,
            TRUE);
    };
    moveControl(edit_, layout.editor);
    moveControl(copyButton_, layout.copyButton);
    moveControl(closeButton_, layout.closeButton);
}

LRESULT CALLBACK OcrTextDialog::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    OcrTextDialog* dialog = nullptr;
    if (message == WM_NCCREATE) {
        const auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        dialog = static_cast<OcrTextDialog*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
    } else {
        dialog = reinterpret_cast<OcrTextDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    return dialog ? dialog->HandleMessage(hwnd, message, wParam, lParam) : DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT OcrTextDialog::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        LayoutControls();
        return 0;
    case WM_GETMINMAXINFO: {
        const UINT dpi = GetDpiForWindow(hwnd);
        RECT minimumRect{
            0,
            0,
            ScaleForDpi(480, dpi),
            ScaleForDpi(320, dpi)};
        AdjustWindowRectExForDpi(
            &minimumRect,
            kDialogStyle,
            FALSE,
            WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            dpi);
        auto* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
        minMaxInfo->ptMinTrackSize.x = minimumRect.right - minimumRect.left;
        minMaxInfo->ptMinTrackSize.y = minimumRect.bottom - minimumRect.top;
        return 0;
    }
    case WM_DPICHANGED: {
        const auto* suggestedRect = reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(
            hwnd,
            nullptr,
            suggestedRect->left,
            suggestedRect->top,
            suggestedRect->right - suggestedRect->left,
            suggestedRect->bottom - suggestedRect->top,
            SWP_NOACTIVATE | SWP_NOZORDER);
        LayoutControls();
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == kCommandCopy) {
            CopyText();
            return 0;
        }
        if (LOWORD(wParam) == kCommandClose) {
            done_ = true;
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        break;
    case WM_CLOSE:
        done_ = true;
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    case WM_DESTROY:
        done_ = true;
        hwnd_ = nullptr;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace mysnip::ocr
