#pragma once

#include <algorithm>
#include <cstdlib>
#include <windows.h>

namespace mysnip::editor {

enum class CropHandle {
    None,
    Move,
    TopLeft,
    Top,
    TopRight,
    Right,
    BottomRight,
    Bottom,
    BottomLeft,
    Left
};

inline bool NearPoint(POINT a, POINT b, int tolerance) {
    return std::abs(a.x - b.x) <= tolerance && std::abs(a.y - b.y) <= tolerance;
}

inline bool NearHorizontal(int y, int left, int right, POINT point, int tolerance) {
    return std::abs(point.y - y) <= tolerance && point.x >= left - tolerance && point.x <= right + tolerance;
}

inline bool NearVertical(int x, int top, int bottom, POINT point, int tolerance) {
    return std::abs(point.x - x) <= tolerance && point.y >= top - tolerance && point.y <= bottom + tolerance;
}

inline CropHandle HitTestCropHandle(RECT crop, POINT point, int tolerance) {
    if (NearPoint(point, POINT{crop.left, crop.top}, tolerance)) {
        return CropHandle::TopLeft;
    }
    if (NearPoint(point, POINT{crop.right, crop.top}, tolerance)) {
        return CropHandle::TopRight;
    }
    if (NearPoint(point, POINT{crop.right, crop.bottom}, tolerance)) {
        return CropHandle::BottomRight;
    }
    if (NearPoint(point, POINT{crop.left, crop.bottom}, tolerance)) {
        return CropHandle::BottomLeft;
    }
    if (NearHorizontal(crop.top, crop.left, crop.right, point, tolerance)) {
        return CropHandle::Top;
    }
    if (NearVertical(crop.right, crop.top, crop.bottom, point, tolerance)) {
        return CropHandle::Right;
    }
    if (NearHorizontal(crop.bottom, crop.left, crop.right, point, tolerance)) {
        return CropHandle::Bottom;
    }
    if (NearVertical(crop.left, crop.top, crop.bottom, point, tolerance)) {
        return CropHandle::Left;
    }
    if (point.x > crop.left && point.x < crop.right && point.y > crop.top && point.y < crop.bottom) {
        return CropHandle::Move;
    }
    return CropHandle::None;
}

inline RECT MoveCropRect(RECT crop, int dx, int dy, SIZE imageSize) {
    const int width = crop.right - crop.left;
    const int height = crop.bottom - crop.top;
    LONG left = crop.left + dx;
    LONG top = crop.top + dy;
    const LONG maxLeft = (std::max)(0L, static_cast<LONG>(imageSize.cx - width));
    const LONG maxTop = (std::max)(0L, static_cast<LONG>(imageSize.cy - height));
    left = (std::clamp)(left, 0L, maxLeft);
    top = (std::clamp)(top, 0L, maxTop);
    return RECT{left, top, left + width, top + height};
}

inline RECT ResizeCropRect(RECT crop, CropHandle handle, POINT point, SIZE imageSize, int minSize) {
    point.x = (std::clamp)(point.x, 0L, static_cast<LONG>(imageSize.cx));
    point.y = (std::clamp)(point.y, 0L, static_cast<LONG>(imageSize.cy));

    switch (handle) {
    case CropHandle::TopLeft:
        crop.left = (std::min)(point.x, crop.right - minSize);
        crop.top = (std::min)(point.y, crop.bottom - minSize);
        break;
    case CropHandle::Top:
        crop.top = (std::min)(point.y, crop.bottom - minSize);
        break;
    case CropHandle::TopRight:
        crop.right = (std::max)(point.x, crop.left + minSize);
        crop.top = (std::min)(point.y, crop.bottom - minSize);
        break;
    case CropHandle::Right:
        crop.right = (std::max)(point.x, crop.left + minSize);
        break;
    case CropHandle::BottomRight:
        crop.right = (std::max)(point.x, crop.left + minSize);
        crop.bottom = (std::max)(point.y, crop.top + minSize);
        break;
    case CropHandle::Bottom:
        crop.bottom = (std::max)(point.y, crop.top + minSize);
        break;
    case CropHandle::BottomLeft:
        crop.left = (std::min)(point.x, crop.right - minSize);
        crop.bottom = (std::max)(point.y, crop.top + minSize);
        break;
    case CropHandle::Left:
        crop.left = (std::min)(point.x, crop.right - minSize);
        break;
    default:
        break;
    }

    crop.left = (std::clamp)(crop.left, 0L, static_cast<LONG>(imageSize.cx));
    crop.right = (std::clamp)(crop.right, 0L, static_cast<LONG>(imageSize.cx));
    crop.top = (std::clamp)(crop.top, 0L, static_cast<LONG>(imageSize.cy));
    crop.bottom = (std::clamp)(crop.bottom, 0L, static_cast<LONG>(imageSize.cy));
    return crop;
}

} // namespace mysnip::editor
