#pragma once

#include "capture/ICaptureBackend.h"

namespace mysnip::capture {

class GdiCaptureBackend : public ICaptureBackend {
public:
    std::optional<CaptureResult> CaptureRect(const RECT& rect) override;
};

} // namespace mysnip::capture
