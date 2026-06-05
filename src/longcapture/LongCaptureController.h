#pragma once

#include "capture/ICaptureBackend.h"

#include <optional>
#include <windows.h>

namespace mysnip::longcapture {

class LongCaptureController {
public:
    std::optional<capture::CaptureResult> Capture(HWND owner, capture::ICaptureBackend& backend);
};

} // namespace mysnip::longcapture
