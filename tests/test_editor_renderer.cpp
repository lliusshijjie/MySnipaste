#include "test_framework.h"

#include "editor/BlurShape.h"
#include "editor/EditorDocument.h"
#include "editor/EditorRenderer.h"
#include "editor/FreehandShape.h"
#include "editor/HighlightShape.h"
#include "editor/MosaicShape.h"
#include "editor/RectangleShape.h"
#include "editor/TagShape.h"
#include "editor/TextShape.h"

#include <vector>

using mysnip::editor::BlurShape;
using mysnip::editor::EditorDocument;
using mysnip::editor::EditorRenderer;
using mysnip::editor::FreehandShape;
using mysnip::editor::HighlightShape;
using mysnip::editor::MosaicShape;
using mysnip::editor::RectangleShape;
using mysnip::editor::TagShape;
using mysnip::editor::TextShape;
using mysnip::image::ImageBuffer;

namespace {

ImageBuffer SolidImage(int width, int height, std::uint8_t b, std::uint8_t g, std::uint8_t r) {
    auto image = ImageBuffer::Create(width, height).value();
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto offset = static_cast<std::size_t>(y * image.stride + x * 4);
            image.pixels[offset + 0] = b;
            image.pixels[offset + 1] = g;
            image.pixels[offset + 2] = r;
            image.pixels[offset + 3] = 255;
        }
    }
    return image;
}

} // namespace

TEST_CASE(EditorRenderer_ReturnsOriginalImageWhenNoEdits) {
    EditorDocument document;
    document.baseImage = SolidImage(4, 3, 10, 20, 30);

    const auto result = EditorRenderer::RenderFinalImage(document);

    REQUIRE(result.has_value());
    REQUIRE(result->width == 4);
    REQUIRE(result->height == 3);
    REQUIRE(result->pixels == document.baseImage.pixels);
}

TEST_CASE(EditorRenderer_CropsFinalImageToCropRect) {
    EditorDocument document;
    document.baseImage = SolidImage(5, 4, 10, 20, 30);
    document.SetCropRect(RECT{1, 1, 4, 3});

    const auto result = EditorRenderer::RenderFinalImage(document);

    REQUIRE(result.has_value());
    REQUIRE(result->width == 3);
    REQUIRE(result->height == 2);
}

TEST_CASE(HighlightShape_AppliesAlphaBlend) {
    EditorDocument document;
    document.baseImage = SolidImage(3, 3, 0, 0, 0);
    document.AddShape(std::make_unique<HighlightShape>(RECT{0, 0, 3, 3}));

    const auto result = EditorRenderer::RenderFinalImage(document);

    REQUIRE(result.has_value());
    REQUIRE(result->pixels[1] > 0);
    REQUIRE(result->pixels[2] > 0);
}

TEST_CASE(MosaicShape_FillsBlocksWithAverageColor) {
    auto image = ImageBuffer::Create(2, 2).value();
    image.pixels = {
        0, 0, 0, 255, 100, 0, 0, 255,
        0, 100, 0, 255, 100, 100, 0, 255,
    };
    EditorDocument document;
    document.baseImage = image;
    document.AddShape(std::make_unique<MosaicShape>(RECT{0, 0, 2, 2}, 2));

    const auto result = EditorRenderer::RenderFinalImage(document);

    REQUIRE(result.has_value());
    REQUIRE(result->pixels[0] == 50);
    REQUIRE(result->pixels[1] == 50);
    REQUIRE(result->pixels[4] == 50);
    REQUIRE(result->pixels[5] == 50);
}

TEST_CASE(BlurShape_AppliesBoxBlurInsideRectOnly) {
    auto image = ImageBuffer::Create(5, 1).value();
    image.pixels = {
        0, 0, 0, 255,
        0, 0, 0, 255,
        255, 255, 255, 255,
        0, 0, 0, 255,
        0, 0, 0, 255,
    };
    EditorDocument document;
    document.baseImage = image;
    document.AddShape(std::make_unique<BlurShape>(RECT{1, 0, 4, 1}, 1));

    const auto result = EditorRenderer::RenderFinalImage(document);

    REQUIRE(result.has_value());
    REQUIRE(result->pixels[0] == 0);
    REQUIRE(result->pixels[16] == 0);
    REQUIRE(result->pixels[8] < 255);
    REQUIRE(result->pixels[8] > 0);
}

TEST_CASE(RectangleShape_RendersOntoImage) {
    EditorDocument document;
    document.baseImage = SolidImage(8, 8, 10, 20, 30);
    document.AddShape(std::make_unique<RectangleShape>(RECT{0, 0, 8, 8}));

    const auto result = EditorRenderer::RenderFinalImage(document);

    REQUIRE(result.has_value());
    REQUIRE(result->pixels != document.baseImage.pixels);
}

TEST_CASE(EditorRenderer_AppliesShapesInOrder) {
    EditorDocument document;
    document.baseImage = SolidImage(2, 2, 0, 0, 0);
    document.AddShape(std::make_unique<HighlightShape>(RECT{0, 0, 2, 2}));
    document.AddShape(std::make_unique<MosaicShape>(RECT{0, 0, 2, 2}, 2));

    const auto result = EditorRenderer::RenderFinalImage(document);

    REQUIRE(result.has_value());
    REQUIRE(result->pixels[1] > 0);
    REQUIRE(result->pixels[2] > 0);
}

TEST_CASE(EditorRenderer_AppliesFreehandTagBlurInOrder) {
    EditorDocument document;
    document.baseImage = SolidImage(80, 40, 0, 0, 0);
    document.AddShape(std::make_unique<FreehandShape>(std::vector<POINT>{POINT{0, 0}, POINT{79, 39}}));
    document.AddShape(std::make_unique<TagShape>(POINT{5, 5}, L"label"));
    document.AddShape(std::make_unique<BlurShape>(RECT{0, 0, 80, 40}, 2));

    const auto result = EditorRenderer::RenderFinalImage(document);

    REQUIRE(result.has_value());
    REQUIRE(result->pixels != document.baseImage.pixels);
}
