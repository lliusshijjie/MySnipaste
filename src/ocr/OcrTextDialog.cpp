#include "ocr/OcrTextDialog.h"

#include "ocr/OcrClipboardText.h"
#include "utils/LogUtils.h"

#include <string>
#include <windowsx.h>

namespace mysnip::ocr {
namespace {

constexpr wchar_t kOcrDialogClass[] = L"MySnipasteOcrTextDialog";
constexpr int kCommandCopy = 1;
constexpr int kCommandClose = 2;

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

    hwnd_ = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        kOcrDialogClass,
        L"MySnipaste OCR",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        520,
        360,
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
        12,
        12,
        480,
        260,
        hwnd_,
        nullptr,
        instance,
        nullptr);
    CreateWindowExW(0, L"BUTTON", L"Copy", WS_CHILD | WS_VISIBLE, 312, 286, 86, 30, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCommandCopy)), instance, nullptr);
    CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE, 406, 286, 86, 30, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCommandClose)), instance, nullptr);

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
