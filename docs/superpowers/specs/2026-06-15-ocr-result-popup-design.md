# OCR 结果浮层设计

## 目标

将现有独立 OCR 文本窗口改为接近飞书截图工具的结果浮层。识别完成后，
编辑器和截图选区继续保留，OCR 结果以小型卡片显示在选区附近。

## 定位与尺寸

- 浮层锚定当前截图可视区域 `EditorViewport::ViewportRect()`。
- 默认宽度 `324px`，默认高度 `206px`，按窗口 DPI 缩放。
- 优先放在选区上方，水平居中并与选区保持 `14px` 间距。
- 上方空间不足时放在选区下方。
- 最终位置 clamp 到虚拟屏幕工作区域内。
- 浮层宽度根据可用屏幕空间收缩，但不得小于 `280px`。
- 文本较长时浮层高度不继续增长，文本区域内部滚动。

## 外观

- 使用 `WS_POPUP | WS_EX_TOPMOST | WS_EX_TOOLWINDOW` 无系统标题栏窗口。
- 背景为白色，圆角半径约 `8px`。
- 外围使用浅灰边框和轻量阴影。
- 标题为“提取文字”，左上内边距 `16px`。
- 右上角使用独立 `×` 图标按钮，hover 时显示浅灰背景。
- 卡片底部绘制指向截图选区的三角尖角；浮层在选区下方时尖角位于顶部。
- 正文使用系统 UI 字体，默认深灰色，保持正常字距。

## 预览态

- OCR 完成后默认进入预览态。
- 文本区域只读、无明显输入框边框，允许选择文字和滚动。
- 底部右侧按钮为：
  - “编辑”：蓝色主按钮；
  - “复制”：蓝色主按钮。
- 点击“复制”将当前文本写入 `CF_UNICODETEXT`。
- 复制成功后按钮文案短暂变为“已复制”，浮层保持打开。
- 点击右上角 `×` 关闭浮层，回到截图编辑器。

## 编辑态

- 点击“编辑”后在同一浮层内切换为可编辑多行文本框。
- 编辑框获得焦点，并选择当前插入位置，不打开新窗口。
- 底部按钮切换为：
  - “取消”：恢复进入编辑态之前的文本并回到预览态；
  - “完成”：保留修改后的文本并回到预览态。
- `Ctrl+Enter` 等同于“完成”。
- `Esc` 在编辑态只取消本次编辑；在预览态关闭 OCR 浮层。
- 编辑完成后的文本用于后续复制，不修改截图图片或编辑历史。

## 窗口行为

- OCR 浮层为编辑器 owner 的模态子流程，但不隐藏或销毁编辑器。
- 浮层消息循环继续允许编辑框、按钮、滚轮和窗口绘制。
- 窗口不可任意调整尺寸，保持轻量结果卡片形态。
- 多显示器负坐标和高 DPI 下，布局统一使用屏幕坐标和 DPI 缩放。
- 当选区或虚拟屏幕很小时，确保关闭按钮、正文和底部操作按钮均可见。

## 接口调整

`OcrTextDialog` 重命名为 `OcrResultPopup`：

```cpp
class OcrResultPopup {
public:
    void ShowModal(HWND owner, RECT anchorScreenRect, const std::wstring& initialText);
};
```

新增纯逻辑布局模块：

```cpp
enum class OcrPopupPlacement {
    Above,
    Below
};

struct OcrPopupLayout {
    RECT windowRect{};
    RECT titleRect{};
    RECT closeButton{};
    RECT textRect{};
    RECT secondaryButton{};
    RECT primaryButton{};
    OcrPopupPlacement placement = OcrPopupPlacement::Above;
};

OcrPopupLayout ComputeOcrPopupLayout(
    RECT anchorScreenRect,
    RECT workArea,
    UINT dpi);
```

编辑器调用方式：

```cpp
ocr::OcrResultPopup popup;
popup.ShowModal(hwnd_, viewport_.ViewportRect(), result->text);
```

## 绘制与控件

- 卡片背景、标题、关闭按钮、按钮背景、边框和尖角使用 GDI 自绘。
- 正文继续使用 Win32 `EDIT` 控件：
  - 预览态设置 `ES_READONLY`；
  - 编辑态移除只读状态并显示细边框；
  - 两种状态都支持多行和垂直滚动。
- 操作按钮使用 owner-draw 或父窗口直接绘制并命中测试，避免系统按钮
  风格与卡片不一致。
- 使用双缓冲绘制，避免模式切换和 hover 时闪烁。

## 测试

自动化测试覆盖：

- 上方空间足够时浮层放在选区上方。
- 上方不足时浮层放在下方。
- 负坐标虚拟屏幕下窗口仍在工作区域内。
- 高 DPI 下尺寸和间距按比例缩放。
- 标题、关闭、正文和两个按钮均在窗口客户区内。
- 预览态按钮为编辑/复制。
- 编辑态按钮为取消/完成。
- 取消编辑恢复原文本。
- 完成编辑保留新文本。
- `Esc` 在两种状态下行为不同。

手动验收覆盖：

- OCR 结果在截图选区附近显示，不遮挡整块截图。
- 点击编辑后在当前浮层内直接修改。
- Copy/Close 和编辑态按钮始终完整可见。
- 长文本可滚动。
- 多显示器、高 DPI 下浮层不越界。
- 浮层切换状态时无明显闪烁。

## 非目标

- 不在识别结果中绘制 OCR 行坐标或逐字高亮。
- 不将修改后的 OCR 文本写回截图。
- 不保留 OCR 历史记录。
- 不实现翻译、搜索或导出文档。
