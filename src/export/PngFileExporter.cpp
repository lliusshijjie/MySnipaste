#include "export/PngFileExporter.h"

#include "utils/LogUtils.h"

#include <wincodec.h>
#include <windows.h>

namespace mysnip::exporter {
namespace {

class ComInitializer {
public:
    ComInitializer() : hr_(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {}
    ~ComInitializer() {
        if (SUCCEEDED(hr_)) {
            CoUninitialize();
        }
    }

    bool IsUsable() const {
        return SUCCEEDED(hr_) || hr_ == RPC_E_CHANGED_MODE;
    }

private:
    HRESULT hr_;
};

template <typename T>
void SafeRelease(T*& ptr) {
    if (ptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

} // namespace

bool PngFileExporter::SavePng(const std::wstring& path, const image::ImageBuffer& image) {
    if (path.empty() || !image.IsValid()) {
        return false;
    }

    ComInitializer com;
    if (!com.IsUsable()) {
        utils::LogError(L"[PNG] COM initialization failed.");
        return false;
    }

    IWICImagingFactory* factory = nullptr;
    IWICBitmapEncoder* encoder = nullptr;
    IWICBitmapFrameEncode* frame = nullptr;
    IWICStream* stream = nullptr;
    IPropertyBag2* properties = nullptr;

    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        utils::LogError(L"[PNG] Create WIC factory failed.");
        return false;
    }

    hr = factory->CreateStream(&stream);
    if (SUCCEEDED(hr)) {
        hr = stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE);
    }
    if (SUCCEEDED(hr)) {
        hr = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
    }
    if (SUCCEEDED(hr)) {
        hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
    }
    if (SUCCEEDED(hr)) {
        hr = encoder->CreateNewFrame(&frame, &properties);
    }
    if (SUCCEEDED(hr)) {
        hr = frame->Initialize(properties);
    }
    if (SUCCEEDED(hr)) {
        hr = frame->SetSize(static_cast<UINT>(image.width), static_cast<UINT>(image.height));
    }
    WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
    if (SUCCEEDED(hr)) {
        hr = frame->SetPixelFormat(&format);
    }
    if (SUCCEEDED(hr) && !IsEqualGUID(format, GUID_WICPixelFormat32bppBGRA)) {
        hr = WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
    }
    if (SUCCEEDED(hr)) {
        hr = frame->WritePixels(
            static_cast<UINT>(image.height),
            static_cast<UINT>(image.stride),
            static_cast<UINT>(image.pixels.size()),
            const_cast<BYTE*>(image.pixels.data()));
    }
    if (SUCCEEDED(hr)) {
        hr = frame->Commit();
    }
    if (SUCCEEDED(hr)) {
        hr = encoder->Commit();
    }

    SafeRelease(properties);
    SafeRelease(frame);
    SafeRelease(encoder);
    SafeRelease(stream);
    SafeRelease(factory);

    if (FAILED(hr)) {
        utils::LogError(L"[PNG] Save failed.");
        return false;
    }
    return true;
}

} // namespace mysnip::exporter
