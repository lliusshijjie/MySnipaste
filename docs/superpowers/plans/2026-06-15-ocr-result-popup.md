# OCR Result Popup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the resizable OCR text dialog with a compact Feishu-style popup anchored to the screenshot, supporting inline edit, cancel, complete, copy, and close.

**Architecture:** Separate pure popup placement/state logic from Win32 rendering. `OcrPopupLayout` computes screen/client geometry, `OcrPopupState` owns preview/edit transitions, and `OcrResultPopup` owns the layered interaction, child edit control, GDI drawing, and modal message loop.

**Tech Stack:** C++20, Win32/GDI, CMake/CTest, existing Unicode clipboard helper.

---

### Task 1: Popup geometry

**Files:**
- Create: `src/ocr/OcrPopupLayout.h`
- Create: `tests/test_ocr_popup_layout.cpp`
- Modify: `CMakeLists.txt`

- [ ] Write failing tests for above/below placement, virtual-screen clamping, DPI scaling, and all client elements remaining visible.
- [ ] Run the filtered test and verify compilation fails because `OcrPopupLayout.h` does not exist.
- [ ] Implement `ComputeOcrPopupLayout(anchor, workArea, dpi)` as pure geometry.
- [ ] Run filtered and full tests.

### Task 2: Preview/edit state model

**Files:**
- Create: `src/ocr/OcrPopupState.h`
- Create: `tests/test_ocr_popup_state.cpp`
- Modify: `CMakeLists.txt`

- [ ] Write failing tests for `BeginEdit`, `CancelEdit`, `CompleteEdit`, preview escape, edit escape, and copied feedback.
- [ ] Verify RED due to missing state model.
- [ ] Implement a small state class that stores committed text, edit baseline, current text, mode, close request, and copy-feedback state.
- [ ] Run filtered and full tests.

### Task 3: Win32 popup shell

**Files:**
- Create: `src/ocr/OcrResultPopup.h`
- Create: `src/ocr/OcrResultPopup.cpp`
- Modify: `CMakeLists.txt`

- [ ] Create a borderless `WS_POPUP | WS_EX_TOPMOST | WS_EX_TOOLWINDOW` window using the pure layout result.
- [ ] Create one multiline `EDIT` child and toggle `EM_SETREADONLY` between preview/edit modes.
- [ ] Double-buffer `WM_PAINT` for white card, border, title, close icon, pointer triangle, and blue action buttons.
- [ ] Implement hover, click, keyboard, scrolling, copy feedback timer, and modal message loop.
- [ ] Ensure `Esc` cancels editing first and closes only from preview mode; `Ctrl+Enter` completes editing.

### Task 4: Editor integration and old dialog removal

**Files:**
- Modify: `src/editor/EditorWindow.cpp`
- Delete: `src/ocr/OcrTextDialog.h`
- Delete: `src/ocr/OcrTextDialog.cpp`
- Delete: `src/ocr/OcrDialogLayout.h`
- Delete: `tests/test_ocr_dialog_layout.cpp`
- Modify: `CMakeLists.txt`

- [ ] Replace `OcrTextDialog::ShowModal(hwnd_, text)` with `OcrResultPopup::ShowModal(hwnd_, viewport_.ViewportRect(), virtualScreen_, text)`.
- [ ] Remove old dialog source and layout tests from the build.
- [ ] Build and run all tests.

### Task 5: Package and manual verification

**Files:**
- No additional source files unless verification reveals a defect.

- [ ] Run `cmake --build build-nmake --clean-first`.
- [ ] Run `ctest --test-dir build-nmake --output-on-failure`.
- [ ] Run `tests\test_msix_dev_workflow.ps1`.
- [ ] Reinstall using `scripts\install_dev_msix.ps1 -BuildDir build-nmake`.
- [ ] Verify the packaged process runs from `C:\Program Files\WindowsApps`.
- [ ] Manually verify preview, edit/cancel/complete, copy, close, long-text scrolling, and popup placement.
