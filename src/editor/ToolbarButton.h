#pragma once

#include <windows.h>
#include <string_view>

namespace mysnip::editor {

enum class ToolbarIcon {
    Select,
    Rectangle,
    Ellipse,
    Line,
    Arrow,
    Freehand,
    Text,
    Tag,
    Mosaic,
    Blur,
    Highlight,
    Pin,
    Number,
    Crop,
    Undo,
    Redo,
    Save,
    Cancel,
    Confirm
};

enum class ToolbarAction {
    Placeholder,
    Select,
    Rectangle,
    Ellipse,
    Line,
    Arrow,
    Freehand,
    Text,
    Tag,
    Number,
    Highlight,
    Mosaic,
    Blur,
    Crop,
    Pin,
    Undo,
    Redo,
    Save,
    Cancel,
    Confirm
};

struct ToolbarButton {
    ToolbarAction action = ToolbarAction::Placeholder;
    ToolbarIcon icon = ToolbarIcon::Select;
    std::wstring_view tooltip;
    RECT bounds{};
    bool enabled = false;
};

} // namespace mysnip::editor
