#pragma once

#include <windows.h>
#include <shellapi.h>

namespace mysnip::tray {

inline DWORD BalloonInfoFlags(bool hasCustomIcon) {
    return hasCustomIcon ? (NIIF_USER | NIIF_LARGE_ICON) : NIIF_INFO;
}

} // namespace mysnip::tray
