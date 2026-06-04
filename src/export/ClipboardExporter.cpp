#include "export/ClipboardExporter.h"

#include "export/ClipboardRetryPolicy.h"
#include "utils/LogUtils.h"

#include <cstring>
#include <windows.h>

namespace mysnip::export_ {
namespace {

bool OpenClipboardWithRetry(HWND owner) {
    for (int attempt = 0; attempt < kOpenClipboardMaxAttempts; ++attempt) {
        if (OpenClipboard(owner)) {
            return true;
        }

        if (!ShouldRetryOpenClipboard(attempt)) {
            utils::LogLastError(L"[Clipboard] OpenClipboard");
            return false;
        }

        Sleep(kOpenClipboardRetryDelayMs);
    }

    utils::LogLastError(L"[Clipboard] OpenClipboard");
    return false;
}

} // namespace

bool ClipboardExporter::CopyImage(HWND owner, const image::ImageBuffer& image) {
    const auto dib = dibBuilder_.BuildCfDib(image);
    HGLOBAL globalMemory = GlobalAlloc(GMEM_MOVEABLE, dib.size());
    if (!globalMemory) {
        utils::LogLastError(L"[Clipboard] GlobalAlloc");
        return false;
    }

    void* locked = GlobalLock(globalMemory);
    if (!locked) {
        utils::LogLastError(L"[Clipboard] GlobalLock");
        GlobalFree(globalMemory);
        return false;
    }

    std::memcpy(locked, dib.data(), dib.size());
    GlobalUnlock(globalMemory);

    if (!OpenClipboardWithRetry(owner)) {
        GlobalFree(globalMemory);
        return false;
    }

    bool success = false;
    if (!EmptyClipboard()) {
        utils::LogLastError(L"[Clipboard] EmptyClipboard");
    } else if (!SetClipboardData(CF_DIB, globalMemory)) {
        utils::LogLastError(L"[Clipboard] SetClipboardData");
    } else {
        success = true;
        globalMemory = nullptr;
    }

    CloseClipboard();

    if (globalMemory) {
        GlobalFree(globalMemory);
    }

    return success;
}

} // namespace mysnip::export_
