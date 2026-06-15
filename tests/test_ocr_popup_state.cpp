#include "test_framework.h"

#include "ocr/OcrPopupState.h"

TEST_CASE(OcrPopupState_CancelEditRestoresCommittedText) {
    mysnip::ocr::OcrPopupState state(L"original");

    state.BeginEdit();
    state.SetText(L"changed");
    state.CancelEdit();

    REQUIRE(state.Mode() == mysnip::ocr::OcrPopupMode::Preview);
    REQUIRE(state.Text() == L"original");
}

TEST_CASE(OcrPopupState_CompleteEditKeepsChangedText) {
    mysnip::ocr::OcrPopupState state(L"original");

    state.BeginEdit();
    state.SetText(L"changed");
    state.CompleteEdit();

    REQUIRE(state.Mode() == mysnip::ocr::OcrPopupMode::Preview);
    REQUIRE(state.Text() == L"changed");
}

TEST_CASE(OcrPopupState_EscapeCancelsEditBeforeClosingPopup) {
    mysnip::ocr::OcrPopupState state(L"original");

    state.BeginEdit();
    state.SetText(L"changed");
    state.HandleEscape();

    REQUIRE(state.Mode() == mysnip::ocr::OcrPopupMode::Preview);
    REQUIRE(!state.CloseRequested());
    REQUIRE(state.Text() == L"original");

    state.HandleEscape();
    REQUIRE(state.CloseRequested());
}

TEST_CASE(OcrPopupState_CopyFeedbackCanBeReset) {
    mysnip::ocr::OcrPopupState state(L"text");

    state.MarkCopied();
    REQUIRE(state.CopyFeedbackVisible());

    state.ClearCopyFeedback();
    REQUIRE(!state.CopyFeedbackVisible());
}
