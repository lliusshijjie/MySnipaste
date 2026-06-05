#include "test_framework.h"

#include "editor/ArrowShape.h"
#include "editor/BlurShape.h"
#include "editor/EditorDocument.h"
#include "editor/EllipseShape.h"
#include "editor/FreehandShape.h"
#include "editor/HighlightShape.h"
#include "editor/LineShape.h"
#include "editor/MosaicShape.h"
#include "editor/NumberShape.h"
#include "editor/RectangleShape.h"
#include "editor/TagShape.h"
#include "editor/TextShape.h"

#include <vector>

using mysnip::editor::ArrowShape;
using mysnip::editor::BlurShape;
using mysnip::editor::EditorDocument;
using mysnip::editor::EllipseShape;
using mysnip::editor::FreehandShape;
using mysnip::editor::HighlightShape;
using mysnip::editor::LineShape;
using mysnip::editor::MosaicShape;
using mysnip::editor::NumberShape;
using mysnip::editor::RectangleShape;
using mysnip::editor::TagShape;
using mysnip::editor::TextShape;

TEST_CASE(EditorDocument_AddsArrowShape) {
    EditorDocument document;
    document.AddShape(std::make_unique<ArrowShape>(POINT{1, 2}, POINT{20, 30}));

    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_AddsHighlightShape) {
    EditorDocument document;
    document.AddShape(std::make_unique<HighlightShape>(RECT{1, 2, 20, 30}));

    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_AddsMosaicShape) {
    EditorDocument document;
    document.AddShape(std::make_unique<MosaicShape>(RECT{1, 2, 20, 30}));

    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_AddsTextShape) {
    EditorDocument document;
    const bool added = document.AddTextShape(POINT{3, 4}, L"hello");

    REQUIRE(added);
    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_UpdatesCropRect) {
    EditorDocument document;
    document.SetCropRect(RECT{1, 2, 20, 30});

    REQUIRE(document.cropRect.has_value());
    REQUIRE(document.cropRect->left == 1);
}

TEST_CASE(EditorDocument_ReplacesOldCropRect) {
    EditorDocument document;
    document.SetCropRect(RECT{1, 2, 20, 30});
    document.SetCropRect(RECT{5, 6, 40, 50});

    REQUIRE(document.cropRect.has_value());
    REQUIRE(document.cropRect->left == 5);
    REQUIRE(document.cropRect->bottom == 50);
}

TEST_CASE(TextShape_IgnoresEmptyText) {
    EditorDocument document;
    const bool added = document.AddTextShape(POINT{3, 4}, L"");

    REQUIRE(!added);
    REQUIRE(document.shapes.empty());
}

TEST_CASE(EditorDocument_AddsLineShape) {
    EditorDocument document;
    document.AddShape(std::make_unique<LineShape>(POINT{1, 2}, POINT{20, 30}));

    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_AddsRectangleShape) {
    EditorDocument document;
    document.AddShape(std::make_unique<RectangleShape>(RECT{1, 2, 20, 30}));

    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_AddsEllipseShape) {
    EditorDocument document;
    document.AddShape(std::make_unique<EllipseShape>(RECT{1, 2, 20, 30}));

    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_AddsNumberShape) {
    EditorDocument document;
    document.AddShape(std::make_unique<NumberShape>(POINT{5, 6}, 1));

    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_AddsFreehandShape) {
    EditorDocument document;
    document.AddShape(std::make_unique<FreehandShape>(std::vector<POINT>{POINT{1, 2}, POINT{5, 6}}));

    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_AddsTagShape) {
    EditorDocument document;
    document.AddShape(std::make_unique<TagShape>(POINT{5, 6}, L"tag"));

    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_AddsBlurShape) {
    EditorDocument document;
    document.AddShape(std::make_unique<BlurShape>(RECT{1, 2, 20, 30}));

    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_UndoRemovesLastShape) {
    EditorDocument document;
    document.AddShape(std::make_unique<RectangleShape>(RECT{0, 0, 10, 10}));
    document.AddShape(std::make_unique<EllipseShape>(RECT{0, 0, 10, 10}));

    REQUIRE(document.Undo());
    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_UndoOnEmptyReturnsFalse) {
    EditorDocument document;
    REQUIRE(!document.Undo());
}

TEST_CASE(EditorDocument_CanUndoCanRedoReflectState) {
    EditorDocument document;
    REQUIRE(!document.CanUndo());
    REQUIRE(!document.CanRedo());

    document.AddShape(std::make_unique<RectangleShape>(RECT{0, 0, 10, 10}));
    REQUIRE(document.CanUndo());
    REQUIRE(!document.CanRedo());

    REQUIRE(document.Undo());
    REQUIRE(!document.CanUndo());
    REQUIRE(document.CanRedo());

    REQUIRE(document.Redo());
    REQUIRE(document.CanUndo());
    REQUIRE(!document.CanRedo());
}

TEST_CASE(EditorDocument_RedoRestoresUndoneShape) {
    EditorDocument document;
    document.AddShape(std::make_unique<RectangleShape>(RECT{0, 0, 10, 10}));

    REQUIRE(document.Undo());
    REQUIRE(document.shapes.empty());

    REQUIRE(document.Redo());
    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_UndoRestoresPreviousCrop) {
    EditorDocument document;
    document.SetCropRect(RECT{1, 2, 20, 30});

    REQUIRE(document.Undo());
    REQUIRE(!document.cropRect.has_value());
}

TEST_CASE(EditorDocument_RedoRestoresCrop) {
    EditorDocument document;
    document.SetCropRect(RECT{1, 2, 20, 30});

    REQUIRE(document.Undo());
    REQUIRE(!document.cropRect.has_value());

    REQUIRE(document.Redo());
    REQUIRE(document.cropRect.has_value());
    REQUIRE(document.cropRect->left == 1);
    REQUIRE(document.cropRect->bottom == 30);
}

TEST_CASE(EditorDocument_NewEditClearsRedoStack) {
    EditorDocument document;
    document.AddShape(std::make_unique<RectangleShape>(RECT{0, 0, 10, 10}));

    REQUIRE(document.Undo());
    REQUIRE(document.CanRedo());

    document.AddShape(std::make_unique<EllipseShape>(RECT{0, 0, 12, 12}));
    REQUIRE(!document.CanRedo());
    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_UndoFollowsOperationOrder) {
    EditorDocument document;
    document.AddShape(std::make_unique<LineShape>(POINT{0, 0}, POINT{5, 5}));
    document.SetCropRect(RECT{1, 1, 8, 8});

    REQUIRE(document.Undo());
    REQUIRE(!document.cropRect.has_value());
    REQUIRE(document.shapes.size() == 1);

    REQUIRE(document.Undo());
    REQUIRE(document.shapes.empty());
}

TEST_CASE(EditorDocument_SelectsTopmostShapeAtPoint) {
    EditorDocument document;
    document.AddShape(std::make_unique<RectangleShape>(RECT{0, 0, 80, 80}));
    document.AddShape(std::make_unique<RectangleShape>(RECT{10, 10, 90, 90}));

    REQUIRE(document.SelectShapeAt(POINT{10, 40}, 4));

    REQUIRE(document.selectedShapeIndex.has_value());
    REQUIRE(*document.selectedShapeIndex == 1);
}

TEST_CASE(EditorDocument_DeleteSelectedShapeIsUndoable) {
    EditorDocument document;
    document.AddShape(std::make_unique<RectangleShape>(RECT{0, 0, 80, 80}));
    document.SelectShapeAt(POINT{0, 40}, 4);

    REQUIRE(document.DeleteSelectedShape());
    REQUIRE(document.shapes.empty());

    REQUIRE(document.Undo());
    REQUIRE(document.shapes.size() == 1);
}

TEST_CASE(EditorDocument_MoveSelectedShapeIsUndoable) {
    EditorDocument document;
    document.baseImage = *mysnip::image::ImageBuffer::Create(200, 200);
    document.AddShape(std::make_unique<RectangleShape>(RECT{10, 10, 60, 60}));
    document.SelectShapeAt(POINT{10, 30}, 4);

    REQUIRE(document.MoveSelectedShape(10, 5));
    auto* moved = dynamic_cast<RectangleShape*>(document.shapes.front().get());
    REQUIRE(moved != nullptr);
    REQUIRE(moved->rect.left == 20);
    REQUIRE(moved->rect.top == 15);

    REQUIRE(document.Undo());
    auto* restored = dynamic_cast<RectangleShape*>(document.shapes.front().get());
    REQUIRE(restored != nullptr);
    REQUIRE(restored->rect.left == 10);
    REQUIRE(restored->rect.top == 10);
}

TEST_CASE(EditorDocument_MoveSelectedShapeReturnsFalseWhenBlockedByBoundary) {
    EditorDocument document;
    document.baseImage = *mysnip::image::ImageBuffer::Create(100, 100);
    document.AddShape(std::make_unique<RectangleShape>(RECT{1, 10, 31, 40}));
    document.SelectShapeAt(POINT{1, 20}, 4);

    REQUIRE(!document.MoveSelectedShape(-20, 0));
}

TEST_CASE(EditorDocument_MoveSelectedShapeDoesNotCreateUndoForNoop) {
    EditorDocument document;
    document.baseImage = *mysnip::image::ImageBuffer::Create(100, 100);
    document.AddShape(std::make_unique<RectangleShape>(RECT{1, 10, 31, 40}));
    document.SelectShapeAt(POINT{1, 20}, 4);
    const auto undoCount = document.undoStack.size();

    REQUIRE(!document.MoveSelectedShape(-20, 0));

    REQUIRE(document.undoStack.size() == undoCount);
}

TEST_CASE(EditorDocument_UpdateTextShapeIsUndoable) {
    EditorDocument document;
    document.AddTextShape(POINT{5, 6}, L"old");
    document.SelectShapeAt(POINT{6, 7}, 8);

    REQUIRE(document.UpdateSelectedText(L"line1\r\nline2"));
    const auto* updated = dynamic_cast<const TextShape*>(document.shapes.front().get());
    REQUIRE(updated != nullptr);
    REQUIRE(updated->text == L"line1\r\nline2");

    REQUIRE(document.Undo());
    const auto* restored = dynamic_cast<const TextShape*>(document.shapes.front().get());
    REQUIRE(restored != nullptr);
    REQUIRE(restored->text == L"old");
}

TEST_CASE(EditorDocument_UpdateTagTextIsUndoable) {
    EditorDocument document;
    document.AddShape(std::make_unique<TagShape>(POINT{5, 6}, L"old"));
    document.SelectShapeAt(POINT{6, 7}, 8);

    REQUIRE(document.UpdateSelectedText(L"new\r\nlabel"));
    const auto* updated = dynamic_cast<const TagShape*>(document.shapes.front().get());
    REQUIRE(updated != nullptr);
    REQUIRE(updated->text == L"new\r\nlabel");

    REQUIRE(document.Undo());
    const auto* restored = dynamic_cast<const TagShape*>(document.shapes.front().get());
    REQUIRE(restored != nullptr);
    REQUIRE(restored->text == L"old");
}

TEST_CASE(EditorDocument_AdjustCropRectIsUndoable) {
    EditorDocument document;
    document.SetCropRect(RECT{10, 10, 100, 100});

    document.AdjustCropRect(RECT{20, 20, 120, 120});
    REQUIRE(document.cropRect->left == 20);

    REQUIRE(document.Undo());
    REQUIRE(document.cropRect->left == 10);
}
