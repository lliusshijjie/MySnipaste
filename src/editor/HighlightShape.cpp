#include "editor/HighlightShape.h"

#include "editor/PixelUtils.h"
#include "editor/ShapeGeometry.h"

#include <memory>
#include <windows.h>

namespace mysnip::editor {

HighlightShape::HighlightShape(RECT highlightRect) : rect(highlightRect) {}

void HighlightShape::Draw(RenderContext& ctx) const {
    if (!ctx.hdc) {
        return;
    }
    RECT drawRect{
        ctx.imageRect.left + rect.left,
        ctx.imageRect.top + rect.top,
        ctx.imageRect.left + rect.right,
        ctx.imageRect.top + rect.bottom,
    };
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(ctx.hdc, &drawRect, brush);
    DeleteObject(brush);
}

void HighlightShape::Apply(image::ImageBuffer& image) const {
    const RECT target = ClampRectToImage(rect, image);
    for (int y = target.top; y < target.bottom; ++y) {
        for (int x = target.left; x < target.right; ++x) {
            BlendPixelBgra(image, x, y, color, alpha);
        }
    }
}

std::unique_ptr<Shape> HighlightShape::Clone() const {
    return std::make_unique<HighlightShape>(*this);
}

bool HighlightShape::HitTest(POINT point, int tolerance) const {
    return PointInRectWithTolerance(rect, point, tolerance);
}

RECT HighlightShape::Bounds() const {
    return rect;
}

void HighlightShape::MoveBy(int dx, int dy, SIZE imageSize) {
    const POINT delta = ComputeBoundedMoveDelta(Bounds(), dx, dy, imageSize);
    rect = OffsetRectCopy(rect, delta.x, delta.y);
}

bool HighlightShape::ApplyStyle(const EditorStyle& style) {
    color = style.strokeColor;
    return true;
}

EditorStyle HighlightShape::Style() const {
    EditorStyle style;
    style.strokeColor = color;
    return style;
}

} // namespace mysnip::editor
