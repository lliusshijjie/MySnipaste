#include "test_framework.h"

#include "capture/ICaptureBackend.h"
#include "editor/EditorDocument.h"
#include "editor/EditorResult.h"
#include "editor/PinPosition.h"
#include "image/ImageBuffer.h"
#include "pin/PinnedImageManager.h"
#include "pin/PinnedImageWindow.h"

#include <memory>
#include <optional>
#include <vector>
#include <windows.h>

using mysnip::editor::EditorStatus;
using mysnip::image::ImageBuffer;

namespace {

ImageBuffer MakeImage(int width = 10, int height = 8) {
    auto image = ImageBuffer::Create(width, height);
    REQUIRE(image.has_value());
    return *image;
}

class FakePinnedWindow final : public mysnip::pin::IPinnedImageWindow {
public:
    bool createResult = true;
    HWND hwnd = reinterpret_cast<HWND>(0x1234);
    bool shown = false;
    POINT createdAt{};
    ImageBuffer createdImage{};

    bool Create(HWND, const ImageBuffer& image, POINT position) override {
        createdImage = image;
        createdAt = position;
        return createResult;
    }

    void Show() override {
        shown = true;
    }

    HWND Hwnd() const override {
        return hwnd;
    }
};

class FakePinnedWindowFactory final : public mysnip::pin::IPinnedImageWindowFactory {
public:
    FakePinnedWindow* lastWindow = nullptr;
    bool createResult = true;

    std::unique_ptr<mysnip::pin::IPinnedImageWindow> CreatePinnedWindow() override {
        auto window = std::make_unique<FakePinnedWindow>();
        window->createResult = createResult;
        lastWindow = window.get();
        return window;
    }
};

} // namespace

TEST_CASE(EditorResult_AllowsPinnedStatus) {
    mysnip::editor::EditorResult result;
    result.status = EditorStatus::Pinned;
    result.finalImage = MakeImage();
    result.pinPosition = POINT{12, -30};

    REQUIRE(result.status == EditorStatus::Pinned);
    REQUIRE(result.finalImage.has_value());
    REQUIRE(result.pinPosition.has_value());
    REQUIRE(result.pinPosition->x == 12);
    REQUIRE(result.pinPosition->y == -30);
}

TEST_CASE(PinnedImageManager_AddsWindowOnCreateSuccess) {
    FakePinnedWindowFactory factory;
    mysnip::pin::PinnedImageManager manager(factory);
    const auto image = MakeImage(4, 3);

    const bool added = manager.AddPinnedImage(nullptr, image, POINT{20, 30});

    REQUIRE(added);
    REQUIRE(manager.Count() == 1);
    REQUIRE(factory.lastWindow != nullptr);
    REQUIRE(factory.lastWindow->shown);
    REQUIRE(factory.lastWindow->createdAt.x == 20);
    REQUIRE(factory.lastWindow->createdAt.y == 30);
    REQUIRE(factory.lastWindow->createdImage.width == 4);
    REQUIRE(factory.lastWindow->createdImage.height == 3);
}

TEST_CASE(PinnedImageManager_DoesNotKeepFailedWindow) {
    FakePinnedWindowFactory factory;
    factory.createResult = false;
    mysnip::pin::PinnedImageManager manager(factory);

    const bool added = manager.AddPinnedImage(nullptr, MakeImage(), POINT{20, 30});

    REQUIRE(!added);
    REQUIRE(manager.Count() == 0);
}

TEST_CASE(PinnedImageManager_RemovesDestroyedWindow) {
    FakePinnedWindowFactory factory;
    mysnip::pin::PinnedImageManager manager(factory);
    REQUIRE(manager.AddPinnedImage(nullptr, MakeImage(), POINT{0, 0}));
    const HWND hwnd = factory.lastWindow->Hwnd();

    manager.RemovePinnedImage(hwnd);

    REQUIRE(manager.Count() == 0);
}

TEST_CASE(PinnedImageWindow_ComputesScaledSizeWithinBounds) {
    REQUIRE(mysnip::pin::ClampPinnedImageScale(0.01f) == 0.2f);
    REQUIRE(mysnip::pin::ClampPinnedImageScale(6.0f) == 5.0f);

    const SIZE minSize = mysnip::pin::ComputePinnedImageSize(100, 50, 0.01f);
    REQUIRE(minSize.cx == 20);
    REQUIRE(minSize.cy == 10);

    const SIZE maxSize = mysnip::pin::ComputePinnedImageSize(100, 50, 6.0f);
    REQUIRE(maxSize.cx == 500);
    REQUIRE(maxSize.cy == 250);
}

TEST_CASE(PinnedImageWindow_CloseButtonLivesAtTopRight) {
    const RECT client{0, 0, 200, 120};

    const RECT close = mysnip::pin::PinnedImageCloseButtonRect(client);

    REQUIRE(close.left == 176);
    REQUIRE(close.top == 0);
    REQUIRE(close.right == 200);
    REQUIRE(close.bottom == 24);
}

TEST_CASE(PinnedImageWindow_HitTestsCloseButton) {
    const RECT client{0, 0, 200, 120};

    REQUIRE(mysnip::pin::IsPinnedImageCloseButtonHit(client, POINT{188, 12}));
    REQUIRE(!mysnip::pin::IsPinnedImageCloseButtonHit(client, POINT{175, 12}));
    REQUIRE(!mysnip::pin::IsPinnedImageCloseButtonHit(client, POINT{188, 25}));
}

TEST_CASE(PinnedImageWindow_DoesNotCloseOnEscKey) {
    REQUIRE(!mysnip::pin::ShouldClosePinnedImageOnKey(VK_ESCAPE));
}

TEST_CASE(PinnedImageWindow_UsesArrowCursor) {
    REQUIRE(mysnip::pin::PinnedImageCursorResourceName() == IDC_ARROW);
}

TEST_CASE(PinnedImagePosition_UsesCaptureTopLeftWithoutCrop) {
    mysnip::capture::CaptureResult capture;
    capture.screenRect = RECT{-120, 40, 180, 240};
    mysnip::editor::EditorDocument document;

    const POINT position = mysnip::editor::ComputePinnedImagePosition(capture, document);

    REQUIRE(position.x == -120);
    REQUIRE(position.y == 40);
}

TEST_CASE(PinnedImagePosition_UsesCropOffsetWhenCropped) {
    mysnip::capture::CaptureResult capture;
    capture.screenRect = RECT{-120, 40, 180, 240};
    mysnip::editor::EditorDocument document;
    document.cropRect = RECT{25, 30, 125, 130};

    const POINT position = mysnip::editor::ComputePinnedImagePosition(capture, document);

    REQUIRE(position.x == -95);
    REQUIRE(position.y == 70);
}
