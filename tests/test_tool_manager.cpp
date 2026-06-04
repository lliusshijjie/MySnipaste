#include "test_framework.h"

#include "editor/ToolManager.h"

using mysnip::editor::ToolManager;
using mysnip::editor::ToolType;
using mysnip::editor::ToolbarAction;

TEST_CASE(ToolManager_DefaultsToSelectTool) {
    ToolManager manager;
    REQUIRE(manager.CurrentTool() == ToolType::Select);
}

TEST_CASE(ToolManager_SelectsSelect) {
    ToolManager manager;
    manager.SetTool(ToolType::Arrow);
    manager.HandleToolbarAction(ToolbarAction::Select);
    REQUIRE(manager.CurrentTool() == ToolType::Select);
}

TEST_CASE(ToolManager_SelectsArrow) {
    ToolManager manager;
    manager.HandleToolbarAction(ToolbarAction::Arrow);
    REQUIRE(manager.CurrentTool() == ToolType::Arrow);
}

TEST_CASE(ToolManager_SelectsRectangle) {
    ToolManager manager;
    manager.HandleToolbarAction(ToolbarAction::Rectangle);
    REQUIRE(manager.CurrentTool() == ToolType::Rectangle);
}

TEST_CASE(ToolManager_SelectsEllipse) {
    ToolManager manager;
    manager.HandleToolbarAction(ToolbarAction::Ellipse);
    REQUIRE(manager.CurrentTool() == ToolType::Ellipse);
}

TEST_CASE(ToolManager_SelectsLine) {
    ToolManager manager;
    manager.HandleToolbarAction(ToolbarAction::Line);
    REQUIRE(manager.CurrentTool() == ToolType::Line);
}

TEST_CASE(ToolManager_SelectsNumber) {
    ToolManager manager;
    manager.HandleToolbarAction(ToolbarAction::Number);
    REQUIRE(manager.CurrentTool() == ToolType::Number);
}

TEST_CASE(ToolManager_SelectsFreehand) {
    ToolManager manager;
    manager.HandleToolbarAction(ToolbarAction::Freehand);
    REQUIRE(manager.CurrentTool() == ToolType::Freehand);
}

TEST_CASE(ToolManager_SelectsTag) {
    ToolManager manager;
    manager.HandleToolbarAction(ToolbarAction::Tag);
    REQUIRE(manager.CurrentTool() == ToolType::Tag);
}

TEST_CASE(ToolManager_SelectsBlur) {
    ToolManager manager;
    manager.HandleToolbarAction(ToolbarAction::Blur);
    REQUIRE(manager.CurrentTool() == ToolType::Blur);
}

TEST_CASE(ToolManager_UndoActionDoesNotChangeTool) {
    ToolManager manager;
    manager.SetTool(ToolType::Arrow);
    manager.HandleToolbarAction(ToolbarAction::Undo);
    REQUIRE(manager.CurrentTool() == ToolType::Arrow);
}

TEST_CASE(ToolManager_SelectsLineByShortcut) {
    ToolManager manager;
    manager.HandleShortcut('L');
    REQUIRE(manager.CurrentTool() == ToolType::Line);
}

TEST_CASE(ToolManager_SelectsRectangleByShortcut) {
    ToolManager manager;
    manager.HandleShortcut('R');
    REQUIRE(manager.CurrentTool() == ToolType::Rectangle);
}

TEST_CASE(ToolManager_SelectsEllipseByShortcut) {
    ToolManager manager;
    manager.HandleShortcut('O');
    REQUIRE(manager.CurrentTool() == ToolType::Ellipse);
}

TEST_CASE(ToolManager_SelectsNumberByShortcut) {
    ToolManager manager;
    manager.HandleShortcut('N');
    REQUIRE(manager.CurrentTool() == ToolType::Number);
}

TEST_CASE(ToolManager_SelectsSelectByShortcut) {
    ToolManager manager;
    manager.SetTool(ToolType::Arrow);
    manager.HandleShortcut('V');
    REQUIRE(manager.CurrentTool() == ToolType::Select);
}

TEST_CASE(ToolManager_SelectsFreehandByShortcut) {
    ToolManager manager;
    manager.HandleShortcut('P');
    REQUIRE(manager.CurrentTool() == ToolType::Freehand);
}

TEST_CASE(ToolManager_SelectsTagByShortcut) {
    ToolManager manager;
    manager.HandleShortcut('G');
    REQUIRE(manager.CurrentTool() == ToolType::Tag);
}

TEST_CASE(ToolManager_SelectsBlurByShortcut) {
    ToolManager manager;
    manager.HandleShortcut('B');
    REQUIRE(manager.CurrentTool() == ToolType::Blur);
}

TEST_CASE(ToolManager_SelectsText) {
    ToolManager manager;
    manager.HandleToolbarAction(ToolbarAction::Text);
    REQUIRE(manager.CurrentTool() == ToolType::Text);
}

TEST_CASE(ToolManager_SelectsHighlight) {
    ToolManager manager;
    manager.HandleToolbarAction(ToolbarAction::Highlight);
    REQUIRE(manager.CurrentTool() == ToolType::Highlight);
}

TEST_CASE(ToolManager_SelectsMosaic) {
    ToolManager manager;
    manager.HandleToolbarAction(ToolbarAction::Mosaic);
    REQUIRE(manager.CurrentTool() == ToolType::Mosaic);
}

TEST_CASE(ToolManager_SelectsCrop) {
    ToolManager manager;
    manager.HandleToolbarAction(ToolbarAction::Crop);
    REQUIRE(manager.CurrentTool() == ToolType::Crop);
}

TEST_CASE(ToolManager_IgnoresToolShortcutWhileTextEditing) {
    ToolManager manager;
    manager.SetTool(ToolType::Text);
    manager.HandleShortcut('A', true);
    REQUIRE(manager.CurrentTool() == ToolType::Text);
}
