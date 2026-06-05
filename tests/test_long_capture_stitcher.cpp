#include "test_framework.h"

#include "image/ImageBuffer.h"
#include "longcapture/LongCaptureStitcher.h"

#include <cstdint>

using mysnip::image::ImageBuffer;
using mysnip::longcapture::LongCaptureStitcher;
using mysnip::longcapture::StitchStatus;

namespace {

ImageBuffer MakeRowImage(int width, int height, int firstRowValue) {
    auto image = ImageBuffer::Create(width, height);
    REQUIRE(image.has_value());
    for (int y = 0; y < height; ++y) {
        const std::uint8_t value = static_cast<std::uint8_t>(firstRowValue + y);
        for (int x = 0; x < width; ++x) {
            auto* px = image->pixels.data() + static_cast<std::size_t>(y * image->stride + x * 4);
            px[0] = value;
            px[1] = value;
            px[2] = value;
            px[3] = 255;
        }
    }
    return *image;
}

} // namespace

TEST_CASE(LongStitcher_FindsVerticalOverlap) {
    LongCaptureStitcher stitcher;
    REQUIRE(stitcher.AppendFrame(MakeRowImage(3, 6, 10)).status == StitchStatus::Appended);

    const auto result = stitcher.AppendFrame(MakeRowImage(3, 6, 13));

    REQUIRE(result.status == StitchStatus::Appended);
    REQUIRE(result.overlapRows == 3);
    REQUIRE(stitcher.Image().height == 9);
}

TEST_CASE(LongStitcher_AppendsOnlyNewRows) {
    LongCaptureStitcher stitcher;
    stitcher.AppendFrame(MakeRowImage(2, 5, 1));
    stitcher.AppendFrame(MakeRowImage(2, 5, 4));

    REQUIRE(stitcher.Image().height == 8);
    const auto* last = stitcher.Image().pixels.data() + static_cast<std::size_t>(7 * stitcher.Image().stride);
    REQUIRE(last[0] == 8);
}

TEST_CASE(LongStitcher_StopsOnRepeatedFrame) {
    LongCaptureStitcher stitcher;
    const auto frame = MakeRowImage(2, 5, 30);
    stitcher.AppendFrame(frame);

    const auto result = stitcher.AppendFrame(frame);

    REQUIRE(result.status == StitchStatus::RepeatedFrame);
    REQUIRE(stitcher.Image().height == 5);
}

TEST_CASE(LongStitcher_RejectsLowConfidenceMatch) {
    LongCaptureStitcher stitcher;
    stitcher.AppendFrame(MakeRowImage(2, 5, 1));

    const auto result = stitcher.AppendFrame(MakeRowImage(2, 5, 90));

    REQUIRE(result.status == StitchStatus::LowConfidence);
    REQUIRE(stitcher.Image().height == 5);
}

TEST_CASE(LongStitcher_CanAppendFullFrameWhenManualScrollHasNoOverlap) {
    LongCaptureStitcher stitcher;
    stitcher.AppendFrame(MakeRowImage(2, 5, 1));

    const auto result = stitcher.AppendFrameWithoutOverlap(MakeRowImage(2, 5, 90));

    REQUIRE(result.status == StitchStatus::Appended);
    REQUIRE(result.overlapRows == 0);
    REQUIRE(stitcher.Image().height == 10);
}

TEST_CASE(LongStitcher_ClampsMaxHeight) {
    LongCaptureStitcher stitcher(7);
    stitcher.AppendFrame(MakeRowImage(2, 5, 1));

    const auto result = stitcher.AppendFrame(MakeRowImage(2, 5, 4));

    REQUIRE(result.status == StitchStatus::MaxHeightReached);
    REQUIRE(stitcher.Image().height == 7);
}
