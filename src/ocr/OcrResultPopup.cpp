#include "ocr/OcrResultPopup.h"

#include "ocr/OcrClipboardText.h"
#include "utils/LogUtils.h"

#include <algorithm>
#include <string>
#include <windowsx.h>

namespace mysnip::ocr {
namespace {

constexpr wchar_t kPopupClass[] = L"MySnipasteOcrResultPopup";
constexpr COLORREF kTransparentColor = RGB(1, 2, 3);
constexpr UINT_PTR kCopyFeedbackTimer = 1;
constexpr UINT kCopyFeedbackDurationMs = 1200;

HFONT CreateUiFont(UINT dpi, int pointSize, int weight) {
    return CreateFontW(
        -MulDiv(pointSize, static_cast<int>(dpi), 72),
        0,
        0,
        0,
        weight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");
}

void FillRoundedRect(HDC hdc, const RECT& rect, int radius, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

} // namespace

void OcrResultPopup::ShowModal(
    HWND owner,
    RECT anchorScreenRect,
    RECT workArea,
    const std::wstring& initialText) {
    owner_ = owner;
    state_ = OcrPopupState(initialText);
    done_ = false;

    const auto instance = reinterpret_cast<HINSTANCE>(
        GetWindowLongPtrW(owner, GWLP_HINSTANCE));
    const UINT dpi = GetDpiForWindow(owner);
    layout_ = ComputeOcrPopupLayout(anchorScreenRect, workArea, dpi);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_DROPSHADOW;
    wc.lpfnWndProc = &OcrResultPopup::WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kPopupClass;
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED,
        kPopupClass,
        L"",
        WS_POPUP,
        layout_.windowRect.left,
        layout_.windowRect.top,
        layout_.windowRect.right - layout_.windowRect.left,
        layout_.windowRect.bottom - layout_.windowRect.top,
        owner,
        nullptr,
        instance,
        this);
    if (!hwnd_) {
        utils::LogLastError(L"[OCR] Create result popup");
        return;
    }

    SetLayeredWindowAttributes(hwnd_, kTransparentColor, 255, LWA_COLORKEY);
    CreateControls(instance);
    ApplyStateToControls();

    EnableWindow(owner_, FALSE);
    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);

    MSG msg{};
    while (!done_ && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_ESCAPE) {
                state_.HandleEscape();
                if (state_.CloseRequested()) {
                    Close();
                } else {
                    ApplyStateToControls();
                }
                continue;
            }
            if (msg.wParam == VK_RETURN &&
                state_.Mode() == OcrPopupMode::Editing &&
                (GetKeyState(VK_CONTROL) & 0x8000) != 0) {
                CompleteEdit();
                continue;
            }
        }

        if (!IsDialogMessageW(hwnd_, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(owner_, TRUE);
    SetForegroundWindow(owner_);
    SetFocus(owner_);
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void OcrResultPopup::CreateControls(HINSTANCE instance) {
    const UINT dpi = GetDpiForWindow(hwnd_);
    titleFont_ = CreateUiFont(dpi, 10, FW_NORMAL);
    bodyFont_ = CreateUiFont(dpi, 10, FW_NORMAL);
    buttonFont_ = CreateUiFont(dpi, 9, FW_SEMIBOLD);

    edit_ = CreateWindowExW(
        0,
        L"EDIT",
        state_.Text().c_str(),
        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
            ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        layout_.textRect.left,
        layout_.textRect.top,
        layout_.textRect.right - layout_.textRect.left,
        layout_.textRect.bottom - layout_.textRect.top,
        hwnd_,
        nullptr,
        instance,
        nullptr);
    if (edit_) {
        SendMessageW(
            edit_,
            WM_SETFONT,
            reinterpret_cast<WPARAM>(bodyFont_),
            TRUE);
        SendMessageW(edit_, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(0, 0));
    }
}

void OcrResultPopup::ApplyStateToControls() {
    if (!edit_) {
        return;
    }

    WriteEditText(state_.Text());
    const bool editing = state_.Mode() == OcrPopupMode::Editing;
    SendMessageW(edit_, EM_SETREADONLY, editing ? FALSE : TRUE, 0);

    LONG_PTR exStyle = GetWindowLongPtrW(edit_, GWL_EXSTYLE);
    if (editing) {
        exStyle |= WS_EX_CLIENTEDGE;
    } else {
        exStyle &= ~static_cast<LONG_PTR>(WS_EX_CLIENTEDGE);
    }
    SetWindowLongPtrW(edit_, GWL_EXSTYLE, exStyle);
    SetWindowPos(
        edit_,
        nullptr,
        layout_.textRect.left,
        layout_.textRect.top,
        layout_.textRect.right - layout_.textRect.left,
        layout_.textRect.bottom - layout_.textRect.top,
        SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);

    if (editing) {
        SetFocus(edit_);
        SendMessageW(edit_, EM_SETSEL, static_cast<WPARAM>(-1), static_cast<LPARAM>(-1));
    } else {
        SetFocus(hwnd_);
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void OcrResultPopup::Paint() {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd_, &ps);
    if (!hdc) {
        return;
    }

    RECT client{};
    GetClientRect(hwnd_, &client);
    HDC memoryDc = CreateCompatibleDC(hdc);
    HBITMAP bitmap = CreateCompatibleBitmap(
        hdc,
        client.right - client.left,
        client.bottom - client.top);
    HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);

    HBRUSH transparentBrush = CreateSolidBrush(kTransparentColor);
    FillRect(memoryDc, &client, transparentBrush);
    DeleteObject(transparentBrush);

    const UINT dpi = GetDpiForWindow(hwnd_);
    const int radius = MulDiv(8, static_cast<int>(dpi), 96);
    FillRoundedRect(memoryDc, layout_.bodyRect, radius, RGB(255, 255, 255));

    POINT triangle[3]{};
    const int halfPointer = MulDiv(9, static_cast<int>(dpi), 96);
    if (layout_.placement == OcrPopupPlacement::Above) {
        triangle[0] = POINT{
            layout_.pointerTip.x - halfPointer,
            layout_.bodyRect.bottom - 1};
        triangle[1] = layout_.pointerTip;
        triangle[2] = POINT{
            layout_.pointerTip.x + halfPointer,
            layout_.bodyRect.bottom - 1};
    } else {
        triangle[0] = POINT{
            layout_.pointerTip.x - halfPointer,
            layout_.bodyRect.top + 1};
        triangle[1] = layout_.pointerTip;
        triangle[2] = POINT{
            layout_.pointerTip.x + halfPointer,
            layout_.bodyRect.top + 1};
    }
    HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    HPEN whitePen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HGDIOBJ oldBrush = SelectObject(memoryDc, whiteBrush);
    HGDIOBJ oldPen = SelectObject(memoryDc, whitePen);
    Polygon(memoryDc, triangle, 3);
    SelectObject(memoryDc, oldPen);
    SelectObject(memoryDc, oldBrush);
    DeleteObject(whitePen);
    DeleteObject(whiteBrush);

    SetBkMode(memoryDc, TRANSPARENT);
    SetTextColor(memoryDc, RGB(80, 86, 96));
    HGDIOBJ oldFont = SelectObject(memoryDc, titleFont_);
    RECT titleRect = layout_.titleRect;
    DrawTextW(
        memoryDc,
        L"\u63d0\u53d6\u6587\u5b57",
        -1,
        &titleRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    const bool closeHovered =
        EqualRect(&hoveredRect_, &layout_.closeButton) != FALSE;
    if (closeHovered) {
        FillRoundedRect(
            memoryDc,
            layout_.closeButton,
            radius / 2,
            RGB(245, 246, 248));
    }
    SetTextColor(memoryDc, RGB(112, 119, 128));
    RECT closeRect = layout_.closeButton;
    DrawTextW(
        memoryDc,
        L"\u00d7",
        -1,
        &closeRect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(memoryDc, buttonFont_);
    const wchar_t* secondaryText =
        state_.Mode() == OcrPopupMode::Editing
        ? L"\u53d6\u6d88"
        : L"\u7f16\u8f91";
    const wchar_t* primaryText =
        state_.Mode() == OcrPopupMode::Editing
        ? L"\u5b8c\u6210"
        : (state_.CopyFeedbackVisible()
            ? L"\u5df2\u590d\u5236"
            : L"\u590d\u5236");

    const auto drawButton = [&](const RECT& bounds, const wchar_t* text) {
        COLORREF color = RGB(22, 100, 255);
        if (EqualRect(&hoveredRect_, &bounds)) {
            color = RGB(11, 83, 224);
        }
        FillRoundedRect(memoryDc, bounds, radius / 2, color);
        SetTextColor(memoryDc, RGB(255, 255, 255));
        RECT textRect = bounds;
        DrawTextW(
            memoryDc,
            text,
            -1,
            &textRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    };
    drawButton(layout_.secondaryButton, secondaryText);
    drawButton(layout_.primaryButton, primaryText);

    SelectObject(memoryDc, oldFont);
    BitBlt(
        hdc,
        0,
        0,
        client.right - client.left,
        client.bottom - client.top,
        memoryDc,
        0,
        0,
        SRCCOPY);

    SelectObject(memoryDc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memoryDc);
    EndPaint(hwnd_, &ps);
}

void OcrResultPopup::BeginEdit() {
    state_.BeginEdit();
    ApplyStateToControls();
}

void OcrResultPopup::CancelEdit() {
    state_.CancelEdit();
    ApplyStateToControls();
}

void OcrResultPopup::CompleteEdit() {
    state_.SetText(ReadEditText());
    state_.CompleteEdit();
    ApplyStateToControls();
}

void OcrResultPopup::CopyText() {
    if (state_.Mode() == OcrPopupMode::Editing) {
        state_.SetText(ReadEditText());
    }
    if (CopyUnicodeText(hwnd_, state_.Text())) {
        state_.MarkCopied();
        SetTimer(hwnd_, kCopyFeedbackTimer, kCopyFeedbackDurationMs, nullptr);
        InvalidateRect(hwnd_, &layout_.primaryButton, FALSE);
    }
}

void OcrResultPopup::Close() {
    done_ = true;
    ShowWindow(hwnd_, SW_HIDE);
}

std::wstring OcrResultPopup::ReadEditText() const {
    if (!edit_) {
        return state_.Text();
    }
    const int length = GetWindowTextLengthW(edit_);
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    GetWindowTextW(edit_, text.data(), length + 1);
    text.resize(static_cast<std::size_t>(length));
    return text;
}

void OcrResultPopup::WriteEditText(const std::wstring& text) {
    if (edit_ && ReadEditText() != text) {
        SetWindowTextW(edit_, text.c_str());
    }
}

void OcrResultPopup::UpdateHover(POINT clientPoint) {
    RECT next{};
    for (const RECT* bounds : {
        &layout_.closeButton,
        &layout_.secondaryButton,
        &layout_.primaryButton}) {
        if (IsPointIn(*bounds, clientPoint)) {
            next = *bounds;
            break;
        }
    }
    if (EqualRect(&hoveredRect_, &next) == FALSE) {
        hoveredRect_ = next;
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

bool OcrResultPopup::IsPointIn(const RECT& rect, POINT point) const {
    return point.x >= rect.left &&
        point.x < rect.right &&
        point.y >= rect.top &&
        point.y < rect.bottom;
}

LRESULT CALLBACK OcrResultPopup::WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) {
    OcrResultPopup* popup = nullptr;
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        popup = static_cast<OcrResultPopup*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(popup));
        popup->hwnd_ = hwnd;
    } else {
        popup = reinterpret_cast<OcrResultPopup*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    return popup
        ? popup->HandleMessage(hwnd, message, wParam, lParam)
        : DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT OcrResultPopup::HandleMessage(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) {
    switch (message) {
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: {
        HDC controlDc = reinterpret_cast<HDC>(wParam);
        SetTextColor(controlDc, RGB(55, 61, 70));
        SetBkColor(controlDc, RGB(255, 255, 255));
        return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        Paint();
        return 0;
    case WM_MOUSEMOVE: {
        if (!trackingMouse_) {
            TRACKMOUSEEVENT tracking{};
            tracking.cbSize = sizeof(tracking);
            tracking.dwFlags = TME_LEAVE;
            tracking.hwndTrack = hwnd;
            TrackMouseEvent(&tracking);
            trackingMouse_ = true;
        }
        UpdateHover(POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
        return 0;
    }
    case WM_MOUSELEAVE:
        trackingMouse_ = false;
        hoveredRect_ = {};
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONUP: {
        const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        if (IsPointIn(layout_.closeButton, point)) {
            Close();
        } else if (IsPointIn(layout_.secondaryButton, point)) {
            if (state_.Mode() == OcrPopupMode::Editing) {
                CancelEdit();
            } else {
                BeginEdit();
            }
        } else if (IsPointIn(layout_.primaryButton, point)) {
            if (state_.Mode() == OcrPopupMode::Editing) {
                CompleteEdit();
            } else {
                CopyText();
            }
        }
        return 0;
    }
    case WM_TIMER:
        if (wParam == kCopyFeedbackTimer) {
            KillTimer(hwnd, kCopyFeedbackTimer);
            state_.ClearCopyFeedback();
            InvalidateRect(hwnd, &layout_.primaryButton, FALSE);
            return 0;
        }
        break;
    case WM_CLOSE:
        Close();
        return 0;
    case WM_DESTROY:
        if (titleFont_) {
            DeleteObject(titleFont_);
            titleFont_ = nullptr;
        }
        if (bodyFont_) {
            DeleteObject(bodyFont_);
            bodyFont_ = nullptr;
        }
        if (buttonFont_) {
            DeleteObject(buttonFont_);
            buttonFont_ = nullptr;
        }
        hwnd_ = nullptr;
        done_ = true;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace mysnip::ocr
