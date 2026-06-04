cmake_minimum_required(VERSION 3.24)

project(ScreenshotMvp LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

set(SCREENSHOT_MVP_SOURCES
    src/app/App.cpp
    src/capture/CaptureOverlayWindow.cpp
    src/capture/GdiCaptureBackend.cpp
    src/editor/ArrowShape.cpp
    src/editor/BlurShape.cpp
    src/editor/EditorToolbar.cpp
    src/editor/EditorWindow.cpp
    src/editor/EditorRenderer.cpp
    src/editor/EditorStyleBar.cpp
    src/editor/EllipseShape.cpp
    src/editor/FreehandShape.cpp
    src/editor/HighlightShape.cpp
    src/editor/ImageRenderUtils.cpp
    src/editor/LineShape.cpp
    src/editor/MosaicShape.cpp
    src/editor/NumberShape.cpp
    src/editor/PinPosition.cpp
    src/editor/RectangleShape.cpp
    src/editor/TagShape.cpp
    src/editor/TextShape.cpp
    src/editor/ToolManager.cpp
    src/export/ClipboardExporter.cpp
    src/export/PngFileExporter.cpp
    src/hotkey/HotkeyManager.cpp
    src/image/DibBuilder.cpp
    src/image/ImageBuffer.cpp
    src/main.cpp
    src/pin/PinnedImageManager.cpp
    src/pin/PinnedImageWindow.cpp
    src/tray/TrayIcon.cpp
    src/utils/DpiUtils.cpp
    src/utils/LogUtils.cpp
)

add_executable(ScreenshotMvp WIN32
    ${SCREENSHOT_MVP_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/app.rc
    ${CMAKE_CURRENT_SOURCE_DIR}/app.manifest
)

target_include_directories(ScreenshotMvp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(ScreenshotMvp
    PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

target_link_libraries(ScreenshotMvp
    PRIVATE
        user32
        gdi32
        shell32
        comctl32
        comdlg32
        ole32
        windowscodecs
)

add_executable(ScreenshotMvpTests
    src/editor/ArrowShape.cpp
    src/editor/BlurShape.cpp
    src/editor/EditorToolbar.cpp
    src/editor/EditorRenderer.cpp
    src/editor/EditorStyleBar.cpp
    src/editor/EllipseShape.cpp
    src/editor/FreehandShape.cpp
    src/editor/HighlightShape.cpp
    src/editor/ImageRenderUtils.cpp
    src/editor/LineShape.cpp
    src/editor/MosaicShape.cpp
    src/editor/NumberShape.cpp
    src/editor/PinPosition.cpp
    src/editor/RectangleShape.cpp
    src/editor/TagShape.cpp
    src/editor/TextShape.cpp
    src/editor/ToolManager.cpp
    src/export/PngFileExporter.cpp
    src/image/DibBuilder.cpp
    src/image/ImageBuffer.cpp
    src/pin/PinnedImageManager.cpp
    src/pin/PinnedImageWindow.cpp
    src/utils/LogUtils.cpp
    tests/test_editor_document.cpp
    tests/test_editor_style_bar.cpp
    tests/test_editor_geometry.cpp
    tests/test_editor_renderer.cpp
    tests/test_editor_toolbar.cpp
    tests/test_editor_paint_config.cpp
    tests/test_crop_adjustment.cpp
    tests/test_shape_editing.cpp
    tests/test_png_file_exporter.cpp
    tests/test_pin_feature.cpp
    tests/test_virtual_screen_utils.cpp
    tests/test_capture_overlay_state.cpp
    tests/test_clipboard_retry_policy.cpp
    tests/test_main.cpp
    tests/test_tool_manager.cpp
    tests/test_tray_notification_policy.cpp
    tests/test_rect_utils.cpp
    tests/test_image_buffer.cpp
    tests/test_dib_builder.cpp
)

target_include_directories(ScreenshotMvpTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(ScreenshotMvpTests
    PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

target_link_libraries(ScreenshotMvpTests
    PRIVATE
        user32
        gdi32
        ole32
        windowscodecs
)

add_test(NAME ScreenshotMvpTests COMMAND ScreenshotMvpTests)
