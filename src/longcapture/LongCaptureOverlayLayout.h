#pragma once

#include <windows.h>

namespace mysnip::longcapture {

enum class LongCaptureOverlayAction {
    None,
    Cancel,
    Confirm
};

struct LongCaptureOverlayLayout {
    RECT selection{};
    RECT virtualScreen{};
    RECT controlWindow{};
    RECT cancelButton{};
    RECT confirmButton{};

    static LongCaptureOverlayLayout Create(RECT selectionRect, RECT virtualScreenRect);
    LongCaptureOverlayAction HitTest(POINT screenPoint) const;
};

} // namespace mysnip::longcapture
