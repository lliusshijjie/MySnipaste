#pragma once

#include <windows.h>

namespace mysnip::longcapture {

enum class ManualDecision {
    Continue,
    Finish,
    Cancel
};

class LongCaptureSessionWindow {
public:
    ManualDecision AskManualDecision(HWND owner) const;
};

} // namespace mysnip::longcapture
