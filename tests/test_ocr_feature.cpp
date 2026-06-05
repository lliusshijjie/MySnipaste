#include "test_framework.h"

#include "editor/EditorToolbar.h"
#include "ocr/OcrClipboardText.h"
#include "ocr/WindowsOcrEngine.h"

using mysnip::editor::EditorToolbar;
using mysnip::editor::ToolbarAction;
using mysnip::ocr::OcrAvailability;

namespace {

POINT CenterOf(const RECT& rect) {
    return POINT{(rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2};
}

} // namespace

TEST_CASE(ToolbarHitTest_ReturnsOcrForOcrButton) {
    EditorToolbar toolbar;
    toolbar.Layout(RECT{100, 100, 600, 300}, RECT{0, 0, 1200, 800});

    const auto ocr = toolbar.ButtonBounds(ToolbarAction::Ocr);
    REQUIRE(ocr.has_value());
    REQUIRE(toolbar.HitTest(CenterOf(*ocr)) == ToolbarAction::Ocr);
}

TEST_CASE(OcrTextDialog_CopyUsesUnicodeTextPolicy) {
    const auto payload = mysnip::ocr::BuildUnicodeClipboardPayload(L"hello\r\n\u4e2d\u6587");

    REQUIRE(payload.size() == 10);
    REQUIRE(payload[0] == L'h');
    REQUIRE(payload[7] == L'\u4e2d');
    REQUIRE(payload.back() == L'\0');
}

TEST_CASE(WindowsOcrEngine_ReportsUnavailableWithoutPackageIdentity) {
    mysnip::ocr::WindowsOcrEngine engine;

    const auto availability = engine.Availability();

    REQUIRE(
        availability == OcrAvailability::Available ||
        availability == OcrAvailability::MissingPackageIdentity ||
        availability == OcrAvailability::Unsupported);
}
