#pragma once

#include "image/ImageBuffer.h"

#include <optional>
#include <windows.h>

namespace mysnip::capture {

struct CaptureResult {
    image::ImageBuffer image;
    RECT screenRect{};
};

class ICaptureBackend {
public:
    virtual ~ICaptureBackend() = default;
    virtual std::optional<CaptureResult> CaptureRect(const RECT& rect) = 0;
};

} // namespace mysnip::capture
