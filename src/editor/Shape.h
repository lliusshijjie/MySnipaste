#pragma once

#include "editor/EditorStyle.h"
#include "editor/RenderContext.h"
#include "image/ImageBuffer.h"

#include <memory>
#include <windows.h>

namespace mysnip::editor {

class Shape {
public:
    virtual ~Shape() = default;
    virtual void Draw(RenderContext& ctx) const = 0;
    virtual void Apply(image::ImageBuffer& image) const = 0;
    virtual std::unique_ptr<Shape> Clone() const = 0;
    virtual bool HitTest(POINT point, int tolerance) const = 0;
    virtual RECT Bounds() const = 0;
    virtual void MoveBy(int dx, int dy, SIZE imageSize) = 0;
    virtual bool ApplyStyle(const EditorStyle& style) { return false; }
    virtual EditorStyle Style() const { return {}; }
};

} // namespace mysnip::editor
