#include "test_framework.h"

#include "export/PngFileExporter.h"
#include "image/ImageBuffer.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>
#include <windows.h>
#include <wincodec.h>

using mysnip::exporter::PngFileExporter;

namespace {

template <typename T>
void SafeRelease(T*& ptr) {
    if (ptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

bool DecodePngSize(const std::wstring& path, UINT& width, UINT& height) {
    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(init) && init != RPC_E_CHANGED_MODE) {
        return false;
    }

    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (SUCCEEDED(hr)) {
        hr = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
    }
    if (SUCCEEDED(hr)) {
        hr = decoder->GetFrame(0, &frame);
    }
    if (SUCCEEDED(hr)) {
        hr = frame->GetSize(&width, &height);
    }

    SafeRelease(frame);
    SafeRelease(decoder);
    SafeRelease(factory);
    if (SUCCEEDED(init)) {
        CoUninitialize();
    }
    return SUCCEEDED(hr);
}

} // namespace

TEST_CASE(PngFileExporter_SavesPngSignature) {
    auto image = mysnip::image::ImageBuffer::Create(2, 2);
    REQUIRE(image.has_value());
    image->pixels = {
        0, 0, 255, 255,
        0, 255, 0, 255,
        255, 0, 0, 255,
        255, 255, 255, 255,
    };

    const auto path = std::filesystem::temp_directory_path() / L"mysnipaste_test_export.png";
    std::filesystem::remove(path);

    REQUIRE(PngFileExporter::SavePng(path.wstring(), *image));

    {
        std::ifstream file(path, std::ios::binary);
        std::vector<unsigned char> header(8);
        file.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));

        REQUIRE(header == std::vector<unsigned char>({0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'}));
    }

    UINT decodedWidth = 0;
    UINT decodedHeight = 0;
    REQUIRE(DecodePngSize(path.wstring(), decodedWidth, decodedHeight));
    REQUIRE(decodedWidth == 2);
    REQUIRE(decodedHeight == 2);
    std::filesystem::remove(path);
}
