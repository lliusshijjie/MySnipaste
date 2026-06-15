#pragma once

#include <algorithm>
#include <windows.h>

namespace mysnip::ocr {

struct OcrDialogLayout {
    RECT editor{};
    RECT copyButton{};
    RECT closeButton{};
};

inline OcrDialogLayout ComputeOcrDialogLayout(const RECT& clientRect, UINT dpi) {
    const auto scale = [dpi](int value) {
        return MulDiv(value, static_cast<int>(dpi), 96);
    };

    const int margin = scale(12);
    const int gap = scale(10);
    const int buttonWidth = scale(88);
    const int buttonHeight = scale(32);
    const int clientTop = static_cast<int>(clientRect.top);
    const int clientBottom = static_cast<int>(clientRect.bottom);
    const int buttonBottom = (std::max)(clientTop + margin, clientBottom - margin);
    const int buttonTop = (std::max)(clientTop + margin, buttonBottom - buttonHeight);

    OcrDialogLayout layout;
    layout.closeButton = RECT{
        clientRect.right - margin - buttonWidth,
        buttonTop,
        clientRect.right - margin,
        buttonBottom};
    layout.copyButton = RECT{
        layout.closeButton.left - gap - buttonWidth,
        buttonTop,
        layout.closeButton.left - gap,
        buttonBottom};
    layout.editor = RECT{
        clientRect.left + margin,
        clientRect.top + margin,
        clientRect.right - margin,
        buttonTop - gap};
    return layout;
}

} // namespace mysnip::ocr
