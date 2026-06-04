#pragma once

#include "image/ImageBuffer.h"

#include <optional>
#include <windows.h>

namespace mysnip::editor {

enum class EditorStatus {
    Confirmed,
    Cancelled,
    Pinned,
    Failed
};

struct EditorResult {
    EditorStatus status = EditorStatus::Failed;
    std::optional<image::ImageBuffer> finalImage;
    std::optional<POINT> pinPosition;
};

} // namespace mysnip::editor
