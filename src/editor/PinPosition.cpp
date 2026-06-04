#include "editor/PinPosition.h"

namespace mysnip::editor {

POINT ComputePinnedImagePosition(const capture::CaptureResult& captureResult, const EditorDocument& document) {
    POINT position{captureResult.screenRect.left, captureResult.screenRect.top};
    if (document.cropRect.has_value()) {
        position.x += document.cropRect->left;
        position.y += document.cropRect->top;
    }
    return position;
}

} // namespace mysnip::editor
