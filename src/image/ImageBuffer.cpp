#include "image/ImageBuffer.h"

#include <limits>

namespace mysnip::image {

std::optional<ImageBuffer> ImageBuffer::Create(int requestedWidth, int requestedHeight) {
    if (requestedWidth <= 0 || requestedHeight <= 0) {
        return std::nullopt;
    }

    constexpr int kBytesPerPixel = 4;
    if (requestedWidth > std::numeric_limits<int>::max() / kBytesPerPixel) {
        return std::nullopt;
    }

    const int strideValue = requestedWidth * kBytesPerPixel;
    const auto strideBytes = static_cast<std::size_t>(strideValue);
    const auto heightValue = static_cast<std::size_t>(requestedHeight);
    if (heightValue > std::numeric_limits<std::size_t>::max() / strideBytes) {
        return std::nullopt;
    }

    const auto totalBytes = strideBytes * heightValue;

    ImageBuffer image;
    image.width = requestedWidth;
    image.height = requestedHeight;
    image.stride = strideValue;
    image.pixels.resize(totalBytes);
    return image;
}

bool ImageBuffer::IsValid() const {
    if (width <= 0 || height <= 0 || stride <= 0) {
        return false;
    }

    if (stride != width * 4) {
        return false;
    }

    return pixels.size() == static_cast<std::size_t>(stride) * static_cast<std::size_t>(height);
}

} // namespace mysnip::image
