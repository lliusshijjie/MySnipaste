#include "test_framework.h"

#include "longcapture/LongCaptureOverlayLayout.h"

using mysnip::longcapture::LongCaptureOverlayLayout;
using mysnip::longcapture::LongCaptureOverlayAction;

namespace {

POINT CenterOf(const RECT& rect) {
    return POINT{(rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2};
}

} // namespace

TEST_CASE(LongCaptureOverlayLayout_PlacesControlWindowBelowSelectionBottomRight) {
    const RECT selection{100, 100, 500, 400};
    const RECT virtualScreen{0, 0, 800, 600};

    const auto layout = LongCaptureOverlayLayout::Create(selection, virtualScreen);

    REQUIRE(layout.controlWindow.right == selection.right);
    REQUIRE(layout.controlWindow.top > selection.bottom);
    REQUIRE(layout.cancelButton.left >= layout.controlWindow.left);
    REQUIRE(layout.confirmButton.right <= layout.controlWindow.right);
    REQUIRE(layout.confirmButton.left > layout.cancelButton.left);
}

TEST_CASE(LongCaptureOverlayLayout_ClampsButtonsInsideVirtualScreen) {
    const RECT selection{700, 520, 790, 590};
    const RECT virtualScreen{0, 0, 800, 600};

    const auto layout = LongCaptureOverlayLayout::Create(selection, virtualScreen);

    REQUIRE(layout.controlWindow.left >= virtualScreen.left);
    REQUIRE(layout.controlWindow.right <= virtualScreen.right);
    REQUIRE(layout.controlWindow.bottom <= virtualScreen.bottom);
}

TEST_CASE(LongCaptureOverlayLayout_HitTestsConfirmAndCancel) {
    const auto layout = LongCaptureOverlayLayout::Create(RECT{100, 100, 500, 400}, RECT{0, 0, 800, 600});

    REQUIRE(layout.HitTest(CenterOf(layout.cancelButton)) == LongCaptureOverlayAction::Cancel);
    REQUIRE(layout.HitTest(CenterOf(layout.confirmButton)) == LongCaptureOverlayAction::Confirm);
    REQUIRE(layout.HitTest(POINT{10, 10}) == LongCaptureOverlayAction::None);
}
