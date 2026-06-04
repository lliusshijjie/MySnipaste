#include "test_framework.h"

#include "export/ClipboardRetryPolicy.h"

using mysnip::export_::kOpenClipboardMaxAttempts;
using mysnip::export_::ShouldRetryOpenClipboard;

TEST_CASE(ClipboardExporter_RetriesOpenClipboard) {
    REQUIRE(ShouldRetryOpenClipboard(0));
    REQUIRE(ShouldRetryOpenClipboard(kOpenClipboardMaxAttempts - 2));
    REQUIRE(!ShouldRetryOpenClipboard(kOpenClipboardMaxAttempts - 1));
}

TEST_CASE(ClipboardExporter_DoesNotRetryInvalidAttempt) {
    REQUIRE(!ShouldRetryOpenClipboard(-1));
}
