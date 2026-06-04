#include "test_framework.h"

#include "image/DibBuilder.h"
#include "image/ImageBuffer.h"

#include <cstdint>
#include <cstring>
#include <windows.h>

using mysnip::image::DibBuilder;
using mysnip::image::ImageBuffer;

namespace {

ImageBuffer MakeTwoByTwoImage() {
    auto image = ImageBuffer::Create(2, 2).value();
    image.pixels = {
        10, 20, 30, 40, 50, 60, 70, 80,
        90, 100, 110, 120, 130, 140, 150, 160,
    };
    return image;
}

BITMAPINFOHEADER ReadHeader(const std::vector<std::uint8_t>& dib) {
    BITMAPINFOHEADER header{};
    std::memcpy(&header, dib.data(), sizeof(header));
    return header;
}

} // namespace

TEST_CASE(DibBuilder_BuildsValidBitmapInfoHeader) {
    const auto dib = DibBuilder{}.BuildCfDib(MakeTwoByTwoImage());
    const auto header = ReadHeader(dib);

    REQUIRE(header.biSize == sizeof(BITMAPINFOHEADER));
    REQUIRE(header.biWidth == 2);
    REQUIRE(header.biHeight == 2);
    REQUIRE(header.biPlanes == 1);
    REQUIRE(header.biBitCount == 32);
    REQUIRE(header.biCompression == BI_RGB);
    REQUIRE(header.biSizeImage == 16);
}

TEST_CASE(DibBuilder_DoesNotIncludeBitmapFileHeader) {
    const auto dib = DibBuilder{}.BuildCfDib(MakeTwoByTwoImage());

    REQUIRE(dib.size() == sizeof(BITMAPINFOHEADER) + 16);
}

TEST_CASE(DibBuilder_ConvertsTopDownToBottomUp) {
    const auto dib = DibBuilder{}.BuildCfDib(MakeTwoByTwoImage());
    const auto* pixels = dib.data() + sizeof(BITMAPINFOHEADER);

    REQUIRE(pixels[0] == 90);
    REQUIRE(pixels[1] == 100);
    REQUIRE(pixels[2] == 110);
    REQUIRE(pixels[4] == 130);
    REQUIRE(pixels[8] == 10);
    REQUIRE(pixels[12] == 50);
}

TEST_CASE(DibBuilder_ForcesAlphaToOpaque) {
    const auto dib = DibBuilder{}.BuildCfDib(MakeTwoByTwoImage());
    const auto* pixels = dib.data() + sizeof(BITMAPINFOHEADER);

    REQUIRE(pixels[3] == 255);
    REQUIRE(pixels[7] == 255);
    REQUIRE(pixels[11] == 255);
    REQUIRE(pixels[15] == 255);
}
