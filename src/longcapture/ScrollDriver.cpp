#include "longcapture/ScrollDriver.h"

namespace mysnip::longcapture {

bool ScrollDriver::ScrollDown(POINT screenPoint, int wheelDelta) const {
    POINT previous{};
    GetCursorPos(&previous);
    SetCursorPos(screenPoint.x, screenPoint.y);

    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = static_cast<DWORD>(wheelDelta);
    const UINT sent = SendInput(1, &input, sizeof(INPUT));

    SetCursorPos(previous.x, previous.y);
    return sent == 1;
}

} // namespace mysnip::longcapture
