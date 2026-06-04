#include "pin/PinnedImageWindow.h"

#include "app/CommandIds.h"
#include "editor/ImageRenderUtils.h"
#include "utils/LogUtils.h"

#include <algorithm>
#include <cmath>
#include <windows.h>
#include <windowsx.h>

namespace mysnip::pin {
namespace {

constexpr wchar_t kPinnedImageWindowClass[] = L"MySnipastePinnedImageWindow";
constexpr float kMinScale = 0.2f;
constexpr float kMaxScale = 5.0f;
constexpr float kScaleStep = 1.1f;
constexpr int kCloseButtonSize = 24;

int ScaledDimension(int dimension, float scale) {
    return (std::max)(1, static_cast<int>(std::lround(static_cast<float>(dimension) * ClampPinnedImageScale(scale))));
}

bool ContainsPoint(const RECT& rect, POINT point) {
    return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
}

void DrawLine(HDC hdc, int x1, int y1, int x2, int y2) {
    MoveToEx(hdc, x1, y1, nullptr);
    LineTo(hdc, x2, y2);
}

} // namespace

float ClampPinnedImageScale(float scale) {
    return (std::clamp)(scale, kMinScale, kMaxScale);
}

SIZE ComputePinnedImageSize(int imageWidth, int imageHeight, float scale) {
    return SIZE{ScaledDimension(imageWidth, scale), ScaledDimension(imageHeight, scale)};
}

RECT PinnedImageCloseButtonRect(const RECT& clientRect) {
    const int width = clientRect.right - clientRect.left;
    const int height = clientRect.bottom - clientRect.top;
    const int size = (std::min)({kCloseButtonSize, (std::max)(0, width), (std::max)(0, height)});
    return RECT{clientRect.right - size, clientRect.top, clientRect.right, clientRect.top + size};
}

bool IsPinnedImageCloseButtonHit(const RECT& clientRect, POINT point) {
    const RECT closeRect = PinnedImageCloseButtonRect(clientRect);
    return ContainsPoint(closeRect, point);
}

bool ShouldClosePinnedImageOnKey(WPARAM) {
    return false;
}

LPCWSTR PinnedImageCursorResourceName() {
    return IDC_ARROW;
}

PinnedImageWindow::~PinnedImageWindow() {
    if (hwnd_ && IsWindow(hwnd_)) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

bool PinnedImageWindow::Create(HWND owner, const image::ImageBuffer& image, POINT position) {
    if (!image.IsValid()) {
        return false;
    }

    owner_ = owner;
    image_ = image;
    scale_ = 1.0f;

    HINSTANCE instance = GetModuleHandleW(nullptr);
    if (!RegisterWindowClass(instance)) {
        return false;
    }

    const SIZE size = ComputePinnedImageSize(image_.width, image_.height, scale_);
    hwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        kPinnedImageWindowClass,
        L"",
        WS_POPUP,
        position.x,
        position.y,
        size.cx,
        size.cy,
        owner,
        nullptr,
        instance,
        this);

    if (!hwnd_) {
        utils::LogLastError(L"[Pin] CreateWindowExW");
        return false;
    }

    return true;
}

void PinnedImageWindow::Show() {
    if (!hwnd_) {
        return;
    }
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

HWND PinnedImageWindow::Hwnd() const {
    return hwnd_;
}

bool PinnedImageWindow::RegisterWindowClass(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = &PinnedImageWindow::WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, PinnedImageCursorResourceName());
    wc.lpszClassName = kPinnedImageWindowClass;

    if (RegisterClassExW(&wc)) {
        return true;
    }
    if (GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
        return true;
    }
    utils::LogLastError(L"[Pin] RegisterClassExW");
    return false;
}

void PinnedImageWindow::OnPaint() {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd_, &ps);
    if (!hdc) {
        return;
    }

    RECT client{};
    GetClientRect(hwnd_, &client);
    if (!editor::ImageRenderUtils::DrawImage(hdc, image_, client)) {
        utils::LogError(L"[Pin] DrawImage failed.");
    }
    DrawCloseButton(hdc, client);

    EndPaint(hwnd_, &ps);
}

void PinnedImageWindow::DrawCloseButton(HDC hdc, const RECT& client) {
    const RECT rect = PinnedImageCloseButtonRect(client);
    if (rect.left >= rect.right || rect.top >= rect.bottom) {
        return;
    }

    HBRUSH background = CreateSolidBrush(RGB(250, 250, 250));
    HPEN border = CreatePen(PS_SOLID, 1, RGB(225, 225, 225));
    HGDIOBJ oldBrush = SelectObject(hdc, background);
    HGDIOBJ oldPen = SelectObject(hdc, border);
    RoundRect(hdc, rect.left + 2, rect.top + 2, rect.right - 2, rect.bottom - 2, 8, 8);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(border);
    DeleteObject(background);

    HPEN crossPen = CreatePen(PS_SOLID, 2, RGB(235, 70, 70));
    HGDIOBJ oldCrossPen = SelectObject(hdc, crossPen);
    const int cx = (rect.left + rect.right) / 2;
    const int cy = (rect.top + rect.bottom) / 2;
    DrawLine(hdc, cx - 5, cy - 5, cx + 5, cy + 5);
    DrawLine(hdc, cx + 5, cy - 5, cx - 5, cy + 5);
    SelectObject(hdc, oldCrossPen);
    DeleteObject(crossPen);
}

void PinnedImageWindow::BeginDrag() {
    dragging_ = true;
    SetCursor(LoadCursorW(nullptr, PinnedImageCursorResourceName()));
    GetCursorPos(&dragStartScreen_);
    GetWindowRect(hwnd_, &dragStartWindow_);
    SetCapture(hwnd_);
}

void PinnedImageWindow::ContinueDrag() {
    if (!dragging_) {
        return;
    }

    POINT current{};
    SetCursor(LoadCursorW(nullptr, PinnedImageCursorResourceName()));
    GetCursorPos(&current);
    const int dx = current.x - dragStartScreen_.x;
    const int dy = current.y - dragStartScreen_.y;
    SetWindowPos(
        hwnd_,
        HWND_TOPMOST,
        dragStartWindow_.left + dx,
        dragStartWindow_.top + dy,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE);
}

void PinnedImageWindow::EndDrag() {
    if (!dragging_) {
        return;
    }
    dragging_ = false;
    if (GetCapture() == hwnd_) {
        ReleaseCapture();
    }
}

void PinnedImageWindow::ApplyScale(float scale) {
    scale_ = ClampPinnedImageScale(scale);
    const SIZE size = ComputePinnedImageSize(image_.width, image_.height, scale_);
    SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, size.cx, size.cy, SWP_NOMOVE | SWP_NOACTIVATE);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

LRESULT CALLBACK PinnedImageWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PinnedImageWindow* window = nullptr;
    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        window = static_cast<PinnedImageWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->hwnd_ = hwnd;
    } else {
        window = reinterpret_cast<PinnedImageWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (window) {
        return window->HandleMessage(message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT PinnedImageWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        OnPaint();
        return 0;
    case WM_SETCURSOR:
        SetCursor(LoadCursorW(nullptr, PinnedImageCursorResourceName()));
        return TRUE;
    case WM_LBUTTONDOWN: {
        RECT client{};
        GetClientRect(hwnd_, &client);
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        if (IsPinnedImageCloseButtonHit(client, point)) {
            DestroyWindow(hwnd_);
            return 0;
        }
        BeginDrag();
        return 0;
    }
    case WM_MOUSEMOVE:
        ContinueDrag();
        return 0;
    case WM_LBUTTONUP:
        EndDrag();
        return 0;
    case WM_CAPTURECHANGED:
        dragging_ = false;
        return 0;
    case WM_LBUTTONDBLCLK:
        DestroyWindow(hwnd_);
        return 0;
    case WM_MOUSEWHEEL:
        if ((GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) != 0) {
            const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            ApplyScale(delta > 0 ? scale_ * kScaleStep : scale_ / kScaleStep);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (ShouldClosePinnedImageOnKey(wParam)) {
            DestroyWindow(hwnd_);
            return 0;
        }
        break;
    case WM_NCDESTROY: {
        const HWND destroyed = hwnd_;
        if (owner_) {
            PostMessageW(owner_, app::kPinnedImageDestroyedMessage, reinterpret_cast<WPARAM>(destroyed), 0);
        }
        SetWindowLongPtrW(destroyed, GWLP_USERDATA, 0);
        return 0;
    }
    default:
        break;
    }

    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

} // namespace mysnip::pin
