#include "longcapture/LongCaptureSessionWindow.h"

namespace mysnip::longcapture {

ManualDecision LongCaptureSessionWindow::AskManualDecision(HWND owner) const {
    const int result = MessageBoxW(
        owner,
        L"Long capture could not continue automatically.\n\nYes: capture after you scroll manually\nNo: finish with current image\nCancel: discard",
        L"MySnipaste Long Capture",
        MB_YESNOCANCEL | MB_ICONINFORMATION | MB_TOPMOST);
    if (result == IDYES) {
        return ManualDecision::Continue;
    }
    if (result == IDNO) {
        return ManualDecision::Finish;
    }
    return ManualDecision::Cancel;
}

} // namespace mysnip::longcapture
