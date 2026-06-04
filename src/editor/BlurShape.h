#pragma once

#include "editor/Shape.h"

#include <windows.h>

namespace mysnip::editor {

struct BlurShape : Shape {
    RECT rect{};
    int radius = 6;

    BlurShape() = default;
    explicit BlurShape(RECT blurRect, int blurRadius = 6);

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
