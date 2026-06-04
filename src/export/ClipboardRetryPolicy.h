#pragma once

namespace mysnip::export_ {

constexpr int kOpenClipboardMaxAttempts = 5;
constexpr unsigned int kOpenClipboardRetryDelayMs = 50;

inline bool ShouldRetryOpenClipboard(int attemptIndex, int maxAttempts = kOpenClipboardMaxAttempts) {
    return attemptIndex >= 0 && attemptIndex + 1 < maxAttempts;
}

} // namespace mysnip::export_
