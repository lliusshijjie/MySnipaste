#include "test_framework.h"

#include "app/CommandIds.h"
#include "tray/TrayMenuModel.h"

TEST_CASE(TrayMenu_ContainsLongCapture) {
    const auto items = mysnip::tray::BuildTrayMenuItems();

    bool foundCapture = false;
    bool foundLongCapture = false;
    for (const auto& item : items) {
        foundCapture = foundCapture || item.commandId == mysnip::app::kCommandCapture;
        foundLongCapture = foundLongCapture || item.commandId == mysnip::app::kCommandLongCapture;
    }

    REQUIRE(foundCapture);
    REQUIRE(foundLongCapture);
}
