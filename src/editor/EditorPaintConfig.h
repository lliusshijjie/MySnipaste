#pragma once

#include <windows.h>

namespace mysnip::editor {

constexpr bool kEraseBackgroundBeforeEditorPaint = false;
constexpr bool kEditorUsesTransparentBackground = true;
constexpr COLORREF kEditorTransparentBackgroundColor = RGB(255, 0, 255);

} // namespace mysnip::editor
