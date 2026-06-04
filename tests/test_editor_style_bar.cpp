#include "test_framework.h"

#include "editor/EditorStyleBar.h"

using mysnip::editor::EditorStyleBar;
using mysnip::editor::StyleBarAction;

namespace {

POINT CenterOf(const RECT& rect) {
    return POINT{(rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2};
}

} // namespace

TEST_CASE(EditorStyleBar_LayoutPlacesBelowToolbar) {
    EditorStyleBar bar;

    bar.Layout(RECT{100, 200, 500, 240}, RECT{0, 0, 800, 600});
    const RECT bounds = bar.Bounds();

    REQUIRE(bounds.top == 246);
    REQUIRE(bounds.bottom == 278);
}

TEST_CASE(EditorStyleBar_HitTestReturnsStrokeWidth) {
    EditorStyleBar bar;
    bar.Layout(RECT{100, 200, 500, 240}, RECT{0, 0, 800, 600});

    const auto item = bar.ItemBounds(StyleBarAction::StrokeWidthMedium);
    REQUIRE(item.has_value());

    const auto action = bar.HitTest(CenterOf(*item));
    REQUIRE(action.has_value());
    REQUIRE(*action == StyleBarAction::StrokeWidthMedium);
}

TEST_CASE(EditorStyleBar_AppliesMosaicStrength) {
    EditorStyleBar bar;
    bar.Layout(RECT{100, 200, 500, 240}, RECT{0, 0, 800, 600});

    auto style = bar.Style();
    REQUIRE(style.mosaicBlockSize == 12);

    REQUIRE(bar.ApplyAction(StyleBarAction::MosaicStrong));
    style = bar.Style();
    REQUIRE(style.mosaicBlockSize == 24);
}

TEST_CASE(EditorStyleBar_AppliesBlurStrength) {
    EditorStyleBar bar;
    bar.Layout(RECT{100, 200, 500, 240}, RECT{0, 0, 800, 600});

    auto style = bar.Style();
    REQUIRE(style.blurRadius == 6);

    REQUIRE(bar.ApplyAction(StyleBarAction::BlurStrong));
    style = bar.Style();
    REQUIRE(style.blurRadius == 10);
}

TEST_CASE(EditorStyleBar_HitTestReturnsBlurStrength) {
    EditorStyleBar bar;
    bar.Layout(RECT{100, 200, 500, 240}, RECT{0, 0, 800, 600});

    const auto item = bar.ItemBounds(StyleBarAction::BlurMedium);
    REQUIRE(item.has_value());

    REQUIRE(bar.HitTest(CenterOf(*item)) == StyleBarAction::BlurMedium);
}

TEST_CASE(EditorStyleBar_IgnoresClickOutside) {
    EditorStyleBar bar;
    bar.Layout(RECT{100, 200, 500, 240}, RECT{0, 0, 800, 600});

    REQUIRE(!bar.HitTest(POINT{10, 10}).has_value());
}
