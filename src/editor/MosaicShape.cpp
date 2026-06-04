#include "editor/MosaicShape.h"

#include "editor/PixelUtils.h"
#include "editor/ShapeGeometry.h"

#include <algorithm>
#include <memory>
#include <windows.h>

namespace mysnip::editor {

MosaicShape::MosaicShape(RECT mosaicRect, int block) : rect(mosaicRect), blockSize(block) {}

void MosaicShape::Draw(RenderContext& ctx) const {
    if (!ctx.hdc) {
        return;
    }
    RECT drawRect{
        ctx.imageRect.left + rect.left,
        ctx.imageRect.top + rect.top,
        ctx.imageRect.left + rect.right,
        ctx.imageRect.top + rect.bottom,
    };
    HBRUSH brush = CreateSolidBrush(RGB(160, 160, 160));
    FillRect(ctx.hdc, &drawRect, brush);
    DeleteObject(brush);
}

void MosaicShape::Apply(image::ImageBuffer& image) const {
    const RECT target = ClampRectToImage(rect, image);
    const int block = (std::max)(1, blockSize);

    for (int by = target.top; by < target.bottom; by += block) {
        for (int bx = target.left; bx < target.right; bx += block) {
            const int right = (std::min)(bx + block, static_cast<int>(target.right));
            const int bottom = (std::min)(by + block, static_cast<int>(target.bottom));
            int count = 0;
            int sumB = 0;
            int sumG = 0;
            int sumR = 0;
            for (int y = by; y < bottom; ++y) {
                for (int x = bx; x < right; ++x) {
                    sumB += PixelChannel(image, x, y, 0);
                    sumG += PixelChannel(image, x, y, 1);
                    sumR += PixelChannel(image, x, y, 2);
                    ++count;
                }
            }
            if (count == 0) {
                continue;
            }
            const COLORREF avg = RGB(sumR / count, sumG / count, sumB / count);
            for (int y = by; y < bottom; ++y) {
                for (int x = bx; x < right; ++x) {
                    SetPixelBgra(image, x, y, avg);
                }
            }
        }
    }
}

std::unique_ptr<Shape> MosaicShape::Clone() const {
    return std::make_unique<MosaicShape>(*this);
}

bool MosaicShape::HitTest(POINT point, int tolerance) const {
    return PointInRectWithTolerance(rect, point, tolerance);
}

RECT MosaicShape::Bounds() const {
    return rect;
}

void MosaicShape::MoveBy(int dx, int dy, SIZE imageSize) {
    rect = ClampMovedRect(OffsetRectCopy(rect, dx, dy), imageSize);
}

bool MosaicShape::ApplyStyle(const EditorStyle& style) {
    blockSize = style.mosaicBlockSize;
    return true;
}

EditorStyle MosaicShape::Style() const {
    EditorStyle style;
    style.mosaicBlockSize = blockSize;
    return style;
}

} // namespace mysnip::editor
