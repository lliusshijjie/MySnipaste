#include "editor/ToolManager.h"

namespace mysnip::editor {

void ToolManager::SetTool(ToolType tool) {
    currentTool_ = tool;
}

ToolType ToolManager::CurrentTool() const {
    return currentTool_;
}

void ToolManager::HandleToolbarAction(ToolbarAction action) {
    switch (action) {
    case ToolbarAction::Select:
        SetTool(ToolType::Select);
        break;
    case ToolbarAction::Rectangle:
        SetTool(ToolType::Rectangle);
        break;
    case ToolbarAction::Ellipse:
        SetTool(ToolType::Ellipse);
        break;
    case ToolbarAction::Line:
        SetTool(ToolType::Line);
        break;
    case ToolbarAction::Arrow:
        SetTool(ToolType::Arrow);
        break;
    case ToolbarAction::Freehand:
        SetTool(ToolType::Freehand);
        break;
    case ToolbarAction::Number:
        SetTool(ToolType::Number);
        break;
    case ToolbarAction::Text:
        SetTool(ToolType::Text);
        break;
    case ToolbarAction::Tag:
        SetTool(ToolType::Tag);
        break;
    case ToolbarAction::Highlight:
        SetTool(ToolType::Highlight);
        break;
    case ToolbarAction::Mosaic:
        SetTool(ToolType::Mosaic);
        break;
    case ToolbarAction::Blur:
        SetTool(ToolType::Blur);
        break;
    case ToolbarAction::Crop:
        SetTool(ToolType::Crop);
        break;
    default:
        break;
    }
}

void ToolManager::HandleShortcut(WPARAM key, bool textEditing) {
    if (textEditing) {
        return;
    }

    switch (key) {
    case 'R':
        SetTool(ToolType::Rectangle);
        break;
    case 'O':
        SetTool(ToolType::Ellipse);
        break;
    case 'L':
        SetTool(ToolType::Line);
        break;
    case 'A':
        SetTool(ToolType::Arrow);
        break;
    case 'P':
        SetTool(ToolType::Freehand);
        break;
    case 'N':
        SetTool(ToolType::Number);
        break;
    case 'T':
        SetTool(ToolType::Text);
        break;
    case 'G':
        SetTool(ToolType::Tag);
        break;
    case 'H':
        SetTool(ToolType::Highlight);
        break;
    case 'M':
        SetTool(ToolType::Mosaic);
        break;
    case 'B':
        SetTool(ToolType::Blur);
        break;
    case 'C':
        SetTool(ToolType::Crop);
        break;
    case 'V':
        SetTool(ToolType::Select);
        break;
    default:
        break;
    }
}

} // namespace mysnip::editor
