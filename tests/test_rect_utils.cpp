#include "test_framework.h"

#include "utils/RectUtils.h"

#include <windows.h>

using mysnip::utils::Height;
using mysnip::utils::IsTooSmall;
using mysnip::utils::NormalizeRect;
using mysnip::utils::Width;

TEST_CASE(RectUtils_NormalizesDraggedRect) {
    const POINT start{120, 90};
    const POINT end{10, 20};

    const RECT rect = NormalizeRect(start, end);

    REQUIRE(rect.left == 10);
    REQUIRE(rect.top == 20);
    REQUIRE(rect.right == 120);
    REQUIRE(rect.bottom == 90);
    REQUIRE(Width(rect) == 110);
    REQUIRE(Height(rect) == 70);
}

TEST_CASE(RectUtils_AllowsNegativeVirtualCoordinates) {
    const POINT start{-1920, -100};
    const POINT end{-1200, 500};

    const RECT rect = NormalizeRect(start, end);

    REQUIRE(rect.left == -1920);
    REQUIRE(rect.top == -100);
    REQUIRE(rect.right == -1200);
    REQUIRE(rect.bottom == 500);
    REQUIRE(Width(rect) == 720);
    REQUIRE(Height(rect) == 600);
}

TEST_CASE(RectUtils_RejectsTooSmallSelection) {
    const RECT tiny{0, 0, 2, 20};
    const RECT ok{0, 0, 3, 3};

    REQUIRE(IsTooSmall(tiny));
    REQUIRE(!IsTooSmall(ok));
}

