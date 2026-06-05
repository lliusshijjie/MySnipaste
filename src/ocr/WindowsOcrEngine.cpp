#include "ocr/WindowsOcrEngine.h"

#include "utils/LogUtils.h"

#include <appmodel.h>
#include <cstring>
#include <string>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Storage.Streams.h>

namespace mysnip::ocr {
namespace {

bool HasPackageIdentity() {
    UINT32 length = 0;
    const LONG result = GetCurrentPackageFullName(&length, nullptr);
    return result != APPMODEL_ERROR_NO_PACKAGE;
}

std::optional<winrt::Windows::Graphics::Imaging::SoftwareBitmap> CreateSoftwareBitmap(const image::ImageBuffer& image) {
    if (!image.IsValid()) {
        return std::nullopt;
    }

    namespace imaging = winrt::Windows::Graphics::Imaging;
    namespace streams = winrt::Windows::Storage::Streams;

    imaging::SoftwareBitmap bitmap(
        imaging::BitmapPixelFormat::Bgra8,
        image.width,
        image.height,
        imaging::BitmapAlphaMode::Premultiplied);

    streams::Buffer buffer(static_cast<uint32_t>(image.pixels.size()));
    buffer.Length(static_cast<uint32_t>(image.pixels.size()));
    std::memcpy(buffer.data(), image.pixels.data(), image.pixels.size());
    bitmap.CopyFromBuffer(buffer);
    return bitmap;
}

} // namespace

OcrAvailability WindowsOcrEngine::Availability() const {
    if (!HasPackageIdentity()) {
        return OcrAvailability::MissingPackageIdentity;
    }
    try {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        const auto engine = winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromUserProfileLanguages();
        return engine ? OcrAvailability::Available : OcrAvailability::Unsupported;
    } catch (...) {
        return OcrAvailability::Unsupported;
    }
}

std::optional<OcrResult> WindowsOcrEngine::Recognize(const image::ImageBuffer& image) {
    if (Availability() != OcrAvailability::Available) {
        return std::nullopt;
    }

    try {
        const auto bitmap = CreateSoftwareBitmap(image);
        if (!bitmap.has_value()) {
            return std::nullopt;
        }

        const auto engine = winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromUserProfileLanguages();
        if (!engine) {
            return std::nullopt;
        }
        const auto result = engine.RecognizeAsync(*bitmap).get();
        std::wstring text;
        for (const auto& line : result.Lines()) {
            if (!text.empty()) {
                text += L"\r\n";
            }
            text += std::wstring(line.Text());
        }
        return OcrResult{text};
    } catch (...) {
        utils::LogError(L"[OCR] Windows OCR failed.");
        return std::nullopt;
    }
}

} // namespace mysnip::ocr
