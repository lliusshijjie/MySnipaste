#pragma once

#include "image/DibBuilder.h"
#include "image/ImageBuffer.h"

#include <windows.h>

namespace mysnip::export_ {

class ClipboardExporter {
public:
    bool CopyImage(HWND owner, const image::ImageBuffer& image);

private:
    image::DibBuilder dibBuilder_;
};

} // namespace mysnip::export_
