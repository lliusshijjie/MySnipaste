#pragma once

#include "image/ImageBuffer.h"

#include <optional>

namespace mysnip::longcapture {

enum class StitchStatus {
    Appended,
    RepeatedFrame,
    LowConfidence,
    MaxHeightReached,
    InvalidFrame
};

struct StitchResult {
    StitchStatus status = StitchStatus::InvalidFrame;
    int overlapRows = 0;
    int appendedRows = 0;
};

class LongCaptureStitcher {
public:
    explicit LongCaptureStitcher(int maxHeight = 30000);

    StitchResult AppendFrame(const image::ImageBuffer& frame);
    StitchResult AppendFrameWithoutOverlap(const image::ImageBuffer& frame);
    const image::ImageBuffer& Image() const;
    image::ImageBuffer TakeImage();
    bool HasImage() const;

private:
    std::optional<int> FindBestOverlap(const image::ImageBuffer& frame) const;
    StitchResult AppendRows(const image::ImageBuffer& frame, int overlapRows);

    image::ImageBuffer stitched_;
    int maxHeight_ = 30000;
};

} // namespace mysnip::longcapture
