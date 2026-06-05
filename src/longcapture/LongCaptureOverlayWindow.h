#pragma once

#include "capture/ICaptureBackend.h"
#include "image/ImageBuffer.h"
#include "longcapture/LongCaptureOverlayLayout.h"
#include "longcapture/LongCaptureStitcher.h"

#include <optional>
#include <windows.h>

namespace mysnip::longcapture {

class LongCaptureOverlayWindow {
public:
    std::optional<capture::CaptureResult> ShowModal(
        HWND owner,
        RECT selectionRect,
        capture::ICaptureBackend& backend,
        const image::ImageBuffer& firstFrame);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    bool CreateOverlayWindow(HWND owner);
    void Complete(bool confirmed);
    void OnPaint();
    void DrawScene(HDC hdc, const RECT& client);
    void DrawControls(HDC hdc, const RECT& client);
    void OnClick(POINT screenPoint);
    void CaptureCurrentSelection();

    HWND hwnd_ = nullptr;
    HWND controlsHwnd_ = nullptr;
    HWND owner_ = nullptr;
    RECT virtualScreen_{};
    RECT selectionRect_{};
    LongCaptureOverlayLayout layout_{};
    capture::ICaptureBackend* backend_ = nullptr;
    LongCaptureStitcher stitcher_;
    image::ImageBuffer currentFrame_;
    std::optional<capture::CaptureResult> result_;
    bool done_ = false;
};

} // namespace mysnip::longcapture
