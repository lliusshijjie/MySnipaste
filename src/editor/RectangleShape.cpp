#include "editor/RectangleShape.h"

#include "editor/ImageRenderUtils.h"
#include "editor/ShapeGeometry.h"

#include <memory>
#include <windows.h>

namespace mysnip::editor {

RectangleShape::RectangleShape(RECT shapeRect) : rect(shapeRect) {}

void RectangleShape::Draw(RenderContext& ctx) const {
    if (!ctx.hdc) {
        return;
    }
    HPEN pen = CreatePen(PS_SOLID, thickness, color);
    HGDIOBJ oldPen = SelectObject(ctx.hdc, pen);
    HGDIOBJ oldBrush = SelectObject(ctx.hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(
        ctx.hdc,
        ctx.imageRect.left + rect.left,
        ctx.imageRect.top + rect.top,
        ctx.imageRect.left + rect.right,
        ctx.imageRect.top + rect.bottom);
    SelectObject(ctx.hdc, oldBrush);
    SelectObject(ctx.hdc, oldPen);
    DeleteObject(pen);
}

void RectangleShape::Apply(image::ImageBuffer& image) const {
    ImageRenderUtils::ApplyShapeOntoImage(image, *this);
}

std::unique_ptr<Shape> RectangleShape::Clone() const {
    return std::make_unique<RectangleShape>(*this);
}

bool RectangleShape::HitTest(POINT point, int tolerance) const {
    return PointNearRectBorder(rect, point, tolerance);
}

RECT RectangleShape::Bounds() const {
    return rect;
}

void RectangleShape::MoveBy(int dx, int dy, SIZE imageSize) {
    rect = ClampMovedRect(OffsetRectCopy(rect, dx, dy), imageSize);
}

bool RectangleShape::ApplyStyle(const EditorStyle& style) {
    color = style.strokeColor;
    thickness = style.strokeWidth;
    return true;
}

EditorStyle RectangleShape::Style() const {
    EditorStyle style;
    style.strokeColor = color;
    style.strokeWidth = thickness;
    return style;
}

} // namespace mysnip::editor
