#include "longcapture/LongCaptureOverlayLayout.h"

#include <algorithm>

namespace mysnip::longcapture {
namespace {

constexpr int kButtonSize = 32;
constexpr int kGap = 8;
constexpr int kMargin = 10;
constexpr int kControlPadding = 8;

bool ContainsPoint(const RECT& rect, POINT point) {
    return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
}

} // namespace

LongCaptureOverlayLayout LongCaptureOverlayLayout::Create(RECT selectionRect, RECT virtualScreenRect) {
    LongCaptureOverlayLayout layout;
    layout.selection = selectionRect;
    layout.virtualScreen = virtualScreenRect;

    const LONG totalWidth = kButtonSize * 2 + kGap + kControlPadding * 2;
    const LONG totalHeight = kButtonSize + kControlPadding * 2;
    LONG left = selectionRect.right - totalWidth;
    LONG top = selectionRect.bottom + kMargin;

    left = (std::clamp)(left, virtualScreenRect.left + kMargin, virtualScreenRect.right - kMargin - totalWidth);
    top = (std::clamp)(top, virtualScreenRect.top + kMargin, virtualScreenRect.bottom - kMargin - totalHeight);

    layout.controlWindow = RECT{left, top, left + totalWidth, top + totalHeight};
    const LONG buttonTop = top + kControlPadding;
    const LONG buttonLeft = left + kControlPadding;
    layout.cancelButton = RECT{buttonLeft, buttonTop, buttonLeft + kButtonSize, buttonTop + kButtonSize};
    layout.confirmButton = RECT{
        buttonLeft + kButtonSize + kGap,
        buttonTop,
        buttonLeft + kButtonSize * 2 + kGap,
        buttonTop + kButtonSize};
    return layout;
}

LongCaptureOverlayAction LongCaptureOverlayLayout::HitTest(POINT screenPoint) const {
    if (ContainsPoint(cancelButton, screenPoint)) {
        return LongCaptureOverlayAction::Cancel;
    }
    if (ContainsPoint(confirmButton, screenPoint)) {
        return LongCaptureOverlayAction::Confirm;
    }
    return LongCaptureOverlayAction::None;
}

} // namespace mysnip::longcapture
