#pragma once

#include <string>
#include <vector>
#include <windows.h>

namespace mysnip::ocr {

std::vector<wchar_t> BuildUnicodeClipboardPayload(const std::wstring& text);
bool CopyUnicodeText(HWND owner, const std::wstring& text);

} // namespace mysnip::ocr
