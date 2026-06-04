#include "app/App.h"
#include "utils/DpiUtils.h"

#include <windows.h>

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int) {
    mysnip::utils::InitializeDpiAwareness();

    mysnip::app::App app;
    if (!app.Initialize(instance)) {
        MessageBoxW(nullptr, L"Failed to initialize MySnipaste.", L"MySnipaste", MB_OK | MB_ICONERROR);
        return 1;
    }

    const int exitCode = app.Run();
    app.Shutdown();
    return exitCode;
}
