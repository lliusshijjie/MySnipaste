#include "test_framework.h"

#include "image/ImageBuffer.h"

using mysnip::image::ImageBuffer;

TEST_CASE(ImageBuffer_ComputesBgraStride) {
    const auto image = ImageBuffer::Create(7, 5);

    REQUIRE(image.has_value());
    REQUIRE(image->width == 7);
    REQUIRE(image->height == 5);
    REQUIRE(image->stride == 28);
    REQUIRE(image->pixels.size() == 140);
}

TEST_CASE(ImageBuffer_ValidatesCapacity) {
    ImageBuffer image;
    image.width = 2;
    image.height = 2;
    image.stride = 8;
    image.pixels.resize(16);

    REQUIRE(image.IsValid());

    image.pixels.pop_back();
    REQUIRE(!image.IsValid());
}

