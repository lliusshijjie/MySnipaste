#include "editor/NumberShape.h"

#include "editor/ImageRenderUtils.h"
#include "editor/ShapeGeometry.h"

#include <cmath>
#include <memory>
#include <string>
#include <windows.h>

namespace mysnip::editor {

NumberShape::NumberShape(POINT centerPoint, int value) : center(centerPoint), number(value) {}

void NumberShape::Draw(RenderContext& ctx) const {
    if (!ctx.hdc) {
        return;
    }
    const int cx = ctx.imageRect.left + center.x;
    const int cy = ctx.imageRect.top + center.y;

    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldBrush = SelectObject(ctx.hdc, brush);
    HGDIOBJ oldPen = SelectObject(ctx.hdc, pen);
    Ellipse(ctx.hdc, cx - radius, cy - radius, cx + radius, cy + radius);
    SelectObject(ctx.hdc, oldPen);
    SelectObject(ctx.hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);

    HFONT font = CreateFontW(
        -(radius + 3),
        0,
        0,
        0,
        FW_BOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        L"Segoe UI");
    HGDIOBJ oldFont = SelectObject(ctx.hdc, font);
    const int oldBkMode = SetBkMode(ctx.hdc, TRANSPARENT);
    const COLORREF oldColor = SetTextColor(ctx.hdc, RGB(255, 255, 255));
    const std::wstring label = std::to_wstring(number);
    RECT textRect{cx - radius, cy - radius, cx + radius, cy + radius};
    DrawTextW(ctx.hdc, label.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SetTextColor(ctx.hdc, oldColor);
    SetBkMode(ctx.hdc, oldBkMode);
    SelectObject(ctx.hdc, oldFont);
    DeleteObject(font);
}

void NumberShape::Apply(image::ImageBuffer& image) const {
    ImageRenderUtils::ApplyShapeOntoImage(image, *this);
}

std::unique_ptr<Shape> NumberShape::Clone() const {
    return std::make_unique<NumberShape>(*this);
}

bool NumberShape::HitTest(POINT point, int tolerance) const {
    const int dx = point.x - center.x;
    const int dy = point.y - center.y;
    const int limit = radius + tolerance;
    return dx * dx + dy * dy <= limit * limit;
}

RECT NumberShape::Bounds() const {
    return RECT{center.x - radius, center.y - radius, center.x + radius, center.y + radius};
}

void NumberShape::MoveBy(int dx, int dy, SIZE imageSize) {
    const POINT delta = ComputeBoundedMoveDelta(Bounds(), dx, dy, imageSize);
    center.x += delta.x;
    center.y += delta.y;
}

bool NumberShape::ApplyStyle(const EditorStyle& style) {
    color = style.strokeColor;
    return true;
}

EditorStyle NumberShape::Style() const {
    EditorStyle style;
    style.strokeColor = color;
    return style;
}

} // namespace mysnip::editor
