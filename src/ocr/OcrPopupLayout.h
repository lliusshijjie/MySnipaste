#pragma once

#include <algorithm>
#include <windows.h>

namespace mysnip::ocr {

enum class OcrPopupPlacement {
    Above,
    Below
};

struct OcrPopupLayout {
    RECT windowRect{};
    RECT bodyRect{};
    RECT titleRect{};
    RECT closeButton{};
    RECT textRect{};
    RECT secondaryButton{};
    RECT primaryButton{};
    POINT pointerTip{};
    OcrPopupPlacement placement = OcrPopupPlacement::Above;
};

inline OcrPopupLayout ComputeOcrPopupLayout(
    RECT anchorScreenRect,
    RECT workArea,
    UINT dpi) {
    const auto scale = [dpi](int value) {
        return MulDiv(value, static_cast<int>(dpi), 96);
    };

    const int preferredWidth = scale(324);
    const int minimumWidth = scale(280);
    const int bodyHeight = scale(206);
    const int pointerHeight = scale(8);
    const int anchorGap = scale(14);
    const int margin = scale(16);
    const int titleHeight = scale(24);
    const int closeSize = scale(24);
    const int textTopGap = scale(12);
    const int buttonWidth = scale(56);
    const int buttonHeight = scale(30);
    const int buttonGap = scale(12);
    const int bottomMargin = scale(16);

    const int workWidth = static_cast<int>(workArea.right - workArea.left);
    const int width = (std::max)(
        (std::min)(preferredWidth, workWidth),
        (std::min)(minimumWidth, workWidth));
    const int height = bodyHeight + pointerHeight;
    const int anchorCenter =
        static_cast<int>(anchorScreenRect.left + anchorScreenRect.right) / 2;
    int left = anchorCenter - width / 2;
    left = (std::clamp)(
        left,
        static_cast<int>(workArea.left),
        static_cast<int>(workArea.right) - width);

    const bool fitsAbove =
        static_cast<int>(anchorScreenRect.top) - anchorGap - height >=
        static_cast<int>(workArea.top);
    const OcrPopupPlacement placement =
        fitsAbove ? OcrPopupPlacement::Above : OcrPopupPlacement::Below;
    int top = placement == OcrPopupPlacement::Above
        ? static_cast<int>(anchorScreenRect.top) - anchorGap - height
        : static_cast<int>(anchorScreenRect.bottom) + anchorGap;
    top = (std::clamp)(
        top,
        static_cast<int>(workArea.top),
        static_cast<int>(workArea.bottom) - height);

    const int bodyTop =
        placement == OcrPopupPlacement::Below ? pointerHeight : 0;
    const int bodyBottom = bodyTop + bodyHeight;
    const int buttonBottom = bodyBottom - bottomMargin;
    const int buttonTop = buttonBottom - buttonHeight;
    const int primaryRight = width - margin;
    const int secondaryRight = primaryRight - buttonWidth - buttonGap;

    OcrPopupLayout layout;
    layout.windowRect = RECT{left, top, left + width, top + height};
    layout.bodyRect = RECT{0, bodyTop, width, bodyBottom};
    layout.titleRect = RECT{
        margin,
        bodyTop + margin,
        width - margin - closeSize,
        bodyTop + margin + titleHeight};
    layout.closeButton = RECT{
        width - margin - closeSize,
        bodyTop + margin,
        width - margin,
        bodyTop + margin + closeSize};
    layout.textRect = RECT{
        margin,
        layout.titleRect.bottom + textTopGap,
        width - margin,
        buttonTop - buttonGap};
    layout.secondaryButton = RECT{
        secondaryRight - buttonWidth,
        buttonTop,
        secondaryRight,
        buttonBottom};
    layout.primaryButton = RECT{
        primaryRight - buttonWidth,
        buttonTop,
        primaryRight,
        buttonBottom};
    layout.pointerTip = POINT{
        (std::clamp)(anchorCenter - left, margin, width - margin),
        placement == OcrPopupPlacement::Above ? height : 0};
    layout.placement = placement;
    return layout;
}

} // namespace mysnip::ocr
