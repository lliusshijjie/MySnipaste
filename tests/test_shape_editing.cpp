#include "test_framework.h"

#include "editor/ArrowShape.h"
#include "editor/BlurShape.h"
#include "editor/EllipseShape.h"
#include "editor/EditorStyle.h"
#include "editor/FreehandShape.h"
#include "editor/HighlightShape.h"
#include "editor/LineShape.h"
#include "editor/MosaicShape.h"
#include "editor/NumberShape.h"
#include "editor/RectangleShape.h"
#include "editor/TagShape.h"
#include "editor/TextShape.h"

using mysnip::editor::ArrowShape;
using mysnip::editor::BlurShape;
using mysnip::editor::EllipseShape;
using mysnip::editor::EditorStyle;
using mysnip::editor::FreehandShape;
using mysnip::editor::HighlightShape;
using mysnip::editor::LineShape;
using mysnip::editor::MosaicShape;
using mysnip::editor::NumberShape;
using mysnip::editor::RectangleShape;
using mysnip::editor::TagShape;
using mysnip::editor::TextShape;

namespace {

void RequireBoundsInside(const mysnip::editor::Shape& shape, SIZE imageSize) {
    const RECT bounds = shape.Bounds();
    REQUIRE(bounds.left >= 0);
    REQUIRE(bounds.top >= 0);
    REQUIRE(bounds.right <= imageSize.cx);
    REQUIRE(bounds.bottom <= imageSize.cy);
}

} // namespace

TEST_CASE(ShapeHitTest_RectangleDetectsBorderPoint) {
    RectangleShape shape(RECT{10, 10, 80, 60});

    REQUIRE(shape.HitTest(POINT{10, 30}, 3));
    REQUIRE(!shape.HitTest(POINT{40, 35}, 3));
}

TEST_CASE(ShapeMove_RectangleMovesBounds) {
    RectangleShape shape(RECT{10, 10, 80, 60});

    shape.MoveBy(5, -3, SIZE{200, 200});
    const RECT bounds = shape.Bounds();

    REQUIRE(bounds.left == 14);
    REQUIRE(bounds.top == 6);
    REQUIRE(bounds.right == 86);
    REQUIRE(bounds.bottom == 58);
}

TEST_CASE(ShapeMove_RectangleClampsVisibleBoundsToImage) {
    RectangleShape shape(RECT{20, 20, 60, 60});
    shape.thickness = 10;

    shape.MoveBy(-100, -100, SIZE{100, 100});

    RequireBoundsInside(shape, SIZE{100, 100});
}

TEST_CASE(ShapeMove_EllipseClampsVisibleBoundsToImage) {
    EllipseShape shape(RECT{20, 20, 60, 60});
    shape.thickness = 10;

    shape.MoveBy(100, 100, SIZE{100, 100});

    RequireBoundsInside(shape, SIZE{100, 100});
}

TEST_CASE(ShapeMove_LineClampsStrokeBoundsToImage) {
    LineShape shape(POINT{20, 20}, POINT{80, 20});
    shape.thickness = 12;

    shape.MoveBy(100, -100, SIZE{100, 100});

    RequireBoundsInside(shape, SIZE{100, 100});
}

TEST_CASE(ShapeMove_ArrowClampsArrowHeadBoundsToImage) {
    ArrowShape shape(POINT{40, 40}, POINT{80, 80});
    shape.thickness = 8;

    shape.MoveBy(100, 100, SIZE{100, 100});

    RequireBoundsInside(shape, SIZE{100, 100});
}

TEST_CASE(ShapeMove_FreehandClampsStrokeBoundsToImage) {
    FreehandShape shape({POINT{20, 20}, POINT{50, 50}, POINT{80, 20}});
    shape.thickness = 12;

    shape.MoveBy(-100, 100, SIZE{100, 100});

    RequireBoundsInside(shape, SIZE{100, 100});
}

TEST_CASE(ShapeMove_TextClampsBoundsToImage) {
    TextShape text(POINT{20, 20}, L"move text");
    text.fontSize = 18;

    text.MoveBy(100, 100, SIZE{120, 80});

    RequireBoundsInside(text, SIZE{120, 80});
}

TEST_CASE(ShapeMove_TagClampsBoundsToImage) {
    TagShape tag(POINT{20, 20}, L"tag");
    tag.fontSize = 18;

    tag.MoveBy(-100, -100, SIZE{120, 80});

    RequireBoundsInside(tag, SIZE{120, 80});
}

TEST_CASE(ShapeMove_NumberClampsCircleToImage) {
    NumberShape number(POINT{30, 30}, 1);
    number.radius = 16;

    number.MoveBy(-100, 100, SIZE{100, 100});

    RequireBoundsInside(number, SIZE{100, 100});
}

TEST_CASE(ShapeMove_HighlightMosaicBlurClampToImage) {
    HighlightShape highlight(RECT{20, 20, 60, 60});
    MosaicShape mosaic(RECT{20, 20, 60, 60});
    BlurShape blur(RECT{20, 20, 60, 60});

    highlight.MoveBy(-100, 100, SIZE{100, 100});
    mosaic.MoveBy(100, -100, SIZE{100, 100});
    blur.MoveBy(100, 100, SIZE{100, 100});

    RequireBoundsInside(highlight, SIZE{100, 100});
    RequireBoundsInside(mosaic, SIZE{100, 100});
    RequireBoundsInside(blur, SIZE{100, 100});
}

TEST_CASE(ShapeStyle_RectangleAppliesStrokeStyle) {
    RectangleShape shape(RECT{10, 10, 80, 60});
    EditorStyle style;
    style.strokeColor = RGB(0, 128, 255);
    style.strokeWidth = 6;

    REQUIRE(shape.ApplyStyle(style));

    REQUIRE(shape.color == RGB(0, 128, 255));
    REQUIRE(shape.thickness == 6);
}

TEST_CASE(ShapeClone_TextPreservesMultilineContent) {
    TextShape text(POINT{5, 8}, L"line1\r\nline2");
    auto clone = text.Clone();
    const auto* copy = dynamic_cast<const TextShape*>(clone.get());

    REQUIRE(copy != nullptr);
    REQUIRE(copy->text == L"line1\r\nline2");
    REQUIRE(copy->position.x == 5);
}

TEST_CASE(ShapeHitTest_ArrowDetectsNearLine) {
    ArrowShape shape(POINT{0, 0}, POINT{100, 100});

    REQUIRE(shape.HitTest(POINT{50, 52}, 4));
    REQUIRE(!shape.HitTest(POINT{50, 70}, 4));
}

TEST_CASE(FreehandShape_ClonesAndMovesPoints) {
    FreehandShape shape({POINT{1, 2}, POINT{10, 12}, POINT{20, 22}});

    shape.MoveBy(5, 3, SIZE{100, 100});
    auto clone = shape.Clone();
    const auto* copy = dynamic_cast<const FreehandShape*>(clone.get());

    REQUIRE(copy != nullptr);
    REQUIRE(copy->points.size() == 3);
    REQUIRE(copy->points[0].x == 6);
    REQUIRE(copy->points[0].y == 5);
    REQUIRE(copy->points[2].x == 25);
}

TEST_CASE(FreehandShape_HitTestsNearStroke) {
    FreehandShape shape({POINT{0, 0}, POINT{100, 0}});

    REQUIRE(shape.HitTest(POINT{50, 3}, 4));
    REQUIRE(!shape.HitTest(POINT{50, 12}, 4));
}

TEST_CASE(FreehandShape_AppliesStrokeStyle) {
    FreehandShape shape({POINT{0, 0}, POINT{100, 0}});
    EditorStyle style;
    style.strokeColor = RGB(0, 128, 255);
    style.strokeWidth = 8;

    REQUIRE(shape.ApplyStyle(style));
    REQUIRE(shape.color == RGB(0, 128, 255));
    REQUIRE(shape.thickness == 8);
}

TEST_CASE(TagShape_PreservesTextStyleAndBounds) {
    TagShape tag(POINT{10, 20}, L"first\r\nsecond");
    EditorStyle style;
    style.strokeColor = RGB(0, 128, 255);
    style.fontSize = 24;

    REQUIRE(tag.ApplyStyle(style));
    auto clone = tag.Clone();
    const auto* copy = dynamic_cast<const TagShape*>(clone.get());

    REQUIRE(copy != nullptr);
    REQUIRE(copy->text == L"first\r\nsecond");
    REQUIRE(copy->position.x == 10);
    REQUIRE(copy->fontSize == 24);
    REQUIRE(copy->color == RGB(0, 128, 255));
    REQUIRE(copy->Bounds().right > copy->Bounds().left);
}

TEST_CASE(TagShape_HitTestAndMove) {
    TagShape tag(POINT{10, 20}, L"tag");

    REQUIRE(tag.HitTest(POINT{14, 24}, 2));

    tag.MoveBy(20, 10, SIZE{200, 200});
    REQUIRE(tag.position.x == 30);
    REQUIRE(tag.position.y == 30);
}

TEST_CASE(BlurShape_UsesStyleRadius) {
    BlurShape blur(RECT{10, 10, 50, 50});
    EditorStyle style;
    style.blurRadius = 10;

    REQUIRE(blur.ApplyStyle(style));
    REQUIRE(blur.radius == 10);
    REQUIRE(blur.Style().blurRadius == 10);
}

TEST_CASE(BlurShape_MoveAndBounds) {
    BlurShape blur(RECT{10, 10, 50, 50});

    blur.MoveBy(15, 5, SIZE{100, 100});
    const RECT bounds = blur.Bounds();

    REQUIRE(bounds.left == 25);
    REQUIRE(bounds.top == 15);
    REQUIRE(bounds.right == 65);
    REQUIRE(bounds.bottom == 55);
    REQUIRE(blur.HitTest(POINT{30, 20}, 2));
}
