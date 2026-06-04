#pragma once

#include "editor/EditorStyle.h"

#include <optional>
#include <vector>
#include <windows.h>

namespace mysnip::editor {

enum class StyleBarAction {
    ColorRed,
    ColorBlue,
    ColorYellow,
    ColorGreen,
    StrokeWidthThin,
    StrokeWidthMedium,
    StrokeWidthThick,
    FontSmall,
    FontMedium,
    FontLarge,
    MosaicWeak,
    MosaicMedium,
    MosaicStrong,
    BlurWeak,
    BlurMedium,
    BlurStrong
};

struct StyleBarItem {
    StyleBarAction action;
    RECT bounds{};
};

class EditorStyleBar {
public:
    void Layout(const RECT& toolbarRect, const RECT& virtualScreenRect);
    void Draw(HDC hdc, POINT origin = POINT{0, 0}) const;
    std::optional<StyleBarAction> HitTest(POINT point) const;
    bool ApplyAction(StyleBarAction action);
    void SetStyle(const EditorStyle& style);
    const EditorStyle& Style() const;
    RECT Bounds() const;
    std::optional<RECT> ItemBounds(StyleBarAction action) const;

private:
    RECT bounds_{};
    std::vector<StyleBarItem> items_;
    EditorStyle style_;
};

} // namespace mysnip::editor
