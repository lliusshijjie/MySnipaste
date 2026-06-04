#include "editor/EditorStyleBar.h"

#include "utils/RectUtils.h"

#include <algorithm>
#include <array>
#include <windows.h>

namespace mysnip::editor {
namespace {

constexpr int kStyleBarHeight = 32;
constexpr int kItemSize = 24;
constexpr int kGap = 8;
constexpr int kMargin = 8;

constexpr std::array<StyleBarAction, 16> kActions{{
    StyleBarAction::ColorRed,
    StyleBarAction::ColorBlue,
    StyleBarAction::ColorYellow,
    StyleBarAction::ColorGreen,
    StyleBarAction::StrokeWidthThin,
    StyleBarAction::StrokeWidthMedium,
    StyleBarAction::StrokeWidthThick,
    StyleBarAction::FontSmall,
    StyleBarAction::FontMedium,
    StyleBarAction::FontLarge,
    StyleBarAction::MosaicWeak,
    StyleBarAction::MosaicMedium,
    StyleBarAction::MosaicStrong,
    StyleBarAction::BlurWeak,
    StyleBarAction::BlurMedium,
    StyleBarAction::BlurStrong,
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

bool IsColorAction(StyleBarAction action) {
    return action == StyleBarAction::ColorRed ||
        action == StyleBarAction::ColorBlue ||
        action == StyleBarAction::ColorYellow ||
        action == StyleBarAction::ColorGreen;
}

COLORREF ColorForAction(StyleBarAction action) {
    switch (action) {
    case StyleBarAction::ColorBlue:
        return RGB(32, 128, 255);
    case StyleBarAction::ColorYellow:
        return RGB(255, 215, 0);
    case StyleBarAction::ColorGreen:
        return RGB(50, 170, 85);
    default:
        return RGB(235, 70, 70);
    }
}

bool IsSelected(const EditorStyle& style, StyleBarAction action) {
    switch (action) {
    case StyleBarAction::ColorRed:
    case StyleBarAction::ColorBlue:
    case StyleBarAction::ColorYellow:
    case StyleBarAction::ColorGreen:
        return style.strokeColor == ColorForAction(action);
    case StyleBarAction::StrokeWidthThin:
        return style.strokeWidth == 2;
    case StyleBarAction::StrokeWidthMedium:
        return style.strokeWidth == 4;
    case StyleBarAction::StrokeWidthThick:
        return style.strokeWidth == 8;
    case StyleBarAction::FontSmall:
        return style.fontSize == 16;
    case StyleBarAction::FontMedium:
        return style.fontSize == 20;
    case StyleBarAction::FontLarge:
        return style.fontSize == 28;
    case StyleBarAction::MosaicWeak:
        return style.mosaicBlockSize == 8;
    case StyleBarAction::MosaicMedium:
        return style.mosaicBlockSize == 12;
    case StyleBarAction::MosaicStrong:
        return style.mosaicBlockSize == 24;
    case StyleBarAction::BlurWeak:
        return style.blurRadius == 3;
    case StyleBarAction::BlurMedium:
        return style.blurRadius == 6;
    case StyleBarAction::BlurStrong:
        return style.blurRadius == 10;
    }
    return false;
}

} // namespace

void EditorStyleBar::Layout(const RECT& toolbarRect, const RECT& virtualScreenRect) {
    const int naturalWidth = (kMargin * 2) + static_cast<int>(kActions.size()) * kItemSize +
        (static_cast<int>(kActions.size()) - 1) * kGap;
    const int width = (std::min)(naturalWidth, (std::max)(0, utils::Width(virtualScreenRect) - kMargin * 2));
    LONG left = toolbarRect.left + (utils::Width(toolbarRect) - width) / 2;
    left = (std::clamp)(left, virtualScreenRect.left + kMargin, virtualScreenRect.right - kMargin - width);

    LONG top = toolbarRect.bottom + 6;
    if (top + kStyleBarHeight > virtualScreenRect.bottom - kMargin) {
        top = toolbarRect.top - 6 - kStyleBarHeight;
    }
    top = (std::clamp)(top, virtualScreenRect.top + kMargin, virtualScreenRect.bottom - kMargin - kStyleBarHeight);
    bounds_ = RECT{left, top, left + width, top + kStyleBarHeight};

    items_.clear();
    LONG x = bounds_.left + kMargin;
    const LONG y = bounds_.top + (kStyleBarHeight - kItemSize) / 2;
    for (const auto action : kActions) {
        items_.push_back(StyleBarItem{action, RECT{x, y, x + kItemSize, y + kItemSize}});
        x += kItemSize + kGap;
    }
}

void EditorStyleBar::Draw(HDC hdc, POINT origin) const {
    const RECT localBounds = OffsetRectCopy(bounds_, origin);
    HBRUSH background = CreateSolidBrush(RGB(250, 250, 250));
    HPEN border = CreatePen(PS_SOLID, 1, RGB(220, 220, 220));
    HGDIOBJ oldBrush = SelectObject(hdc, background);
    HGDIOBJ oldPen = SelectObject(hdc, border);
    RoundRect(hdc, localBounds.left, localBounds.top, localBounds.right, localBounds.bottom, 7, 7);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(border);
    DeleteObject(background);

    for (const auto& item : items_) {
        const RECT r = OffsetRectCopy(item.bounds, origin);
        if (IsSelected(style_, item.action)) {
            HBRUSH selected = CreateSolidBrush(RGB(232, 236, 240));
            FillRect(hdc, &r, selected);
            DeleteObject(selected);
        }

        if (IsColorAction(item.action)) {
            HBRUSH swatch = CreateSolidBrush(ColorForAction(item.action));
            HGDIOBJ old = SelectObject(hdc, swatch);
            HGDIOBJ oldItemPen = SelectObject(hdc, GetStockObject(NULL_PEN));
            Ellipse(hdc, r.left + 5, r.top + 5, r.right - 5, r.bottom - 5);
            SelectObject(hdc, oldItemPen);
            SelectObject(hdc, old);
            DeleteObject(swatch);
        } else {
            const wchar_t* label = L"";
            switch (item.action) {
            case StyleBarAction::StrokeWidthThin:
                label = L"2";
                break;
            case StyleBarAction::StrokeWidthMedium:
                label = L"4";
                break;
            case StyleBarAction::StrokeWidthThick:
                label = L"8";
                break;
            case StyleBarAction::FontSmall:
                label = L"S";
                break;
            case StyleBarAction::FontMedium:
                label = L"M";
                break;
            case StyleBarAction::FontLarge:
                label = L"L";
                break;
            case StyleBarAction::MosaicWeak:
                label = L"W";
                break;
            case StyleBarAction::MosaicMedium:
                label = L"M";
                break;
            case StyleBarAction::MosaicStrong:
                label = L"H";
                break;
            case StyleBarAction::BlurWeak:
                label = L"b";
                break;
            case StyleBarAction::BlurMedium:
                label = L"B";
                break;
            case StyleBarAction::BlurStrong:
                label = L"BB";
                break;
            default:
                break;
            }
            const int oldMode = SetBkMode(hdc, TRANSPARENT);
            RECT textRect = r;
            DrawTextW(hdc, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            SetBkMode(hdc, oldMode);
        }
    }
}

std::optional<StyleBarAction> EditorStyleBar::HitTest(POINT point) const {
    for (const auto& item : items_) {
        if (ContainsPoint(item.bounds, point)) {
            return item.action;
        }
    }
    return std::nullopt;
}

bool EditorStyleBar::ApplyAction(StyleBarAction action) {
    switch (action) {
    case StyleBarAction::ColorRed:
    case StyleBarAction::ColorBlue:
    case StyleBarAction::ColorYellow:
    case StyleBarAction::ColorGreen:
        style_.strokeColor = ColorForAction(action);
        return true;
    case StyleBarAction::StrokeWidthThin:
        style_.strokeWidth = 2;
        return true;
    case StyleBarAction::StrokeWidthMedium:
        style_.strokeWidth = 4;
        return true;
    case StyleBarAction::StrokeWidthThick:
        style_.strokeWidth = 8;
        return true;
    case StyleBarAction::FontSmall:
        style_.fontSize = 16;
        return true;
    case StyleBarAction::FontMedium:
        style_.fontSize = 20;
        return true;
    case StyleBarAction::FontLarge:
        style_.fontSize = 28;
        return true;
    case StyleBarAction::MosaicWeak:
        style_.mosaicBlockSize = 8;
        return true;
    case StyleBarAction::MosaicMedium:
        style_.mosaicBlockSize = 12;
        return true;
    case StyleBarAction::MosaicStrong:
        style_.mosaicBlockSize = 24;
        return true;
    case StyleBarAction::BlurWeak:
        style_.blurRadius = 3;
        return true;
    case StyleBarAction::BlurMedium:
        style_.blurRadius = 6;
        return true;
    case StyleBarAction::BlurStrong:
        style_.blurRadius = 10;
        return true;
    }
    return false;
}

void EditorStyleBar::SetStyle(const EditorStyle& style) {
    style_ = style;
}

const EditorStyle& EditorStyleBar::Style() const {
    return style_;
}

RECT EditorStyleBar::Bounds() const {
    return bounds_;
}

std::optional<RECT> EditorStyleBar::ItemBounds(StyleBarAction action) const {
    for (const auto& item : items_) {
        if (item.action == action) {
            return item.bounds;
        }
    }
    return std::nullopt;
}

} // namespace mysnip::editor
