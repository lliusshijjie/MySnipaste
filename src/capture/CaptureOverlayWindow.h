#pragma once

#include <windows.h>

namespace mysnip::capture {

enum class SelectionStatus {
    Completed,
    Cancelled,
    TooSmall,
    Failed
};

struct SelectionResult {
    SelectionStatus status = SelectionStatus::Failed;
    RECT screenRect{};
};

class CaptureOverlayWindow {
public:
    SelectionResult SelectRegion(HWND owner);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    bool CreateOverlayWindow(HWND owner);
    void Complete(SelectionStatus status, RECT rect = {});
    void OnMouseDown(POINT pt);
    void OnMouseMove(POINT pt);
    void OnMouseUp(POINT pt);
    void OnPaint();
    void DrawScene(HDC dc, const RECT& client);

    HWND hwnd_ = nullptr;
    HWND owner_ = nullptr;
    bool selecting_ = false;
    bool done_ = false;
    POINT start_{};
    POINT current_{};
    RECT virtualScreen_{};
    SelectionResult result_{};
};

} // namespace mysnip::capture
