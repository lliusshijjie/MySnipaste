#include "test_framework.h"

#include "utils/VirtualScreenUtils.h"

using mysnip::utils::ClampRectToBounds;
using mysnip::utils::ScreenRectToVirtualClientRect;
using mysnip::utils::ScreenToVirtualClientPoint;

TEST_CASE(VirtualScreen_AllowsNegativeCoordinates) {
    const RECT virtualScreen{-1920, -100, 1920, 1080};
    const RECT screenRect{-1800, -50, -1200, 300};

    const RECT clientRect = ScreenRectToVirtualClientRect(screenRect, virtualScreen);

    REQUIRE(clientRect.left == 120);
    REQUIRE(clientRect.top == 50);
    REQUIRE(clientRect.right == 720);
    REQUIRE(clientRect.bottom == 400);
}

TEST_CASE(VirtualScreen_ConvertsScreenToClientWithNegativeOrigin) {
    const RECT virtualScreen{-1920, -100, 1920, 1080};
    const POINT screenPoint{-1910, -90};

    const POINT clientPoint = ScreenToVirtualClientPoint(screenPoint, virtualScreen);

    REQUIRE(clientPoint.x == 10);
    REQUIRE(clientPoint.y == 10);
}

TEST_CASE(VirtualScreen_ClampsRectInsideBounds) {
    const RECT bounds{-100, -50, 500, 300};
    const RECT input{-200, -20, 600, 400};

    const RECT clamped = ClampRectToBounds(input, bounds);

    REQUIRE(clamped.left == -100);
    REQUIRE(clamped.top == -20);
    REQUIRE(clamped.right == 500);
    REQUIRE(clamped.bottom == 300);
}
