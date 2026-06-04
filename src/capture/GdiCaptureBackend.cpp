#include "capture/GdiCaptureBackend.h"

#include "utils/LogUtils.h"
#include "utils/RectUtils.h"

#include <windows.h>

namespace mysnip::capture {
namespace {

class ScreenDc {
public:
    ScreenDc() : dc_(GetDC(nullptr)) {}
    ~ScreenDc() {
        if (dc_) {
            ReleaseDC(nullptr, dc_);
        }
    }
    HDC get() const { return dc_; }

private:
    HDC dc_ = nullptr;
};

class MemoryDc {
public:
    explicit MemoryDc(HDC compatibleDc) : dc_(CreateCompatibleDC(compatibleDc)) {}
    ~MemoryDc() {
        if (dc_) {
            DeleteDC(dc_);
        }
    }
    HDC get() const { return dc_; }

private:
    HDC dc_ = nullptr;
};

class BitmapHandle {
public:
    BitmapHandle(HDC dc, int width, int height) : bitmap_(CreateCompatibleBitmap(dc, width, height)) {}
    ~BitmapHandle() {
        if (bitmap_) {
            DeleteObject(bitmap_);
        }
    }
    HBITMAP get() const { return bitmap_; }

private:
    HBITMAP bitmap_ = nullptr;
};

} // namespace

std::optional<CaptureResult> GdiCaptureBackend::CaptureRect(const RECT& rect) {
    const int width = utils::Width(rect);
    const int height = utils::Height(rect);
    if (width <= 0 || height <= 0) {
        return std::nullopt;
    }

    ScreenDc screenDc;
    if (!screenDc.get()) {
        utils::LogLastError(L"[GdiCapture] GetDC");
        return std::nullopt;
    }

    MemoryDc memoryDc(screenDc.get());
    if (!memoryDc.get()) {
        utils::LogLastError(L"[GdiCapture] CreateCompatibleDC");
        return std::nullopt;
    }

    BitmapHandle bitmap(screenDc.get(), width, height);
    if (!bitmap.get()) {
        utils::LogLastError(L"[GdiCapture] CreateCompatibleBitmap");
        return std::nullopt;
    }

    HGDIOBJ oldObject = SelectObject(memoryDc.get(), bitmap.get());
    if (!oldObject) {
        utils::LogLastError(L"[GdiCapture] SelectObject");
        return std::nullopt;
    }

    const BOOL bitBltOk = BitBlt(
        memoryDc.get(),
        0,
        0,
        width,
        height,
        screenDc.get(),
        rect.left,
        rect.top,
        SRCCOPY | CAPTUREBLT);

    if (!bitBltOk) {
        SelectObject(memoryDc.get(), oldObject);
        utils::LogLastError(L"[GdiCapture] BitBlt");
        return std::nullopt;
    }

    auto image = image::ImageBuffer::Create(width, height);
    if (!image.has_value()) {
        SelectObject(memoryDc.get(), oldObject);
        return std::nullopt;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    const int scanLines = GetDIBits(
        memoryDc.get(),
        bitmap.get(),
        0,
        static_cast<UINT>(height),
        image->pixels.data(),
        &bmi,
        DIB_RGB_COLORS);

    SelectObject(memoryDc.get(), oldObject);

    if (scanLines != height) {
        utils::LogLastError(L"[GdiCapture] GetDIBits");
        return std::nullopt;
    }

    CaptureResult result;
    result.image = std::move(*image);
    result.screenRect = rect;
    return result;
}

} // namespace mysnip::capture
