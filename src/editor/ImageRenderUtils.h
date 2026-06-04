#pragma once

#include "editor/Shape.h"
#include "image/ImageBuffer.h"

#include <windows.h>

namespace mysnip::editor {

class ImageRenderUtils {
public:
    static bool DrawImage(HDC hdc, const image::ImageBuffer& image, const RECT& dstRect);
    // Rasterize a shape onto the pixel buffer through an in-memory DIB (GDI).
    static void ApplyShapeOntoImage(image::ImageBuffer& image, const Shape& shape);
};

} // namespace mysnip::editor
