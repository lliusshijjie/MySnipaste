#pragma once

#include <string>
#include <utility>

namespace mysnip::ocr {

enum class OcrPopupMode {
    Preview,
    Editing
};

class OcrPopupState {
public:
    explicit OcrPopupState(std::wstring text)
        : text_(std::move(text)),
          editBaseline_(text_) {}

    OcrPopupMode Mode() const {
        return mode_;
    }

    const std::wstring& Text() const {
        return text_;
    }

    void SetText(std::wstring text) {
        text_ = std::move(text);
    }

    void BeginEdit() {
        editBaseline_ = text_;
        mode_ = OcrPopupMode::Editing;
        copyFeedbackVisible_ = false;
    }

    void CancelEdit() {
        text_ = editBaseline_;
        mode_ = OcrPopupMode::Preview;
    }

    void CompleteEdit() {
        editBaseline_ = text_;
        mode_ = OcrPopupMode::Preview;
    }

    void HandleEscape() {
        if (mode_ == OcrPopupMode::Editing) {
            CancelEdit();
        } else {
            closeRequested_ = true;
        }
    }

    bool CloseRequested() const {
        return closeRequested_;
    }

    void RequestClose() {
        closeRequested_ = true;
    }

    void MarkCopied() {
        copyFeedbackVisible_ = true;
    }

    void ClearCopyFeedback() {
        copyFeedbackVisible_ = false;
    }

    bool CopyFeedbackVisible() const {
        return copyFeedbackVisible_;
    }

private:
    std::wstring text_;
    std::wstring editBaseline_;
    OcrPopupMode mode_ = OcrPopupMode::Preview;
    bool closeRequested_ = false;
    bool copyFeedbackVisible_ = false;
};

} // namespace mysnip::ocr
