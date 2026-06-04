#include "image/DibBuilder.h"

#include <cstring>
#include <stdexcept>
#include <windows.h>

namespace mysnip::image {

std::vector<std::uint8_t> DibBuilder::BuildCfDib(const ImageBuffer& image) const {
    if (!image.IsValid()) {
        throw std::invalid_argument("ImageBuffer is invalid");
    }

    const auto headerSize = sizeof(BITMAPINFOHEADER);
    const auto pixelBytes = static_cast<std::size_t>(image.stride) * static_cast<std::size_t>(image.height);
    std::vector<std::uint8_t> dib(headerSize + pixelBytes);

    BITMAPINFOHEADER header{};
    header.biSize = static_cast<DWORD>(sizeof(BITMAPINFOHEADER));
    header.biWidth = image.width;
    header.biHeight = image.height;
    header.biPlanes = 1;
    header.biBitCount = 32;
    header.biCompression = BI_RGB;
    header.biSizeImage = static_cast<DWORD>(pixelBytes);

    std::memcpy(dib.data(), &header, sizeof(header));

    auto* dstPixels = dib.data() + headerSize;
    for (int y = 0; y < image.height; ++y) {
        const int srcY = image.height - 1 - y;
        const auto* srcRow = image.pixels.data() + static_cast<std::size_t>(srcY) * image.stride;
        auto* dstRow = dstPixels + static_cast<std::size_t>(y) * image.stride;
        std::memcpy(dstRow, srcRow, static_cast<std::size_t>(image.stride));

        for (int x = 0; x < image.width; ++x) {
            dstRow[static_cast<std::size_t>(x) * 4 + 3] = 0xFF;
        }
    }

    return dib;
}

} // namespace mysnip::image
