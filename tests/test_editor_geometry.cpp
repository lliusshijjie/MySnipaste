#include "test_framework.h"

#include "editor/EditorGeometry.h"

using mysnip::editor::ClientPointToImagePoint;
using mysnip::editor::ClampPointToImageBounds;
using mysnip::editor::NormalizeImageRect;
using mysnip::editor::ScreenPointToImagePoint;

TEST_CASE(EditorCoordinates_ConvertsScreenPointToImagePoint) {
    const RECT screenRect{100, 200, 900, 600};
    const auto result = ScreenPointToImagePoint(POINT{300, 250}, screenRect);

    REQUIRE(result.has_value());
    REQUIRE(result->x == 200);
    REQUIRE(result->y == 50);
}

TEST_CASE(EditorCoordinates_ConvertsClientPointToImagePoint) {
    const RECT imageClientRect{50, 80, 850, 480};
    const auto result = ClientPointToImagePoint(POINT{300, 250}, imageClientRect);

    REQUIRE(result.has_value());
    REQUIRE(result->x == 250);
    REQUIRE(result->y == 170);
}

TEST_CASE(EditorCoordinates_ClampsPointToImageBounds) {
    const auto point = ClampPointToImageBounds(POINT{-5, 999}, 100, 50);

    REQUIRE(point.x == 0);
    REQUIRE(point.y == 49);
}

TEST_CASE(EditorCoordinates_RejectsPointOutsideImage) {
    const RECT screenRect{100, 200, 900, 600};

    REQUIRE(!ScreenPointToImagePoint(POINT{99, 250}, screenRect).has_value());
    REQUIRE(!ScreenPointToImagePoint(POINT{300, 600}, screenRect).has_value());
}

TEST_CASE(RectTool_NormalizesDragInAllDirections) {
    const RECT rect = NormalizeImageRect(POINT{80, 10}, POINT{20, 50});

    REQUIRE(rect.left == 20);
    REQUIRE(rect.top == 10);
    REQUIRE(rect.right == 80);
    REQUIRE(rect.bottom == 50);
}
