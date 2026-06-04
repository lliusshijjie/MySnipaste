#pragma once

namespace mysnip::capture {

inline bool ShouldCancelOnCaptureChanged(bool selecting, bool done) {
    return selecting && !done;
}

} // namespace mysnip::capture
