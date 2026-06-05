#include "longcapture/LongCaptureOverlayWindow.h"

#include "longcapture/LongCaptureOverlayConfig.h"
#include "utils/LogUtils.h"
#include "utils/RectUtils.h"
#include "utils/VirtualScreenUtils.h"

#include <algorithm>
#include <windowsx.h>

namespace mysnip::longcapture {
namespace {

constexpr wchar_t kLongOverlayClass[] = L"MySnipasteLongCaptureOverlayWindow";
constexpr wchar_t kLongControlsClass[] = L"MySnipasteLongCaptureControlsWindow";
constexpr UINT_PTR kCaptureTimerId = 1;

POINT ScreenPointFromLParam(HWND hwnd, LPARAM lParam) {
    POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    ClientToScreen(hwnd, &point);
    return point;
}

RECT ToClientRect(RECT screenRect, RECT virtualScreen) {
    return RECT{
        screenRect.left - virtualScreen.left,
        screenRect.top - virtualScreen.top,
        screenRect.right - virtualScreen.left,
        screenRect.bottom - virtualScreen.top,
    };
}

RECT OffsetFromScreen(RECT screenRect, RECT origin) {
    return RECT{
        screenRect.left - origin.left,
        screenRect.top - origin.top,
        screenRect.right - origin.left,
        screenRect.bottom - origin.top,
    };
}

void DrawButton(HDC hdc, RECT rect, bool confirm) {
    HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
    HPEN border = CreatePen(PS_SOLID, 1, RGB(210, 216, 224));
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, border);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 7, 7);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(border);
    DeleteObject(brush);

    const int cx = (rect.left + rect.right) / 2;
    const int cy = (rect.top + rect.bottom) / 2;
    HPEN pen = CreatePen(PS_SOLID, 2, confirm ? RGB(50, 170, 85) : RGB(235, 70, 70));
    oldPen = SelectObject(hdc, pen);
    if (confirm) {
        MoveToEx(hdc, cx - 7, cy, nullptr);
        LineTo(hdc, cx - 2, cy + 5);
        LineTo(hdc, cx + 8, cy - 6);
    } else {
        MoveToEx(hdc, cx - 7, cy - 7, nullptr);
        LineTo(hdc, cx + 7, cy + 7);
        MoveToEx(hdc, cx + 7, cy - 7, nullptr);
        LineTo(hdc, cx - 7, cy + 7);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

bool FramesDiffer(const image::ImageBuffer& a, const image::ImageBuffer& b) {
    if (!a.IsValid() || !b.IsValid() || a.width != b.width || a.height != b.height) {
        return true;
    }

    const int stepY = (std::max)(1, a.height / 20);
    const int stepX = (std::max)(1, a.width / 20);
    for (int y = 0; y < a.height; y += stepY) {
        for (int x = 0; x < a.width; x += stepX) {
            const auto offset = static_cast<std::size_t>(y * a.stride + x * 4);
            if (a.pixels[offset] != b.pixels[offset] ||
                a.pixels[offset + 1] != b.pixels[offset + 1] ||
                a.pixels[offset + 2] != b.pixels[offset + 2]) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

std::optional<capture::CaptureResult> LongCaptureOverlayWindow::ShowModal(
    HWND owner,
    RECT selectionRect,
    capture::ICaptureBackend& backend,
    const image::ImageBuffer& firstFrame) {
    owner_ = owner;
    backend_ = &backend;
    selectionRect_ = selectionRect;
    currentFrame_ = firstFrame;
    result_.reset();
    done_ = false;
    stitcher_ = LongCaptureStitcher{};
    stitcher_.AppendFrame(firstFrame);

    if (!CreateOverlayWindow(owner)) {
        return std::nullopt;
    }

    ShowWindow(hwnd_, SW_SHOW);
    ShowWindow(controlsHwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    UpdateWindow(controlsHwnd_);
    SetForegroundWindow(controlsHwnd_);
    SetFocus(controlsHwnd_);
    SetTimer(hwnd_, kCaptureTimerId, kLongCaptureSampleTimerMs, nullptr);

    MSG msg{};
    while (!done_ && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hwnd_) {
        KillTimer(hwnd_, kCaptureTimerId);
    }
    if (controlsHwnd_) {
        DestroyWindow(controlsHwnd_);
        controlsHwnd_ = nullptr;
    }
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    return result_;
}

bool LongCaptureOverlayWindow::CreateOverlayWindow(HWND owner) {
    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(owner, GWLP_HINSTANCE));

    WNDCLASSEXW overlayClass{};
    overlayClass.cbSize = sizeof(overlayClass);
    overlayClass.lpfnWndProc = &LongCaptureOverlayWindow::WindowProc;
    overlayClass.hInstance = instance;
    overlayClass.lpszClassName = kLongOverlayClass;
    overlayClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    overlayClass.hbrBackground = nullptr;
    RegisterClassExW(&overlayClass);

    WNDCLASSEXW controlsClass = overlayClass;
    controlsClass.lpszClassName = kLongControlsClass;
    RegisterClassExW(&controlsClass);

    virtualScreen_ = utils::GetVirtualScreenRect();
    layout_ = LongCaptureOverlayLayout::Create(selectionRect_, virtualScreen_);

    hwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        kLongOverlayClass,
        L"MySnipaste Long Capture Mask",
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
        utils::LogLastError(L"[LongCaptureOverlay] Create mask");
        return false;
    }
    SetLayeredWindowAttributes(hwnd_, 0, kLongCaptureMaskAlpha, LWA_ALPHA);

    HRGN fullRegion = CreateRectRgn(0, 0, utils::Width(virtualScreen_), utils::Height(virtualScreen_));
    const RECT selectionClient = ToClientRect(selectionRect_, virtualScreen_);
    HRGN holeRegion = CreateRectRgn(selectionClient.left, selectionClient.top, selectionClient.right, selectionClient.bottom);
    CombineRgn(fullRegion, fullRegion, holeRegion, RGN_DIFF);
    DeleteObject(holeRegion);
    SetWindowRgn(hwnd_, fullRegion, TRUE);

    controlsHwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        kLongControlsClass,
        L"MySnipaste Long Capture Controls",
        WS_POPUP,
        layout_.controlWindow.left,
        layout_.controlWindow.top,
        layout_.controlWindow.right - layout_.controlWindow.left,
        layout_.controlWindow.bottom - layout_.controlWindow.top,
        owner,
        nullptr,
        instance,
        this);
    if (!controlsHwnd_) {
        utils::LogLastError(L"[LongCaptureOverlay] Create controls");
        return false;
    }
    return true;
}

void LongCaptureOverlayWindow::Complete(bool confirmed) {
    done_ = true;
    if (confirmed && stitcher_.HasImage()) {
        capture::CaptureResult capture;
        capture.image = stitcher_.TakeImage();
        capture.screenRect = selectionRect_;
        result_ = std::move(capture);
    } else {
        result_.reset();
    }
    if (controlsHwnd_) {
        ShowWindow(controlsHwnd_, SW_HIDE);
    }
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

void LongCaptureOverlayWindow::OnPaint() {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd_, &ps);
    if (!dc) {
        return;
    }
    RECT client{};
    GetClientRect(hwnd_, &client);
    DrawScene(dc, client);
    EndPaint(hwnd_, &ps);
}

void LongCaptureOverlayWindow::DrawScene(HDC hdc, const RECT& client) {
    HBRUSH shade = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &client, shade);
    DeleteObject(shade);

    const RECT selectionClient = ToClientRect(selectionRect_, virtualScreen_);
    HPEN border = CreatePen(PS_SOLID, 2, RGB(64, 145, 255));
    HGDIOBJ oldPen = SelectObject(hdc, border);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, selectionClient.left, selectionClient.top, selectionClient.right, selectionClient.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(border);
}

void LongCaptureOverlayWindow::DrawControls(HDC hdc, const RECT& client) {
    HBRUSH background = CreateSolidBrush(RGB(245, 245, 245));
    HPEN border = CreatePen(PS_SOLID, 1, RGB(218, 222, 228));
    HGDIOBJ oldBrush = SelectObject(hdc, background);
    HGDIOBJ oldPen = SelectObject(hdc, border);
    RoundRect(hdc, client.left, client.top, client.right, client.bottom, 9, 9);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(border);
    DeleteObject(background);

    DrawButton(hdc, OffsetFromScreen(layout_.cancelButton, layout_.controlWindow), false);
    DrawButton(hdc, OffsetFromScreen(layout_.confirmButton, layout_.controlWindow), true);
}

void LongCaptureOverlayWindow::OnClick(POINT screenPoint) {
    const LongCaptureOverlayAction action = layout_.HitTest(screenPoint);
    if (action == LongCaptureOverlayAction::Cancel) {
        Complete(false);
    } else if (action == LongCaptureOverlayAction::Confirm) {
        Complete(true);
    }
}

void LongCaptureOverlayWindow::CaptureCurrentSelection() {
    if (!backend_) {
        return;
    }
    const auto frame = backend_->CaptureRect(selectionRect_);
    if (!frame.has_value()) {
        utils::LogError(L"[LongCaptureOverlay] Timer capture failed.");
        return;
    }

    if (!FramesDiffer(currentFrame_, frame->image)) {
        return;
    }

    currentFrame_ = frame->image;
    const auto stitch = stitcher_.AppendFrame(currentFrame_);
    if (stitch.status == StitchStatus::LowConfidence) {
        stitcher_.AppendFrameWithoutOverlap(frame->image);
    } else if (stitch.status == StitchStatus::InvalidFrame) {
        utils::LogError(L"[LongCaptureOverlay] Stitch frame failed.");
    }
}

LRESULT CALLBACK LongCaptureOverlayWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LongCaptureOverlayWindow* window = nullptr;
    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        window = static_cast<LongCaptureOverlayWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<LongCaptureOverlayWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    return window ? window->HandleMessage(hwnd, message, wParam, lParam) : DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT LongCaptureOverlayWindow::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_LBUTTONUP:
        if (hwnd == controlsHwnd_) {
            OnClick(ScreenPointFromLParam(hwnd, lParam));
        }
        return 0;
    case WM_TIMER:
        if (wParam == kCaptureTimerId) {
            CaptureCurrentSelection();
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            Complete(false);
            return 0;
        }
        break;
    case WM_PAINT: {
        if (hwnd == controlsHwnd_) {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            RECT client{};
            GetClientRect(hwnd, &client);
            DrawControls(dc, client);
            EndPaint(hwnd, &ps);
        } else {
            OnPaint();
        }
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        if (hwnd == controlsHwnd_) {
            controlsHwnd_ = nullptr;
        } else if (hwnd == hwnd_) {
            hwnd_ = nullptr;
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace mysnip::longcapture
