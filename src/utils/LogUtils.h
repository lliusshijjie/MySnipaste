#pragma once

#include <string_view>
#include <windows.h>

namespace mysnip::utils {

void LogInfo(std::wstring_view message);
void LogError(std::wstring_view message);
void LogLastError(std::wstring_view action);
void LogRect(std::wstring_view label, const RECT& rect);
void LogDpi(std::wstring_view label, HWND hwnd);

} // namespace mysnip::utils
