#include "test_framework.h"

#include "editor/EditorToolbar.h"

#include <set>
#include <windows.h>

using mysnip::editor::ToolbarAction;
using mysnip::editor::EditorToolbar;

namespace {

POINT CenterOf(const RECT& rect) {
    return POINT{(rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2};
}

} // namespace

TEST_CASE(ToolbarLayout_PlacesBelowCaptureWhenSpaceAllows) {
    EditorToolbar toolbar;
    const RECT capture{300, 100, 700, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);
    const RECT bounds = toolbar.Bounds();

    REQUIRE(bounds.top == 308);
    REQUIRE(bounds.bottom == 348);
    REQUIRE((bounds.left + bounds.right) / 2 == 500);
}

TEST_CASE(ToolbarLayout_PlacesAboveCaptureWhenBelowWouldOverflow) {
    EditorToolbar toolbar;
    const RECT capture{100, 700, 500, 790};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);
    const RECT bounds = toolbar.Bounds();

    REQUIRE(bounds.bottom == 692);
    REQUIRE(bounds.top == 652);
}

TEST_CASE(ToolbarLayout_ClampsToVirtualScreenHorizontally) {
    EditorToolbar toolbar;
    const RECT capture{0, 100, 40, 200};
    const RECT virtualScreen{0, 0, 300, 500};

    toolbar.Layout(capture, virtualScreen);
    const RECT bounds = toolbar.Bounds();

    REQUIRE(bounds.left == 8);
    REQUIRE(bounds.right <= 292);
}

TEST_CASE(ToolbarLayout_ClampsToVirtualScreenVertically) {
    EditorToolbar toolbar;
    const RECT capture{50, 10, 250, 90};
    const RECT virtualScreen{0, 0, 300, 120};

    toolbar.Layout(capture, virtualScreen);
    const RECT bounds = toolbar.Bounds();

    REQUIRE(bounds.top >= 8);
    REQUIRE(bounds.bottom <= 112);
}

TEST_CASE(ToolbarHitTest_ReturnsConfirmForCheckButton) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);
    const auto confirm = toolbar.ButtonBounds(ToolbarAction::Confirm);
    REQUIRE(confirm.has_value());

    const auto action = toolbar.HitTest(CenterOf(*confirm));
    REQUIRE(action.has_value());
    REQUIRE(*action == ToolbarAction::Confirm);
}

TEST_CASE(ToolbarHitTest_ReturnsCancelForCloseButton) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);
    const auto cancel = toolbar.ButtonBounds(ToolbarAction::Cancel);
    REQUIRE(cancel.has_value());

    const auto action = toolbar.HitTest(CenterOf(*cancel));
    REQUIRE(action.has_value());
    REQUIRE(*action == ToolbarAction::Cancel);
}

TEST_CASE(ToolbarHitTest_ReturnsFreehandForPenButton) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);
    const auto freehand = toolbar.ButtonBounds(ToolbarAction::Freehand);
    REQUIRE(freehand.has_value());

    REQUIRE(toolbar.HitTest(CenterOf(*freehand)) == ToolbarAction::Freehand);
}

TEST_CASE(ToolbarHitTest_ReturnsTagForTagButton) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);
    const auto tag = toolbar.ButtonBounds(ToolbarAction::Tag);
    REQUIRE(tag.has_value());

    REQUIRE(toolbar.HitTest(CenterOf(*tag)) == ToolbarAction::Tag);
}

TEST_CASE(ToolbarHitTest_ReturnsBlurForBlurButton) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);
    const auto blur = toolbar.ButtonBounds(ToolbarAction::Blur);
    REQUIRE(blur.has_value());

    REQUIRE(toolbar.HitTest(CenterOf(*blur)) == ToolbarAction::Blur);
}

TEST_CASE(ToolbarHitTest_ReturnsPinForPinButton) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);
    const auto pin = toolbar.ButtonBounds(ToolbarAction::Pin);
    REQUIRE(pin.has_value());

    REQUIRE(toolbar.HitTest(CenterOf(*pin)) == ToolbarAction::Pin);
}

TEST_CASE(ToolbarButtons_HaveNoDisabledPlaceholderAfterPinEnabled) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);

    REQUIRE(!toolbar.FirstPlaceholderBounds().has_value());
}

TEST_CASE(ToolbarHitTest_IgnoresClickOutsideToolbar) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);

    const auto action = toolbar.HitTest(POINT{10, 10});
    REQUIRE(!action.has_value());
}

TEST_CASE(ToolbarMove_MovesToolbarAndButtons) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1200, 900};

    toolbar.Layout(capture, virtualScreen);
    const RECT beforeToolbar = toolbar.Bounds();
    const auto beforeConfirm = toolbar.ButtonBounds(ToolbarAction::Confirm);
    REQUIRE(beforeConfirm.has_value());

    toolbar.MoveBy(40, 25, virtualScreen);

    const RECT afterToolbar = toolbar.Bounds();
    const auto afterConfirm = toolbar.ButtonBounds(ToolbarAction::Confirm);
    REQUIRE(afterConfirm.has_value());
    REQUIRE(afterToolbar.left == beforeToolbar.left + 40);
    REQUIRE(afterToolbar.top == beforeToolbar.top + 25);
    REQUIRE(afterConfirm->left == beforeConfirm->left + 40);
    REQUIRE(afterConfirm->top == beforeConfirm->top + 25);
}

TEST_CASE(ToolbarMove_ClampsToVirtualScreen) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 900, 500};

    toolbar.Layout(capture, virtualScreen);
    toolbar.MoveBy(-10000, -10000, virtualScreen);
    RECT bounds = toolbar.Bounds();
    REQUIRE(bounds.left >= virtualScreen.left);
    REQUIRE(bounds.top >= virtualScreen.top);

    toolbar.MoveBy(10000, 10000, virtualScreen);
    bounds = toolbar.Bounds();
    REQUIRE(bounds.right <= virtualScreen.right);
    REQUIRE(bounds.bottom <= virtualScreen.bottom);
}

TEST_CASE(ToolbarMove_PreservesButtonHitTesting) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1200, 900};

    toolbar.Layout(capture, virtualScreen);
    toolbar.MoveBy(30, 10, virtualScreen);

    const auto save = toolbar.ButtonBounds(ToolbarAction::Save);
    REQUIRE(save.has_value());
    REQUIRE(toolbar.HitTest(CenterOf(*save)) == ToolbarAction::Save);
}

TEST_CASE(ToolbarHistory_DisablesUndoRedoWhenUnavailable) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);
    toolbar.SetHistoryState(false, false);

    const auto undo = toolbar.ButtonBounds(ToolbarAction::Undo);
    const auto redo = toolbar.ButtonBounds(ToolbarAction::Redo);
    REQUIRE(undo.has_value());
    REQUIRE(redo.has_value());
    REQUIRE(!toolbar.HitTest(CenterOf(*undo)).has_value());
    REQUIRE(!toolbar.HitTest(CenterOf(*redo)).has_value());
}

TEST_CASE(ToolbarHistory_EnablesUndoRedoWhenAvailable) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);
    toolbar.SetHistoryState(true, true);

    const auto undo = toolbar.ButtonBounds(ToolbarAction::Undo);
    const auto redo = toolbar.ButtonBounds(ToolbarAction::Redo);
    REQUIRE(undo.has_value());
    REQUIRE(redo.has_value());
    REQUIRE(toolbar.HitTest(CenterOf(*undo)) == ToolbarAction::Undo);
    REQUIRE(toolbar.HitTest(CenterOf(*redo)) == ToolbarAction::Redo);
}

TEST_CASE(ToolbarHitTest_IgnoresDisabledUndoRedo) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);
    toolbar.SetHistoryState(false, true);

    const auto undo = toolbar.ButtonBounds(ToolbarAction::Undo);
    const auto redo = toolbar.ButtonBounds(ToolbarAction::Redo);
    REQUIRE(undo.has_value());
    REQUIRE(redo.has_value());
    REQUIRE(!toolbar.HitTest(CenterOf(*undo)).has_value());
    REQUIRE(toolbar.HitTest(CenterOf(*redo)) == ToolbarAction::Redo);
}

TEST_CASE(ToolbarButtons_HaveTooltipsForEveryButton) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);

    for (const auto& button : toolbar.Buttons()) {
        REQUIRE(!button.tooltip.empty());
    }
}

TEST_CASE(ToolbarButtons_UseUniqueIcons) {
    EditorToolbar toolbar;
    const RECT capture{100, 100, 600, 300};
    const RECT virtualScreen{0, 0, 1000, 800};

    toolbar.Layout(capture, virtualScreen);

    std::set<mysnip::editor::ToolbarIcon> icons;
    for (const auto& button : toolbar.Buttons()) {
        REQUIRE(icons.insert(button.icon).second);
    }
}
