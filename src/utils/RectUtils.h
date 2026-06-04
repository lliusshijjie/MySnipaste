#pragma once

#include <algorithm>
#include <windows.h>

namespace mysnip::utils {

inline RECT NormalizeRect(POINT start, POINT end) {
    RECT rect{};
    rect.left = std::min(start.x, end.x);
    rect.top = std::min(start.y, end.y);
    rect.right = std::max(start.x, end.x);
    rect.bottom = std::max(start.y, end.y);
    return rect;
}

inline int Width(const RECT& rect) {
    return rect.right - rect.left;
}

inline int Height(const RECT& rect) {
    return rect.bottom - rect.top;
}

inline bool IsTooSmall(const RECT& rect, int minSize = 3) {
    return Width(rect) < minSize || Height(rect) < minSize;
}

inline bool IsEmpty(const RECT& rect) {
    return Width(rect) <= 0 || Height(rect) <= 0;
}

} // namespace mysnip::utils
