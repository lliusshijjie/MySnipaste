#pragma once

#include "image/ImageBuffer.h"

#include <optional>
#include <windows.h>

namespace mysnip::editor {

class EditorViewport {
public:
    void Layout(const image::ImageBuffer& image, RECT anchorScreenRect, RECT virtualScreenRect, bool longImage);
    std::optional<POINT> ScreenToImage(POINT screenPoint) const;
    POINT ClampScreenToImage(POINT screenPoint) const;
    RECT ImageRectToClient(RECT imageRect) const;
    POINT ImagePointToClient(POINT imagePoint) const;
    RECT DisplayRect() const;
    RECT DisplayClientRect() const;
    RECT ViewportRect() const;
    RECT ViewportClientRect() const;
    float Scale() const;
    void ScrollBy(int dy);
    void ZoomAt(POINT screenPoint, float factor);

private:
    void UpdateDisplayRect();
    void ClampScroll();

    int imageWidth_ = 0;
    int imageHeight_ = 0;
    RECT anchorScreenRect_{};
    RECT virtualScreenRect_{};
    RECT viewportScreenRect_{};
    RECT displayScreenRect_{};
    float scale_ = 1.0f;
    int scrollY_ = 0;
    bool longImage_ = false;
};

} // namespace mysnip::editor
