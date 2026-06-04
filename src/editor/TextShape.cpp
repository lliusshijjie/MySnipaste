#include "editor/TextShape.h"

#include "editor/ShapeGeometry.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <utility>
#include <windows.h>

namespace mysnip::editor {

TextShape::TextShape(POINT textPosition, std::wstring value)
    : position(textPosition), text(std::move(value)) {}

void TextShape::Draw(RenderContext& ctx) const {
    if (!ctx.hdc || text.empty()) {
        return;
    }
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
    RECT textRect{
        ctx.imageRect.left + position.x,
        ctx.imageRect.top + position.y,
        ctx.imageRect.left + position.x + 600,
        ctx.imageRect.top + position.y + 400,
    };
    DrawTextW(ctx.hdc, text.c_str(), -1, &textRect, DT_LEFT | DT_TOP | DT_NOPREFIX | DT_WORDBREAK);
    SetTextColor(ctx.hdc, oldColor);
    SetBkMode(ctx.hdc, oldBkMode);
    SelectObject(ctx.hdc, oldFont);
    DeleteObject(font);
}

void TextShape::Apply(image::ImageBuffer& image) const {
    if (!image.IsValid() || text.empty()) {
        return;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = image.width;
    bmi.bmiHeader.biHeight = -image.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC screen = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dc || !bitmap || !bits) {
        if (bitmap) {
            DeleteObject(bitmap);
        }
        if (dc) {
            DeleteDC(dc);
        }
        if (screen) {
            ReleaseDC(nullptr, screen);
        }
        return;
    }

    std::memcpy(bits, image.pixels.data(), image.pixels.size());
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    RenderContext ctx{dc, RECT{0, 0, image.width, image.height}, 1.0f};
    Draw(ctx);
    SelectObject(dc, oldBitmap);
    std::memcpy(image.pixels.data(), bits, image.pixels.size());

    DeleteObject(bitmap);
    DeleteDC(dc);
    ReleaseDC(nullptr, screen);
}

std::unique_ptr<Shape> TextShape::Clone() const {
    return std::make_unique<TextShape>(*this);
}

bool TextShape::HitTest(POINT point, int tolerance) const {
    return PointInRectWithTolerance(Bounds(), point, tolerance);
}

RECT TextShape::Bounds() const {
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
    const int width = (std::max)(24, maxLineLength * (std::max)(8, fontSize / 2));
    const int height = (std::max)(fontSize + 8, lineCount * (fontSize + 6));
    return RECT{position.x, position.y, position.x + width, position.y + height};
}

void TextShape::MoveBy(int dx, int dy, SIZE imageSize) {
    const RECT moved = ClampMovedRect(OffsetRectCopy(Bounds(), dx, dy), imageSize);
    position = POINT{moved.left, moved.top};
}

bool TextShape::ApplyStyle(const EditorStyle& style) {
    color = style.strokeColor;
    fontSize = style.fontSize;
    return true;
}

EditorStyle TextShape::Style() const {
    EditorStyle style;
    style.strokeColor = color;
    style.fontSize = fontSize;
    return style;
}

} // namespace mysnip::editor
