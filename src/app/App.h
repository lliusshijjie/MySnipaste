#pragma once

#include "capture/GdiCaptureBackend.h"
#include "editor/EditorWindow.h"
#include "export/ClipboardExporter.h"
#include "hotkey/HotkeyManager.h"
#include "pin/PinnedImageManager.h"
#include "tray/TrayIcon.h"

#include <windows.h>

namespace mysnip::app {

enum class AppState {
    Idle,
    Selecting,
    Capturing,
    Editing,
    CopyingToClipboard,
    Exiting
};

class App {
public:
    bool Initialize(HINSTANCE instance);
    int Run();
    void Shutdown();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    bool CreateMainWindow();
    void StartCapture();
    void SetState(AppState state);

    HINSTANCE instance_ = nullptr;
    HWND mainWindow_ = nullptr;
    AppState state_ = AppState::Idle;
    hotkey::HotkeyManager hotkey_;
    tray::TrayIcon trayIcon_;
    capture::GdiCaptureBackend captureBackend_;
    export_::ClipboardExporter clipboardExporter_;
    pin::PinnedImageManager pinnedImageManager_;
};

} // namespace mysnip::app
