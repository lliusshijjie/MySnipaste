#include "utils/DpiUtils.h"

#include "utils/LogUtils.h"

#include <windows.h>

namespace mysnip::utils {

bool InitializeDpiAwareness() {
    if (SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        return true;
    }

    LogLastError(L"[DPI] SetProcessDpiAwarenessContext");
    if (SetProcessDPIAware()) {
        return true;
    }

    LogLastError(L"[DPI] SetProcessDPIAware");
    return false;
}

} // namespace mysnip::utils
