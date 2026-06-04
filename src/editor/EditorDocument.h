#pragma once

#include "editor/Shape.h"
#include "editor/TagShape.h"
#include "editor/TextShape.h"
#include "image/ImageBuffer.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>
#include <windows.h>

namespace mysnip::editor {

struct EditorDocument {
    struct DocumentSnapshot {
        std::vector<std::unique_ptr<Shape>> shapes;
        std::optional<RECT> cropRect;
        std::optional<std::size_t> selectedShapeIndex;
    };

    struct HistoryEntry {
        DocumentSnapshot before;
        DocumentSnapshot after;
    };

    image::ImageBuffer baseImage;
    RECT screenRect{};
    std::vector<std::unique_ptr<Shape>> shapes;
    std::optional<RECT> cropRect;
    std::optional<std::size_t> selectedShapeIndex;
    std::vector<HistoryEntry> undoStack;
    std::vector<HistoryEntry> redoStack;

    DocumentSnapshot CaptureSnapshot() const {
        DocumentSnapshot snapshot;
        snapshot.cropRect = cropRect;
        snapshot.selectedShapeIndex = selectedShapeIndex;
        snapshot.shapes.reserve(shapes.size());
        for (const auto& shape : shapes) {
            snapshot.shapes.push_back(shape->Clone());
        }
        return snapshot;
    }

    void RestoreSnapshot(const DocumentSnapshot& snapshot) {
        shapes.clear();
        shapes.reserve(snapshot.shapes.size());
        for (const auto& shape : snapshot.shapes) {
            shapes.push_back(shape->Clone());
        }
        cropRect = snapshot.cropRect;
        selectedShapeIndex = snapshot.selectedShapeIndex;
        if (selectedShapeIndex.has_value() && *selectedShapeIndex >= shapes.size()) {
            selectedShapeIndex.reset();
        }
    }

    void CommitSnapshot(DocumentSnapshot before) {
        HistoryEntry entry;
        entry.before = std::move(before);
        entry.after = CaptureSnapshot();
        undoStack.push_back(std::move(entry));
        redoStack.clear();
    }

    void AddShape(std::unique_ptr<Shape> shape) {
        if (!shape) {
            return;
        }
        auto before = CaptureSnapshot();
        shapes.push_back(std::move(shape));
        selectedShapeIndex = shapes.size() - 1;
        CommitSnapshot(std::move(before));
    }

    bool AddTextShape(POINT position, const std::wstring& text, const EditorStyle& style = {}) {
        if (text.empty()) {
            return false;
        }
        auto shape = std::make_unique<TextShape>(position, text);
        shape->ApplyStyle(style);
        AddShape(std::move(shape));
        return true;
    }

    void SetCropRect(RECT rect) {
        auto before = CaptureSnapshot();
        cropRect = rect;
        CommitSnapshot(std::move(before));
    }

    void AdjustCropRect(RECT rect) {
        SetCropRect(rect);
    }

    bool SelectShapeAt(POINT point, int tolerance) {
        for (std::size_t i = shapes.size(); i > 0; --i) {
            const std::size_t index = i - 1;
            if (shapes[index]->HitTest(point, tolerance)) {
                selectedShapeIndex = index;
                return true;
            }
        }
        selectedShapeIndex.reset();
        return false;
    }

    Shape* SelectedShape() {
        if (!selectedShapeIndex.has_value() || *selectedShapeIndex >= shapes.size()) {
            return nullptr;
        }
        return shapes[*selectedShapeIndex].get();
    }

    const Shape* SelectedShape() const {
        if (!selectedShapeIndex.has_value() || *selectedShapeIndex >= shapes.size()) {
            return nullptr;
        }
        return shapes[*selectedShapeIndex].get();
    }

    bool DeleteSelectedShape() {
        if (!selectedShapeIndex.has_value() || *selectedShapeIndex >= shapes.size()) {
            return false;
        }
        auto before = CaptureSnapshot();
        shapes.erase(shapes.begin() + static_cast<std::ptrdiff_t>(*selectedShapeIndex));
        selectedShapeIndex.reset();
        CommitSnapshot(std::move(before));
        return true;
    }

    bool MoveSelectedShape(int dx, int dy) {
        Shape* selected = SelectedShape();
        if (!selected) {
            return false;
        }
        const SIZE imageSize = baseImage.IsValid()
            ? SIZE{baseImage.width, baseImage.height}
            : SIZE{1000000, 1000000};
        auto before = CaptureSnapshot();
        selected->MoveBy(dx, dy, imageSize);
        CommitSnapshot(std::move(before));
        return true;
    }

    bool ApplySelectedStyle(const EditorStyle& style) {
        Shape* selected = SelectedShape();
        if (!selected) {
            return false;
        }
        auto before = CaptureSnapshot();
        if (!selected->ApplyStyle(style)) {
            return false;
        }
        CommitSnapshot(std::move(before));
        return true;
    }

    bool UpdateSelectedText(const std::wstring& text) {
        Shape* selected = SelectedShape();
        auto* textShape = dynamic_cast<TextShape*>(selected);
        auto* tagShape = dynamic_cast<TagShape*>(selected);
        if ((!textShape && !tagShape) || text.empty()) {
            return false;
        }
        auto before = CaptureSnapshot();
        if (textShape) {
            textShape->text = text;
        } else {
            tagShape->text = text;
        }
        CommitSnapshot(std::move(before));
        return true;
    }

    bool CanUndo() const {
        return !undoStack.empty();
    }

    bool CanRedo() const {
        return !redoStack.empty();
    }

    bool Undo() {
        if (undoStack.empty()) {
            return false;
        }
        HistoryEntry entry = std::move(undoStack.back());
        undoStack.pop_back();
        RestoreSnapshot(entry.before);
        redoStack.push_back(std::move(entry));
        return true;
    }

    bool Redo() {
        if (redoStack.empty()) {
            return false;
        }
        HistoryEntry entry = std::move(redoStack.back());
        redoStack.pop_back();
        RestoreSnapshot(entry.after);
        undoStack.push_back(std::move(entry));
        return true;
    }
};

} // namespace mysnip::editor
