#include "editor/ArrowShape.h"

#include "editor/PixelUtils.h"
#include "editor/ShapeGeometry.h"

#include <cmath>
#include <memory>
#include <windows.h>

namespace mysnip::editor {
namespace {

void DrawCpuLine(image::ImageBuffer& image, POINT start, POINT end, COLORREF color, int thickness) {
    const int dx = std::abs(end.x - start.x);
    const int sx = start.x < end.x ? 1 : -1;
    const int dy = -std::abs(end.y - start.y);
    const int sy = start.y < end.y ? 1 : -1;
    int err = dx + dy;
    POINT p = start;
    const int radius = (std::max)(0, thickness / 2);

    while (true) {
        for (int yy = -radius; yy <= radius; ++yy) {
            for (int xx = -radius; xx <= radius; ++xx) {
                SetPixelBgra(image, p.x + xx, p.y + yy, color);
            }
        }
        if (p.x == end.x && p.y == end.y) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            p.x += sx;
        }
        if (e2 <= dx) {
            err += dx;
            p.y += sy;
        }
    }
}

void DrawArrowHeadCpu(image::ImageBuffer& image, POINT start, POINT end, COLORREF color, int thickness) {
    const double angle = std::atan2(static_cast<double>(end.y - start.y), static_cast<double>(end.x - start.x));
    constexpr double kWing = 0.72;
    constexpr double kLength = 14.0;
    POINT left{
        static_cast<LONG>(end.x - std::cos(angle - kWing) * kLength),
        static_cast<LONG>(end.y - std::sin(angle - kWing) * kLength),
    };
    POINT right{
        static_cast<LONG>(end.x - std::cos(angle + kWing) * kLength),
        static_cast<LONG>(end.y - std::sin(angle + kWing) * kLength),
    };
    DrawCpuLine(image, end, left, color, thickness);
    DrawCpuLine(image, end, right, color, thickness);
}

} // namespace

ArrowShape::ArrowShape(POINT startPoint, POINT endPoint) : start(startPoint), end(endPoint) {}

void ArrowShape::Draw(RenderContext& ctx) const {
    if (!ctx.hdc) {
        return;
    }
    HPEN pen = CreatePen(PS_SOLID, thickness, color);
    HGDIOBJ oldPen = SelectObject(ctx.hdc, pen);
    const POINT s{ctx.imageRect.left + start.x, ctx.imageRect.top + start.y};
    const POINT e{ctx.imageRect.left + end.x, ctx.imageRect.top + end.y};
    MoveToEx(ctx.hdc, s.x, s.y, nullptr);
    LineTo(ctx.hdc, e.x, e.y);

    const double angle = std::atan2(static_cast<double>(e.y - s.y), static_cast<double>(e.x - s.x));
    constexpr double kWing = 0.72;
    constexpr double kLength = 14.0;
    POINT left{
        static_cast<LONG>(e.x - std::cos(angle - kWing) * kLength),
        static_cast<LONG>(e.y - std::sin(angle - kWing) * kLength),
    };
    POINT right{
        static_cast<LONG>(e.x - std::cos(angle + kWing) * kLength),
        static_cast<LONG>(e.y - std::sin(angle + kWing) * kLength),
    };
    MoveToEx(ctx.hdc, e.x, e.y, nullptr);
    LineTo(ctx.hdc, left.x, left.y);
    MoveToEx(ctx.hdc, e.x, e.y, nullptr);
    LineTo(ctx.hdc, right.x, right.y);

    SelectObject(ctx.hdc, oldPen);
    DeleteObject(pen);
}

void ArrowShape::Apply(image::ImageBuffer& image) const {
    DrawCpuLine(image, start, end, color, thickness);
    DrawArrowHeadCpu(image, start, end, color, thickness);
}

std::unique_ptr<Shape> ArrowShape::Clone() const {
    return std::make_unique<ArrowShape>(*this);
}

bool ArrowShape::HitTest(POINT point, int tolerance) const {
    return DistancePointToSegment(point, start, end) <= static_cast<double>(tolerance);
}

RECT ArrowShape::Bounds() const {
    return BoundsFromPoints(start, end, thickness + 14);
}

void ArrowShape::MoveBy(int dx, int dy, SIZE imageSize) {
    RECT moved = ClampMovedRect(OffsetRectCopy(BoundsFromPoints(start, end), dx, dy), imageSize);
    const RECT current = BoundsFromPoints(start, end);
    const int actualDx = moved.left - current.left;
    const int actualDy = moved.top - current.top;
    start.x += actualDx;
    end.x += actualDx;
    start.y += actualDy;
    end.y += actualDy;
}

bool ArrowShape::ApplyStyle(const EditorStyle& style) {
    color = style.strokeColor;
    thickness = style.strokeWidth;
    return true;
}

EditorStyle ArrowShape::Style() const {
    EditorStyle style;
    style.strokeColor = color;
    style.strokeWidth = thickness;
    return style;
}

} // namespace mysnip::editor
