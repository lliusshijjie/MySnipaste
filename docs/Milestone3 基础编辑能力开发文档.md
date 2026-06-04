# Milestone 3：基础编辑能力开发文档

## 1. 阶段目标

Milestone 3 的目标是在 M2 编辑器外壳基础上，实现截图工具的基础编辑能力。

完整流程变为：

```text
截图
    ↓
进入编辑器
    ↓
箭头 / 文字 / 高亮 / 马赛克 / 裁剪
    ↓
点击 ✔ 或按 Enter
    ↓
导出最终图片
    ↓
复制到剪贴板
```

M3 使用非破坏性编辑模型：

```text
baseImage + shapes + cropRect
```

用户的编辑操作不会直接修改底图。编辑器预览和最终导出时，再把底图、标注层、马赛克、裁剪统一合成。

---

## 2. 本阶段范围

### 2.1 M3 实现内容

M3 实现以下能力：

```text
1. EditorDocument 编辑文档模型
2. Shape 基类
3. ArrowShape
4. TextShape
5. HighlightShape
6. MosaicShape
7. CropRect 非破坏性裁剪
8. ToolType 工具系统
9. ToolManager 当前工具管理
10. 工具栏按钮真实启用
11. 鼠标拖动生成图形
12. 文字输入框
13. 实时预览当前拖动对象
14. 预览渲染
15. 最终图片导出
16. 导出结果写入剪贴板
```

### 2.2 M3 不实现内容

以下内容留到 M4 或后续版本：

```text
1. 撤销 / 重做
2. Ctrl + Z
3. 颜色选择器
4. 线宽选择器
5. 字体选择器
6. 保存文件
7. OCR
8. 上传图床
9. 多个裁剪历史
10. 图形选中后再次编辑
11. 拖动已有图形
12. 删除单个 shape
```

M3 的原则是：

```text
工具能用，导出正确，不做复杂编辑器能力。
```

---

## 3. 编辑模型

### 3.1 EditorDocument

```cpp
struct EditorDocument {
    image::ImageBuffer baseImage;
    RECT screenRect{};
    std::vector<std::unique_ptr<Shape>> shapes;
    std::optional<RECT> cropRect;
};
```

字段说明：

```text
baseImage：
    截图原图，内部 top-down BGRA。

screenRect：
    截图在虚拟屏幕中的原始位置。

shapes：
    用户已经提交的标注对象。

cropRect：
    当前裁剪区域，使用图片局部坐标。
```

### 3.2 坐标规则

M3 必须统一坐标系统。

```text
屏幕坐标：
    Windows 虚拟屏幕物理坐标。

编辑器客户区坐标：
    EditorWindow 内部坐标。

图片局部坐标：
    以截图左上角为 (0, 0) 的坐标。
```

所有 shape 坐标必须保存为：

```text
图片局部坐标
```

不要保存屏幕绝对坐标。

例如截图区域为：

```text
screenRect = { left: 100, top: 200, right: 900, bottom: 600 }
```

用户点击屏幕点：

```text
screenPoint = { x: 300, y: 250 }
```

转换为图片局部坐标：

```text
imagePoint = { x: 200, y: 50 }
```

也就是：

```cpp
imageX = screenX - screenRect.left;
imageY = screenY - screenRect.top;
```

如果存在多显示器负坐标，也仍然使用这个规则。

---

## 4. Shape 设计

### 4.1 Shape 基类

```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual void Draw(RenderContext& ctx) const = 0;
};
```

### 4.2 RenderContext

```cpp
struct RenderContext {
    HDC hdc = nullptr;
    RECT imageRect{};
    float scale = 1.0f;
};
```

M3 默认不做图片缩放，`scale = 1.0f`。

后续如果编辑器支持缩放，可以通过 `RenderContext` 扩展。

---

## 5. Shape 类型

### 5.1 ArrowShape

```cpp
struct ArrowShape : Shape {
    POINT start{};
    POINT end{};
    COLORREF color = RGB(255, 0, 0);
    int thickness = 2;

    void Draw(RenderContext& ctx) const override;
};
```

行为：

```text
1. 用户按下鼠标左键开始
2. 拖动时实时预览
3. 松开鼠标后生成 ArrowShape
4. 小于最小距离的箭头忽略
```

默认样式：

```text
颜色：红色
线宽：2px
箭头头部：固定长度
```

箭头由三部分组成：

```text
主线
箭头左翼
箭头右翼
```

---

### 5.2 HighlightShape

```cpp
struct HighlightShape : Shape {
    RECT rect{};
    COLORREF color = RGB(255, 255, 0);
    int alpha = 90;

    void Draw(RenderContext& ctx) const override;
};
```

行为：

```text
1. 用户拖动矩形
2. 松开后生成 HighlightShape
3. 小于 3x3 的区域忽略
```

默认样式：

```text
颜色：黄色
透明度：约 35%
```

M3 可以使用半透明混合绘制。

如果 GDI 半透明实现复杂，可以先通过内存 buffer 做 alpha blend，再绘制到窗口。

---

### 5.3 MosaicShape

```cpp
struct MosaicShape : Shape {
    RECT rect{};
    int blockSize = 12;

    void Draw(RenderContext& ctx) const override;
};
```

行为：

```text
1. 用户拖动矩形
2. 松开后生成 MosaicShape
3. 小于 3x3 的区域忽略
4. 马赛克不直接修改 baseImage
```

马赛克算法：

```text
1. 将 rect 分割为 blockSize x blockSize 小块
2. 每个小块计算平均颜色
3. 使用平均颜色填充整个小块
```

注意：

```text
马赛克的颜色必须基于当前渲染结果计算，而不是永远只基于 baseImage。
```

原因是如果用户先画了文字，再对该区域打马赛克，导出时应该能把文字一起马赛克掉。

因此导出顺序必须按 shapes 顺序执行。

---

### 5.4 TextShape

```cpp
struct TextShape : Shape {
    POINT position{};
    std::wstring text;
    COLORREF color = RGB(255, 0, 0);
    int fontSize = 20;
    std::wstring fontName = L"Segoe UI";

    void Draw(RenderContext& ctx) const override;
};
```

行为：

```text
1. 用户选择文字工具
2. 点击截图区域
3. 创建 Win32 EDIT 子控件
4. 输入文字
5. Enter 提交
6. Esc 取消
7. 失焦时，如果文本非空则提交
8. 空文本不生成 TextShape
```

文字输入状态下：

```text
Enter：
    提交当前文字，不导出图片。

Esc：
    取消当前文字输入，不关闭编辑器。
```

没有活动文字输入框时：

```text
Enter：
    导出最终图片。

Esc：
    取消整个编辑器。
```

提交文字后：

```text
继续停留在文字工具。
```

---

## 6. 裁剪设计

### 6.1 cropRect

裁剪不作为 Shape，而是单独保存在：

```cpp
std::optional<RECT> cropRect;
```

裁剪区域使用：

```text
图片局部坐标
```

### 6.2 裁剪行为

```text
1. 用户选择裁剪工具
2. 鼠标拖动矩形
3. 松开后归一化矩形
4. 小于 3x3 的裁剪区域忽略
5. 合法区域更新 document.cropRect
```

M3 只支持一个裁剪区域。

再次裁剪时：

```text
新的 cropRect 覆盖旧的 cropRect。
```

### 6.3 裁剪预览

编辑器预览中：

```text
1. 显示裁剪框
2. 裁剪框外部叠加半透明暗色遮罩
3. 不实际修改 baseImage
```

### 6.4 导出裁剪

导出时：

```text
1. 先渲染完整 baseImage + shapes
2. 如果存在 cropRect
3. 最后裁剪 cropRect 区域
4. 返回裁剪后的 ImageBuffer
```

裁剪必须最后执行。

---

## 7. 工具系统

### 7.1 ToolType

```cpp
enum class ToolType {
    Select,
    Arrow,
    Text,
    Highlight,
    Mosaic,
    Crop
};
```

### 7.2 ToolManager

```cpp
class ToolManager {
public:
    void SetTool(ToolType tool);
    ToolType CurrentTool() const;

    void HandleToolbarAction(ToolbarAction action);
    void HandleShortcut(WPARAM key);

private:
    ToolType currentTool_ = ToolType::Select;
};
```

### 7.3 快捷键

```text
A：箭头
T：文字
H：高亮
M：马赛克
C：裁剪
Enter：导出
Esc：取消
```

文字输入框激活时：

```text
A / T / H / M / C 不切换工具
Enter 提交文字
Esc 取消文字输入
```

---

## 8. 工具栏改造

### 8.1 ToolbarAction

M2 中的工具栏 action 扩展为：

```cpp
enum class ToolbarAction {
    Placeholder,
    Arrow,
    Text,
    Highlight,
    Mosaic,
    Crop,
    Cancel,
    Confirm
};
```

### 8.2 启用按钮

M3 启用：

```text
箭头
文字
高亮
马赛克
裁剪
取消 X
完成 ✔
```

M3 暂不启用：

```text
撤销
下载
OCR
更多
钉住
颜色
线宽
```

### 8.3 当前工具选中态

当前工具需要绘制选中态。

简单规则：

```text
当前工具按钮背景浅灰
其他按钮正常背景
禁用按钮置灰
```

### 8.4 点击行为

```text
点击箭头：
    currentTool = Arrow

点击文字：
    currentTool = Text

点击高亮：
    currentTool = Highlight

点击马赛克：
    currentTool = Mosaic

点击裁剪：
    currentTool = Crop

点击 X：
    取消编辑器，不修改剪贴板

点击 ✔：
    导出最终图片到剪贴板
```

---

## 9. EditorRenderer 设计

### 9.1 接口

```cpp
class EditorRenderer {
public:
    static bool DrawPreview(
        HDC hdc,
        const EditorDocument& document,
        const RECT& imageClientRect,
        const Shape* previewShape
    );

    static std::optional<image::ImageBuffer> RenderFinalImage(
        const EditorDocument& document
    );
};
```

### 9.2 预览渲染

预览渲染用于编辑器窗口显示。

流程：

```text
1. 绘制暗色编辑器背景
2. 绘制 baseImage
3. 按顺序绘制 document.shapes
4. 绘制当前临时 previewShape
5. 绘制 cropRect 预览遮罩和裁剪框
6. 绘制工具栏
```

### 9.3 最终导出

最终导出用于剪贴板。

流程：

```text
1. 创建一份 baseImage 副本
2. 创建内存 HDC / DIBSection
3. 把 baseImage 绘制到内存图像
4. 按 shapes 顺序依次绘制所有 Shape
5. 如果存在 cropRect，裁剪最终图
6. 返回 ImageBuffer
```

导出时不绘制：

```text
1. 编辑器暗色背景
2. 工具栏
3. 当前未提交的拖动预览
4. 裁剪框边框
5. 裁剪外部遮罩
```

只导出：

```text
baseImage + 已提交 shapes + crop 结果
```

---

## 10. 鼠标交互

### 10.1 通用规则

编辑操作只允许发生在截图区域内。

如果鼠标点不在截图区域内：

```text
1. 不开始绘制 shape
2. 不开始裁剪
3. 不创建文字输入框
```

拖动过程中如果鼠标移出截图区域：

```text
坐标 clamp 到图片边界内。
```

### 10.2 箭头

```text
WM_LBUTTONDOWN：
    如果当前工具是 Arrow 且点在图片内：
        开始绘制
        记录 start

WM_MOUSEMOVE：
    更新 preview end

WM_LBUTTONUP：
    如果距离足够：
        添加 ArrowShape
    清空 preview
```

### 10.3 高亮 / 马赛克 / 裁剪

```text
WM_LBUTTONDOWN：
    记录起点

WM_MOUSEMOVE：
    更新 preview rect

WM_LBUTTONUP：
    归一化 rect
    如果 rect >= 3x3：
        Highlight -> 添加 HighlightShape
        Mosaic -> 添加 MosaicShape
        Crop -> 更新 cropRect
    清空 preview
```

### 10.4 文字

```text
WM_LBUTTONUP：
    如果当前工具是 Text 且点击图片内：
        创建 EDIT 子控件
```

文字输入期间：

```text
1. 不处理工具快捷键
2. 不导出
3. 不开始其他绘制
```

---

## 11. EditorWindow 改造

### 11.1 新增成员

```cpp
class EditorWindow {
private:
    EditorDocument document_;
    ToolManager toolManager_;

    std::unique_ptr<Shape> previewShape_;
    bool dragging_ = false;
    POINT dragStartImage_{};
    POINT dragCurrentImage_{};

    HWND textEditHwnd_ = nullptr;
    POINT textPositionImage_{};
};
```

### 11.2 需要处理的消息

```text
WM_PAINT
WM_LBUTTONDOWN
WM_MOUSEMOVE
WM_LBUTTONUP
WM_KEYDOWN
WM_COMMAND
WM_KILLFOCUS
WM_DESTROY
```

如果使用子类化处理文字输入框，还需要处理 EDIT 控件中的：

```text
WM_KEYDOWN
WM_KILLFOCUS
```

---

## 12. 文字输入框设计

### 12.1 创建方式

点击文字工具后，在对应屏幕位置创建 Win32 `EDIT` 子控件。

```cpp
CreateWindowExW(
    0,
    L"EDIT",
    L"",
    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
    x,
    y,
    width,
    height,
    editorHwnd,
    ...
);
```

M3 可以使用固定尺寸：

```text
宽度：200px
高度：32px
```

后续再支持自动扩展。

### 12.2 提交文字

提交条件：

```text
1. 输入框内按 Enter
2. 输入框失焦且文本非空
```

提交后：

```text
1. 读取文本
2. 如果非空，创建 TextShape
3. 销毁 EDIT 控件
4. textEditHwnd = nullptr
5. 重绘编辑器
```

### 12.3 取消文字

取消条件：

```text
输入框内按 Esc
```

取消后：

```text
1. 不创建 TextShape
2. 销毁 EDIT 控件
3. textEditHwnd = nullptr
4. 重绘编辑器
```

---

## 13. 导出流程

### 13.1 EditorResult

M3 建议让 `EditorResult` 携带最终图片。

```cpp
enum class EditorStatus {
    Confirmed,
    Cancelled,
    Failed
};

struct EditorResult {
    EditorStatus status = EditorStatus::Failed;
    std::optional<image::ImageBuffer> finalImage;
};
```

### 13.2 确认导出

当用户点击 ✔ 或在无文字输入时按 Enter：

```text
1. 调用 EditorRenderer::RenderFinalImage(document)
2. 如果成功：
       result.status = Confirmed
       result.finalImage = 渲染结果
3. 如果失败：
       result.status = Failed
4. 退出 EditorWindow modal loop
```

### 13.3 App 处理结果

```cpp
auto editorResult = editor.ShowModal(mainWindow_, *captureResult);

if (editorResult.status == EditorStatus::Confirmed &&
    editorResult.finalImage.has_value()) {
    SetState(AppState::CopyingToClipboard);
    clipboardExporter.CopyImage(mainWindow_, *editorResult.finalImage);
}

SetState(AppState::Idle);
```

取消和失败时：

```text
Cancelled：
    不修改剪贴板，静默回到 Idle。

Failed：
    不修改剪贴板，可托盘提示，回到 Idle。
```

---

## 14. 默认样式

M3 不实现样式面板，所有工具使用固定默认样式。

```text
箭头：
    红色，2px

文字：
    红色，20px，Segoe UI

高亮：
    黄色，35% alpha

马赛克：
    blockSize = 12px

裁剪：
    白色边框，外部半透明暗色遮罩
```

---

## 15. 自动化测试

自动化测试只覆盖纯逻辑和图像处理函数。

### 15.1 工具系统测试

```text
ToolManager_SelectsArrow
ToolManager_SelectsText
ToolManager_SelectsHighlight
ToolManager_SelectsMosaic
ToolManager_SelectsCrop
ToolManager_IgnoresToolShortcutWhileTextEditing
```

### 15.2 坐标转换测试

```text
EditorCoordinates_ConvertsScreenPointToImagePoint
EditorCoordinates_ConvertsClientPointToImagePoint
EditorCoordinates_ClampsPointToImageBounds
EditorCoordinates_RejectsPointOutsideImage
```

### 15.3 矩形工具测试

```text
RectTool_NormalizesDragInAllDirections
RectTool_RejectsTooSmallRect
RectTool_ClampsRectToImageBounds
```

### 15.4 文档模型测试

```text
EditorDocument_AddsArrowShape
EditorDocument_AddsHighlightShape
EditorDocument_AddsMosaicShape
EditorDocument_AddsTextShape
EditorDocument_UpdatesCropRect
EditorDocument_ReplacesOldCropRect
TextShape_IgnoresEmptyText
```

### 15.5 渲染测试

```text
EditorRenderer_ReturnsOriginalImageWhenNoEdits
EditorRenderer_CropsFinalImageToCropRect
EditorRenderer_AppliesShapesInOrder
MosaicShape_FillsBlocksWithAverageColor
HighlightShape_AppliesAlphaBlend
```

---

## 16. 手动验收

### 16.1 基础流程

```text
1. 截图后进入编辑器
2. 工具栏显示箭头、文字、高亮、马赛克、裁剪、X、✔
3. 点击工具按钮可以切换工具
4. 当前工具有选中态
```

### 16.2 箭头

```text
1. 选择箭头工具
2. 在截图区域拖动
3. 拖动时有实时预览
4. 松开后箭头保留
5. 点击 ✔ 后导出的图片包含箭头
```

### 16.3 高亮

```text
1. 选择高亮工具
2. 拖动矩形区域
3. 松开后生成半透明黄色高亮
4. 点击 ✔ 后导出的图片包含高亮
```

### 16.4 马赛克

```text
1. 选择马赛克工具
2. 拖动矩形区域
3. 松开后预览区域被块状模糊
4. 点击 ✔ 后导出的图片对应区域被马赛克处理
```

### 16.5 文字

```text
1. 选择文字工具
2. 点击截图区域
3. 出现文字输入框
4. 输入文字后按 Enter
5. 文本提交并显示在截图上
6. 空文本不生成文字
7. 输入框内 Esc 取消当前文字输入
8. 无输入框时 Esc 取消整个编辑器
9. 点击 ✔ 后导出的图片包含文字
```

### 16.6 裁剪

```text
1. 选择裁剪工具
2. 拖动裁剪区域
3. 显示裁剪框
4. 裁剪框外部有半透明遮罩
5. 点击 ✔ 后导出的图片尺寸变为裁剪区域尺寸
6. 再次裁剪会覆盖上一次裁剪区域
```

### 16.7 取消与导出

```text
1. 点击 X 取消，不修改剪贴板
2. 无文字输入时按 Esc 取消，不修改剪贴板
3. 点击 ✔ 导出最终编辑图
4. 无文字输入时按 Enter 导出最终编辑图
5. 导出图可以粘贴到画图、Office、微信
```

### 16.8 多显示器和高 DPI

```text
1. 多显示器负坐标下，绘制位置正确
2. 高 DPI 下，鼠标命中位置正确
3. 高 DPI 下，导出图尺寸正确
4. 副屏截图后，编辑器仍能正常绘制和导出
```

---

## 17. 实施顺序

### Step 1：新增文档模型和工具类型

```text
1. EditorDocument
2. Shape 基类
3. ToolType
4. ToolManager
5. 基础单元测试
```

### Step 2：实现坐标转换和矩形工具逻辑

```text
1. screen/client -> image local
2. clamp 到图片边界
3. rect 归一化
4. 小矩形过滤
5. 单元测试
```

### Step 3：改造工具栏

```text
1. 启用 Arrow / Text / Highlight / Mosaic / Crop
2. 增加当前工具选中态
3. X / ✔ 保持原行为
```

### Step 4：实现 Shape 绘制

```text
1. ArrowShape
2. HighlightShape
3. MosaicShape
4. TextShape
```

### Step 5：改造 EditorWindow 交互

```text
1. 持有 EditorDocument
2. 持有 ToolManager
3. 鼠标拖动生成 previewShape
4. 松开后提交 shape
5. 处理文字输入框
6. 处理裁剪预览
```

### Step 6：实现 EditorRenderer

```text
1. DrawPreview
2. RenderFinalImage
3. crop 最后应用
4. 单元测试
```

### Step 7：改造导出流程

```text
1. EditorResult 增加 finalImage
2. 确认时生成最终图
3. App 复制 finalImage
4. 取消和失败不复制
```

---

## 18. 构建与验证命令

```bat
cmake --build build-nmake

ctest --test-dir build-nmake --output-on-failure
```

---

## 19. M3 完成标准

M3 完成后应满足：

```text
1. 截图后进入编辑器
2. 可以切换箭头、文字、高亮、马赛克、裁剪工具
3. 箭头可以拖动绘制并导出
4. 文字可以输入并导出
5. 高亮可以拖动绘制并导出
6. 马赛克可以拖动绘制并导出
7. 裁剪可以设置并在导出时生效
8. 所有 shape 坐标均为图片局部坐标
9. 编辑操作不直接修改 baseImage
10. ✔ / Enter 导出最终编辑图
11. X / Esc 取消且不修改剪贴板
12. 多显示器和高 DPI 下坐标基本正确
13. M1/M2 的热键、托盘、选区、编辑器外壳、剪贴板链路不被破坏
```

---

## 20. 和 Milestone 4 的衔接

M4 可以在 M3 基础上继续完善：

```text
1. 撤销 / 重做
2. Ctrl + Z / Ctrl + Y
3. 更完整的多显示器和高 DPI 测试
4. 错误处理
5. 资源泄漏检查
6. 编辑器体验细节
7. 工具样式配置
```

M3 需要保留良好的数据结构，使 M4 可以自然加入撤销。

例如后续可以把每次编辑操作封装成：

```text
Command
```

但 M3 暂时不实现 Command Pattern，避免复杂化。

---

## 21. 总结

Milestone 3 是截图工具从“编辑器外壳”进入“真正可用”的关键阶段。

本阶段最重要的设计点是：

```text
1. 非破坏性编辑
2. shape 坐标统一使用图片局部坐标
3. 预览和导出使用同一套渲染逻辑
4. 裁剪最后应用
5. 文字输入状态不能和 Enter 导出冲突
6. 马赛克必须按 shape 顺序参与渲染
7. 取消不修改剪贴板
```

完成 M3 后，工具已经具备一个截图产品的核心能力：

```text
截图
    ↓
基础编辑
    ↓
复制到剪贴板
```
