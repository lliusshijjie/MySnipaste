#pragma once

#include "capture/ICaptureBackend.h"
#include "editor/EditorDocument.h"

#include <windows.h>

namespace mysnip::editor {

POINT ComputePinnedImagePosition(const capture::CaptureResult& captureResult, const EditorDocument& document);

} // namespace mysnip::editor
