#include "editor/ImageRenderUtils.h"

#include "editor/RenderContext.h"

#include <cstring>
#include <windows.h>

namespace mysnip::editor {

bool ImageRenderUtils::DrawImage(HDC hdc, const image::ImageBuffer& image, const RECT& dstRect) {
    if (!hdc || !image.IsValid()) {
        return false;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = image.width;
    bmi.bmiHeader.biHeight = -image.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    const int dstWidth = dstRect.right - dstRect.left;
    const int dstHeight = dstRect.bottom - dstRect.top;

    const int lines = StretchDIBits(
        hdc,
        dstRect.left,
        dstRect.top,
        dstWidth,
        dstHeight,
        0,
        0,
        image.width,
        image.height,
        image.pixels.data(),
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY);

    return lines != GDI_ERROR;
}

void ImageRenderUtils::ApplyShapeOntoImage(image::ImageBuffer& image, const Shape& shape) {
    if (!image.IsValid()) {
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
    shape.Draw(ctx);
    SelectObject(dc, oldBitmap);
    std::memcpy(image.pixels.data(), bits, image.pixels.size());

    DeleteObject(bitmap);
    DeleteDC(dc);
    ReleaseDC(nullptr, screen);
}

} // namespace mysnip::editor
