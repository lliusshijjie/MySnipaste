#pragma once

#include <windows.h>

namespace mysnip::editor {

struct EditorStyle {
    COLORREF strokeColor = RGB(255, 0, 0);
    int strokeWidth = 2;
    int fontSize = 20;
    int mosaicBlockSize = 12;
    int blurRadius = 6;
};

} // namespace mysnip::editor
