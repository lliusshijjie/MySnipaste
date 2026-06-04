#include "test_framework.h"

#include "tray/TrayNotificationPolicy.h"

#include <windows.h>
#include <shellapi.h>

using mysnip::tray::BalloonInfoFlags;

TEST_CASE(TrayNotificationPolicy_UsesCustomLargeIconWhenAvailable) {
    REQUIRE(BalloonInfoFlags(true) == (NIIF_USER | NIIF_LARGE_ICON));
}

TEST_CASE(TrayNotificationPolicy_FallsBackToInfoIconWithoutCustomIcon) {
    REQUIRE(BalloonInfoFlags(false) == NIIF_INFO);
}
