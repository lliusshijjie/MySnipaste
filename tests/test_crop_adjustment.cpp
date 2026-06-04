#include "test_framework.h"

#include "editor/CropGeometry.h"

using mysnip::editor::CropHandle;
using mysnip::editor::HitTestCropHandle;
using mysnip::editor::MoveCropRect;
using mysnip::editor::ResizeCropRect;

TEST_CASE(CropGeometry_HitTestsCornerHandle) {
    const RECT crop{20, 30, 120, 90};

    REQUIRE(HitTestCropHandle(crop, POINT{22, 32}, 6) == CropHandle::TopLeft);
}

TEST_CASE(CropGeometry_HitTestsInsideMove) {
    const RECT crop{20, 30, 120, 90};

    REQUIRE(HitTestCropHandle(crop, POINT{60, 50}, 6) == CropHandle::Move);
}

TEST_CASE(CropGeometry_MoveClampsToImageBounds) {
    const RECT crop{20, 30, 120, 90};
    const RECT moved = MoveCropRect(crop, 200, 100, SIZE{160, 120});

    REQUIRE(moved.right == 160);
    REQUIRE(moved.bottom == 120);
    REQUIRE(moved.left == 60);
    REQUIRE(moved.top == 60);
}

TEST_CASE(CropGeometry_ResizeKeepsMinimumSize) {
    const RECT crop{20, 30, 120, 90};
    const RECT resized = ResizeCropRect(crop, CropHandle::BottomRight, POINT{22, 32}, SIZE{200, 200}, 12);

    REQUIRE(resized.right - resized.left == 12);
    REQUIRE(resized.bottom - resized.top == 12);
}
