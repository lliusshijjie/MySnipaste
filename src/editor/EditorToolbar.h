#pragma once

#include "editor/ToolbarButton.h"
#include "editor/ToolType.h"

#include <optional>
#include <vector>
#include <windows.h>

namespace mysnip::editor {

class EditorToolbar {
public:
    void Layout(const RECT& captureRect, const RECT& virtualScreenRect);
    void Draw(HDC hdc, POINT origin = POINT{0, 0}) const;
    std::optional<ToolbarAction> HitTest(POINT pt) const;
    void SetCurrentTool(ToolType tool);
    void SetHistoryState(bool canUndo, bool canRedo);

    RECT Bounds() const;
    std::optional<RECT> ButtonBounds(ToolbarAction action) const;
    std::optional<RECT> FirstPlaceholderBounds() const;
    const std::vector<ToolbarButton>& Buttons() const;

private:
    void BuildButtons();

    RECT toolbarBounds_{};
    std::vector<ToolbarButton> buttons_;
    ToolType currentTool_ = ToolType::Select;
};

} // namespace mysnip::editor
