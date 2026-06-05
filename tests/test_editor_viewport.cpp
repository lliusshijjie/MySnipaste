#include "test_framework.h"

#include "editor/EditorViewport.h"
#include "image/ImageBuffer.h"

using mysnip::editor::EditorViewport;
using mysnip::image::ImageBuffer;

TEST_CASE(EditorViewport_NormalCaptureUsesOneToOneMapping) {
    auto image = ImageBuffer::Create(200, 120);
    REQUIRE(image.has_value());

    EditorViewport viewport;
    viewport.Layout(*image, RECT{100, 80, 300, 200}, RECT{0, 0, 800, 600}, false);

    const auto point = viewport.ScreenToImage(POINT{150, 100});
    REQUIRE(point.has_value());
    REQUIRE(point->x == 50);
    REQUIRE(point->y == 20);
    REQUIRE(viewport.DisplayRect().right - viewport.DisplayRect().left == 200);
    REQUIRE(viewport.DisplayRect().bottom - viewport.DisplayRect().top == 120);
}

TEST_CASE(EditorViewport_LongImageFitsSelectionWidth) {
    auto image = ImageBuffer::Create(400, 1600);
    REQUIRE(image.has_value());

    EditorViewport viewport;
    viewport.Layout(*image, RECT{100, 50, 500, 450}, RECT{0, 0, 900, 700}, true);

    REQUIRE(viewport.DisplayRect().left == 100);
    REQUIRE(viewport.DisplayRect().right == 500);
    REQUIRE(viewport.Scale() == 1.0f);
    REQUIRE(viewport.DisplayRect().bottom - viewport.DisplayRect().top == 1600);
}

TEST_CASE(EditorViewport_LongImageViewportStaysAtSelectionSize) {
    auto image = ImageBuffer::Create(400, 1600);
    REQUIRE(image.has_value());

    EditorViewport viewport;
    viewport.Layout(*image, RECT{100, 50, 500, 450}, RECT{0, 0, 900, 700}, true);

    const RECT viewportRect = viewport.ViewportRect();
    REQUIRE(viewportRect.left == 100);
    REQUIRE(viewportRect.top == 50);
    REQUIRE(viewportRect.right == 500);
    REQUIRE(viewportRect.bottom == 450);
}

TEST_CASE(EditorViewport_LongImageRejectsPointOutsideViewport) {
    auto image = ImageBuffer::Create(400, 1600);
    REQUIRE(image.has_value());

    EditorViewport viewport;
    viewport.Layout(*image, RECT{100, 50, 500, 450}, RECT{0, 0, 900, 700}, true);

    REQUIRE(!viewport.ScreenToImage(POINT{120, 500}).has_value());
}

TEST_CASE(EditorViewport_ScreenToImageAccountsForScroll) {
    auto image = ImageBuffer::Create(400, 1600);
    REQUIRE(image.has_value());

    EditorViewport viewport;
    viewport.Layout(*image, RECT{100, 50, 500, 450}, RECT{0, 0, 900, 700}, true);
    viewport.ScrollBy(300);

    const auto point = viewport.ScreenToImage(POINT{120, 80});
    REQUIRE(point.has_value());
    REQUIRE(point->x == 20);
    REQUIRE(point->y == 330);
}

TEST_CASE(EditorViewport_ZoomKeepsPointStable) {
    auto image = ImageBuffer::Create(400, 1600);
    REQUIRE(image.has_value());

    EditorViewport viewport;
    viewport.Layout(*image, RECT{100, 50, 500, 450}, RECT{0, 0, 900, 700}, true);
    const auto before = viewport.ScreenToImage(POINT{200, 150});
    REQUIRE(before.has_value());

    viewport.ZoomAt(POINT{200, 150}, 1.25f);

    const auto after = viewport.ScreenToImage(POINT{200, 150});
    REQUIRE(after.has_value());
    REQUIRE(after->x == before->x);
    REQUIRE(after->y == before->y);
}
