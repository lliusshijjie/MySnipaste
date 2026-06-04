#include "editor/EditorRenderer.h"

#include "editor/ImageRenderUtils.h"
#include "editor/PixelUtils.h"

#include <cstring>
#include <windows.h>

namespace mysnip::editor {
namespace {

std::optional<image::ImageBuffer> CropImage(const image::ImageBuffer& image, RECT cropRect) {
    cropRect = ClampRectToImage(cropRect, image);
    const int width = cropRect.right - cropRect.left;
    const int height = cropRect.bottom - cropRect.top;
    if (width <= 0 || height <= 0) {
        return std::nullopt;
    }

    auto output = image::ImageBuffer::Create(width, height);
    if (!output.has_value()) {
        return std::nullopt;
    }

    for (int y = 0; y < height; ++y) {
        const auto* src = image.pixels.data() + static_cast<std::size_t>((cropRect.top + y) * image.stride + cropRect.left * 4);
        auto* dst = output->pixels.data() + static_cast<std::size_t>(y * output->stride);
        std::memcpy(dst, src, static_cast<std::size_t>(width * 4));
    }
    return output;
}

} // namespace

std::optional<image::ImageBuffer> EditorRenderer::RenderImageWithoutCrop(const EditorDocument& document, const Shape* previewShape) {
    if (!document.baseImage.IsValid()) {
        return std::nullopt;
    }

    image::ImageBuffer image = document.baseImage;
    for (const auto& shape : document.shapes) {
        shape->Apply(image);
    }
    if (previewShape) {
        previewShape->Apply(image);
    }
    return image;
}

std::optional<image::ImageBuffer> EditorRenderer::RenderFinalImage(const EditorDocument& document) {
    auto image = RenderImageWithoutCrop(document);
    if (!image.has_value()) {
        return std::nullopt;
    }
    if (document.cropRect.has_value()) {
        return CropImage(*image, *document.cropRect);
    }
    return image;
}

bool EditorRenderer::DrawPreview(HDC hdc, const EditorDocument& document, const RECT& imageClientRect, const Shape* previewShape) {
    auto image = RenderImageWithoutCrop(document, previewShape);
    if (!image.has_value()) {
        return false;
    }
    return ImageRenderUtils::DrawImage(hdc, *image, imageClientRect);
}

} // namespace mysnip::editor
