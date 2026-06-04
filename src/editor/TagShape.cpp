#include "editor/TagShape.h"

#include "editor/ImageRenderUtils.h"
#include "editor/ShapeGeometry.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <windows.h>

namespace mysnip::editor {
namespace {

COLORREF LightBackgroundFor(COLORREF color) {
    return RGB(
        (GetRValue(color) + 255 * 4) / 5,
        (GetGValue(color) + 255 * 4) / 5,
        (GetBValue(color) + 255 * 4) / 5);
}

} // namespace

TagShape::TagShape(POINT tagPosition, std::wstring value)
    : position(tagPosition), text(std::move(value)) {}

void TagShape::Draw(RenderContext& ctx) const {
    if (!ctx.hdc || text.empty()) {
        return;
    }

    RECT bounds = Bounds();
    OffsetRect(&bounds, ctx.imageRect.left, ctx.imageRect.top);

    HBRUSH background = CreateSolidBrush(LightBackgroundFor(color));
    HPEN border = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldBrush = SelectObject(ctx.hdc, background);
    HGDIOBJ oldPen = SelectObject(ctx.hdc, border);
    RoundRect(ctx.hdc, bounds.left, bounds.top, bounds.right, bounds.bottom, 8, 8);
    SelectObject(ctx.hdc, oldPen);
    SelectObject(ctx.hdc, oldBrush);
    DeleteObject(border);
    DeleteObject(background);

    HFONT font = CreateFontW(
        -fontSize,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        fontName.c_str());
    HGDIOBJ oldFont = SelectObject(ctx.hdc, font);
    const int oldBkMode = SetBkMode(ctx.hdc, TRANSPARENT);
    const COLORREF oldColor = SetTextColor(ctx.hdc, color);
    RECT textRect{bounds.left + 8, bounds.top + 5, bounds.right - 8, bounds.bottom - 5};
    DrawTextW(ctx.hdc, text.c_str(), -1, &textRect, DT_LEFT | DT_TOP | DT_NOPREFIX | DT_WORDBREAK);
    SetTextColor(ctx.hdc, oldColor);
    SetBkMode(ctx.hdc, oldBkMode);
    SelectObject(ctx.hdc, oldFont);
    DeleteObject(font);
}

void TagShape::Apply(image::ImageBuffer& image) const {
    ImageRenderUtils::ApplyShapeOntoImage(image, *this);
}

std::unique_ptr<Shape> TagShape::Clone() const {
    return std::make_unique<TagShape>(*this);
}

bool TagShape::HitTest(POINT point, int tolerance) const {
    return PointInRectWithTolerance(Bounds(), point, tolerance);
}

RECT TagShape::Bounds() const {
    int lineCount = 1;
    int maxLineLength = 0;
    int currentLength = 0;
    for (wchar_t ch : text) {
        if (ch == L'\r') {
            continue;
        }
        if (ch == L'\n') {
            maxLineLength = (std::max)(maxLineLength, currentLength);
            currentLength = 0;
            ++lineCount;
            continue;
        }
        ++currentLength;
    }
    maxLineLength = (std::max)(maxLineLength, currentLength);
    const int width = (std::max)(44, maxLineLength * (std::max)(8, fontSize / 2) + 18);
    const int height = (std::max)(fontSize + 14, lineCount * (fontSize + 6) + 10);
    return RECT{position.x, position.y, position.x + width, position.y + height};
}

void TagShape::MoveBy(int dx, int dy, SIZE imageSize) {
    const RECT moved = ClampMovedRect(OffsetRectCopy(Bounds(), dx, dy), imageSize);
    position = POINT{moved.left, moved.top};
}

bool TagShape::ApplyStyle(const EditorStyle& style) {
    color = style.strokeColor;
    fontSize = style.fontSize;
    return true;
}

EditorStyle TagShape::Style() const {
    EditorStyle style;
    style.strokeColor = color;
    style.fontSize = fontSize;
    return style;
}

} // namespace mysnip::editor
