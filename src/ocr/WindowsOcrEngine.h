#pragma once

#include "ocr/IOcrEngine.h"

namespace mysnip::ocr {

class WindowsOcrEngine : public IOcrEngine {
public:
    OcrAvailability Availability() const override;
    std::optional<OcrResult> Recognize(const image::ImageBuffer& image) override;
};

} // namespace mysnip::ocr
