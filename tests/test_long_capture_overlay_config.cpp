#include "test_framework.h"

#include "longcapture/LongCaptureOverlayConfig.h"

TEST_CASE(LongCaptureOverlayConfig_UsesSemiTransparentMask) {
    REQUIRE(mysnip::longcapture::kLongCaptureMaskAlpha > 0);
    REQUIRE(mysnip::longcapture::kLongCaptureMaskAlpha < 255);
}
