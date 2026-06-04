#pragma once

#include "editor/EditorDocument.h"

#include <optional>
#include <windows.h>

namespace mysnip::editor {

class EditorRenderer {
public:
    static bool DrawPreview(HDC hdc, const EditorDocument& document, const RECT& imageClientRect, const Shape* previewShape);
    static std::optional<image::ImageBuffer> RenderFinalImage(const EditorDocument& document);
    static std::optional<image::ImageBuffer> RenderImageWithoutCrop(const EditorDocument& document, const Shape* previewShape = nullptr);
};

} // namespace mysnip::editor
