#pragma once

#include "editor/Shape.h"

#include <windows.h>

namespace mysnip::editor {

struct LineShape : Shape {
    POINT start{};
    POINT end{};
    COLORREF color = RGB(255, 0, 0);
    int thickness = 2;

    LineShape() = default;
    LineShape(POINT startPoint, POINT endPoint);

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
