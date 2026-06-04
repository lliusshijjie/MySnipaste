#include "pin/PinnedImageManager.h"

#include <algorithm>

namespace mysnip::pin {

std::unique_ptr<IPinnedImageWindow> PinnedImageWindowFactory::CreatePinnedWindow() {
    return std::make_unique<PinnedImageWindow>();
}

PinnedImageManager::PinnedImageManager()
    : factory_(&defaultFactory_) {
}

PinnedImageManager::PinnedImageManager(IPinnedImageWindowFactory& factory)
    : factory_(&factory) {
}

bool PinnedImageManager::AddPinnedImage(HWND owner, const image::ImageBuffer& image, POINT position) {
    auto window = factory_->CreatePinnedWindow();
    if (!window || !window->Create(owner, image, position)) {
        return false;
    }

    window->Show();
    windows_.push_back(std::move(window));
    return true;
}

void PinnedImageManager::RemovePinnedImage(HWND hwnd) {
    const auto it = std::remove_if(
        windows_.begin(),
        windows_.end(),
        [hwnd](const std::unique_ptr<IPinnedImageWindow>& window) {
            return window && window->Hwnd() == hwnd;
        });
    windows_.erase(it, windows_.end());
}

std::size_t PinnedImageManager::Count() const {
    return windows_.size();
}

} // namespace mysnip::pin
