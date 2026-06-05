#include "editor/EditorWindow.h"

#include "app/CommandIds.h"
#include "editor/ArrowShape.h"
#include "editor/BlurShape.h"
#include "editor/EditorPaintConfig.h"
#include "editor/EditorRenderer.h"
#include "editor/EllipseShape.h"
#include "editor/FreehandShape.h"
#include "editor/HighlightShape.h"
#include "editor/ImageRenderUtils.h"
#include "editor/LineShape.h"
#include "editor/MosaicShape.h"
#include "editor/NumberShape.h"
#include "editor/PinPosition.h"
#include "editor/RectangleShape.h"
#include "editor/TagShape.h"
#include "editor/TextShape.h"
#include "export/PngFileExporter.h"
#include "ocr/OcrTextDialog.h"
#include "ocr/WindowsOcrEngine.h"
#include "utils/LogUtils.h"
#include "utils/RectUtils.h"
#include "utils/VirtualScreenUtils.h"

#include <memory>
#include <string>
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <windowsx.h>

namespace mysnip::editor {
namespace {

constexpr wchar_t kEditorWindowClass[] = L"MySnipasteEditorWindow";
constexpr int kTextInputWidth = 240;
constexpr int kTextInputHeight = 96;
constexpr int kTextInputPaddingX = 10;
constexpr int kTextInputPaddingY = 7;
constexpr int kSelectionTolerance = 6;
constexpr int kCropHandleTolerance = 7;
constexpr int kCropMinimumSize = 12;

POINT ScreenPointFromLParam(HWND hwnd, LPARAM lParam) {
    POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    ClientToScreen(hwnd, &pt);
    return pt;
}

} // namespace

EditorResult EditorWindow::ShowModal(HWND owner, const capture::CaptureResult& captureResult) {
    owner_ = owner;
    document_ = {};
    document_.baseImage = captureResult.image;
    document_.screenRect = captureResult.screenRect;
    result_ = {};
    done_ = false;

    if (!CreateEditorWindow(owner)) {
        result_.status = EditorStatus::Failed;
        return result_;
    }

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    SetForegroundWindow(hwnd_);
    SetFocus(hwnd_);

    MSG msg{};
    while (!done_ && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hwnd_) {
        DestroyToolbarTooltips();
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }

    return result_;
}

bool EditorWindow::CreateEditorWindow(HWND owner) {
    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(owner, GWLP_HINSTANCE));

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &EditorWindow::WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kEditorWindowClass;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.style = CS_DBLCLKS;

    RegisterClassExW(&wc);

    virtualScreen_ = utils::GetVirtualScreenRect();
    imageRect_ = document_.screenRect;
    const bool longImage = document_.baseImage.width != utils::Width(imageRect_) ||
        document_.baseImage.height != utils::Height(imageRect_);
    viewport_.Layout(document_.baseImage, imageRect_, virtualScreen_, longImage);
    RefreshViewportClientRect();
    utils::LogRect(L"[Editor] virtual screen", virtualScreen_);
    utils::LogRect(L"[Editor] image screen", imageRect_);
    toolbar_.Layout(imageRect_, virtualScreen_);
    toolbar_.SetCurrentTool(toolManager_.CurrentTool());
    toolbar_.SetHistoryState(document_.CanUndo(), document_.CanRedo());
    styleBar_.Layout(toolbar_.Bounds(), virtualScreen_);

    hwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        kEditorWindowClass,
        L"MySnipaste Editor",
        WS_POPUP,
        virtualScreen_.left,
        virtualScreen_.top,
        utils::Width(virtualScreen_),
        utils::Height(virtualScreen_),
        owner,
        nullptr,
        instance,
        this);

    if (!hwnd_) {
        utils::LogLastError(L"[Editor] CreateWindowExW");
        return false;
    }

    if (kEditorUsesTransparentBackground &&
        !SetLayeredWindowAttributes(hwnd_, kEditorTransparentBackgroundColor, 0, LWA_COLORKEY)) {
        utils::LogLastError(L"[Editor] SetLayeredWindowAttributes");
    }

    utils::LogDpi(L"[Editor] window", hwnd_);
    CreateToolbarTooltips();
    return true;
}

void EditorWindow::CreateToolbarTooltips() {
    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&controls);

    tooltipHwnd_ = CreateWindowExW(
        WS_EX_TOPMOST,
        TOOLTIPS_CLASSW,
        nullptr,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        hwnd_,
        nullptr,
        reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd_, GWLP_HINSTANCE)),
        nullptr);
    if (!tooltipHwnd_) {
        utils::LogLastError(L"[Editor] Create tooltip");
        return;
    }

    SetWindowPos(tooltipHwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SendMessageW(tooltipHwnd_, TTM_ACTIVATE, TRUE, 0);
    SendMessageW(tooltipHwnd_, TTM_SETDELAYTIME, TTDT_INITIAL, 250);
    SendMessageW(tooltipHwnd_, TTM_SETDELAYTIME, TTDT_RESHOW, 80);
    SendMessageW(tooltipHwnd_, TTM_SETDELAYTIME, TTDT_AUTOPOP, 5000);
    SendMessageW(tooltipHwnd_, TTM_SETMAXTIPWIDTH, 0, 320);

    int toolId = 1;
    for (const auto& button : toolbar_.Buttons()) {
        TOOLINFOW info{};
        info.cbSize = sizeof(info);
        info.uFlags = TTF_TRANSPARENT;
        info.hwnd = hwnd_;
        info.uId = static_cast<UINT_PTR>(toolId++);
        info.rect = utils::ScreenRectToVirtualClientRect(button.bounds, virtualScreen_);
        info.lpszText = const_cast<LPWSTR>(button.tooltip.data());
        SendMessageW(tooltipHwnd_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&info));
    }
}

void EditorWindow::UpdateToolbarTooltipRects() {
    if (!tooltipHwnd_) {
        return;
    }

    int toolId = 1;
    for (const auto& button : toolbar_.Buttons()) {
        TOOLINFOW info{};
        info.cbSize = sizeof(info);
        info.hwnd = hwnd_;
        info.uId = static_cast<UINT_PTR>(toolId++);
        info.rect = utils::ScreenRectToVirtualClientRect(button.bounds, virtualScreen_);
        SendMessageW(tooltipHwnd_, TTM_NEWTOOLRECTW, 0, reinterpret_cast<LPARAM>(&info));
    }
}

void EditorWindow::RelayTooltipMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    if (!tooltipHwnd_) {
        return;
    }

    const DWORD messagePos = GetMessagePos();
    MSG msg{};
    msg.hwnd = hwnd_;
    msg.message = message;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.time = GetMessageTime();
    msg.pt = POINT{GET_X_LPARAM(messagePos), GET_Y_LPARAM(messagePos)};
    SendMessageW(tooltipHwnd_, TTM_RELAYEVENT, 0, reinterpret_cast<LPARAM>(&msg));
}

void EditorWindow::DestroyToolbarTooltips() {
    if (tooltipHwnd_) {
        DestroyWindow(tooltipHwnd_);
        tooltipHwnd_ = nullptr;
    }
}

void EditorWindow::Complete(EditorStatus status) {
    if (HasActiveTextInput()) {
        CommitTextInput(true);
    }
    result_.status = status;
    done_ = true;
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

void EditorWindow::ConfirmAndExport() {
    if (HasActiveTextInput()) {
        CommitTextInput(true);
        return;
    }

    auto finalImage = EditorRenderer::RenderFinalImage(document_);
    if (!finalImage.has_value()) {
        Complete(EditorStatus::Failed);
        return;
    }

    result_.status = EditorStatus::Confirmed;
    result_.finalImage = std::move(*finalImage);
    done_ = true;
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

void EditorWindow::PinAndClose() {
    if (HasActiveTextInput()) {
        CommitTextInput(true);
        return;
    }

    auto finalImage = EditorRenderer::RenderFinalImage(document_);
    if (!finalImage.has_value()) {
        Complete(EditorStatus::Failed);
        return;
    }

    result_.status = EditorStatus::Pinned;
    result_.finalImage = std::move(*finalImage);
    result_.pinPosition = ComputePinnedImagePosition(capture::CaptureResult{document_.baseImage, document_.screenRect}, document_);
    done_ = true;
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

void EditorWindow::SavePngFromDialog() {
    if (HasActiveTextInput()) {
        CommitTextInput(true);
    }

    auto finalImage = EditorRenderer::RenderFinalImage(document_);
    if (!finalImage.has_value()) {
        PostMessageW(owner_, app::kEditorNotificationMessage, app::kEditorNotifyPngSaveFailed, 0);
        return;
    }

    wchar_t path[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"PNG Image (*.png)\0*.png\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"png";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetSaveFileNameW(&ofn)) {
        return;
    }

    std::wstring savePath(path);
    if (savePath.size() < 4 || _wcsicmp(savePath.c_str() + savePath.size() - 4, L".png") != 0) {
        savePath += L".png";
    }

    const bool saved = exporter::PngFileExporter::SavePng(savePath, *finalImage);
    PostMessageW(
        owner_,
        app::kEditorNotificationMessage,
        saved ? app::kEditorNotifyPngSaved : app::kEditorNotifyPngSaveFailed,
        0);
}

void EditorWindow::RunOcr() {
    if (HasActiveTextInput()) {
        CommitTextInput(true);
    }

    auto finalImage = EditorRenderer::RenderFinalImage(document_);
    if (!finalImage.has_value()) {
        PostMessageW(owner_, app::kEditorNotificationMessage, app::kEditorNotifyOcrFailed, 0);
        return;
    }

    ocr::WindowsOcrEngine engine;
    if (engine.Availability() != ocr::OcrAvailability::Available) {
        PostMessageW(owner_, app::kEditorNotificationMessage, app::kEditorNotifyOcrUnavailable, 0);
        return;
    }

    const auto result = engine.Recognize(*finalImage);
    if (!result.has_value()) {
        PostMessageW(owner_, app::kEditorNotificationMessage, app::kEditorNotifyOcrFailed, 0);
        return;
    }

    ocr::OcrTextDialog dialog;
    dialog.ShowModal(hwnd_, result->text);
    SetForegroundWindow(hwnd_);
    SetFocus(hwnd_);
}

void EditorWindow::OnPaint() {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd_, &ps);
    if (!hdc) {
        return;
    }

    RECT client{};
    GetClientRect(hwnd_, &client);
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    HDC memoryDc = CreateCompatibleDC(hdc);
    HBITMAP bitmap = memoryDc ? CreateCompatibleBitmap(hdc, width, height) : nullptr;
    if (memoryDc && bitmap) {
        HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);
        DrawScene(memoryDc, client);
        BitBlt(hdc, 0, 0, width, height, memoryDc, 0, 0, SRCCOPY);
        SelectObject(memoryDc, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memoryDc);
        EndPaint(hwnd_, &ps);
        return;
    }
    if (bitmap) {
        DeleteObject(bitmap);
    }
    if (memoryDc) {
        DeleteDC(memoryDc);
    }

    DrawScene(hdc, client);
    EndPaint(hwnd_, &ps);
}

void EditorWindow::DrawScene(HDC hdc, const RECT& client) {
    HBRUSH backgroundBrush = CreateSolidBrush(kEditorTransparentBackgroundColor);
    FillRect(hdc, &client, backgroundBrush);
    DeleteObject(backgroundBrush);

    const int savedDc = SaveDC(hdc);
    IntersectClipRect(
        hdc,
        viewportClientRect_.left,
        viewportClientRect_.top,
        viewportClientRect_.right,
        viewportClientRect_.bottom);
    if (!EditorRenderer::DrawPreview(hdc, document_, imageClientRect_, previewShape_.get())) {
        utils::LogError(L"[Editor] DrawImage failed.");
    }
    DrawCropPreview(hdc, imageClientRect_);
    DrawSelectedShapeBounds(hdc);
    DrawTextInputFrame(hdc);
    RestoreDC(hdc, savedDc);

    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(40, 130, 240));
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, viewportClientRect_.left, viewportClientRect_.top, viewportClientRect_.right, viewportClientRect_.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);

    toolbar_.Draw(hdc, POINT{virtualScreen_.left, virtualScreen_.top});
    styleBar_.Draw(hdc, POINT{virtualScreen_.left, virtualScreen_.top});
}

void EditorWindow::InvalidateEditor() {
    if (hwnd_) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void EditorWindow::RefreshToolbarHistoryState() {
    toolbar_.SetHistoryState(document_.CanUndo(), document_.CanRedo());
}

void EditorWindow::OnClick(POINT screenPoint) {
    const auto styleAction = styleBar_.HitTest(screenPoint);
    if (styleAction.has_value()) {
        if (HasActiveTextInput()) {
            CommitTextInput(true);
        }
        if (styleBar_.ApplyAction(*styleAction)) {
            if (document_.SelectedShape()) {
                document_.ApplySelectedStyle(styleBar_.Style());
                RefreshToolbarHistoryState();
            }
            InvalidateEditor();
        }
        return;
    }

    const auto action = toolbar_.HitTest(screenPoint);
    if (!action.has_value()) {
        return;
    }

    if (*action == ToolbarAction::Confirm) {
        ConfirmAndExport();
    } else if (*action == ToolbarAction::Cancel) {
        Complete(EditorStatus::Cancelled);
    } else if (*action == ToolbarAction::Pin) {
        PinAndClose();
    } else if (*action == ToolbarAction::Save) {
        SavePngFromDialog();
    } else if (*action == ToolbarAction::Ocr) {
        RunOcr();
    } else if (*action == ToolbarAction::Undo) {
        if (HasActiveTextInput()) {
            CommitTextInput(true);
        }
        if (document_.Undo()) {
            RefreshToolbarHistoryState();
            if (const Shape* selected = document_.SelectedShape()) {
                styleBar_.SetStyle(selected->Style());
            }
        }
        InvalidateEditor();
    } else if (*action == ToolbarAction::Redo) {
        if (HasActiveTextInput()) {
            CommitTextInput(true);
        }
        if (document_.Redo()) {
            RefreshToolbarHistoryState();
            if (const Shape* selected = document_.SelectedShape()) {
                styleBar_.SetStyle(selected->Style());
            }
        }
        InvalidateEditor();
    } else {
        if (HasActiveTextInput()) {
            CommitTextInput(true);
        }
        toolManager_.HandleToolbarAction(*action);
        toolbar_.SetCurrentTool(toolManager_.CurrentTool());
        InvalidateEditor();
    }
}

std::optional<POINT> EditorWindow::ScreenPointToImage(POINT screenPoint) const {
    return viewport_.ScreenToImage(screenPoint);
}

POINT EditorWindow::ClampScreenPointToImage(POINT screenPoint) const {
    return viewport_.ClampScreenToImage(screenPoint);
}

std::unique_ptr<Shape> EditorWindow::MakeShapeForCurrentDrag() const {
    std::unique_ptr<Shape> shape;
    switch (toolManager_.CurrentTool()) {
    case ToolType::Arrow:
        shape = std::make_unique<ArrowShape>(dragStartImage_, dragCurrentImage_);
        break;
    case ToolType::Line:
        shape = std::make_unique<LineShape>(dragStartImage_, dragCurrentImage_);
        break;
    case ToolType::Freehand:
        shape = std::make_unique<FreehandShape>(freehandPoints_);
        break;
    case ToolType::Rectangle:
        shape = std::make_unique<RectangleShape>(NormalizeImageRect(dragStartImage_, dragCurrentImage_));
        break;
    case ToolType::Ellipse:
        shape = std::make_unique<EllipseShape>(NormalizeImageRect(dragStartImage_, dragCurrentImage_));
        break;
    case ToolType::Highlight:
        shape = std::make_unique<HighlightShape>(NormalizeImageRect(dragStartImage_, dragCurrentImage_));
        break;
    case ToolType::Mosaic:
        shape = std::make_unique<MosaicShape>(NormalizeImageRect(dragStartImage_, dragCurrentImage_));
        break;
    case ToolType::Blur:
        shape = std::make_unique<BlurShape>(NormalizeImageRect(dragStartImage_, dragCurrentImage_));
        break;
    default:
        break;
    }
    if (shape) {
        shape->ApplyStyle(styleBar_.Style());
    }
    return shape;
}

void EditorWindow::UpdatePreviewShape() {
    if (toolManager_.CurrentTool() == ToolType::Crop) {
        previewCropRect_ = NormalizeImageRect(dragStartImage_, dragCurrentImage_);
        previewShape_.reset();
        return;
    }
    previewCropRect_.reset();
    previewShape_ = MakeShapeForCurrentDrag();
}

void EditorWindow::OnMouseDown(POINT screenPoint) {
    if (HasActiveTextInput()) {
        CommitTextInput(true);
    }
    if (toolbar_.IsDragSurface(screenPoint)) {
        draggingToolbar_ = true;
        toolbarDragLastScreen_ = screenPoint;
        SetCapture(hwnd_);
        return;
    }
    if (toolbar_.HitTest(screenPoint).has_value() || styleBar_.HitTest(screenPoint).has_value()) {
        return;
    }
    const auto imagePoint = ScreenPointToImage(screenPoint);
    if (!imagePoint.has_value()) {
        return;
    }
    const ToolType tool = toolManager_.CurrentTool();

    if (tool == ToolType::Select) {
        if (document_.SelectShapeAt(*imagePoint, kSelectionTolerance)) {
            if (const Shape* selected = document_.SelectedShape()) {
                styleBar_.SetStyle(selected->Style());
            }
            movingSelected_ = true;
            movedSelectedDuringDrag_ = false;
            lastMoveImage_ = *imagePoint;
            dragBeforeSnapshot_ = std::make_unique<EditorDocument::DocumentSnapshot>(document_.CaptureSnapshot());
            SetCapture(hwnd_);
        }
        InvalidateEditor();
        return;
    }

    if (tool == ToolType::Crop && document_.cropRect.has_value()) {
        const CropHandle handle = HitTestCropHandle(*document_.cropRect, *imagePoint, kCropHandleTolerance);
        if (handle != CropHandle::None) {
            adjustingCrop_ = true;
            activeCropHandle_ = handle;
            cropDragOriginal_ = *document_.cropRect;
            dragStartImage_ = *imagePoint;
            dragCurrentImage_ = *imagePoint;
            previewCropRect_ = cropDragOriginal_;
            SetCapture(hwnd_);
            InvalidateEditor();
            return;
        }
    }

    const bool draggableTool =
        tool == ToolType::Arrow ||
        tool == ToolType::Line ||
        tool == ToolType::Freehand ||
        tool == ToolType::Rectangle ||
        tool == ToolType::Ellipse ||
        tool == ToolType::Highlight ||
        tool == ToolType::Mosaic ||
        tool == ToolType::Blur ||
        tool == ToolType::Crop;
    if (!draggableTool) {
        return;
    }

    dragging_ = true;
    dragStartImage_ = *imagePoint;
    dragCurrentImage_ = *imagePoint;
    freehandPoints_.clear();
    if (tool == ToolType::Freehand) {
        freehandPoints_.push_back(*imagePoint);
    }
    SetCapture(hwnd_);
    UpdatePreviewShape();
    InvalidateEditor();
}

void EditorWindow::OnMouseMove(POINT screenPoint) {
    if (draggingToolbar_) {
        const int dx = screenPoint.x - toolbarDragLastScreen_.x;
        const int dy = screenPoint.y - toolbarDragLastScreen_.y;
        if (dx != 0 || dy != 0) {
            toolbar_.MoveBy(dx, dy, virtualScreen_);
            styleBar_.Layout(toolbar_.Bounds(), virtualScreen_);
            UpdateToolbarTooltipRects();
            toolbarDragLastScreen_ = screenPoint;
            InvalidateEditor();
        }
        return;
    }

    if (movingSelected_) {
        const POINT imagePoint = ClampScreenPointToImage(screenPoint);
        const int dx = imagePoint.x - lastMoveImage_.x;
        const int dy = imagePoint.y - lastMoveImage_.y;
        if (dx != 0 || dy != 0) {
            if (Shape* selected = document_.SelectedShape()) {
                const RECT beforeBounds = selected->Bounds();
                selected->MoveBy(dx, dy, SIZE{document_.baseImage.width, document_.baseImage.height});
                const RECT afterBounds = selected->Bounds();
                if (!EqualRect(&beforeBounds, &afterBounds)) {
                    movedSelectedDuringDrag_ = true;
                    InvalidateEditor();
                }
                lastMoveImage_ = imagePoint;
            }
        }
        return;
    }

    if (adjustingCrop_) {
        dragCurrentImage_ = ClampScreenPointToImage(screenPoint);
        if (activeCropHandle_ == CropHandle::Move) {
            previewCropRect_ = MoveCropRect(
                cropDragOriginal_,
                dragCurrentImage_.x - dragStartImage_.x,
                dragCurrentImage_.y - dragStartImage_.y,
                SIZE{document_.baseImage.width, document_.baseImage.height});
        } else {
            previewCropRect_ = ResizeCropRect(
                cropDragOriginal_,
                activeCropHandle_,
                dragCurrentImage_,
                SIZE{document_.baseImage.width, document_.baseImage.height},
                kCropMinimumSize);
        }
        InvalidateEditor();
        return;
    }

    if (!dragging_) {
        return;
    }
    dragCurrentImage_ = ClampScreenPointToImage(screenPoint);
    if (toolManager_.CurrentTool() == ToolType::Freehand) {
        if (freehandPoints_.empty() ||
            freehandPoints_.back().x != dragCurrentImage_.x ||
            freehandPoints_.back().y != dragCurrentImage_.y) {
            freehandPoints_.push_back(dragCurrentImage_);
        }
    }
    UpdatePreviewShape();
    InvalidateEditor();
}

void EditorWindow::OnMouseUp(POINT screenPoint) {
    if (draggingToolbar_) {
        draggingToolbar_ = false;
        ReleaseCapture();
        InvalidateEditor();
        return;
    }

    if (movingSelected_) {
        movingSelected_ = false;
        ReleaseCapture();
        if (dragBeforeSnapshot_ && movedSelectedDuringDrag_) {
            document_.CommitSnapshot(std::move(*dragBeforeSnapshot_));
            RefreshToolbarHistoryState();
        }
        dragBeforeSnapshot_.reset();
        movedSelectedDuringDrag_ = false;
        InvalidateEditor();
        return;
    }

    if (adjustingCrop_) {
        dragCurrentImage_ = ClampScreenPointToImage(screenPoint);
        if (previewCropRect_.has_value()) {
            document_.AdjustCropRect(*previewCropRect_);
            RefreshToolbarHistoryState();
        }
        adjustingCrop_ = false;
        activeCropHandle_ = CropHandle::None;
        previewCropRect_.reset();
        ReleaseCapture();
        InvalidateEditor();
        return;
    }

    if (dragging_) {
        dragCurrentImage_ = ClampScreenPointToImage(screenPoint);
        const ToolType tool = toolManager_.CurrentTool();
        const RECT rect = NormalizeImageRect(dragStartImage_, dragCurrentImage_);
        bool tooSmall;
        if (tool == ToolType::Arrow || tool == ToolType::Line) {
            const int dx = dragCurrentImage_.x - dragStartImage_.x;
            const int dy = dragCurrentImage_.y - dragStartImage_.y;
            tooSmall = (dx * dx + dy * dy) < 9;
        } else if (tool == ToolType::Freehand) {
            tooSmall = freehandPoints_.size() < 2;
        } else {
            tooSmall = (rect.right - rect.left) < 3 || (rect.bottom - rect.top) < 3;
        }
        if (!tooSmall) {
            if (tool == ToolType::Crop) {
                document_.SetCropRect(rect);
            } else {
                document_.AddShape(MakeShapeForCurrentDrag());
            }
            RefreshToolbarHistoryState();
        }
        dragging_ = false;
        freehandPoints_.clear();
        previewShape_.reset();
        previewCropRect_.reset();
        ReleaseCapture();
        InvalidateEditor();
        return;
    }

    OnClick(screenPoint);
    if (toolbar_.HitTest(screenPoint).has_value() || styleBar_.HitTest(screenPoint).has_value()) {
        return;
    }
    const ToolType tool = toolManager_.CurrentTool();
    if (tool == ToolType::Text) {
        const auto imagePoint = ScreenPointToImage(screenPoint);
        if (imagePoint.has_value()) {
            BeginTextInput(*imagePoint);
        }
    } else if (tool == ToolType::Tag) {
        const auto imagePoint = ScreenPointToImage(screenPoint);
        if (imagePoint.has_value()) {
            BeginTextInput(*imagePoint);
        }
    } else if (tool == ToolType::Number) {
        const auto imagePoint = ScreenPointToImage(screenPoint);
        if (imagePoint.has_value()) {
            PlaceNumber(*imagePoint);
        }
    }
}

void EditorWindow::OnMouseWheel(POINT screenPoint, int delta, bool ctrlDown) {
    if (HasActiveTextInput()) {
        CommitTextInput(true);
    }
    if (ctrlDown) {
        viewport_.ZoomAt(screenPoint, delta > 0 ? 1.1f : 0.9f);
    } else {
        viewport_.ScrollBy(-delta);
    }
    RefreshViewportClientRect();
    InvalidateEditor();
}

void EditorWindow::OnDoubleClick(POINT screenPoint) {
    if (toolbar_.HitTest(screenPoint).has_value() || styleBar_.HitTest(screenPoint).has_value()) {
        return;
    }
    const auto imagePoint = ScreenPointToImage(screenPoint);
    if (!imagePoint.has_value()) {
        return;
    }
    if (!document_.SelectShapeAt(*imagePoint, kSelectionTolerance)) {
        return;
    }
    Shape* selectedShape = document_.SelectedShape();
    auto* textShape = dynamic_cast<TextShape*>(selectedShape);
    auto* tagShape = dynamic_cast<TagShape*>(selectedShape);
    if ((!textShape && !tagShape) || !document_.selectedShapeIndex.has_value()) {
        InvalidateEditor();
        return;
    }
    if (textShape) {
        styleBar_.SetStyle(textShape->Style());
        BeginTextInput(textShape->position, textShape->text, document_.selectedShapeIndex);
    } else {
        styleBar_.SetStyle(tagShape->Style());
        BeginTextInput(tagShape->position, tagShape->text, document_.selectedShapeIndex);
    }
}

void EditorWindow::PlaceNumber(POINT imagePoint) {
    int count = 0;
    for (const auto& shape : document_.shapes) {
        if (dynamic_cast<const NumberShape*>(shape.get()) != nullptr) {
            ++count;
        }
    }
    document_.AddShape(std::make_unique<NumberShape>(imagePoint, count + 1));
    RefreshToolbarHistoryState();
    InvalidateEditor();
}

void EditorWindow::OnKeyDown(WPARAM key) {
    if (key == 'O' && (GetKeyState(VK_CONTROL) & 0x8000) != 0 && !HasActiveTextInput()) {
        RunOcr();
        return;
    }
    if (key == 'S' && (GetKeyState(VK_CONTROL) & 0x8000) != 0 && !HasActiveTextInput()) {
        SavePngFromDialog();
        return;
    }
    if ((key == VK_DELETE || key == VK_BACK) && !HasActiveTextInput()) {
        if (document_.DeleteSelectedShape()) {
            RefreshToolbarHistoryState();
            InvalidateEditor();
        }
        return;
    }
    if (key == VK_RETURN) {
        ConfirmAndExport();
        return;
    }
    if (key == VK_ESCAPE) {
        if (document_.selectedShapeIndex.has_value() && !HasActiveTextInput()) {
            document_.selectedShapeIndex.reset();
            InvalidateEditor();
            return;
        }
        Complete(EditorStatus::Cancelled);
        return;
    }
    if (key == 'Z' && (GetKeyState(VK_CONTROL) & 0x8000) != 0 && !HasActiveTextInput()) {
        if (document_.Undo()) {
            RefreshToolbarHistoryState();
        }
        InvalidateEditor();
        return;
    }
    if (key == 'Y' && (GetKeyState(VK_CONTROL) & 0x8000) != 0 && !HasActiveTextInput()) {
        if (document_.Redo()) {
            RefreshToolbarHistoryState();
        }
        InvalidateEditor();
        return;
    }
    toolManager_.HandleShortcut(key, HasActiveTextInput());
    toolbar_.SetCurrentTool(toolManager_.CurrentTool());
    InvalidateEditor();
}

bool EditorWindow::HasActiveTextInput() const {
    return textEditHwnd_ != nullptr;
}

void EditorWindow::BeginTextInput(POINT imagePoint, const std::wstring& initialText, std::optional<std::size_t> editIndex) {
    if (HasActiveTextInput()) {
        CommitTextInput(true);
    }
    textPositionImage_ = imagePoint;
    editingTextIndex_ = editIndex;
    textInputCreatesTag_ = !editIndex.has_value() && toolManager_.CurrentTool() == ToolType::Tag;
    const POINT clientPoint = viewport_.ImagePointToClient(imagePoint);
    const int x = clientPoint.x;
    const int y = clientPoint.y;
    const int availableWidth = static_cast<int>(imageClientRect_.right - x);
    const int availableHeight = static_cast<int>(imageClientRect_.bottom - y);
    const int frameWidth = (std::min)(kTextInputWidth + 80, availableWidth);
    const int frameHeight = (std::min)(kTextInputHeight, availableHeight);
    textInputFrameClient_ = RECT{x, y, x + (std::max)(120, frameWidth), y + (std::max)(48, frameHeight)};
    textEditHwnd_ = CreateWindowExW(
        0,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        x + kTextInputPaddingX,
        y + kTextInputPaddingY,
        (textInputFrameClient_.right - textInputFrameClient_.left) - (kTextInputPaddingX * 2),
        (textInputFrameClient_.bottom - textInputFrameClient_.top) - (kTextInputPaddingY * 2),
        hwnd_,
        nullptr,
        reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd_, GWLP_HINSTANCE)),
        nullptr);
    if (!textEditHwnd_) {
        utils::LogLastError(L"[Editor] Create text edit");
        return;
    }
    if (!initialText.empty()) {
        SetWindowTextW(textEditHwnd_, initialText.c_str());
    }
    textEditBrush_ = CreateSolidBrush(RGB(255, 255, 255));
    textEditFont_ = CreateFontW(
        -styleBar_.Style().fontSize,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Segoe UI");
    if (textEditFont_) {
        SendMessageW(textEditHwnd_, WM_SETFONT, reinterpret_cast<WPARAM>(textEditFont_), TRUE);
    }
    SetWindowLongPtrW(textEditHwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    originalTextEditProc_ = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(textEditHwnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&EditorWindow::TextEditProc)));
    InvalidateEditor();
    SetFocus(textEditHwnd_);
}

void EditorWindow::CommitTextInput(bool keepText) {
    if (!textEditHwnd_ || closingTextEdit_) {
        return;
    }
    closingTextEdit_ = true;
    std::wstring text;
    if (keepText) {
        const int length = GetWindowTextLengthW(textEditHwnd_);
        if (length > 0) {
            std::wstring buffer(static_cast<std::size_t>(length) + 1, L'\0');
            GetWindowTextW(textEditHwnd_, buffer.data(), length + 1);
            text.assign(buffer.c_str());
        }
    }
    HWND edit = textEditHwnd_;
    textEditHwnd_ = nullptr;
    if (originalTextEditProc_) {
        SetWindowLongPtrW(edit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalTextEditProc_));
        originalTextEditProc_ = nullptr;
    }
    DestroyWindow(edit);
    if (textEditFont_) {
        DeleteObject(textEditFont_);
        textEditFont_ = nullptr;
    }
    if (textEditBrush_) {
        DeleteObject(textEditBrush_);
        textEditBrush_ = nullptr;
    }
    textInputFrameClient_ = {};
    closingTextEdit_ = false;
    if (!text.empty()) {
        if (editingTextIndex_.has_value()) {
            document_.selectedShapeIndex = editingTextIndex_;
            document_.UpdateSelectedText(text);
        } else if (textInputCreatesTag_) {
            auto tag = std::make_unique<TagShape>(textPositionImage_, text);
            tag->ApplyStyle(styleBar_.Style());
            document_.AddShape(std::move(tag));
        } else {
            document_.AddTextShape(textPositionImage_, text, styleBar_.Style());
        }
        RefreshToolbarHistoryState();
    }
    editingTextIndex_.reset();
    textInputCreatesTag_ = false;
    InvalidateEditor();
}

void EditorWindow::DrawTextInputFrame(HDC hdc) {
    if (!HasActiveTextInput()) {
        return;
    }

    HBRUSH fill = CreateSolidBrush(RGB(255, 255, 255));
    HPEN border = CreatePen(PS_SOLID, 1, RGB(52, 132, 240));
    HGDIOBJ oldBrush = SelectObject(hdc, fill);
    HGDIOBJ oldPen = SelectObject(hdc, border);
    RoundRect(
        hdc,
        textInputFrameClient_.left,
        textInputFrameClient_.top,
        textInputFrameClient_.right,
        textInputFrameClient_.bottom,
        8,
        8);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(border);
    DeleteObject(fill);
}

void EditorWindow::DrawSelectedShapeBounds(HDC hdc) {
    const Shape* selected = document_.SelectedShape();
    if (!selected || HasActiveTextInput()) {
        return;
    }

    const RECT bounds = selected->Bounds();
    RECT clientBounds = viewport_.ImageRectToClient(bounds);
    HPEN pen = CreatePen(PS_DOT, 1, RGB(32, 128, 255));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, clientBounds.left, clientBounds.top, clientBounds.right, clientBounds.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void EditorWindow::DrawCropPreview(HDC hdc, const RECT& imageClientRect) {
    const std::optional<RECT> crop = previewCropRect_.has_value() ? previewCropRect_ : document_.cropRect;
    if (!crop.has_value()) {
        return;
    }

    RECT cropClient = viewport_.ImageRectToClient(*crop);
    HBRUSH shade = CreateSolidBrush(RGB(55, 55, 55));
    RECT top{imageClientRect.left, imageClientRect.top, imageClientRect.right, cropClient.top};
    RECT left{imageClientRect.left, cropClient.top, cropClient.left, cropClient.bottom};
    RECT right{cropClient.right, cropClient.top, imageClientRect.right, cropClient.bottom};
    RECT bottom{imageClientRect.left, cropClient.bottom, imageClientRect.right, imageClientRect.bottom};
    FillRect(hdc, &top, shade);
    FillRect(hdc, &left, shade);
    FillRect(hdc, &right, shade);
    FillRect(hdc, &bottom, shade);
    DeleteObject(shade);

    HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, cropClient.left, cropClient.top, cropClient.right, cropClient.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    HBRUSH handleBrush = CreateSolidBrush(RGB(255, 255, 255));
    const POINT handles[] = {
        {cropClient.left, cropClient.top},
        {(cropClient.left + cropClient.right) / 2, cropClient.top},
        {cropClient.right, cropClient.top},
        {cropClient.right, (cropClient.top + cropClient.bottom) / 2},
        {cropClient.right, cropClient.bottom},
        {(cropClient.left + cropClient.right) / 2, cropClient.bottom},
        {cropClient.left, cropClient.bottom},
        {cropClient.left, (cropClient.top + cropClient.bottom) / 2},
    };
    for (const auto& handle : handles) {
        RECT handleRect{handle.x - 3, handle.y - 3, handle.x + 4, handle.y + 4};
        FillRect(hdc, &handleRect, handleBrush);
    }
    DeleteObject(handleBrush);
}

void EditorWindow::RefreshViewportClientRect() {
    imageClientRect_ = viewport_.DisplayClientRect();
    viewportClientRect_ = viewport_.ViewportClientRect();
}

LRESULT CALLBACK EditorWindow::TextEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* window = reinterpret_cast<EditorWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (window) {
        if (message == WM_KEYDOWN && wParam == VK_RETURN && (GetKeyState(VK_CONTROL) & 0x8000) != 0) {
            window->CommitTextInput(true);
            return 0;
        }
        if (message == WM_KEYDOWN && wParam == VK_ESCAPE) {
            window->CommitTextInput(false);
            return 0;
        }
        if (message == WM_KILLFOCUS) {
            window->CommitTextInput(true);
            return 0;
        }
        return CallWindowProcW(window->originalTextEditProc_, hwnd, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK EditorWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    EditorWindow* window = nullptr;
    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        window = static_cast<EditorWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<EditorWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (window) {
        return window->HandleMessage(hwnd, message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT EditorWindow::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT:
        OnPaint();
        return 0;
    case WM_ERASEBKGND:
        return kEraseBackgroundBeforeEditorPaint ? DefWindowProcW(hwnd, message, wParam, lParam) : 1;
    case WM_KEYDOWN:
        OnKeyDown(wParam);
        return 0;
    case WM_LBUTTONDOWN:
        RelayTooltipMessage(message, wParam, lParam);
        OnMouseDown(ScreenPointFromLParam(hwnd, lParam));
        return 0;
    case WM_MOUSEMOVE:
        RelayTooltipMessage(message, wParam, lParam);
        OnMouseMove(ScreenPointFromLParam(hwnd, lParam));
        return 0;
    case WM_MOUSEWHEEL:
        RelayTooltipMessage(message, wParam, lParam);
        OnMouseWheel(
            POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)},
            GET_WHEEL_DELTA_WPARAM(wParam),
            (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) != 0);
        return 0;
    case WM_LBUTTONUP:
        RelayTooltipMessage(message, wParam, lParam);
        OnMouseUp(ScreenPointFromLParam(hwnd, lParam));
        return 0;
    case WM_LBUTTONDBLCLK:
        RelayTooltipMessage(message, wParam, lParam);
        OnDoubleClick(ScreenPointFromLParam(hwnd, lParam));
        return 0;
    case WM_CAPTURECHANGED:
        draggingToolbar_ = false;
        return 0;
    case WM_CTLCOLOREDIT:
        if (reinterpret_cast<HWND>(lParam) == textEditHwnd_) {
            SetTextColor(reinterpret_cast<HDC>(wParam), RGB(32, 38, 46));
            SetBkColor(reinterpret_cast<HDC>(wParam), RGB(255, 255, 255));
            return reinterpret_cast<LRESULT>(textEditBrush_ ? textEditBrush_ : GetStockObject(WHITE_BRUSH));
        }
        break;
    case WM_DESTROY:
        if (textEditHwnd_) {
            CommitTextInput(false);
        }
        DestroyToolbarTooltips();
        if (result_.status == EditorStatus::Failed) {
            result_.status = EditorStatus::Cancelled;
        }
        hwnd_ = nullptr;
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace mysnip::editor
