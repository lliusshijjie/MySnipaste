#include "test_framework.h"

#include "capture/CaptureOverlayState.h"

using mysnip::capture::ShouldCancelOnCaptureChanged;

TEST_CASE(CaptureOverlayState_CancelsWhenCaptureLostDuringSelection) {
    REQUIRE(ShouldCancelOnCaptureChanged(true, false));
}

TEST_CASE(CaptureOverlayState_IgnoresCaptureChangedAfterSelectionCompleted) {
    REQUIRE(!ShouldCancelOnCaptureChanged(true, true));
    REQUIRE(!ShouldCancelOnCaptureChanged(false, true));
}
