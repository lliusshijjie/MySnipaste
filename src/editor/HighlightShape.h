#pragma once

#include "editor/Shape.h"

#include <windows.h>

namespace mysnip::editor {

struct HighlightShape : Shape {
    RECT rect{};
    COLORREF color = RGB(255, 255, 0);
    int alpha = 90;

    HighlightShape() = default;
    explicit HighlightShape(RECT highlightRect);

    void Draw(RenderContext& ctx) const override;
    void Apply(image::ImageBuffer& image) const override;
    std::unique_ptr<Shape> Clone() const override;
    bool HitTest(POINT point, int tolerance) const override;
    RECT Bounds() const override;
    void MoveBy(int dx, int dy, SIZE imageSize) override;
    bool ApplyStyle(const EditorStyle& style) override;
    EditorStyle Style() const override;
};

} // namespace mysnip::editor
