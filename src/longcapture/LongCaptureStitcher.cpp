#include "longcapture/LongCaptureStitcher.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace mysnip::longcapture {
namespace {

constexpr double kMaxAverageChannelDiff = 0.1;

double OverlapDiff(const image::ImageBuffer& stitched, const image::ImageBuffer& frame, int overlapRows) {
    unsigned long long diff = 0;
    const int widthBytes = stitched.width * 4;
    const int stitchedStartRow = stitched.height - overlapRows;
    for (int y = 0; y < overlapRows; ++y) {
        const auto* a = stitched.pixels.data() + static_cast<std::size_t>((stitchedStartRow + y) * stitched.stride);
        const auto* b = frame.pixels.data() + static_cast<std::size_t>(y * frame.stride);
        for (int x = 0; x < widthBytes; x += 4) {
            diff += static_cast<unsigned>(std::abs(static_cast<int>(a[x]) - static_cast<int>(b[x])));
            diff += static_cast<unsigned>(std::abs(static_cast<int>(a[x + 1]) - static_cast<int>(b[x + 1])));
            diff += static_cast<unsigned>(std::abs(static_cast<int>(a[x + 2]) - static_cast<int>(b[x + 2])));
        }
    }
    return static_cast<double>(diff) / static_cast<double>(overlapRows * stitched.width * 3);
}

} // namespace

LongCaptureStitcher::LongCaptureStitcher(int maxHeight)
    : maxHeight_(maxHeight) {}

bool LongCaptureStitcher::HasImage() const {
    return stitched_.IsValid();
}

const image::ImageBuffer& LongCaptureStitcher::Image() const {
    return stitched_;
}

image::ImageBuffer LongCaptureStitcher::TakeImage() {
    return std::move(stitched_);
}

StitchResult LongCaptureStitcher::AppendFrame(const image::ImageBuffer& frame) {
    if (!frame.IsValid() || maxHeight_ <= 0) {
        return StitchResult{StitchStatus::InvalidFrame};
    }
    if (!stitched_.IsValid()) {
        const int height = (std::min)(frame.height, maxHeight_);
        auto first = image::ImageBuffer::Create(frame.width, height);
        if (!first.has_value()) {
            return StitchResult{StitchStatus::InvalidFrame};
        }
        for (int y = 0; y < height; ++y) {
            std::memcpy(
                first->pixels.data() + static_cast<std::size_t>(y * first->stride),
                frame.pixels.data() + static_cast<std::size_t>(y * frame.stride),
                static_cast<std::size_t>(frame.width * 4));
        }
        stitched_ = std::move(*first);
        return StitchResult{height == frame.height ? StitchStatus::Appended : StitchStatus::MaxHeightReached, 0, height};
    }
    if (stitched_.width != frame.width) {
        return StitchResult{StitchStatus::InvalidFrame};
    }

    const auto overlap = FindBestOverlap(frame);
    if (!overlap.has_value()) {
        return StitchResult{StitchStatus::LowConfidence};
    }
    if (*overlap == frame.height) {
        return StitchResult{StitchStatus::RepeatedFrame, *overlap, 0};
    }
    return AppendRows(frame, *overlap);
}

StitchResult LongCaptureStitcher::AppendFrameWithoutOverlap(const image::ImageBuffer& frame) {
    if (!frame.IsValid()) {
        return StitchResult{StitchStatus::InvalidFrame};
    }
    if (!stitched_.IsValid()) {
        return AppendFrame(frame);
    }
    if (stitched_.width != frame.width) {
        return StitchResult{StitchStatus::InvalidFrame};
    }
    return AppendRows(frame, 0);
}

std::optional<int> LongCaptureStitcher::FindBestOverlap(const image::ImageBuffer& frame) const {
    const int maxOverlap = (std::min)(stitched_.height, frame.height);
    int bestOverlap = 0;
    double bestDiff = std::numeric_limits<double>::max();
    for (int overlap = maxOverlap; overlap >= 1; --overlap) {
        const double diff = OverlapDiff(stitched_, frame, overlap);
        if (diff < bestDiff) {
            bestDiff = diff;
            bestOverlap = overlap;
        }
        if (diff <= kMaxAverageChannelDiff) {
            return overlap;
        }
    }
    if (bestDiff <= kMaxAverageChannelDiff) {
        return bestOverlap;
    }
    return std::nullopt;
}

StitchResult LongCaptureStitcher::AppendRows(const image::ImageBuffer& frame, int overlapRows) {
    const int availableRows = frame.height - overlapRows;
    if (availableRows <= 0) {
        return StitchResult{StitchStatus::RepeatedFrame, overlapRows, 0};
    }

    const int allowedRows = (std::min)(availableRows, maxHeight_ - stitched_.height);
    if (allowedRows <= 0) {
        return StitchResult{StitchStatus::MaxHeightReached, overlapRows, 0};
    }

    auto next = image::ImageBuffer::Create(stitched_.width, stitched_.height + allowedRows);
    if (!next.has_value()) {
        return StitchResult{StitchStatus::InvalidFrame, overlapRows, 0};
    }
    std::memcpy(next->pixels.data(), stitched_.pixels.data(), stitched_.pixels.size());
    for (int y = 0; y < allowedRows; ++y) {
        std::memcpy(
            next->pixels.data() + static_cast<std::size_t>((stitched_.height + y) * next->stride),
            frame.pixels.data() + static_cast<std::size_t>((overlapRows + y) * frame.stride),
            static_cast<std::size_t>(frame.width * 4));
    }
    stitched_ = std::move(*next);
    return StitchResult{
        allowedRows == availableRows ? StitchStatus::Appended : StitchStatus::MaxHeightReached,
        overlapRows,
        allowedRows};
}

} // namespace mysnip::longcapture
