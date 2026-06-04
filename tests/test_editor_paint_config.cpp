#include "test_framework.h"

#include "capture/CaptureOverlayConfig.h"
#include "editor/EditorPaintConfig.h"

TEST_CASE(CaptureOverlayConfig_UsesLighterSelectionMask) {
    REQUIRE(mysnip::capture::kSelectionOverlayAlpha < 190);
}

TEST_CASE(EditorPaintConfig_DisablesBackgroundEraseToAvoidFlicker) {
    REQUIRE(!mysnip::editor::kEraseBackgroundBeforeEditorPaint);
}
