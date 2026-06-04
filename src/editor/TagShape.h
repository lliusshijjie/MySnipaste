#pragma once

#include "editor/Shape.h"

#include <string>
#include <windows.h>

namespace mysnip::editor {

struct TagShape : Shape {
    POINT position{};
    std::wstring text;
    COLORREF color = RGB(255, 0, 0);
    int fontSize = 20;
    std::wstring fontName = L"Segoe UI";

    TagShape() = default;
    TagShape(POINT tagPosition, std::wstring value);

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
