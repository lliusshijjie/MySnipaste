#include "editor/EditorViewport.h"

#include "utils/RectUtils.h"

#include <algorithm>
#include <cmath>

namespace mysnip::editor {
namespace {

constexpr float kMinScale = 0.2f;
constexpr float kMaxScale = 5.0f;

int Rounded(float value) {
    return static_cast<int>(std::lround(value));
}

} // namespace

void EditorViewport::Layout(const image::ImageBuffer& image, RECT anchorScreenRect, RECT virtualScreenRect, bool longImage) {
    imageWidth_ = image.width;
    imageHeight_ = image.height;
    anchorScreenRect_ = anchorScreenRect;
    virtualScreenRect_ = virtualScreenRect;
    viewportScreenRect_ = anchorScreenRect;
    longImage_ = longImage;
    scrollY_ = 0;

    if (!longImage_ || imageWidth_ <= 0 || imageHeight_ <= 0) {
        scale_ = 1.0f;
    } else {
        const int anchorWidth = (std::max)(1, utils::Width(anchorScreenRect_));
        scale_ = (std::clamp)(static_cast<float>(anchorWidth) / static_cast<float>(imageWidth_), kMinScale, kMaxScale);
    }
    UpdateDisplayRect();
}

std::optional<POINT> EditorViewport::ScreenToImage(POINT screenPoint) const {
    if (imageWidth_ <= 0 || imageHeight_ <= 0 || scale_ <= 0.0f) {
        return std::nullopt;
    }
    if (screenPoint.x < viewportScreenRect_.left || screenPoint.x >= viewportScreenRect_.right ||
        screenPoint.y < viewportScreenRect_.top || screenPoint.y >= viewportScreenRect_.bottom) {
        return std::nullopt;
    }

    const int x = static_cast<int>((screenPoint.x - displayScreenRect_.left) / scale_);
    const int y = static_cast<int>((screenPoint.y - displayScreenRect_.top) / scale_);
    if (x < 0 || x >= imageWidth_ || y < 0 || y >= imageHeight_) {
        return std::nullopt;
    }
    return POINT{x, y};
}

POINT EditorViewport::ClampScreenToImage(POINT screenPoint) const {
    POINT point{
        static_cast<LONG>((screenPoint.x - displayScreenRect_.left) / scale_),
        static_cast<LONG>((screenPoint.y - displayScreenRect_.top) / scale_),
    };
    point.x = (std::clamp)(point.x, 0L, static_cast<LONG>((std::max)(0, imageWidth_ - 1)));
    point.y = (std::clamp)(point.y, 0L, static_cast<LONG>((std::max)(0, imageHeight_ - 1)));
    return point;
}

RECT EditorViewport::ImageRectToClient(RECT imageRect) const {
    RECT screen{
        displayScreenRect_.left + Rounded(imageRect.left * scale_),
        displayScreenRect_.top + Rounded(imageRect.top * scale_),
        displayScreenRect_.left + Rounded(imageRect.right * scale_),
        displayScreenRect_.top + Rounded(imageRect.bottom * scale_),
    };
    return RECT{
        screen.left - virtualScreenRect_.left,
        screen.top - virtualScreenRect_.top,
        screen.right - virtualScreenRect_.left,
        screen.bottom - virtualScreenRect_.top,
    };
}

POINT EditorViewport::ImagePointToClient(POINT imagePoint) const {
    return POINT{
        displayScreenRect_.left - virtualScreenRect_.left + Rounded(imagePoint.x * scale_),
        displayScreenRect_.top - virtualScreenRect_.top + Rounded(imagePoint.y * scale_),
    };
}

RECT EditorViewport::DisplayRect() const {
    return displayScreenRect_;
}

RECT EditorViewport::DisplayClientRect() const {
    return RECT{
        displayScreenRect_.left - virtualScreenRect_.left,
        displayScreenRect_.top - virtualScreenRect_.top,
        displayScreenRect_.right - virtualScreenRect_.left,
        displayScreenRect_.bottom - virtualScreenRect_.top,
    };
}

RECT EditorViewport::ViewportRect() const {
    return viewportScreenRect_;
}

RECT EditorViewport::ViewportClientRect() const {
    return RECT{
        viewportScreenRect_.left - virtualScreenRect_.left,
        viewportScreenRect_.top - virtualScreenRect_.top,
        viewportScreenRect_.right - virtualScreenRect_.left,
        viewportScreenRect_.bottom - virtualScreenRect_.top,
    };
}

float EditorViewport::Scale() const {
    return scale_;
}

void EditorViewport::ScrollBy(int dy) {
    if (!longImage_) {
        return;
    }
    scrollY_ += dy;
    ClampScroll();
    UpdateDisplayRect();
}

void EditorViewport::ZoomAt(POINT screenPoint, float factor) {
    if (imageWidth_ <= 0 || imageHeight_ <= 0 || factor <= 0.0f) {
        return;
    }
    const float oldScale = scale_;
    const float imageX = static_cast<float>(screenPoint.x - displayScreenRect_.left) / oldScale;
    const float imageY = static_cast<float>(screenPoint.y - displayScreenRect_.top) / oldScale;

    scale_ = (std::clamp)(scale_ * factor, kMinScale, kMaxScale);
    anchorScreenRect_.left = screenPoint.x - Rounded(imageX * scale_);
    anchorScreenRect_.right = anchorScreenRect_.left + Rounded(imageWidth_ * scale_);
    scrollY_ = Rounded(imageY * scale_) - (screenPoint.y - anchorScreenRect_.top);
    ClampScroll();
    UpdateDisplayRect();
}

void EditorViewport::UpdateDisplayRect() {
    const int width = Rounded(imageWidth_ * scale_);
    const int height = Rounded(imageHeight_ * scale_);
    displayScreenRect_ = RECT{
        anchorScreenRect_.left,
        anchorScreenRect_.top - scrollY_,
        anchorScreenRect_.left + width,
        anchorScreenRect_.top - scrollY_ + height,
    };
}

void EditorViewport::ClampScroll() {
    const int visibleHeight = (std::max)(1, utils::Height(anchorScreenRect_));
    const int scaledHeight = Rounded(imageHeight_ * scale_);
    const int maxScroll = (std::max)(0, scaledHeight - visibleHeight);
    scrollY_ = (std::clamp)(scrollY_, 0, maxScroll);
}

} // namespace mysnip::editor
