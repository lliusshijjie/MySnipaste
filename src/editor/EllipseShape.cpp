#include "editor/EllipseShape.h"

#include "editor/ImageRenderUtils.h"
#include "editor/ShapeGeometry.h"

#include <memory>
#include <windows.h>

namespace mysnip::editor {

EllipseShape::EllipseShape(RECT shapeRect) : rect(shapeRect) {}

void EllipseShape::Draw(RenderContext& ctx) const {
    if (!ctx.hdc) {
        return;
    }
    HPEN pen = CreatePen(PS_SOLID, thickness, color);
    HGDIOBJ oldPen = SelectObject(ctx.hdc, pen);
    HGDIOBJ oldBrush = SelectObject(ctx.hdc, GetStockObject(HOLLOW_BRUSH));
    Ellipse(
        ctx.hdc,
        ctx.imageRect.left + rect.left,
        ctx.imageRect.top + rect.top,
        ctx.imageRect.left + rect.right,
        ctx.imageRect.top + rect.bottom);
    SelectObject(ctx.hdc, oldBrush);
    SelectObject(ctx.hdc, oldPen);
    DeleteObject(pen);
}

void EllipseShape::Apply(image::ImageBuffer& image) const {
    ImageRenderUtils::ApplyShapeOntoImage(image, *this);
}

std::unique_ptr<Shape> EllipseShape::Clone() const {
    return std::make_unique<EllipseShape>(*this);
}

bool EllipseShape::HitTest(POINT point, int tolerance) const {
    return PointInRectWithTolerance(rect, point, tolerance);
}

RECT EllipseShape::Bounds() const {
    return rect;
}

void EllipseShape::MoveBy(int dx, int dy, SIZE imageSize) {
    rect = ClampMovedRect(OffsetRectCopy(rect, dx, dy), imageSize);
}

bool EllipseShape::ApplyStyle(const EditorStyle& style) {
    color = style.strokeColor;
    thickness = style.strokeWidth;
    return true;
}

EditorStyle EllipseShape::Style() const {
    EditorStyle style;
    style.strokeColor = color;
    style.strokeWidth = thickness;
    return style;
}

} // namespace mysnip::editor
