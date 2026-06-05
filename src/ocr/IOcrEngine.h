#pragma once

#include "image/ImageBuffer.h"

#include <optional>
#include <string>

namespace mysnip::ocr {

enum class OcrAvailability {
    Available,
    MissingPackageIdentity,
    Unsupported
};

struct OcrResult {
    std::wstring text;
};

class IOcrEngine {
public:
    virtual ~IOcrEngine() = default;
    virtual OcrAvailability Availability() const = 0;
    virtual std::optional<OcrResult> Recognize(const image::ImageBuffer& image) = 0;
};

} // namespace mysnip::ocr
