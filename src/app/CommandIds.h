#pragma once

#include <windows.h>

namespace mysnip::app {

constexpr int kHotkeyCaptureId = 1;
constexpr UINT kTrayCallbackMessage = WM_APP + 1;
constexpr UINT kEditorNotificationMessage = WM_APP + 2;
constexpr UINT kPinnedImageDestroyedMessage = WM_APP + 3;
constexpr WPARAM kEditorNotifyPngSaved = 1;
constexpr WPARAM kEditorNotifyPngSaveFailed = 2;
constexpr UINT kCommandCapture = 1001;
constexpr UINT kCommandExit = 1002;

} // namespace mysnip::app
