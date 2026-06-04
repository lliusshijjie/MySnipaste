#pragma once

#include "editor/ToolbarButton.h"
#include "editor/ToolType.h"

#include <windows.h>

namespace mysnip::editor {

class ToolManager {
public:
    void SetTool(ToolType tool);
    ToolType CurrentTool() const;
    void HandleToolbarAction(ToolbarAction action);
    void HandleShortcut(WPARAM key, bool textEditing = false);

private:
    ToolType currentTool_ = ToolType::Select;
};

} // namespace mysnip::editor
