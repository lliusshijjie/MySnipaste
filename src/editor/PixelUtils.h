#pragma once

#include "image/ImageBuffer.h"

#include <algorithm>
#include <cstdint>
#include <windows.h>

namespace mysnip::editor {

inline bool IsInsideImage(const image::ImageBuffer& image, int x, int y) {
    return image.IsValid() && x >= 0 && x < image.width && y >= 0 && y < image.height;
}

inline std::uint8_t& PixelChannel(image::ImageBuffer& image, int x, int y, int channel) {
    return image.pixels[static_cast<std::size_t>(y * image.stride + x * 4 + channel)];
}

inline void SetPixelBgra(image::ImageBuffer& image, int x, int y, COLORREF color) {
    if (!IsInsideImage(image, x, y)) {
        return;
    }
    PixelChannel(image, x, y, 0) = static_cast<std::uint8_t>(GetBValue(color));
    PixelChannel(image, x, y, 1) = static_cast<std::uint8_t>(GetGValue(color));
    PixelChannel(image, x, y, 2) = static_cast<std::uint8_t>(GetRValue(color));
    PixelChannel(image, x, y, 3) = 255;
}

inline void BlendPixelBgra(image::ImageBuffer& image, int x, int y, COLORREF color, int alpha) {
    if (!IsInsideImage(image, x, y)) {
        return;
    }
    alpha = (std::clamp)(alpha, 0, 255);
    const int invAlpha = 255 - alpha;
    PixelChannel(image, x, y, 0) = static_cast<std::uint8_t>((GetBValue(color) * alpha + PixelChannel(image, x, y, 0) * invAlpha) / 255);
    PixelChannel(image, x, y, 1) = static_cast<std::uint8_t>((GetGValue(color) * alpha + PixelChannel(image, x, y, 1) * invAlpha) / 255);
    PixelChannel(image, x, y, 2) = static_cast<std::uint8_t>((GetRValue(color) * alpha + PixelChannel(image, x, y, 2) * invAlpha) / 255);
    PixelChannel(image, x, y, 3) = 255;
}

inline RECT NormalizeRectLocal(RECT rect) {
    if (rect.left > rect.right) {
        std::swap(rect.left, rect.right);
    }
    if (rect.top > rect.bottom) {
        std::swap(rect.top, rect.bottom);
    }
    return rect;
}

inline RECT ClampRectToImage(RECT rect, const image::ImageBuffer& image) {
    rect = NormalizeRectLocal(rect);
    rect.left = (std::clamp)(rect.left, 0L, static_cast<LONG>(image.width));
    rect.right = (std::clamp)(rect.right, 0L, static_cast<LONG>(image.width));
    rect.top = (std::clamp)(rect.top, 0L, static_cast<LONG>(image.height));
    rect.bottom = (std::clamp)(rect.bottom, 0L, static_cast<LONG>(image.height));
    return rect;
}

} // namespace mysnip::editor
