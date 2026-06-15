#pragma once

#include "ocr/OcrPopupLayout.h"
#include "ocr/OcrPopupState.h"

#include <string>
#include <windows.h>

namespace mysnip::ocr {

class OcrResultPopup {
public:
    void ShowModal(
        HWND owner,
        RECT anchorScreenRect,
        RECT workArea,
        const std::wstring& initialText);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void CreateControls(HINSTANCE instance);
    void ApplyStateToControls();
    void Paint();
    void BeginEdit();
    void CancelEdit();
    void CompleteEdit();
    void CopyText();
    void Close();
    std::wstring ReadEditText() const;
    void WriteEditText(const std::wstring& text);
    void UpdateHover(POINT clientPoint);
    bool IsPointIn(const RECT& rect, POINT point) const;

    HWND hwnd_ = nullptr;
    HWND owner_ = nullptr;
    HWND edit_ = nullptr;
    HFONT titleFont_ = nullptr;
    HFONT bodyFont_ = nullptr;
    HFONT buttonFont_ = nullptr;
    OcrPopupLayout layout_{};
    OcrPopupState state_{L""};
    RECT hoveredRect_{};
    bool trackingMouse_ = false;
    bool done_ = false;
};

} // namespace mysnip::ocr
