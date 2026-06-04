贴图功能本质是：

```text
把一张截图变成一个置顶、无边框、可拖动、可缩放的小窗口。
```

也就是：

```text
截图 ImageBuffer
    ↓
创建一个独立的 PictureWindow
    ↓
窗口内容绘制这张图
    ↓
设置 always-on-top
    ↓
支持拖动 / 缩放 / 关闭 / 透明度
```

---

## 1. 功能定位

你现在的流程是：

```text
截图 -> 编辑器 -> 导出到剪贴板
```

加入贴图后，可以变成：

```text
截图 -> 编辑器 -> 点击“钉住/贴图”
                 ↓
            创建贴图窗口
```

贴图窗口独立存在，不影响主程序后台运行。

---

## 2. 核心架构

新增模块：

```text
src/pin/
 ├─ PinnedImageWindow.h
 ├─ PinnedImageWindow.cpp
 ├─ PinnedImageManager.h
 └─ PinnedImageManager.cpp
```

### PinnedImageWindow

负责单个贴图窗口：

```cpp
class PinnedImageWindow {
public:
    bool Create(HWND owner, const image::ImageBuffer& image, POINT position);
    void Show();
    void Close();

private:
    void OnPaint();
    void OnMouseDown(POINT pt);
    void OnMouseMove(POINT pt);
    void OnMouseUp(POINT pt);
    void OnMouseWheel(int delta);
    void OnKeyDown(WPARAM key);

private:
    HWND hwnd_ = nullptr;
    image::ImageBuffer image_;

    bool dragging_ = false;
    POINT dragStart_{};
    POINT windowStart_{};

    float scale_ = 1.0f;
    float opacity_ = 1.0f;
};
```

### PinnedImageManager

管理多个贴图：

```cpp
class PinnedImageManager {
public:
    void AddPinnedImage(const image::ImageBuffer& image, POINT position);
    void RemovePinnedImage(HWND hwnd);

private:
    std::vector<std::unique_ptr<PinnedImageWindow>> windows_;
};
```

---

## 3. 窗口样式

贴图窗口应该是：

```text
无边框
置顶
工具窗口
不显示在任务栏
可绘制透明/阴影效果
```

推荐样式：

```cpp
DWORD style = WS_POPUP;

DWORD exStyle =
    WS_EX_TOPMOST |
    WS_EX_TOOLWINDOW;
```

如果要支持整体透明度，可以加：

```cpp
WS_EX_LAYERED
```

然后：

```cpp
SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
```

例如透明度 85%：

```cpp
SetLayeredWindowAttributes(hwnd, 0, 216, LWA_ALPHA);
```

---

## 4. 显示图片

贴图窗口大小默认等于图片大小：

```cpp
int width = image.width;
int height = image.height;
```

创建窗口：

```cpp
CreateWindowExW(
    WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
    L"PinnedImageWindow",
    L"",
    WS_POPUP,
    position.x,
    position.y,
    width,
    height,
    owner,
    nullptr,
    hInstance,
    this
);
```

在 `WM_PAINT` 里绘制图片：

```cpp
case WM_PAINT:
    BeginPaint(hwnd, &ps);
    ImageRenderUtils::DrawImage(ps.hdc, image_, clientRect);
    EndPaint(hwnd, &ps);
    break;
```

也就是复用你 M2/M3 里的：

```text
ImageRenderUtils
```

---

## 5. 拖动实现

贴图窗口没有标题栏，所以你要自己实现拖动。

逻辑：

```text
鼠标左键按下：
    记录鼠标起点
    记录窗口起点
    SetCapture

鼠标移动：
    如果 dragging
        计算偏移
        MoveWindow / SetWindowPos

鼠标松开：
    ReleaseCapture
```

示例：

```cpp
void OnMouseDown(POINT pt) {
    dragging_ = true;
    dragStart_ = pt;

    RECT rc;
    GetWindowRect(hwnd_, &rc);
    windowStart_ = { rc.left, rc.top };

    SetCapture(hwnd_);
}

void OnMouseMove(POINT pt) {
    if (!dragging_) return;

    int dx = pt.x - dragStart_.x;
    int dy = pt.y - dragStart_.y;

    SetWindowPos(
        hwnd_,
        HWND_TOPMOST,
        windowStart_.x + dx,
        windowStart_.y + dy,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE
    );
}

void OnMouseUp() {
    dragging_ = false;
    ReleaseCapture();
}
```

注意：`POINT pt` 是窗口客户区坐标。拖动时这样算可以用，但如果遇到 DPI/多屏问题，更稳的是用 `GetCursorPos()` 获取屏幕坐标。

---

## 6. 缩放实现

贴图窗口可以支持：

```text
Ctrl + 鼠标滚轮：缩放
```

逻辑：

```cpp
void OnMouseWheel(int delta) {
    if (delta > 0) {
        scale_ *= 1.1f;
    } else {
        scale_ /= 1.1f;
    }

    scale_ = std::clamp(scale_, 0.2f, 5.0f);

    int newW = static_cast<int>(image_.width * scale_);
    int newH = static_cast<int>(image_.height * scale_);

    SetWindowPos(
        hwnd_,
        HWND_TOPMOST,
        0,
        0,
        newW,
        newH,
        SWP_NOMOVE | SWP_NOACTIVATE
    );

    InvalidateRect(hwnd_, nullptr, TRUE);
}
```

绘制时把图片拉伸到窗口客户区：

```cpp
RECT rc;
GetClientRect(hwnd_, &rc);
ImageRenderUtils::DrawImage(hdc, image_, rc);
```

---

## 7. 关闭贴图

支持几种方式：

```text
Esc：关闭当前贴图
双击：关闭
右键菜单：关闭
工具栏 X：关闭
```

MVP 推荐：

```text
双击关闭
右键弹菜单
```

处理：

```cpp
case WM_LBUTTONDBLCLK:
    DestroyWindow(hwnd);
    break;
```

右键菜单：

```text
关闭
复制
透明度 100%
透明度 80%
透明度 60%
```

第一版只做“关闭”就够。

---

## 8. 贴图按钮如何接入编辑器

你 M2/M3 工具栏里已经有一个“钉住”图标。

增加 action：

```cpp
enum class ToolbarAction {
    Placeholder,
    Arrow,
    Text,
    Highlight,
    Mosaic,
    Crop,
    Pin,
    Cancel,
    Confirm
};
```

点击 Pin 时：

```text
1. 如果当前有编辑内容：
       RenderFinalImage(document)
2. 如果没有编辑内容：
       使用 captureResult.image
3. 创建 PinnedImageWindow
4. 编辑器可以继续保留，或者自动关闭
```

建议 MVP 行为：

```text
点击贴图：
    创建贴图窗口
    关闭编辑器
    不写剪贴板
```

这样简单清晰。

流程：

```cpp
case ToolbarAction::Pin: {
    auto finalImage = EditorRenderer::RenderFinalImage(document_);
    if (finalImage) {
        pinnedImageManager.AddPinnedImage(*finalImage, { screenRect.left, screenRect.top });
        result_.status = EditorStatus::Pinned;
        done_ = true;
    }
    break;
}
```

`EditorStatus` 增加：

```cpp
enum class EditorStatus {
    Confirmed,
    Cancelled,
    Pinned,
    Failed
};
```

App 处理：

```cpp
if (editorResult.status == EditorStatus::Confirmed) {
    clipboardExporter.CopyImage(...);
} else if (editorResult.status == EditorStatus::Pinned) {
    // 不复制剪贴板，贴图窗口已创建
}
```

---

## 9. 贴图窗口是否需要保存 ImageBuffer

需要。

因为窗口重绘时会收到：

```text
WM_PAINT
WM_SIZE
窗口被遮挡后重新显示
DPI 改变
```

如果不保存图片数据，窗口无法重绘。

所以 `PinnedImageWindow` 内部必须持有一份：

```cpp
image::ImageBuffer image_;
```

注意这里要做深拷贝，不能引用编辑器里的临时对象。

---

## 10. 贴图窗口层级

默认设置为置顶：

```cpp
SetWindowPos(hwnd, HWND_TOPMOST, ...);
```

可以支持快捷键切换：

```text
T：切换置顶 / 非置顶
```

MVP 可以一直置顶。

---

## 11. 推荐 MVP 功能范围

贴图第一版不要做太复杂。

建议只实现：

```text
1. 点击钉住按钮生成贴图窗口
2. 贴图窗口无边框
3. 贴图窗口始终置顶
4. 鼠标左键拖动移动
5. Ctrl + 滚轮缩放
6. 双击关闭
7. Esc 关闭
```

暂时不要做：

```text
1. 透明度调节
2. 旋转
3. 多贴图管理面板
4. 贴图分组
5. 保存贴图位置
6. 贴图阴影
7. 贴图吸附边缘
```

---

## 12. 最小实现流程

```text
编辑器点击钉住按钮
    ↓
EditorRenderer::RenderFinalImage(document)
    ↓
PinnedImageManager::AddPinnedImage(image, position)
    ↓
new PinnedImageWindow
    ↓
CreateWindowEx(WS_POPUP, WS_EX_TOPMOST | WS_EX_TOOLWINDOW)
    ↓
WM_PAINT 绘制 ImageBuffer
    ↓
用户拖动 / 缩放 / 关闭
```

总结一句：

```text
贴图 = ImageBuffer + 无边框置顶窗口 + 自绘图片 + 鼠标拖动缩放
```
