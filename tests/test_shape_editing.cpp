#include "test_framework.h"

#include "editor/ArrowShape.h"
#include "editor/BlurShape.h"
#include "editor/EditorStyle.h"
#include "editor/FreehandShape.h"
#include "editor/RectangleShape.h"
#include "editor/TagShape.h"
#include "editor/TextShape.h"

using mysnip::editor::ArrowShape;
using mysnip::editor::BlurShape;
using mysnip::editor::EditorStyle;
using mysnip::editor::FreehandShape;
using mysnip::editor::RectangleShape;
using mysnip::editor::TagShape;
using mysnip::editor::TextShape;

TEST_CASE(ShapeHitTest_RectangleDetectsBorderPoint) {
    RectangleShape shape(RECT{10, 10, 80, 60});

    REQUIRE(shape.HitTest(POINT{10, 30}, 3));
    REQUIRE(!shape.HitTest(POINT{40, 35}, 3));
}

TEST_CASE(ShapeMove_RectangleMovesBounds) {
    RectangleShape shape(RECT{10, 10, 80, 60});

    shape.MoveBy(5, -3, SIZE{200, 200});
    const RECT bounds = shape.Bounds();

    REQUIRE(bounds.left == 15);
    REQUIRE(bounds.top == 7);
    REQUIRE(bounds.right == 85);
    REQUIRE(bounds.bottom == 57);
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
