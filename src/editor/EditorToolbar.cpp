#include "editor/EditorToolbar.h"

#include "utils/RectUtils.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>
#include <windows.h>

namespace mysnip::editor {
namespace {

constexpr int kToolbarHeight = 40;
constexpr int kButtonSize = 32;
constexpr int kGap = 6;
constexpr int kMargin = 8;
constexpr int kButtonCount = 20;

struct ButtonSpec {
    ToolbarIcon icon;
    ToolbarAction action;
    bool enabled;
    std::wstring_view tooltip;
};

constexpr std::array<ButtonSpec, kButtonCount> kButtonSpecs{{
    {ToolbarIcon::Select, ToolbarAction::Select, true, L"\u9009\u62e9/\u79fb\u52a8"},
    {ToolbarIcon::Rectangle, ToolbarAction::Rectangle, true, L"\u77e9\u5f62"},
    {ToolbarIcon::Ellipse, ToolbarAction::Ellipse, true, L"\u692d\u5706"},
    {ToolbarIcon::Line, ToolbarAction::Line, true, L"\u76f4\u7ebf"},
    {ToolbarIcon::Arrow, ToolbarAction::Arrow, true, L"\u7bad\u5934"},
    {ToolbarIcon::Freehand, ToolbarAction::Freehand, true, L"\u753b\u7b14"},
    {ToolbarIcon::Text, ToolbarAction::Text, true, L"\u6587\u5b57"},
    {ToolbarIcon::Ocr, ToolbarAction::Ocr, true, L"\u8bc6\u522b\u6587\u5b57"},
    {ToolbarIcon::Tag, ToolbarAction::Tag, true, L"\u6807\u7b7e"},
    {ToolbarIcon::Mosaic, ToolbarAction::Mosaic, true, L"\u9a6c\u8d5b\u514b"},
    {ToolbarIcon::Blur, ToolbarAction::Blur, true, L"\u6a21\u7cca"},
    {ToolbarIcon::Highlight, ToolbarAction::Highlight, true, L"\u9ad8\u4eae"},
    {ToolbarIcon::Pin, ToolbarAction::Pin, true, L"\u8d34\u56fe"},
    {ToolbarIcon::Number, ToolbarAction::Number, true, L"\u5e8f\u53f7"},
    {ToolbarIcon::Crop, ToolbarAction::Crop, true, L"\u88c1\u526a"},
    {ToolbarIcon::Undo, ToolbarAction::Undo, false, L"\u64a4\u9500"},
    {ToolbarIcon::Redo, ToolbarAction::Redo, false, L"\u91cd\u505a"},
    {ToolbarIcon::Save, ToolbarAction::Save, true, L"\u4fdd\u5b58 PNG"},
    {ToolbarIcon::Cancel, ToolbarAction::Cancel, true, L"\u53d6\u6d88"},
    {ToolbarIcon::Confirm, ToolbarAction::Confirm, true, L"\u5b8c\u6210\u5e76\u590d\u5236"},
}};

bool ContainsPoint(const RECT& rect, POINT pt) {
    return pt.x >= rect.left && pt.x < rect.right && pt.y >= rect.top && pt.y < rect.bottom;
}

RECT OffsetRectCopy(RECT rect, POINT origin) {
    rect.left -= origin.x;
    rect.right -= origin.x;
    rect.top -= origin.y;
    rect.bottom -= origin.y;
    return rect;
}

void DrawLine(HDC hdc, int x1, int y1, int x2, int y2) {
    MoveToEx(hdc, x1, y1, nullptr);
    LineTo(hdc, x2, y2);
}

void FillDot(HDC hdc, int x, int y, int radius, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
    Ellipse(hdc, x - radius, y - radius, x + radius + 1, y + radius + 1);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(brush);
}

void FillSquare(HDC hdc, int left, int top, int size, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    RECT cell{left, top, left + size, top + size};
    FillRect(hdc, &cell, brush);
    DeleteObject(brush);
}

void DrawCheck(HDC hdc, int cx, int cy) {
    const POINT pts[] = {{cx - 7, cy + 1}, {cx - 2, cy + 6}, {cx + 8, cy - 6}};
    Polyline(hdc, pts, 3);
}

void DrawCross(HDC hdc, int cx, int cy) {
    DrawLine(hdc, cx - 7, cy - 7, cx + 7, cy + 7);
    DrawLine(hdc, cx + 7, cy - 7, cx - 7, cy + 7);
}

void DrawArrow(HDC hdc, int cx, int cy) {
    DrawLine(hdc, cx - 7, cy + 7, cx + 6, cy - 6);
    const POINT head[] = {{cx - 1, cy - 6}, {cx + 6, cy - 6}, {cx + 6, cy + 1}};
    Polyline(hdc, head, 3);
}

void DrawToolbarIcon(HDC hdc, const RECT& r, ToolbarIcon icon, COLORREF color) {
    const int cx = (r.left + r.right) / 2;
    const int cy = (r.top + r.bottom) / 2;

    switch (icon) {
    case ToolbarIcon::Select: {
        const POINT cursor[] = {{cx - 7, cy - 9}, {cx - 7, cy + 8}, {cx - 1, cy + 3}, {cx + 3, cy + 9}, {cx + 7, cy + 7}, {cx + 3, cy + 1}, {cx + 10, cy + 1}, {cx - 7, cy - 9}};
        Polyline(hdc, cursor, 8);
        break;
    }
    case ToolbarIcon::Rectangle:
        RoundRect(hdc, cx - 8, cy - 7, cx + 8, cy + 7, 5, 5);
        break;
    case ToolbarIcon::Ellipse:
        Ellipse(hdc, cx - 8, cy - 7, cx + 8, cy + 7);
        break;
    case ToolbarIcon::Line:
        DrawLine(hdc, cx - 8, cy + 7, cx + 8, cy - 7);
        break;
    case ToolbarIcon::Arrow:
        DrawArrow(hdc, cx, cy);
        break;
    case ToolbarIcon::Freehand: {
        const POINT stroke[] = {{cx - 9, cy + 5}, {cx - 4, cy - 3}, {cx + 1, cy + 4}, {cx + 6, cy - 4}, {cx + 9, cy + 1}};
        Polyline(hdc, stroke, 5);
        break;
    }
    case ToolbarIcon::Text:
        DrawLine(hdc, cx - 7, cy - 7, cx + 7, cy - 7);
        DrawLine(hdc, cx, cy - 7, cx, cy + 8);
        break;
    case ToolbarIcon::Ocr:
        RoundRect(hdc, cx - 9, cy - 8, cx + 9, cy + 8, 3, 3);
        DrawLine(hdc, cx - 4, cy - 3, cx + 4, cy - 3);
        DrawLine(hdc, cx - 4, cy + 2, cx + 4, cy + 2);
        DrawLine(hdc, cx - 4, cy + 7, cx + 2, cy + 7);
        break;
    case ToolbarIcon::Tag: {
        const POINT tag[] = {{cx - 8, cy - 6}, {cx + 2, cy - 6}, {cx + 8, cy}, {cx + 2, cy + 6}, {cx - 8, cy + 6}, {cx - 8, cy - 6}};
        Polyline(hdc, tag, 6);
        Ellipse(hdc, cx - 6, cy - 2, cx - 2, cy + 2);
        break;
    }
    case ToolbarIcon::Mosaic:
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                if (((row + col) & 1) == 0) {
                    FillSquare(hdc, cx - 7 + col * 5, cy - 7 + row * 5, 4, color);
                }
            }
        }
        break;
    case ToolbarIcon::Blur:
        Ellipse(hdc, cx - 8, cy - 8, cx + 8, cy + 8);
        FillDot(hdc, cx - 3, cy - 2, 1, color);
        FillDot(hdc, cx + 3, cy - 3, 1, color);
        FillDot(hdc, cx + 1, cy + 3, 1, color);
        FillDot(hdc, cx - 4, cy + 4, 1, color);
        FillDot(hdc, cx + 5, cy + 2, 1, color);
        break;
    case ToolbarIcon::Highlight: {
        const POINT nib[] = {{cx + 2, cy - 8}, {cx + 8, cy - 2}, {cx - 2, cy + 6}, {cx - 8, cy}, {cx + 2, cy - 8}};
        Polyline(hdc, nib, 5);
        DrawLine(hdc, cx - 8, cy + 8, cx + 4, cy + 8);
        break;
    }
    case ToolbarIcon::Pin:
        FillDot(hdc, cx, cy - 2, 5, color);
        DrawLine(hdc, cx, cy + 3, cx, cy + 9);
        break;
    case ToolbarIcon::Number:
        Ellipse(hdc, cx - 8, cy - 8, cx + 8, cy + 8);
        DrawLine(hdc, cx - 2, cy - 2, cx, cy - 5);
        DrawLine(hdc, cx, cy - 5, cx, cy + 5);
        break;
    case ToolbarIcon::Crop: {
        const POINT bracketA[] = {{cx - 7, cy - 3}, {cx - 7, cy + 7}, {cx + 3, cy + 7}};
        const POINT bracketB[] = {{cx - 3, cy - 7}, {cx + 7, cy - 7}, {cx + 7, cy + 3}};
        Polyline(hdc, bracketA, 3);
        Polyline(hdc, bracketB, 3);
        break;
    }
    case ToolbarIcon::Undo: {
        Arc(hdc, cx - 7, cy - 6, cx + 9, cy + 10, cx + 8, cy + 2, cx - 6, cy - 1);
        const POINT head[] = {{cx - 2, cy - 5}, {cx - 7, cy - 2}, {cx - 3, cy + 3}};
        Polyline(hdc, head, 3);
        break;
    }
    case ToolbarIcon::Redo: {
        Arc(hdc, cx - 9, cy - 6, cx + 7, cy + 10, cx - 8, cy + 2, cx + 6, cy - 1);
        const POINT head[] = {{cx + 2, cy - 5}, {cx + 7, cy - 2}, {cx + 3, cy + 3}};
        Polyline(hdc, head, 3);
        break;
    }
    case ToolbarIcon::Save:
        DrawLine(hdc, cx - 7, cy - 7, cx + 7, cy - 7);
        DrawLine(hdc, cx - 7, cy - 7, cx - 7, cy + 7);
        DrawLine(hdc, cx + 7, cy - 7, cx + 7, cy + 7);
        DrawLine(hdc, cx - 7, cy + 7, cx + 7, cy + 7);
        DrawLine(hdc, cx - 3, cy - 2, cx + 3, cy - 2);
        DrawLine(hdc, cx, cy - 2, cx, cy + 5);
        DrawLine(hdc, cx - 4, cy + 2, cx, cy + 6);
        DrawLine(hdc, cx + 4, cy + 2, cx, cy + 6);
        break;
    case ToolbarIcon::Cancel:
        DrawCross(hdc, cx, cy);
        break;
    case ToolbarIcon::Confirm:
        DrawCheck(hdc, cx, cy);
        break;
    }
}

} // namespace

void EditorToolbar::Layout(const RECT& captureRect, const RECT& virtualScreenRect) {
    const int naturalWidth = (kMargin * 2) + (kButtonCount * kButtonSize) + ((kButtonCount - 1) * kGap);
    const int availableWidth = (std::max)(0, utils::Width(virtualScreenRect) - (kMargin * 2));
    const int toolbarWidth = (std::min)(naturalWidth, availableWidth);
    const int captureCenterX = captureRect.left + utils::Width(captureRect) / 2;

    int left = captureCenterX - toolbarWidth / 2;
    const int minLeft = virtualScreenRect.left + kMargin;
    const int maxLeft = virtualScreenRect.right - kMargin - toolbarWidth;
    if (maxLeft >= minLeft) {
        left = (std::clamp)(left, minLeft, maxLeft);
    } else {
        left = minLeft;
    }

    int top = captureRect.bottom + kMargin;
    if (top + kToolbarHeight > virtualScreenRect.bottom - kMargin) {
        top = captureRect.top - kMargin - kToolbarHeight;
    }
    if (top < virtualScreenRect.top + kMargin || top + kToolbarHeight > virtualScreenRect.bottom - kMargin) {
        const int minTop = virtualScreenRect.top + kMargin;
        const int maxTop = virtualScreenRect.bottom - kMargin - kToolbarHeight;
        top = maxTop >= minTop ? (std::clamp)(top, minTop, maxTop) : minTop;
    }

    toolbarBounds_ = RECT{left, top, left + toolbarWidth, top + kToolbarHeight};
    BuildButtons();
}

void EditorToolbar::BuildButtons() {
    buttons_.clear();
    buttons_.reserve(kButtonCount);

    int x = toolbarBounds_.left + kMargin;
    const int y = toolbarBounds_.top + (utils::Height(toolbarBounds_) - kButtonSize) / 2;

    for (int i = 0; i < kButtonCount; ++i) {
        const auto& spec = kButtonSpecs[static_cast<std::size_t>(i)];
        ToolbarButton button;
        button.action = spec.action;
        button.icon = spec.icon;
        button.enabled = spec.enabled;
        button.tooltip = spec.tooltip;
        button.bounds = RECT{x, y, x + kButtonSize, y + kButtonSize};
        buttons_.push_back(button);
        x += kButtonSize + kGap;
    }
}

void EditorToolbar::Draw(HDC hdc, POINT origin) const {
    const RECT localToolbar = OffsetRectCopy(toolbarBounds_, origin);

    HBRUSH backgroundBrush = CreateSolidBrush(RGB(250, 250, 250));
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(220, 220, 220));
    HGDIOBJ oldBrush = SelectObject(hdc, backgroundBrush);
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    RoundRect(hdc, localToolbar.left, localToolbar.top, localToolbar.right, localToolbar.bottom, 8, 8);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);
    DeleteObject(backgroundBrush);

    for (int i = 0; i < static_cast<int>(buttons_.size()); ++i) {
        const auto& button = buttons_[static_cast<std::size_t>(i)];
        const RECT localButton = OffsetRectCopy(button.bounds, origin);
        const bool selected =
            (button.action == ToolbarAction::Rectangle && currentTool_ == ToolType::Rectangle) ||
            (button.action == ToolbarAction::Select && currentTool_ == ToolType::Select) ||
            (button.action == ToolbarAction::Ellipse && currentTool_ == ToolType::Ellipse) ||
            (button.action == ToolbarAction::Line && currentTool_ == ToolType::Line) ||
            (button.action == ToolbarAction::Arrow && currentTool_ == ToolType::Arrow) ||
            (button.action == ToolbarAction::Freehand && currentTool_ == ToolType::Freehand) ||
            (button.action == ToolbarAction::Number && currentTool_ == ToolType::Number) ||
            (button.action == ToolbarAction::Text && currentTool_ == ToolType::Text) ||
            (button.action == ToolbarAction::Tag && currentTool_ == ToolType::Tag) ||
            (button.action == ToolbarAction::Highlight && currentTool_ == ToolType::Highlight) ||
            (button.action == ToolbarAction::Mosaic && currentTool_ == ToolType::Mosaic) ||
            (button.action == ToolbarAction::Blur && currentTool_ == ToolType::Blur) ||
            (button.action == ToolbarAction::Crop && currentTool_ == ToolType::Crop);

        if (selected) {
            HBRUSH selectedBrush = CreateSolidBrush(RGB(232, 236, 240));
            HPEN selectedPen = CreatePen(PS_SOLID, 1, RGB(210, 216, 224));
            HGDIOBJ oldSelectedBrush = SelectObject(hdc, selectedBrush);
            HGDIOBJ oldSelectedPen = SelectObject(hdc, selectedPen);
            RoundRect(hdc, localButton.left, localButton.top, localButton.right, localButton.bottom, 6, 6);
            SelectObject(hdc, oldSelectedPen);
            SelectObject(hdc, oldSelectedBrush);
            DeleteObject(selectedPen);
            DeleteObject(selectedBrush);
        }

        const COLORREF color = button.action == ToolbarAction::Cancel
            ? RGB(235, 70, 70)
            : button.action == ToolbarAction::Confirm
                ? RGB(50, 170, 85)
                : button.enabled ? RGB(45, 52, 60) : RGB(170, 176, 184);

        LOGBRUSH iconLogBrush{};
        iconLogBrush.lbStyle = BS_SOLID;
        iconLogBrush.lbColor = color;
        HPEN iconPen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND, 2, &iconLogBrush, 0, nullptr);
        HGDIOBJ previousPen = SelectObject(hdc, iconPen);
        HGDIOBJ previousBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));

        DrawToolbarIcon(hdc, localButton, button.icon, color);

        SelectObject(hdc, previousBrush);
        SelectObject(hdc, previousPen);
        DeleteObject(iconPen);
    }
}

std::optional<ToolbarAction> EditorToolbar::HitTest(POINT pt) const {
    for (const auto& button : buttons_) {
        if (ContainsPoint(button.bounds, pt)) {
            if (!button.enabled) {
                return std::nullopt;
            }
            return button.action;
        }
    }
    return std::nullopt;
}

void EditorToolbar::MoveBy(int dx, int dy, const RECT& virtualScreenRect) {
    if (utils::Width(toolbarBounds_) <= 0 || utils::Height(toolbarBounds_) <= 0) {
        return;
    }

    const int minDx = virtualScreenRect.left - toolbarBounds_.left;
    const int maxDx = virtualScreenRect.right - toolbarBounds_.right;
    const int minDy = virtualScreenRect.top - toolbarBounds_.top;
    const int maxDy = virtualScreenRect.bottom - toolbarBounds_.bottom;
    const int actualDx = (std::clamp)(dx, minDx, maxDx);
    const int actualDy = (std::clamp)(dy, minDy, maxDy);
    if (actualDx == 0 && actualDy == 0) {
        return;
    }

    OffsetRect(&toolbarBounds_, actualDx, actualDy);
    for (auto& button : buttons_) {
        OffsetRect(&button.bounds, actualDx, actualDy);
    }
}

bool EditorToolbar::IsDragSurface(POINT pt) const {
    if (!ContainsPoint(toolbarBounds_, pt)) {
        return false;
    }
    for (const auto& button : buttons_) {
        if (ContainsPoint(button.bounds, pt)) {
            return false;
        }
    }
    return true;
}

void EditorToolbar::SetCurrentTool(ToolType tool) {
    currentTool_ = tool;
}

void EditorToolbar::SetHistoryState(bool canUndo, bool canRedo) {
    for (auto& button : buttons_) {
        if (button.action == ToolbarAction::Undo) {
            button.enabled = canUndo;
        } else if (button.action == ToolbarAction::Redo) {
            button.enabled = canRedo;
        }
    }
}

RECT EditorToolbar::Bounds() const {
    return toolbarBounds_;
}

std::optional<RECT> EditorToolbar::ButtonBounds(ToolbarAction action) const {
    for (const auto& button : buttons_) {
        if (button.action == action) {
            return button.bounds;
        }
    }
    return std::nullopt;
}

std::optional<RECT> EditorToolbar::FirstPlaceholderBounds() const {
    for (const auto& button : buttons_) {
        if (button.action == ToolbarAction::Placeholder) {
            return button.bounds;
        }
    }
    return std::nullopt;
}

const std::vector<ToolbarButton>& EditorToolbar::Buttons() const {
    return buttons_;
}

} // namespace mysnip::editor
