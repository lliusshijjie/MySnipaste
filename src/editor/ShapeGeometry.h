#pragma once

#include <algorithm>
#include <cmath>
#include <windows.h>

namespace mysnip::editor {

inline RECT OffsetRectCopy(RECT rect, int dx, int dy) {
    rect.left += dx;
    rect.right += dx;
    rect.top += dy;
    rect.bottom += dy;
    return rect;
}

inline RECT ClampMovedRect(RECT rect, SIZE imageSize) {
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    if (width <= 0 || height <= 0 || imageSize.cx <= 0 || imageSize.cy <= 0) {
        return rect;
    }

    if (rect.left < 0) {
        rect.right -= rect.left;
        rect.left = 0;
    }
    if (rect.top < 0) {
        rect.bottom -= rect.top;
        rect.top = 0;
    }
    if (rect.right > imageSize.cx) {
        const int shift = rect.right - imageSize.cx;
        rect.left -= shift;
        rect.right -= shift;
    }
    if (rect.bottom > imageSize.cy) {
        const int shift = rect.bottom - imageSize.cy;
        rect.top -= shift;
        rect.bottom -= shift;
    }
    if (width <= imageSize.cx) {
        rect.left = (std::max)(0L, rect.left);
        rect.right = rect.left + width;
    }
    if (height <= imageSize.cy) {
        rect.top = (std::max)(0L, rect.top);
        rect.bottom = rect.top + height;
    }
    return rect;
}

inline bool PointNearRectBorder(RECT rect, POINT point, int tolerance) {
    const bool insideExpanded =
        point.x >= rect.left - tolerance && point.x <= rect.right + tolerance &&
        point.y >= rect.top - tolerance && point.y <= rect.bottom + tolerance;
    const bool insideInner =
        point.x > rect.left + tolerance && point.x < rect.right - tolerance &&
        point.y > rect.top + tolerance && point.y < rect.bottom - tolerance;
    return insideExpanded && !insideInner;
}

inline bool PointInRectWithTolerance(RECT rect, POINT point, int tolerance) {
    return point.x >= rect.left - tolerance && point.x <= rect.right + tolerance &&
        point.y >= rect.top - tolerance && point.y <= rect.bottom + tolerance;
}

inline double DistancePointToSegment(POINT point, POINT start, POINT end) {
    const double vx = static_cast<double>(end.x - start.x);
    const double vy = static_cast<double>(end.y - start.y);
    const double wx = static_cast<double>(point.x - start.x);
    const double wy = static_cast<double>(point.y - start.y);
    const double lengthSquared = vx * vx + vy * vy;
    if (lengthSquared <= 0.0) {
        const double dx = static_cast<double>(point.x - start.x);
        const double dy = static_cast<double>(point.y - start.y);
        return std::sqrt(dx * dx + dy * dy);
    }

    const double t = (std::max)(0.0, (std::min)(1.0, (wx * vx + wy * vy) / lengthSquared));
    const double projectionX = static_cast<double>(start.x) + t * vx;
    const double projectionY = static_cast<double>(start.y) + t * vy;
    const double dx = static_cast<double>(point.x) - projectionX;
    const double dy = static_cast<double>(point.y) - projectionY;
    return std::sqrt(dx * dx + dy * dy);
}

inline RECT BoundsFromPoints(POINT a, POINT b, int padding = 0) {
    return RECT{
        (std::min)(a.x, b.x) - padding,
        (std::min)(a.y, b.y) - padding,
        (std::max)(a.x, b.x) + padding,
        (std::max)(a.y, b.y) + padding,
    };
}

} // namespace mysnip::editor
