#pragma once

#include <algorithm>
#include <optional>
#include <windows.h>

namespace mysnip::editor {

inline bool ContainsPointExclusive(const RECT& rect, POINT point) {
    return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
}

inline std::optional<POINT> PointToLocal(POINT point, const RECT& rect) {
    if (!ContainsPointExclusive(rect, point)) {
        return std::nullopt;
    }
    return POINT{point.x - rect.left, point.y - rect.top};
}

inline std::optional<POINT> ScreenPointToImagePoint(POINT screenPoint, const RECT& imageScreenRect) {
    return PointToLocal(screenPoint, imageScreenRect);
}

inline std::optional<POINT> ClientPointToImagePoint(POINT clientPoint, const RECT& imageClientRect) {
    return PointToLocal(clientPoint, imageClientRect);
}

inline POINT ClampPointToImageBounds(POINT point, int width, int height) {
    return POINT{
        (std::clamp)(point.x, 0L, static_cast<LONG>(width - 1)),
        (std::clamp)(point.y, 0L, static_cast<LONG>(height - 1)),
    };
}

inline RECT NormalizeImageRect(POINT start, POINT end) {
    return RECT{
        (std::min)(start.x, end.x),
        (std::min)(start.y, end.y),
        (std::max)(start.x, end.x),
        (std::max)(start.y, end.y),
    };
}

inline RECT ClampRectToImageBounds(RECT rect, int width, int height) {
    rect.left = (std::clamp)(rect.left, 0L, static_cast<LONG>(width));
    rect.right = (std::clamp)(rect.right, 0L, static_cast<LONG>(width));
    rect.top = (std::clamp)(rect.top, 0L, static_cast<LONG>(height));
    rect.bottom = (std::clamp)(rect.bottom, 0L, static_cast<LONG>(height));
    return rect;
}

} // namespace mysnip::editor
