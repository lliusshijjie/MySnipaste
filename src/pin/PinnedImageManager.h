#pragma once

#include "image/ImageBuffer.h"
#include "pin/PinnedImageWindow.h"

#include <cstddef>
#include <memory>
#include <vector>
#include <windows.h>

namespace mysnip::pin {

class IPinnedImageWindowFactory {
public:
    virtual ~IPinnedImageWindowFactory() = default;
    virtual std::unique_ptr<IPinnedImageWindow> CreatePinnedWindow() = 0;
};

class PinnedImageWindowFactory final : public IPinnedImageWindowFactory {
public:
    std::unique_ptr<IPinnedImageWindow> CreatePinnedWindow() override;
};

class PinnedImageManager {
public:
    PinnedImageManager();
    explicit PinnedImageManager(IPinnedImageWindowFactory& factory);

    bool AddPinnedImage(HWND owner, const image::ImageBuffer& image, POINT position);
    void RemovePinnedImage(HWND hwnd);
    std::size_t Count() const;

private:
    PinnedImageWindowFactory defaultFactory_;
    IPinnedImageWindowFactory* factory_ = nullptr;
    std::vector<std::unique_ptr<IPinnedImageWindow>> windows_;
};

} // namespace mysnip::pin
