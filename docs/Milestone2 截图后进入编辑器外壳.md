# Milestone 2：截图后进入编辑器外壳开发文档

## 1. 阶段目标

Milestone 2 的目标是把 Milestone 1 的流程：

```text
截图选区 -> GDI CaptureResult -> 直接复制到剪贴板
```

改造为：

```text
截图选区
    ↓
GDI CaptureResult
    ↓
进入原位编辑器浮层
    ↓
用户确认或取消
    ↓
确认：复制原图到剪贴板
    ↓
取消：回到后台，不修改剪贴板
```

本阶段只实现编辑器外壳，不实现真实编辑能力。

也就是说，M2 的重点不是箭头、文字、马赛克、裁剪，而是：

```text
1. 截图后进入编辑器
2. 编辑器能正确显示截图
3. 编辑器有工具栏外观
4. Enter / ✔ 可以确认导出
5. Esc / X 可以取消
6. App 状态机能正确防止重入
```

---

## 2. 本阶段范围

### 2.1 M2 实现内容

M2 实现以下功能：

```text
1. 新增 EditorWindow
2. 新增 EditorResult
3. 新增 EditorToolbar
4. 新增 ToolbarButton
5. 新增 ImageRenderUtils
6. 截图后不再直接复制，而是进入编辑器
7. 编辑器覆盖完整虚拟屏幕
8. 截图显示在原始选区位置
9. 截图外区域暗化
10. 截图区域绘制原图
11. 截图区域可显示 1px 边框
12. 底部显示浮动工具栏
13. 工具栏包含全量占位按钮
14. M2 只启用 X 和 ✔
15. Enter 等同 ✔
16. Esc 等同 X
17. 确认后复制原始 CaptureResult.image 到剪贴板
18. 取消后静默返回后台，不修改剪贴板
19. App 新增 Editing 状态，防止截图重入
```

### 2.2 M2 不实现内容

以下内容留到 M3：

```text
1. 箭头绘制
2. 文字输入
3. 高亮
4. 马赛克
5. 裁剪
6. 撤销
7. ShapeLayer
8. 图片合成管线
9. 编辑对象持久化
10. 工具按钮真实切换状态
```

M2 的原则是：

```text
只做编辑器壳子，不做真实编辑。
```

---

## 3. M2 后的新流程

### 3.1 M1 流程

```text
WM_HOTKEY / 托盘截图
    ↓
CaptureOverlayWindow::SelectRegion()
    ↓
GdiCaptureBackend::CaptureRect()
    ↓
ClipboardExporter::CopyImage()
    ↓
Idle
```

### 3.2 M2 流程

```text
WM_HOTKEY / 托盘截图
    ↓
CaptureOverlayWindow::SelectRegion()
    ↓
GdiCaptureBackend::CaptureRect()
    ↓
EditorWindow::ShowModal()
    ↓
用户确认？
    ├─ 是：ClipboardExporter::CopyImage(captureResult.image)
    └─ 否：直接回到 Idle
```

### 3.3 关键行为

```text
确认：
    Enter
    点击 ✔

取消：
    Esc
    点击 X
```

确认时复制的是：

```text
原始 CaptureResult.image
```

取消时：

```text
不调用 ClipboardExporter
不修改剪贴板
静默回到后台
```

---

## 4. AppState 修改

M2 需要在原有状态机中增加 `Editing`。

```cpp
enum class AppState {
    Idle,
    Selecting,
    Capturing,
    Editing,
    CopyingToClipboard,
    Exiting
};
```

状态规则：

```text
Idle：
    允许开始截图。

Selecting：
    正在选区，忽略新的截图请求。

Capturing：
    正在 GDI 截图，忽略新的截图请求。

Editing：
    编辑器正在显示，忽略热键和托盘截图。

CopyingToClipboard：
    正在写剪贴板，忽略新的截图请求。

Exiting：
    正在退出，不处理截图请求。
```

M2 的关键要求：

```text
编辑器打开期间，再次按 Ctrl + Shift + A 不应创建第二个截图流程。
编辑器打开期间，点击托盘截图也不应创建第二个截图流程。
```

---

## 5. 新增模块

M2 新增目录：

```text
src/editor/
 ├─ EditorWindow.h
 ├─ EditorWindow.cpp
 ├─ EditorResult.h
 ├─ EditorToolbar.h
 ├─ EditorToolbar.cpp
 ├─ ToolbarButton.h
 ├─ ImageRenderUtils.h
 └─ ImageRenderUtils.cpp
```

模块职责：

| 模块               | 职责                     |
| ---------------- | ---------------------- |
| EditorWindow     | 创建原位编辑器浮层，显示截图，处理确认和取消 |
| EditorResult     | 表示编辑器返回结果              |
| EditorToolbar    | 计算工具栏位置、按钮布局、命中测试      |
| ToolbarButton    | 描述单个工具栏按钮              |
| ImageRenderUtils | 将 ImageBuffer 绘制到 HDC  |

---

## 6. EditorWindow 设计

### 6.1 对外接口

```cpp
namespace mysnip::editor {

enum class EditorStatus {
    Confirmed,
    Cancelled,
    Failed
};

struct EditorResult {
    EditorStatus status = EditorStatus::Failed;
};

class EditorWindow {
public:
    EditorResult ShowModal(HWND owner, const capture::CaptureResult& captureResult);
};

}
```

### 6.2 编辑器窗口形态

编辑器窗口是一个覆盖完整虚拟屏幕的顶层窗口。

窗口样式：

```cpp
WS_POPUP
```

扩展样式：

```cpp
WS_EX_TOPMOST | WS_EX_TOOLWINDOW
```

编辑器窗口不使用普通边框，不出现在任务栏中。

窗口覆盖范围：

```cpp
SM_XVIRTUALSCREEN
SM_YVIRTUALSCREEN
SM_CXVIRTUALSCREEN
SM_CYVIRTUALSCREEN
```

### 6.3 Modal 消息循环

M2 的 `EditorWindow::ShowModal()` 使用局部 modal 消息循环，和 M1 的 `CaptureOverlayWindow::SelectRegion()` 风格保持一致。

基本流程：

```text
ShowModal()
    ↓
创建编辑器窗口
    ↓
显示窗口
    ↓
进入局部消息循环
    ↓
用户 Enter / Esc / 点击工具栏
    ↓
设置 EditorResult
    ↓
退出局部消息循环
    ↓
销毁窗口
    ↓
返回 EditorResult
```

注意：

```text
ShowModal 内部必须保证窗口最终销毁。
App 在调用 ShowModal 前需要进入 Editing 状态。
ShowModal 返回后，App 再根据结果进入 CopyingToClipboard 或 Idle。
```

---

## 7. 编辑器显示规则

### 7.1 坐标规则

M2 继续沿用 M1 的坐标规则：

```text
所有坐标使用虚拟屏幕物理坐标。
```

`CaptureResult.screenRect` 表示截图在虚拟屏幕中的原始位置。

编辑器绘制时：

```text
截图应该绘制在 CaptureResult.screenRect 对应位置。
```

例如：

```text
screenRect = { left: 100, top: 200, right: 900, bottom: 600 }

截图显示位置：
x = 100
y = 200
width = 800
height = 400
```

如果副屏在主屏左侧，`left` 可以是负数。绘制时需要把虚拟屏幕坐标转换成编辑器窗口客户区坐标：

```text
clientX = screenX - virtualScreen.left
clientY = screenY - virtualScreen.top
```

### 7.2 绘制内容

编辑器窗口绘制内容：

```text
1. 整个虚拟屏幕绘制半透明暗色遮罩
2. 截图区域绘制原始截图
3. 截图区域绘制 1px 细边框
4. 工具栏绘制在截图下方或上方
```

M2 不需要复杂动画，不需要缩放，不需要拖动截图。

---

## 8. ImageRenderUtils 设计

### 8.1 职责

`ImageRenderUtils` 只负责显示，不负责导出。

它用于把 M1 中的 `ImageBuffer` 绘制到 Win32 `HDC`。

### 8.2 接口草案

```cpp
class ImageRenderUtils {
public:
    static bool DrawImage(
        HDC hdc,
        const ImageBuffer& image,
        const RECT& dstRect
    );
};
```

### 8.3 绘制方式

推荐使用：

```cpp
StretchDIBits
```

或：

```cpp
SetDIBitsToDevice
```

绘制时构造 top-down 32bpp DIB：

```cpp
BITMAPINFO bmi{};
bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
bmi.bmiHeader.biWidth = image.width;
bmi.bmiHeader.biHeight = -image.height;
bmi.bmiHeader.biPlanes = 1;
bmi.bmiHeader.biBitCount = 32;
bmi.bmiHeader.biCompression = BI_RGB;
```

注意：

```text
这里 biHeight 使用负数，表示 top-down DIB。
这样可以直接使用 ImageBuffer 的 top-down 内存布局，不需要上下翻转。
```

### 8.4 和剪贴板导出的区别

M1 中的 `ClipboardExporter` 为了兼容性，会构造 bottom-up CF_DIB。

M2 中的 `ImageRenderUtils` 只是屏幕显示，可以使用 top-down DIB。

两者不要混用：

```text
ImageRenderUtils：
    用于显示，top-down。

DibBuilder / ClipboardExporter：
    用于剪贴板导出，bottom-up。
```

---

## 9. EditorToolbar 设计

### 9.1 工具栏目标

M2 工具栏只做外观和确认/取消。

工具栏样式：

```text
白色圆角矩形
高度约 40px
按钮尺寸约 32px
按钮间距 4px ~ 6px
图标深灰
禁用按钮置灰
X 可用
✔ 可用
```

M2 可以使用 GDI 自绘简化图标，不引入第三方 icon 库。

### 9.2 工具栏按钮

M2 工具栏包含全量占位按钮，用于模拟截图工具完整形态。

按钮可以包括：

```text
拖动手柄
矩形
圆形
线条
箭头
画笔
文字
标签
马赛克
钉住
识别
更多
裁剪
撤销
下载
取消 X
完成 ✔
```

但 M2 只启用：

```text
取消 X
完成 ✔
```

其他按钮全部禁用。

### 9.3 ToolbarAction

```cpp
enum class ToolbarAction {
    Placeholder,
    Cancel,
    Confirm
};
```

### 9.4 ToolbarButton

```cpp
struct ToolbarButton {
    ToolbarAction action = ToolbarAction::Placeholder;
    RECT bounds{};
    bool enabled = false;
};
```

规则：

```text
点击 enabled = true 的 Cancel：
    返回取消

点击 enabled = true 的 Confirm：
    返回确认

点击 disabled 的 Placeholder：
    不做任何事情

点击工具栏外区域：
    不关闭编辑器
```

---

## 10. 工具栏布局规则

### 10.1 优先位置

工具栏优先显示在截图下方居中。

```text
截图区域
    ↓
工具栏
```

如果截图下方空间不足，则显示在截图上方。

```text
工具栏
    ↓
截图区域
```

如果上方和下方都不足，则把工具栏夹在虚拟屏幕范围内。

### 10.2 水平方向

工具栏默认相对截图区域水平居中。

```text
toolbar.centerX = captureRect.centerX
```

如果超出虚拟屏幕：

```text
左侧越界：
    toolbar.left = virtualScreen.left + margin

右侧越界：
    toolbar.right = virtualScreen.right - margin
```

### 10.3 垂直方向

推荐参数：

```text
toolbarHeight = 40
buttonSize = 32
gap = 6
margin = 8
```

布局规则：

```text
如果截图下方空间 >= toolbarHeight + margin：
    放在截图下方

否则如果截图上方空间 >= toolbarHeight + margin：
    放在截图上方

否则：
    clamp 到虚拟屏幕内
```

### 10.4 EditorToolbar 接口草案

```cpp
class EditorToolbar {
public:
    void Layout(const RECT& captureRect, const RECT& virtualScreenRect);
    void Draw(HDC hdc);
    std::optional<ToolbarAction> HitTest(POINT pt) const;

private:
    RECT toolbarBounds_{};
    std::vector<ToolbarButton> buttons_;
};
```

`HitTest` 返回规则：

```text
命中启用按钮：
    返回对应 ToolbarAction

命中禁用按钮：
    返回 std::nullopt

未命中：
    返回 std::nullopt
```

---

## 11. EditorWindow 消息处理

EditorWindow 需要处理：

```text
WM_PAINT
WM_KEYDOWN
WM_LBUTTONUP
WM_DESTROY
```

### 11.1 WM_PAINT

绘制内容：

```text
1. 绘制暗色背景
2. 绘制截图原图
3. 绘制截图边框
4. 绘制工具栏
```

### 11.2 WM_KEYDOWN

规则：

```text
Enter：
    result = Confirmed
    退出 modal loop

Esc：
    result = Cancelled
    退出 modal loop
```

### 11.3 WM_LBUTTONUP

流程：

```text
1. 获取鼠标位置
2. 调用 toolbar.HitTest(pt)
3. 如果是 Confirm：
       result = Confirmed
       退出 modal loop
4. 如果是 Cancel：
       result = Cancelled
       退出 modal loop
5. 其他位置：
       不处理
```

### 11.4 WM_DESTROY

窗口销毁时：

```text
如果 result 仍是 Failed：
    设置为 Cancelled 或 Failed
```

建议用户主动关闭或异常销毁时，不应误导出到剪贴板。

---

## 12. App 串联方式

### 12.1 StartCapture 修改前

M1 中：

```cpp
auto captureResult = captureBackend.CaptureRect(selection.screenRect);
clipboardExporter.CopyImage(hwnd, captureResult->image);
```

### 12.2 StartCapture 修改后

M2 中：

```cpp
auto captureResult = captureBackend.CaptureRect(selection.screenRect);
if (!captureResult) {
    SetState(AppState::Idle);
    return;
}

SetState(AppState::Editing);

editor::EditorWindow editor;
auto editorResult = editor.ShowModal(mainWindow_, *captureResult);

if (editorResult.status == editor::EditorStatus::Confirmed) {
    SetState(AppState::CopyingToClipboard);
    clipboardExporter.CopyImage(mainWindow_, captureResult->image);
}

SetState(AppState::Idle);
```

### 12.3 行为要求

```text
EditorStatus::Confirmed：
    调用 ClipboardExporter

EditorStatus::Cancelled：
    不调用 ClipboardExporter
    静默返回 Idle

EditorStatus::Failed：
    不调用 ClipboardExporter
    可用托盘气泡或日志提示
    返回 Idle
```

---

## 13. 错误处理

M2 错误处理保持简单。

| 场景                    | 处理                        |
| --------------------- | ------------------------- |
| EditorWindow 创建失败     | 返回 Failed                 |
| ImageRenderUtils 绘制失败 | 记录日志，窗口仍可关闭               |
| 工具栏布局失败               | 返回 Failed 或使用默认位置         |
| 用户 Esc 取消             | 静默                        |
| 用户点击 X                | 静默                        |
| 用户确认后剪贴板失败            | 复用 M1 剪贴板失败提示             |
| 编辑器异常销毁               | 不复制，返回 Failed 或 Cancelled |

原则：

```text
取消不是错误，不提示。
失败才提示或记录。
```

---

## 14. 自动化测试

M2 自动化测试只覆盖纯逻辑。

### 14.1 ToolbarLayout 测试

```text
ToolbarLayout_PlacesBelowCaptureWhenSpaceAllows
ToolbarLayout_PlacesAboveCaptureWhenBelowWouldOverflow
ToolbarLayout_ClampsToVirtualScreenHorizontally
ToolbarLayout_ClampsToVirtualScreenVertically
```

### 14.2 ToolbarHitTest 测试

```text
ToolbarHitTest_ReturnsConfirmForCheckButton
ToolbarHitTest_ReturnsCancelForCloseButton
ToolbarHitTest_IgnoresDisabledPlaceholderButtons
ToolbarHitTest_IgnoresClickOutsideToolbar
```

### 14.3 EditorFlow 测试

可以把 App 流程拆成纯逻辑函数测试。

```text
EditorFlow_ConfirmedCopiesOriginalImage
EditorFlow_CancelledDoesNotCopyImage
EditorFlow_FailedDoesNotCopyImage
```

注意：

```text
EditorWindow 本身、Win32 绘制、键盘鼠标消息以手动验收为主。
```

---

## 15. 手动验收

### 15.1 基础流程

```text
1. 启动程序后后台运行
2. 按 Ctrl + Shift + A 进入截图选区
3. 拖选区域
4. 截图后不再直接复制
5. 进入编辑器浮层
6. 截图显示在原位置
7. 工具栏显示在截图附近
```

### 15.2 确认行为

```text
1. 按 Enter 后复制到剪贴板
2. 点击 ✔ 后复制到剪贴板
3. 复制后的图片可粘贴到画图、Office、微信
4. 复制内容为原始截图
```

### 15.3 取消行为

```text
1. 按 Esc 后取消
2. 点击 X 后取消
3. 取消后不修改剪贴板
4. 取消后程序回到后台
```

### 15.4 防重入

```text
1. 编辑器打开期间按 Ctrl + Shift + A，不创建第二个截图流程
2. 编辑器打开期间点击托盘截图，不创建第二个截图流程
3. 编辑器关闭后可以再次正常截图
```

### 15.5 多显示器

```text
1. 单显示器正常显示
2. 副屏在右侧时正常显示
3. 副屏在左侧且存在负坐标时正常显示
4. 副屏在上方时正常显示
5. 编辑器窗口覆盖完整虚拟屏幕
6. 截图显示位置与原选区一致
7. 工具栏不会跑出虚拟屏幕
```

### 15.6 高 DPI

测试缩放比例：

```text
100%
125%
150%
200%
```

验收点：

```text
1. 截图显示尺寸与选区尺寸一致
2. 截图位置不明显偏移
3. 工具栏位置正确
4. Enter / Esc 行为正常
5. 鼠标点击 ✔ / X 命中准确
```

---

## 16. 构建与验证命令

```bat
cmake --build build --config Debug

ctest --test-dir build -C Debug --output-on-failure
```

如果使用 NMake：

```bat
cmake --build build

ctest --test-dir build --output-on-failure
```

---

## 17. 实施顺序

建议按以下顺序开发。

### Step 1：新增 editor 目录和基础类型

完成：

```text
1. EditorResult
2. ToolbarAction
3. ToolbarButton
4. 空 EditorWindow
```

验收：

```text
工程可编译。
```

---

### Step 2：实现 EditorToolbar 纯逻辑

完成：

```text
1. 工具栏布局
2. 按钮 bounds 计算
3. HitTest
4. 单元测试
```

验收：

```text
ToolbarLayout 和 ToolbarHitTest 测试通过。
```

---

### Step 3：实现 ImageRenderUtils

完成：

```text
1. ImageBuffer 到 HDC 绘制
2. top-down DIB 显示
3. 不修改原 ImageBuffer
```

验收：

```text
可以在测试窗口中绘制截图图片。
```

---

### Step 4：实现 EditorWindow

完成：

```text
1. 创建覆盖虚拟屏幕的 WS_POPUP 窗口
2. 显示暗色遮罩
3. 绘制截图
4. 绘制工具栏
5. 支持 Enter / Esc
6. 支持点击 ✔ / X
7. 返回 EditorResult
```

验收：

```text
截图后可以进入编辑器浮层。
确认和取消行为正确。
```

---

### Step 5：串联 App

完成：

```text
1. 新增 AppState::Editing
2. 修改 StartCapture
3. CaptureResult 交给 EditorWindow
4. Confirmed 后调用 ClipboardExporter
5. Cancelled / Failed 不调用 ClipboardExporter
```

验收：

```text
M2 完整链路打通。
```

---

## 18. M2 完成标准

M2 完成后，应满足：

```text
1. 截图后进入编辑器，而不是直接复制
2. 编辑器覆盖完整虚拟屏幕
3. 截图显示在原选区位置
4. 截图外区域暗化
5. 工具栏显示在截图附近
6. 工具栏不会超出虚拟屏幕
7. M2 只有 X 和 ✔ 可用
8. Enter 等同 ✔
9. Esc 等同 X
10. 确认后复制原始截图到剪贴板
11. 取消后不修改剪贴板
12. 编辑器打开期间不能重复截图
13. 多显示器下位置正确
14. 高 DPI 下位置和点击命中基本正确
15. M1 的截图、托盘、热键、剪贴板能力不被破坏
```

---

## 19. 和 Milestone 3 的衔接

M2 不实现真实编辑，但需要为 M3 留出空间。

M3 后的流程会变成：

```text
CaptureResult
    ↓
EditorWindow
    ↓
EditorDocument
    ↓
ShapeLayer
    ↓
ExportFinalImage
    ↓
ClipboardExporter
```

M2 暂时是：

```text
CaptureResult
    ↓
EditorWindow
    ↓
确认后直接 ClipboardExporter(captureResult.image)
```

所以 M2 里不要把工具按钮逻辑写死到无法扩展。

建议保留：

```text
ToolbarAction
ToolbarButton
EditorToolbar
ImageRenderUtils
```

这样 M3 可以自然增加：

```text
Arrow
Text
Highlight
Mosaic
Crop
Undo
```

---

## 20. 总结

Milestone 2 的核心目标是让截图工具从“截图后直接复制”升级为“截图后进入编辑器外壳”。

本阶段最重要的点是：

```text
1. 编辑器窗口覆盖完整虚拟屏幕
2. 截图显示在原始选区位置
3. 工具栏布局稳定
4. 确认和取消行为明确
5. 取消不修改剪贴板
6. 编辑器期间防止截图重入
7. 多显示器和高 DPI 坐标正确
8. 不提前实现 M3 的真实编辑能力
```

M2 完成后，用户虽然还不能真正画箭头或打文字，但已经能看到完整截图工具的交互形态：

```text
截图选区
    ↓
原位编辑器浮层
    ↓
确认复制 / 取消返回
```

这为 Milestone 3 增加箭头、文字、高亮、马赛克和裁剪打好了 UI 与流程基础。
