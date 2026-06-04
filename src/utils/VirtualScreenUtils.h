#pragma once

#include <algorithm>
#include <windows.h>

namespace mysnip::utils {

inline RECT GetVirtualScreenRect() {
    RECT rect{};
    rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    rect.right = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    rect.bottom = rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
    return rect;
}

inline POINT ScreenToVirtualClientPoint(POINT screenPoint, const RECT& virtualScreen) {
    return POINT{
        screenPoint.x - virtualScreen.left,
        screenPoint.y - virtualScreen.top,
    };
}

inline RECT ScreenRectToVirtualClientRect(RECT screenRect, const RECT& virtualScreen) {
    return RECT{
        screenRect.left - virtualScreen.left,
        screenRect.top - virtualScreen.top,
        screenRect.right - virtualScreen.left,
        screenRect.bottom - virtualScreen.top,
    };
}

inline RECT ClampRectToBounds(RECT rect, const RECT& bounds) {
    rect.left = (std::clamp)(rect.left, bounds.left, bounds.right);
    rect.right = (std::clamp)(rect.right, bounds.left, bounds.right);
    rect.top = (std::clamp)(rect.top, bounds.top, bounds.bottom);
    rect.bottom = (std::clamp)(rect.bottom, bounds.top, bounds.bottom);
    return rect;
}

} // namespace mysnip::utils
