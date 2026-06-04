#pragma once

#include "editor/Shape.h"

#include <windows.h>

namespace mysnip::editor {

struct MosaicShape : Shape {
    RECT rect{};
    int blockSize = 12;

    MosaicShape() = default;
    explicit MosaicShape(RECT mosaicRect, int block = 12);

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
