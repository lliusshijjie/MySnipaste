#include "app/App.h"

#include "app/CommandIds.h"
#include "app/AppResources.h"
#include "capture/CaptureOverlayWindow.h"
#include "editor/EditorWindow.h"
#include "utils/LogUtils.h"
#include "utils/RectUtils.h"

#include <string>
#include <windows.h>

namespace mysnip::app {
namespace {

constexpr wchar_t kMainWindowClass[] = L"MySnipasteHiddenMainWindow";

void ShowWarning(HWND owner, const wchar_t* title, const wchar_t* message) {
    MessageBoxW(owner, message, title, MB_OK | MB_ICONWARNING);
}

} // namespace

bool App::Initialize(HINSTANCE instance) {
    instance_ = instance;

    if (!CreateMainWindow()) {
        return false;
    }

    if (!trayIcon_.Add(mainWindow_, kTrayCallbackMessage)) {
        ShowWarning(mainWindow_, L"MySnipaste", L"Failed to create tray icon.");
    }

    if (!hotkey_.Register(mainWindow_, kHotkeyCaptureId, MOD_CONTROL | MOD_SHIFT, 'A')) {
        trayIcon_.ShowBalloon(L"MySnipaste", L"Ctrl+Shift+A is unavailable. Use the tray menu to capture.");
    }

    return true;
}

int App::Run() {
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}

void App::Shutdown() {
    if (state_ == AppState::Exiting) {
        return;
    }

    SetState(AppState::Exiting);
    hotkey_.Unregister(mainWindow_, kHotkeyCaptureId);
    trayIcon_.Remove();

    if (mainWindow_) {
        HWND hwnd = mainWindow_;
        mainWindow_ = nullptr;
        if (IsWindow(hwnd)) {
            DestroyWindow(hwnd);
        }
    }
}

bool App::CreateMainWindow() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &App::WindowProc;
    wc.hInstance = instance_;
    wc.lpszClassName = kMainWindowClass;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON));
    if (!wc.hIcon) {
        wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }
    wc.hIconSm = reinterpret_cast<HICON>(LoadImageW(
        instance_,
        MAKEINTRESOURCEW(IDI_APP_ICON),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR | LR_SHARED));
    if (!wc.hIconSm) {
        wc.hIconSm = wc.hIcon;
    }

    if (!RegisterClassExW(&wc)) {
        utils::LogLastError(L"[App] RegisterClassExW");
        return false;
    }

    mainWindow_ = CreateWindowExW(
        0,
        kMainWindowClass,
        L"MySnipaste",
        WS_OVERLAPPED,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        nullptr,
        nullptr,
        instance_,
        this);

    if (!mainWindow_) {
        utils::LogLastError(L"[App] CreateWindowExW");
        return false;
    }

    return true;
}

void App::StartCapture() {
    if (state_ != AppState::Idle) {
        utils::LogInfo(L"[App] Capture ignored because the app is busy.");
        return;
    }

    SetState(AppState::Selecting);

    capture::CaptureOverlayWindow overlay;
    const auto selection = overlay.SelectRegion(mainWindow_);

    if (selection.status != capture::SelectionStatus::Completed ||
        utils::IsTooSmall(selection.screenRect)) {
        SetState(AppState::Idle);
        return;
    }

    SetState(AppState::Capturing);
    Sleep(30);

    auto captureResult = captureBackend_.CaptureRect(selection.screenRect);
    if (!captureResult.has_value()) {
        trayIcon_.ShowBalloon(L"MySnipaste", L"Screen capture failed.");
        SetState(AppState::Idle);
        return;
    }

    OpenEditorForCapture(*captureResult);
}

void App::StartLongCapture() {
    if (state_ != AppState::Idle) {
        utils::LogInfo(L"[App] Long capture ignored because the app is busy.");
        return;
    }

    SetState(AppState::LongSelecting);
    SetState(AppState::LongCapturing);
    const auto captureResult = longCaptureController_.Capture(mainWindow_, captureBackend_);
    if (!captureResult.has_value()) {
        SetState(AppState::Idle);
        return;
    }

    OpenEditorForCapture(*captureResult);
}

void App::OpenEditorForCapture(const capture::CaptureResult& captureResult) {
    SetState(AppState::Editing);
    editor::EditorWindow editor;
    const auto editorResult = editor.ShowModal(mainWindow_, captureResult);
    if (editorResult.status == editor::EditorStatus::Cancelled) {
        SetState(AppState::Idle);
        return;
    }
    if (editorResult.status == editor::EditorStatus::Failed) {
        trayIcon_.ShowBalloon(L"MySnipaste", L"Editor failed.");
        SetState(AppState::Idle);
        return;
    }
    if (editorResult.status == editor::EditorStatus::Pinned) {
        if (!editorResult.finalImage.has_value() || !editorResult.pinPosition.has_value()) {
            trayIcon_.ShowBalloon(L"MySnipaste", L"Failed to create pinned image.");
            SetState(AppState::Idle);
            return;
        }
        if (!pinnedImageManager_.AddPinnedImage(mainWindow_, *editorResult.finalImage, *editorResult.pinPosition)) {
            trayIcon_.ShowBalloon(L"MySnipaste", L"Failed to create pinned image.");
        }
        SetState(AppState::Idle);
        return;
    }
    if (!editorResult.finalImage.has_value()) {
        trayIcon_.ShowBalloon(L"MySnipaste", L"Editor did not produce an image.");
        SetState(AppState::Idle);
        return;
    }

    SetState(AppState::CopyingToClipboard);
    if (!clipboardExporter_.CopyImage(mainWindow_, *editorResult.finalImage)) {
        trayIcon_.ShowBalloon(L"MySnipaste", L"Failed to copy image to clipboard.");
        SetState(AppState::Idle);
        return;
    }

    trayIcon_.ShowBalloon(L"MySnipaste", L"Copied to clipboard.");
    SetState(AppState::Idle);
}

void App::SetState(AppState state) {
    state_ = state;
}

LRESULT CALLBACK App::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    App* app = nullptr;
    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    } else {
        app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (app) {
        return app->HandleMessage(hwnd, message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT App::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_HOTKEY:
        if (static_cast<int>(wParam) == kHotkeyCaptureId) {
            StartCapture();
            return 0;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case kCommandCapture:
            StartCapture();
            return 0;
        case kCommandLongCapture:
            StartLongCapture();
            return 0;
        case kCommandExit:
            Shutdown();
            PostQuitMessage(0);
            return 0;
        default:
            break;
        }
        break;
    case kTrayCallbackMessage:
        if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
            StartCapture();
            return 0;
        }
        if (LOWORD(lParam) == WM_RBUTTONUP) {
            trayIcon_.ShowMenu(hwnd);
            return 0;
        }
        break;
    case kEditorNotificationMessage:
        if (wParam == kEditorNotifyPngSaved) {
            trayIcon_.ShowBalloon(L"MySnipaste", L"Saved PNG.");
            return 0;
        }
        if (wParam == kEditorNotifyPngSaveFailed) {
            trayIcon_.ShowBalloon(L"MySnipaste", L"Failed to save PNG.");
            return 0;
        }
        if (wParam == kEditorNotifyOcrUnavailable) {
            trayIcon_.ShowBalloon(L"MySnipaste", L"OCR requires the packaged app.");
            return 0;
        }
        if (wParam == kEditorNotifyOcrFailed) {
            trayIcon_.ShowBalloon(L"MySnipaste", L"Failed to recognize text.");
            return 0;
        }
        break;
    case kPinnedImageDestroyedMessage:
        pinnedImageManager_.RemovePinnedImage(reinterpret_cast<HWND>(wParam));
        return 0;
    case WM_DESTROY:
        SetState(AppState::Exiting);
        hotkey_.Unregister(hwnd, kHotkeyCaptureId);
        trayIcon_.Remove();
        mainWindow_ = nullptr;
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace mysnip::app
