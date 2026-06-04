#include "utils/LogUtils.h"

#include <string>
#include <windows.h>

namespace mysnip::utils {

void LogInfo(std::wstring_view message) {
    std::wstring text(message);
    text.append(L"\n");
    OutputDebugStringW(text.c_str());
}

void LogError(std::wstring_view message) {
    LogInfo(message);
}

void LogLastError(std::wstring_view action) {
    const DWORD error = GetLastError();
    std::wstring text(action);
    text.append(L" failed. GetLastError=");
    text.append(std::to_wstring(error));
    LogError(text);
}

void LogRect(std::wstring_view label, const RECT& rect) {
    std::wstring text(label);
    text.append(L" rect=(");
    text.append(std::to_wstring(rect.left));
    text.append(L",");
    text.append(std::to_wstring(rect.top));
    text.append(L",");
    text.append(std::to_wstring(rect.right));
    text.append(L",");
    text.append(std::to_wstring(rect.bottom));
    text.append(L") size=(");
    text.append(std::to_wstring(rect.right - rect.left));
    text.append(L"x");
    text.append(std::to_wstring(rect.bottom - rect.top));
    text.append(L")");
    LogInfo(text);
}

void LogDpi(std::wstring_view label, HWND hwnd) {
    std::wstring text(label);
    text.append(L" dpi=");
    text.append(std::to_wstring(GetDpiForWindow(hwnd)));
    LogInfo(text);
}

} // namespace mysnip::utils
