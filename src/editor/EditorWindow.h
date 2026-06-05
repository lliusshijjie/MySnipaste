#pragma once

#include "capture/ICaptureBackend.h"
#include "editor/EditorDocument.h"
#include "editor/EditorGeometry.h"
#include "editor/ToolManager.h"
#include "editor/EditorResult.h"
#include "editor/EditorToolbar.h"
#include "editor/EditorViewport.h"
#include "editor/EditorStyleBar.h"
#include "editor/CropGeometry.h"

#include <memory>
#include <optional>
#include <vector>
#include <windows.h>

namespace mysnip::editor {

class EditorWindow {
public:
    EditorResult ShowModal(HWND owner, const capture::CaptureResult& captureResult);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    bool CreateEditorWindow(HWND owner);
    void CreateToolbarTooltips();
    void DestroyToolbarTooltips();
    void UpdateToolbarTooltipRects();
    void RelayTooltipMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void Complete(EditorStatus status);
    void ConfirmAndExport();
    void PinAndClose();
    void SavePngFromDialog();
    void RunOcr();
    void OnPaint();
    void DrawScene(HDC hdc, const RECT& client);
    void InvalidateEditor();
    void RefreshToolbarHistoryState();
    void OnClick(POINT screenPoint);
    void OnDoubleClick(POINT screenPoint);
    void OnMouseDown(POINT screenPoint);
    void OnMouseMove(POINT screenPoint);
    void OnMouseUp(POINT screenPoint);
    void OnMouseWheel(POINT screenPoint, int delta, bool ctrlDown);
    void OnKeyDown(WPARAM key);
    void BeginTextInput(POINT imagePoint, const std::wstring& initialText = L"", std::optional<std::size_t> editIndex = std::nullopt);
    void PlaceNumber(POINT imagePoint);
    void CommitTextInput(bool keepText);
    void DrawTextInputFrame(HDC hdc);
    void DrawSelectedShapeBounds(HDC hdc);
    bool HasActiveTextInput() const;
    std::optional<POINT> ScreenPointToImage(POINT screenPoint) const;
    POINT ClampScreenPointToImage(POINT screenPoint) const;
    std::unique_ptr<Shape> MakeShapeForCurrentDrag() const;
    void UpdatePreviewShape();
    void DrawCropPreview(HDC hdc, const RECT& imageClientRect);
    void RefreshViewportClientRect();

    static LRESULT CALLBACK TextEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    HWND hwnd_ = nullptr;
    HWND owner_ = nullptr;
    HWND tooltipHwnd_ = nullptr;
    RECT virtualScreen_{};
    RECT imageRect_{};
    RECT imageClientRect_{};
    RECT viewportClientRect_{};
    EditorViewport viewport_;
    EditorToolbar toolbar_;
    EditorStyleBar styleBar_;
    EditorDocument document_;
    ToolManager toolManager_;
    std::unique_ptr<Shape> previewShape_;
    std::optional<RECT> previewCropRect_;
    bool dragging_ = false;
    bool draggingToolbar_ = false;
    bool movingSelected_ = false;
    bool movedSelectedDuringDrag_ = false;
    bool adjustingCrop_ = false;
    CropHandle activeCropHandle_ = CropHandle::None;
    RECT cropDragOriginal_{};
    POINT dragStartImage_{};
    POINT dragCurrentImage_{};
    POINT lastMoveImage_{};
    POINT toolbarDragLastScreen_{};
    std::vector<POINT> freehandPoints_;
    std::unique_ptr<EditorDocument::DocumentSnapshot> dragBeforeSnapshot_;
    HWND textEditHwnd_ = nullptr;
    HBRUSH textEditBrush_ = nullptr;
    HFONT textEditFont_ = nullptr;
    WNDPROC originalTextEditProc_ = nullptr;
    bool closingTextEdit_ = false;
    bool textInputCreatesTag_ = false;
    POINT textPositionImage_{};
    std::optional<std::size_t> editingTextIndex_;
    RECT textInputFrameClient_{};
    EditorResult result_{};
    bool done_ = false;
};

} // namespace mysnip::editor
