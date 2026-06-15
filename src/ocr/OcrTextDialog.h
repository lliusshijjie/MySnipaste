#pragma once

#include <string>
#include <windows.h>

namespace mysnip::ocr {

class OcrTextDialog {
public:
    void ShowModal(HWND owner, const std::wstring& initialText);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void CopyText();
    void LayoutControls();

    HWND hwnd_ = nullptr;
    HWND edit_ = nullptr;
    HWND copyButton_ = nullptr;
    HWND closeButton_ = nullptr;
    HWND owner_ = nullptr;
    bool done_ = false;
};

} // namespace mysnip::ocr
