#include "editor/BlurShape.h"

#include "editor/PixelUtils.h"
#include "editor/ShapeGeometry.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>
#include <windows.h>

namespace mysnip::editor {

BlurShape::BlurShape(RECT blurRect, int blurRadius) : rect(blurRect), radius(blurRadius) {}

void BlurShape::Draw(RenderContext& ctx) const {
    if (!ctx.hdc) {
        return;
    }
    RECT drawRect{
        ctx.imageRect.left + rect.left,
        ctx.imageRect.top + rect.top,
        ctx.imageRect.left + rect.right,
        ctx.imageRect.top + rect.bottom,
    };
    HBRUSH brush = CreateSolidBrush(RGB(225, 230, 235));
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(120, 130, 140));
    HGDIOBJ oldBrush = SelectObject(ctx.hdc, brush);
    HGDIOBJ oldPen = SelectObject(ctx.hdc, pen);
    Rectangle(ctx.hdc, drawRect.left, drawRect.top, drawRect.right, drawRect.bottom);
    SelectObject(ctx.hdc, oldPen);
    SelectObject(ctx.hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void BlurShape::Apply(image::ImageBuffer& image) const {
    if (!image.IsValid()) {
        return;
    }
    const RECT target = ClampRectToImage(rect, image);
    const int blurRadius = (std::max)(1, radius);
    const std::vector<std::uint8_t> original = image.pixels;

    for (int y = target.top; y < target.bottom; ++y) {
        for (int x = target.left; x < target.right; ++x) {
            int count = 0;
            int sumB = 0;
            int sumG = 0;
            int sumR = 0;
            const int minY = (std::max)(static_cast<int>(target.top), y - blurRadius);
            const int maxY = (std::min)(static_cast<int>(target.bottom) - 1, y + blurRadius);
            const int minX = (std::max)(static_cast<int>(target.left), x - blurRadius);
            const int maxX = (std::min)(static_cast<int>(target.right) - 1, x + blurRadius);
            for (int sy = minY; sy <= maxY; ++sy) {
                for (int sx = minX; sx <= maxX; ++sx) {
                    const auto offset = static_cast<std::size_t>(sy * image.stride + sx * 4);
                    sumB += original[offset + 0];
                    sumG += original[offset + 1];
                    sumR += original[offset + 2];
                    ++count;
                }
            }
            if (count == 0) {
                continue;
            }
            PixelChannel(image, x, y, 0) = static_cast<std::uint8_t>(sumB / count);
            PixelChannel(image, x, y, 1) = static_cast<std::uint8_t>(sumG / count);
            PixelChannel(image, x, y, 2) = static_cast<std::uint8_t>(sumR / count);
            PixelChannel(image, x, y, 3) = 255;
        }
    }
}

std::unique_ptr<Shape> BlurShape::Clone() const {
    return std::make_unique<BlurShape>(*this);
}

bool BlurShape::HitTest(POINT point, int tolerance) const {
    return PointInRectWithTolerance(rect, point, tolerance);
}

RECT BlurShape::Bounds() const {
    return rect;
}

void BlurShape::MoveBy(int dx, int dy, SIZE imageSize) {
    const POINT delta = ComputeBoundedMoveDelta(Bounds(), dx, dy, imageSize);
    rect = OffsetRectCopy(rect, delta.x, delta.y);
}

bool BlurShape::ApplyStyle(const EditorStyle& style) {
    radius = style.blurRadius;
    return true;
}

EditorStyle BlurShape::Style() const {
    EditorStyle style;
    style.blurRadius = radius;
    return style;
}

} // namespace mysnip::editor
