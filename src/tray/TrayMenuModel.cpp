#include "tray/TrayMenuModel.h"

#include "app/CommandIds.h"

namespace mysnip::tray {

std::vector<TrayMenuItem> BuildTrayMenuItems() {
    return {
        TrayMenuItem{mysnip::app::kCommandCapture, L"Capture", false},
        TrayMenuItem{mysnip::app::kCommandLongCapture, L"Long Capture", false},
        TrayMenuItem{0, {}, true},
        TrayMenuItem{mysnip::app::kCommandExit, L"Exit", false},
    };
}

} // namespace mysnip::tray
