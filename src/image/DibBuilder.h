#pragma once

#include "image/ImageBuffer.h"

#include <cstdint>
#include <vector>

namespace mysnip::image {

class DibBuilder {
public:
    std::vector<std::uint8_t> BuildCfDib(const ImageBuffer& image) const;
};

} // namespace mysnip::image
