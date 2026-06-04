#include "editor/FreehandShape.h"

#include "editor/ImageRenderUtils.h"
#include "editor/ShapeGeometry.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <windows.h>

namespace mysnip::editor {

FreehandShape::FreehandShape(std::vector<POINT> pathPoints) : points(std::move(pathPoints)) {}

void FreehandShape::Draw(RenderContext& ctx) const {
    if (!ctx.hdc || points.size() < 2) {
        return;
    }

    LOGBRUSH brush{};
    brush.lbStyle = BS_SOLID;
    brush.lbColor = color;
    HPEN pen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND, (std::max)(1, thickness), &brush, 0, nullptr);
    HGDIOBJ oldPen = SelectObject(ctx.hdc, pen);
    MoveToEx(ctx.hdc, ctx.imageRect.left + points.front().x, ctx.imageRect.top + points.front().y, nullptr);
    for (std::size_t i = 1; i < points.size(); ++i) {
        LineTo(ctx.hdc, ctx.imageRect.left + points[i].x, ctx.imageRect.top + points[i].y);
    }
    SelectObject(ctx.hdc, oldPen);
    DeleteObject(pen);
}

void FreehandShape::Apply(image::ImageBuffer& image) const {
    ImageRenderUtils::ApplyShapeOntoImage(image, *this);
}

std::unique_ptr<Shape> FreehandShape::Clone() const {
    return std::make_unique<FreehandShape>(*this);
}

bool FreehandShape::HitTest(POINT point, int tolerance) const {
    if (points.size() < 2) {
        return false;
    }
    const int effectiveTolerance = tolerance + (std::max)(1, thickness) / 2;
    for (std::size_t i = 1; i < points.size(); ++i) {
        if (DistancePointToSegment(point, points[i - 1], points[i]) <= static_cast<double>(effectiveTolerance)) {
            return true;
        }
    }
    return false;
}

RECT FreehandShape::Bounds() const {
    if (points.empty()) {
        return RECT{0, 0, 0, 0};
    }
    LONG left = points.front().x;
    LONG right = points.front().x;
    LONG top = points.front().y;
    LONG bottom = points.front().y;
    for (const POINT pt : points) {
        left = (std::min)(left, pt.x);
        right = (std::max)(right, pt.x);
        top = (std::min)(top, pt.y);
        bottom = (std::max)(bottom, pt.y);
    }
    const int padding = (std::max)(1, thickness);
    return RECT{left - padding, top - padding, right + padding, bottom + padding};
}

void FreehandShape::MoveBy(int dx, int dy, SIZE imageSize) {
    const RECT current = Bounds();
    const RECT moved = ClampMovedRect(OffsetRectCopy(current, dx, dy), imageSize);
    const int actualDx = moved.left - current.left;
    const int actualDy = moved.top - current.top;
    for (auto& point : points) {
        point.x += actualDx;
        point.y += actualDy;
    }
}

bool FreehandShape::ApplyStyle(const EditorStyle& style) {
    color = style.strokeColor;
    thickness = style.strokeWidth;
    return true;
}

EditorStyle FreehandShape::Style() const {
    EditorStyle style;
    style.strokeColor = color;
    style.strokeWidth = thickness;
    return style;
}

} // namespace mysnip::editor
