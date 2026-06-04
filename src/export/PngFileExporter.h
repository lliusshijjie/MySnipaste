#pragma once

#include "image/ImageBuffer.h"

#include <string>

namespace mysnip::exporter {

class PngFileExporter {
public:
    static bool SavePng(const std::wstring& path, const image::ImageBuffer& image);
};

} // namespace mysnip::exporter
