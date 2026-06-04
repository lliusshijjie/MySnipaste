#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace mysnip::image {

struct ImageBuffer {
    int width = 0;
    int height = 0;
    int stride = 0;
    std::vector<std::uint8_t> pixels;

    static std::optional<ImageBuffer> Create(int width, int height);
    bool IsValid() const;
};

} // namespace mysnip::image
