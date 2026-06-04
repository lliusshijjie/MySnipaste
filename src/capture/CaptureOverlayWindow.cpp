#include "capture/CaptureOverlayWindow.h"

#include "capture/CaptureOverlayConfig.h"
#include "capture/CaptureOverlayState.h"
#include "utils/LogUtils.h"
#include "utils/RectUtils.h"
#include "utils/VirtualScreenUtils.h"

#include <windows.h>
#include <windowsx.h>

namespace mysnip::capture {
namespace {

constexpr wchar_t kOverlayWindowClass[] = L"MySnipasteCaptureOverlayWindow";

POINT ScreenPointFromLParam(HWND hwnd, LPARAM lParam) {
    POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    ClientToScreen(hwnd, &pt);
    return pt;
}

} // namespace

SelectionResult CaptureOverlayWindow::SelectRegion(HWND owner) {
    owner_ = owner;
    result_ = {};
    done_ = false;

    if (!CreateOverlayWindow(owner)) {
        result_.status = SelectionStatus::Failed;
        return result_;
    }

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    SetForegroundWindow(hwnd_);
    SetFocus(hwnd_);

    MSG msg{};
    while (!done_ && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }

    return result_;
}

bool CaptureOverlayWindow::CreateOverlayWindow(HWND owner) {
    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(owner, GWLP_HINSTANCE));

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &CaptureOverlayWindow::WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kOverlayWindowClass;
    wc.hCursor = LoadCursorW(nullptr, IDC_CROSS);
    wc.hbrBackground = nullptr;

    RegisterClassExW(&wc);

    virtualScreen_ = utils::GetVirtualScreenRect();
    utils::LogRect(L"[Overlay] virtual screen", virtualScreen_);
    hwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        kOverlayWindowClass,
        L"MySnipaste Capture",
        WS_POPUP,
        virtualScreen_.left,
        virtualScreen_.top,
        utils::Width(virtualScreen_),
        utils::Height(virtualScreen_),
        owner,
        nullptr,
        instance,
        this);

    if (!hwnd_) {
        utils::LogLastError(L"[Overlay] CreateWindowExW");
        return false;
    }

    SetLayeredWindowAttributes(hwnd_, 0, kSelectionOverlayAlpha, LWA_ALPHA);
    utils::LogDpi(L"[Overlay] window", hwnd_);
    return true;
}

void CaptureOverlayWindow::Complete(SelectionStatus status, RECT rect) {
    result_.status = status;
    result_.screenRect = rect;
    done_ = true;
    if (selecting_) {
        selecting_ = false;
        ReleaseCapture();
    }
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

void CaptureOverlayWindow::OnMouseDown(POINT pt) {
    start_ = pt;
    current_ = pt;
    selecting_ = true;
    SetCapture(hwnd_);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void CaptureOverlayWindow::OnMouseMove(POINT pt) {
    if (!selecting_) {
        return;
    }

    current_ = pt;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void CaptureOverlayWindow::OnMouseUp(POINT pt) {
    if (!selecting_) {
        return;
    }

    current_ = pt;
    const RECT rect = utils::NormalizeRect(start_, current_);
    if (utils::IsTooSmall(rect)) {
        Complete(SelectionStatus::TooSmall, rect);
        return;
    }

    Complete(SelectionStatus::Completed, rect);
}

void CaptureOverlayWindow::OnPaint() {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd_, &ps);
    if (!dc) {
        return;
    }

    RECT client{};
    GetClientRect(hwnd_, &client);
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    HDC memoryDc = CreateCompatibleDC(dc);
    HBITMAP bitmap = memoryDc ? CreateCompatibleBitmap(dc, width, height) : nullptr;
    if (memoryDc && bitmap) {
        HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);
        DrawScene(memoryDc, client);
        BitBlt(dc, 0, 0, width, height, memoryDc, 0, 0, SRCCOPY);
        SelectObject(memoryDc, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memoryDc);
        EndPaint(hwnd_, &ps);
        return;
    }
    if (bitmap) {
        DeleteObject(bitmap);
    }
    if (memoryDc) {
        DeleteDC(memoryDc);
    }

    DrawScene(dc, client);
    EndPaint(hwnd_, &ps);
}

void CaptureOverlayWindow::DrawScene(HDC dc, const RECT& client) {
    HBRUSH overlayBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(dc, &client, overlayBrush);
    DeleteObject(overlayBrush);

    if (selecting_) {
        RECT screenRect = utils::NormalizeRect(start_, current_);
        RECT clientRect = utils::ScreenRectToVirtualClientRect(screenRect, virtualScreen_);

        HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
        HGDIOBJ oldPen = SelectObject(dc, pen);
        HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
        Rectangle(dc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }
}

LRESULT CALLBACK CaptureOverlayWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    CaptureOverlayWindow* window = nullptr;
    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        window = static_cast<CaptureOverlayWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<CaptureOverlayWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (window) {
        return window->HandleMessage(hwnd, message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CaptureOverlayWindow::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_LBUTTONDOWN:
        OnMouseDown(ScreenPointFromLParam(hwnd, lParam));
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove(ScreenPointFromLParam(hwnd, lParam));
        return 0;
    case WM_LBUTTONUP:
        OnMouseUp(ScreenPointFromLParam(hwnd, lParam));
        return 0;
    case WM_CAPTURECHANGED:
        if (ShouldCancelOnCaptureChanged(selecting_, done_)) {
            Complete(SelectionStatus::Cancelled);
        }
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            Complete(SelectionStatus::Cancelled);
            return 0;
        }
        break;
    case WM_PAINT:
        OnPaint();
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        hwnd_ = nullptr;
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace mysnip::capture
