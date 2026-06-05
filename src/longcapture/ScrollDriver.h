#pragma once

#include <windows.h>

namespace mysnip::longcapture {

class ScrollDriver {
public:
    bool ScrollDown(POINT screenPoint, int wheelDelta = -WHEEL_DELTA * 3) const;
};

} // namespace mysnip::longcapture
