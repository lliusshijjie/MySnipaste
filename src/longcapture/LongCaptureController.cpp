#include "longcapture/LongCaptureController.h"

#include "capture/CaptureOverlayWindow.h"
#include "longcapture/LongCaptureOverlayWindow.h"
#include "utils/LogUtils.h"
#include "utils/RectUtils.h"

#include <windows.h>

namespace mysnip::longcapture {

std::optional<capture::CaptureResult> LongCaptureController::Capture(HWND owner, capture::ICaptureBackend& backend) {
    capture::CaptureOverlayWindow overlay;
    const auto selection = overlay.SelectRegion(owner);
    if (selection.status != capture::SelectionStatus::Completed || utils::IsTooSmall(selection.screenRect)) {
        return std::nullopt;
    }

    Sleep(30);
    const auto firstFrame = backend.CaptureRect(selection.screenRect);
    if (!firstFrame.has_value()) {
        utils::LogError(L"[LongCapture] Capture first frame failed.");
        return std::nullopt;
    }

    LongCaptureOverlayWindow captureWindow;
    return captureWindow.ShowModal(owner, selection.screenRect, backend, firstFrame->image);
}

} // namespace mysnip::longcapture
