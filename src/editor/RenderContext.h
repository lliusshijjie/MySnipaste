#pragma once

#include <windows.h>

namespace mysnip::editor {

struct RenderContext {
    HDC hdc = nullptr;
    RECT imageRect{};
    float scale = 1.0f;
};

} // namespace mysnip::editor
