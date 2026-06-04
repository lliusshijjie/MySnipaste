#include "editor/LineShape.h"

#include "editor/ImageRenderUtils.h"
#include "editor/ShapeGeometry.h"

#include <memory>
#include <windows.h>

namespace mysnip::editor {

LineShape::LineShape(POINT startPoint, POINT endPoint) : start(startPoint), end(endPoint) {}

void LineShape::Draw(RenderContext& ctx) const {
    if (!ctx.hdc) {
        return;
    }
    HPEN pen = CreatePen(PS_SOLID, thickness, color);
    HGDIOBJ oldPen = SelectObject(ctx.hdc, pen);
    MoveToEx(ctx.hdc, ctx.imageRect.left + start.x, ctx.imageRect.top + start.y, nullptr);
    LineTo(ctx.hdc, ctx.imageRect.left + end.x, ctx.imageRect.top + end.y);
    SelectObject(ctx.hdc, oldPen);
    DeleteObject(pen);
}

void LineShape::Apply(image::ImageBuffer& image) const {
    ImageRenderUtils::ApplyShapeOntoImage(image, *this);
}

std::unique_ptr<Shape> LineShape::Clone() const {
    return std::make_unique<LineShape>(*this);
}

bool LineShape::HitTest(POINT point, int tolerance) const {
    return DistancePointToSegment(point, start, end) <= static_cast<double>(tolerance);
}

RECT LineShape::Bounds() const {
    return BoundsFromPoints(start, end, thickness);
}

void LineShape::MoveBy(int dx, int dy, SIZE imageSize) {
    RECT moved = ClampMovedRect(OffsetRectCopy(BoundsFromPoints(start, end), dx, dy), imageSize);
    const RECT current = BoundsFromPoints(start, end);
    const int actualDx = moved.left - current.left;
    const int actualDy = moved.top - current.top;
    start.x += actualDx;
    end.x += actualDx;
    start.y += actualDy;
    end.y += actualDy;
}

bool LineShape::ApplyStyle(const EditorStyle& style) {
    color = style.strokeColor;
    thickness = style.strokeWidth;
    return true;
}

EditorStyle LineShape::Style() const {
    EditorStyle style;
    style.strokeColor = color;
    style.strokeWidth = thickness;
    return style;
}

} // namespace mysnip::editor
