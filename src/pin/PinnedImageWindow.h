#pragma once

#include "image/ImageBuffer.h"

#include <windows.h>

namespace mysnip::pin {

float ClampPinnedImageScale(float scale);
SIZE ComputePinnedImageSize(int imageWidth, int imageHeight, float scale);
RECT PinnedImageCloseButtonRect(const RECT& clientRect);
bool IsPinnedImageCloseButtonHit(const RECT& clientRect, POINT point);
bool ShouldClosePinnedImageOnKey(WPARAM key);
LPCWSTR PinnedImageCursorResourceName();

class IPinnedImageWindow {
public:
    virtual ~IPinnedImageWindow() = default;
    virtual bool Create(HWND owner, const image::ImageBuffer& image, POINT position) = 0;
    virtual void Show() = 0;
    virtual HWND Hwnd() const = 0;
};

class PinnedImageWindow final : public IPinnedImageWindow {
public:
    ~PinnedImageWindow() override;

    bool Create(HWND owner, const image::ImageBuffer& image, POINT position) override;
    void Show() override;
    HWND Hwnd() const override;

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    bool RegisterWindowClass(HINSTANCE instance);
    void OnPaint();
    void DrawCloseButton(HDC hdc, const RECT& client);
    void BeginDrag();
    void ContinueDrag();
    void EndDrag();
    void ApplyScale(float scale);

    HWND hwnd_ = nullptr;
    HWND owner_ = nullptr;
    image::ImageBuffer image_{};
    bool dragging_ = false;
    POINT dragStartScreen_{};
    RECT dragStartWindow_{};
    float scale_ = 1.0f;
};

} // namespace mysnip::pin
