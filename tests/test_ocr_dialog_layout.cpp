#include "test_framework.h"

#include "ocr/OcrDialogLayout.h"

namespace {

bool IsInside(const RECT& child, const RECT& parent) {
    return child.left >= parent.left &&
        child.top >= parent.top &&
        child.right <= parent.right &&
        child.bottom <= parent.bottom;
}

} // namespace

TEST_CASE(OcrDialogLayout_KeepsEditorAndButtonsInsideClientArea) {
    const RECT clientRect{0, 0, 500, 320};

    const auto layout = mysnip::ocr::ComputeOcrDialogLayout(clientRect, 96);

    REQUIRE(IsInside(layout.editor, clientRect));
    REQUIRE(IsInside(layout.copyButton, clientRect));
    REQUIRE(IsInside(layout.closeButton, clientRect));
    REQUIRE(layout.editor.bottom < layout.copyButton.top);
    REQUIRE(layout.copyButton.right < layout.closeButton.left);
}

TEST_CASE(OcrDialogLayout_ScalesMarginsAndControlsForHighDpi) {
    const RECT clientRect{0, 0, 1000, 640};

    const auto normal = mysnip::ocr::ComputeOcrDialogLayout(clientRect, 96);
    const auto highDpi = mysnip::ocr::ComputeOcrDialogLayout(clientRect, 192);

    REQUIRE(highDpi.editor.left == normal.editor.left * 2);
    REQUIRE(
        highDpi.copyButton.bottom - highDpi.copyButton.top ==
        (normal.copyButton.bottom - normal.copyButton.top) * 2);
}
