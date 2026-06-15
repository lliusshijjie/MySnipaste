#include "test_framework.h"

#include "ocr/OcrPopupLayout.h"

namespace {

bool Contains(const RECT& outer, const RECT& inner) {
    return inner.left >= outer.left &&
        inner.top >= outer.top &&
        inner.right <= outer.right &&
        inner.bottom <= outer.bottom;
}

} // namespace

TEST_CASE(OcrPopupLayout_PlacesPopupAboveAnchorWhenSpaceAllows) {
    const auto layout = mysnip::ocr::ComputeOcrPopupLayout(
        RECT{400, 400, 800, 650},
        RECT{0, 0, 1200, 800},
        96);

    REQUIRE(layout.placement == mysnip::ocr::OcrPopupPlacement::Above);
    REQUIRE(layout.windowRect.bottom < 400);
}

TEST_CASE(OcrPopupLayout_FallsBelowWhenAboveWouldOverflow) {
    const auto layout = mysnip::ocr::ComputeOcrPopupLayout(
        RECT{300, 20, 700, 250},
        RECT{0, 0, 1200, 800},
        96);

    REQUIRE(layout.placement == mysnip::ocr::OcrPopupPlacement::Below);
    REQUIRE(layout.windowRect.top > 250);
}

TEST_CASE(OcrPopupLayout_ClampsWindowInsideNegativeWorkArea) {
    const RECT workArea{-1600, -200, 1920, 1080};
    const auto layout = mysnip::ocr::ComputeOcrPopupLayout(
        RECT{-1590, 300, -1400, 600},
        workArea,
        96);

    REQUIRE(Contains(workArea, layout.windowRect));
}

TEST_CASE(OcrPopupLayout_KeepsAllControlsInsideClientArea) {
    const auto layout = mysnip::ocr::ComputeOcrPopupLayout(
        RECT{400, 400, 800, 650},
        RECT{0, 0, 1200, 800},
        96);
    const RECT client{
        0,
        0,
        layout.windowRect.right - layout.windowRect.left,
        layout.windowRect.bottom - layout.windowRect.top};

    REQUIRE(Contains(client, layout.titleRect));
    REQUIRE(Contains(client, layout.closeButton));
    REQUIRE(Contains(client, layout.textRect));
    REQUIRE(Contains(client, layout.secondaryButton));
    REQUIRE(Contains(client, layout.primaryButton));
}

TEST_CASE(OcrPopupLayout_ScalesAtHighDpi) {
    const auto normal = mysnip::ocr::ComputeOcrPopupLayout(
        RECT{500, 500, 900, 800},
        RECT{0, 0, 2400, 1600},
        96);
    const auto highDpi = mysnip::ocr::ComputeOcrPopupLayout(
        RECT{1000, 1000, 1800, 1600},
        RECT{0, 0, 4800, 3200},
        192);

    REQUIRE(
        highDpi.windowRect.right - highDpi.windowRect.left ==
        (normal.windowRect.right - normal.windowRect.left) * 2);
    REQUIRE(
        highDpi.primaryButton.bottom - highDpi.primaryButton.top ==
        (normal.primaryButton.bottom - normal.primaryButton.top) * 2);
}
