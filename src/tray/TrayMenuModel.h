#pragma once

#include <string_view>
#include <vector>

namespace mysnip::tray {

struct TrayMenuItem {
    unsigned int commandId = 0;
    std::wstring_view text;
    bool separator = false;
};

std::vector<TrayMenuItem> BuildTrayMenuItems();

} // namespace mysnip::tray
