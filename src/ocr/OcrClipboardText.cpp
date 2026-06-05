#include "ocr/OcrClipboardText.h"

#include "utils/LogUtils.h"

#include <cstring>

namespace mysnip::ocr {

std::vector<wchar_t> BuildUnicodeClipboardPayload(const std::wstring& text) {
    std::vector<wchar_t> payload(text.begin(), text.end());
    payload.push_back(L'\0');
    return payload;
}

bool CopyUnicodeText(HWND owner, const std::wstring& text) {
    const auto payload = BuildUnicodeClipboardPayload(text);
    const SIZE_T bytes = payload.size() * sizeof(wchar_t);
    HGLOBAL global = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!global) {
        utils::LogLastError(L"[OCR] GlobalAlloc text");
        return false;
    }

    void* memory = GlobalLock(global);
    if (!memory) {
        utils::LogLastError(L"[OCR] GlobalLock text");
        GlobalFree(global);
        return false;
    }
    std::memcpy(memory, payload.data(), bytes);
    GlobalUnlock(global);

    if (!OpenClipboard(owner)) {
        utils::LogLastError(L"[OCR] OpenClipboard text");
        GlobalFree(global);
        return false;
    }
    if (!EmptyClipboard()) {
        utils::LogLastError(L"[OCR] EmptyClipboard text");
        CloseClipboard();
        GlobalFree(global);
        return false;
    }
    if (!SetClipboardData(CF_UNICODETEXT, global)) {
        utils::LogLastError(L"[OCR] SetClipboardData text");
        CloseClipboard();
        GlobalFree(global);
        return false;
    }
    CloseClipboard();
    return true;
}

} // namespace mysnip::ocr
