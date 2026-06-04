# Windows MVP 截屏工具设计文档

## 1. 项目背景

本项目目标是在 Windows 平台实现一个轻量级截屏工具，支持用户通过全局快捷键进入截图模式，拖动鼠标选择截图区域，对截图进行基础编辑，并最终将编辑后的图片输出到系统剪贴板。

该工具的第一版定位为 MVP 版本，重点是打通完整核心链路，而不是一次性实现完整商业截图工具的所有能力。

核心链路如下：

```text
用户按快捷键
    ↓
全局热键捕获
    ↓
显示截图选区层
    ↓
用户拖动选择区域
    ↓
得到截图矩形
    ↓
调用 GDI BitBlt 捕获屏幕区域
    ↓
得到像素数据
    ↓
进入编辑器
    ↓
用户进行基础标注和裁剪
    ↓
导出最终图片
    ↓
写入系统剪贴板
```

---

## 2. MVP 目标

### 2.1 第一版必须支持的功能

MVP 第一版需要支持以下功能：

```text
1. 后台运行
2. 注册全局快捷键
3. 按快捷键进入截图模式
4. 全屏半透明选区层
5. 鼠标拖动选择截图区域
6. 基于 GDI BitBlt 截取屏幕区域
7. 打开截图编辑器
8. 支持基础编辑功能
   - 画箭头
   - 马赛克
   - 文字
   - 高亮
   - 裁剪
9. 支持撤销
10. 将最终图片复制到剪贴板
11. 支持 Esc 取消截图或编辑
```

### 2.2 第一版暂不支持的功能

以下功能不放入 MVP 第一阶段：

```text
1. 截长屏
2. 保存文件
3. 上传图床
4. OCR
5. 录屏
6. GIF
7. 窗口自动识别
8. PrintWindow 截窗口
9. Desktop Duplication API
10. Windows Graphics Capture API
11. 云同步
12. 历史截图管理
```

这些能力可以作为后续版本扩展。

---

## 3. 技术选型

### 3.1 推荐技术路线

MVP 推荐使用：

```text
C++17 / C++20
Win32 API
GDI / GDI+
WIC 或 CF_DIB 剪贴板输出
```

### 3.2 核心 Windows API

| 功能     | 主要 API                                            |
| ------ | ------------------------------------------------- |
| 全局快捷键  | RegisterHotKey                                    |
| 消息循环   | GetMessage / DispatchMessage                      |
| 全屏窗口   | CreateWindowEx                                    |
| 置顶窗口   | WS_EX_TOPMOST                                     |
| 分层窗口   | WS_EX_LAYERED                                     |
| 鼠标拖选   | WM_LBUTTONDOWN / WM_MOUSEMOVE / WM_LBUTTONUP      |
| 屏幕截图   | GetDC / CreateCompatibleDC / BitBlt               |
| 像素读取   | GetDIBits                                         |
| 剪贴板输出  | OpenClipboard / EmptyClipboard / SetClipboardData |
| DPI 适配 | SetProcessDpiAwarenessContext                     |

---

## 4. 总体架构

项目整体分为以下模块：

```text
ScreenshotMVP
 ├─ App
 ├─ HotkeyManager
 ├─ CaptureOverlayWindow
 ├─ ICaptureBackend
 ├─ GdiCaptureBackend
 ├─ ImageBuffer
 ├─ EditorWindow
 ├─ EditorDocument
 ├─ ShapeLayer
 ├─ ToolManager
 ├─ ClipboardExporter
 └─ Utils
```

模块职责如下：

| 模块                   | 职责                       |
| -------------------- | ------------------------ |
| App                  | 程序入口、初始化、消息循环、生命周期管理     |
| HotkeyManager        | 注册和处理全局快捷键               |
| CaptureOverlayWindow | 全屏选区层，负责鼠标拖选截图区域         |
| ICaptureBackend      | 截图后端抽象接口                 |
| GdiCaptureBackend    | 基于 GDI BitBlt 实现截图       |
| ImageBuffer          | 统一的内存图像结构                |
| EditorWindow         | 编辑器窗口，负责显示图片和处理编辑操作      |
| EditorDocument       | 保存底图、标注层、裁剪信息            |
| ShapeLayer           | 管理箭头、文字、高亮、马赛克等编辑对象      |
| ToolManager          | 管理当前编辑工具                 |
| ClipboardExporter    | 将最终图片写入系统剪贴板             |
| Utils                | 坐标转换、DPI、多屏处理、矩形归一化等工具函数 |

---

## 5. 核心流程设计

### 5.1 启动流程

```text
程序启动
    ↓
设置 DPI Awareness
    ↓
创建隐藏主窗口
    ↓
注册全局快捷键
    ↓
进入消息循环
```

启动时需要调用 DPI 相关 API，避免高 DPI 屏幕下鼠标坐标和真实截图坐标不一致。

建议调用：

```cpp
SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
```

---

### 5.2 快捷键触发流程

```text
用户按下快捷键
    ↓
HiddenMainWindow 收到 WM_HOTKEY
    ↓
创建 CaptureOverlayWindow
    ↓
进入截图选区模式
```

默认快捷键建议：

```text
Ctrl + Shift + A
```

如果快捷键注册失败，需要提示用户或记录错误。

---

### 5.3 区域选择流程

用户进入截图模式后，显示一个覆盖所有显示器的全屏透明窗口。

```text
CaptureOverlayWindow 显示
    ↓
用户按下鼠标左键
    ↓
记录起点 start
    ↓
用户拖动鼠标
    ↓
实时更新当前点 current
    ↓
绘制选区矩形
    ↓
用户松开鼠标
    ↓
计算最终截图矩形
    ↓
隐藏选区窗口
    ↓
调用截图后端
```

选区窗口需要处理：

```text
WM_LBUTTONDOWN
WM_MOUSEMOVE
WM_LBUTTONUP
WM_PAINT
WM_KEYDOWN
```

Esc 行为：

```text
如果用户在选区阶段按 Esc：
    关闭选区窗口
    取消截图流程
```

---

### 5.4 截图捕获流程

MVP 第一版只实现 GDI BitBlt。

```text
输入截图矩形 RECT
    ↓
GetDC(nullptr) 获取屏幕 DC
    ↓
CreateCompatibleDC 创建内存 DC
    ↓
CreateCompatibleBitmap 创建位图
    ↓
BitBlt 拷贝屏幕区域
    ↓
GetDIBits 转换为 ImageBuffer
    ↓
返回 CaptureResult
```

截图时需要注意：

```text
1. 选区窗口必须先隐藏，否则可能截到遮罩层
2. 多显示器下坐标可能是负数
3. 高 DPI 下坐标必须保持物理像素一致
4. BitBlt 可以附加 CAPTUREBLT，但不能保证捕获所有窗口
```

---

### 5.5 编辑流程

截图完成后打开编辑器窗口。

```text
CaptureResult
    ↓
EditorDocument
    ↓
EditorWindow 显示截图
    ↓
用户选择工具
    ↓
用户在图片上绘制编辑对象
    ↓
编辑对象保存到 ShapeLayer
    ↓
实时重绘预览
```

编辑器中的图片不应一开始就被破坏性修改。

推荐设计为：

```text
底图 baseImage
    +
标注层 shapes
    +
裁剪信息 cropRect
```

导出时再统一合成最终图片。

---

### 5.6 导出流程

```text
用户点击完成或按 Enter
    ↓
复制 baseImage
    ↓
按顺序应用所有 Shape
    ↓
如果存在 cropRect，执行裁剪
    ↓
生成 finalImage
    ↓
ClipboardExporter 写入剪贴板
    ↓
关闭编辑器窗口
```

---

## 6. 多显示器与 DPI 设计

### 6.1 多显示器处理

不能假设屏幕左上角一定是 `(0, 0)`。

需要使用虚拟屏幕坐标：

```cpp
int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
```

选区窗口应覆盖整个虚拟屏幕：

```text
left = SM_XVIRTUALSCREEN
top = SM_YVIRTUALSCREEN
width = SM_CXVIRTUALSCREEN
height = SM_CYVIRTUALSCREEN
```

### 6.2 DPI 处理

截图工具必须启用 DPI 感知。

否则会出现：

```text
鼠标选择的是逻辑坐标
BitBlt 使用的是物理像素坐标
最终截图区域偏移或尺寸不正确
```

MVP 建议启动时设置：

```cpp
SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
```

---

## 7. 关键数据结构设计

### 7.1 ImageBuffer

程序内部不要到处传递 HBITMAP，建议统一使用 ImageBuffer。

```cpp
struct ImageBuffer {
    int width;
    int height;
    int stride;
    std::vector<uint8_t> pixels;
};
```

像素格式统一为：

```text
BGRA 8-bit
```

即每个像素 4 字节：

```text
B G R A
```

### 7.2 CaptureResult

```cpp
struct CaptureResult {
    ImageBuffer image;
    RECT screenRect;
};
```

其中：

```text
image：截图得到的图像
screenRect：该图像在屏幕上的原始坐标
```

### 7.3 EditorDocument

```cpp
struct EditorDocument {
    ImageBuffer baseImage;
    std::vector<std::unique_ptr<Shape>> shapes;
    std::optional<RECT> cropRect;
};
```

### 7.4 Shape

所有编辑对象统一抽象为 Shape。

```cpp
class Shape {
public:
    virtual void Draw(RenderContext& ctx) = 0;
    virtual ~Shape() = default;
};
```

具体 Shape 类型：

```text
ArrowShape
TextShape
HighlightShape
MosaicShape
```

裁剪可以不作为 Shape，而是放在 `cropRect` 中单独管理。

---

## 8. 编辑器功能设计

### 8.1 工具栏

MVP 编辑器工具栏包含：

```text
箭头
文字
高亮
马赛克
裁剪
撤销
完成
取消
```

快捷键建议：

| 快捷键      | 功能        |
| -------- | --------- |
| A        | 箭头        |
| T        | 文字        |
| H        | 高亮        |
| M        | 马赛克       |
| C        | 裁剪        |
| Ctrl + Z | 撤销        |
| Enter    | 完成并复制到剪贴板 |
| Esc      | 取消        |

---

### 8.2 箭头工具

用户操作：

```text
按下鼠标左键
    ↓
记录箭头起点
    ↓
拖动鼠标
    ↓
实时预览箭头
    ↓
松开鼠标
    ↓
生成 ArrowShape
```

数据结构：

```cpp
struct ArrowShape : Shape {
    POINT start;
    POINT end;
    int thickness;
    COLORREF color;
};
```

箭头绘制由一条主线和两条箭头边组成。

---

### 8.3 高亮工具

用户操作：

```text
按下鼠标左键
    ↓
拖动选择矩形区域
    ↓
松开鼠标
    ↓
生成 HighlightShape
```

数据结构：

```cpp
struct HighlightShape : Shape {
    RECT rect;
    COLORREF color;
    float alpha;
};
```

高亮本质是绘制一个半透明矩形。

---

### 8.4 文字工具

用户操作：

```text
选择文字工具
    ↓
点击图片位置
    ↓
创建输入框
    ↓
用户输入文字
    ↓
按 Enter 或失焦确认
    ↓
生成 TextShape
```

数据结构：

```cpp
struct TextShape : Shape {
    POINT position;
    std::wstring text;
    int fontSize;
    COLORREF color;
};
```

MVP 可以先不支持复杂文字样式，只支持：

```text
单行或多行文本
固定字体
固定颜色
固定字号
```

---

### 8.5 马赛克工具

用户操作：

```text
按下鼠标左键
    ↓
拖动选择矩形区域
    ↓
松开鼠标
    ↓
生成 MosaicShape
```

数据结构：

```cpp
struct MosaicShape : Shape {
    RECT rect;
    int blockSize;
};
```

马赛克算法：

```text
1. 将矩形区域切成 blockSize x blockSize 的小块
2. 计算每个小块的平均颜色
3. 用平均颜色填充整个小块
```

为了支持撤销，马赛克不要直接修改底图，而是在导出阶段应用。

---

### 8.6 裁剪工具

用户操作：

```text
选择裁剪工具
    ↓
拖动选择裁剪区域
    ↓
保存 cropRect
```

裁剪不应立即修改 baseImage。

导出时执行：

```text
如果 cropRect 存在：
    先合成所有编辑内容
    再按照 cropRect 裁剪最终图片
```

---

### 8.7 撤销设计

MVP 只支持撤销最后一步编辑。

实现方式：

```text
shapes：当前编辑对象列表
redoStack：被撤销的编辑对象列表
```

新增 Shape 时：

```text
push 到 shapes
清空 redoStack
```

撤销时：

```text
将 shapes.back() 移动到 redoStack
```

重做可以作为后续能力，MVP 可以只做撤销。

---

## 9. 剪贴板输出设计

MVP 最终只要求输出到剪贴板。

推荐使用剪贴板格式：

```text
CF_DIB
```

输出流程：

```text
ImageBuffer
    ↓
构造 BITMAPINFOHEADER
    ↓
分配全局内存 GlobalAlloc
    ↓
写入 DIB 数据
    ↓
OpenClipboard
    ↓
EmptyClipboard
    ↓
SetClipboardData(CF_DIB, hGlobal)
    ↓
CloseClipboard
```

注意事项：

```text
1. SetClipboardData 成功后，hGlobal 所有权转移给系统
2. 不要在成功后手动释放 hGlobal
3. 写入剪贴板前应先 EmptyClipboard
4. CF_DIB 不包含 BITMAPFILEHEADER
```

---

## 10. 类设计草案

### 10.1 HotkeyManager

```cpp
class HotkeyManager {
public:
    bool Register(HWND hwnd, int id, UINT modifiers, UINT vk);
    void Unregister(HWND hwnd, int id);
};
```

职责：

```text
1. 注册全局快捷键
2. 释放快捷键
3. 处理注册失败
```

---

### 10.2 CaptureOverlayWindow

```cpp
class CaptureOverlayWindow {
public:
    bool Create();
    void Show();
    void Close();

private:
    void OnMouseDown(POINT pt);
    void OnMouseMove(POINT pt);
    void OnMouseUp(POINT pt);
    void OnPaint();
};
```

职责：

```text
1. 创建覆盖虚拟屏幕的透明窗口
2. 处理鼠标拖选
3. 绘制遮罩和选区矩形
4. 返回用户选择的截图区域
```

---

### 10.3 ICaptureBackend

```cpp
class ICaptureBackend {
public:
    virtual CaptureResult CaptureRect(const RECT& rect) = 0;
    virtual ~ICaptureBackend() = default;
};
```

---

### 10.4 GdiCaptureBackend

```cpp
class GdiCaptureBackend : public ICaptureBackend {
public:
    CaptureResult CaptureRect(const RECT& rect) override;
};
```

职责：

```text
1. 调用 GDI API 截取屏幕区域
2. 将 HBITMAP 转换为 ImageBuffer
```

---

### 10.5 EditorWindow

```cpp
class EditorWindow {
public:
    bool Create(const CaptureResult& result);
    void Show();

private:
    void SetTool(ToolType tool);
    void OnMouseDown(POINT pt);
    void OnMouseMove(POINT pt);
    void OnMouseUp(POINT pt);
    void OnPaint();
    void ExportToClipboard();
};
```

职责：

```text
1. 显示截图
2. 管理当前编辑工具
3. 处理鼠标绘制行为
4. 渲染预览
5. 导出最终图片
```

---

### 10.6 ClipboardExporter

```cpp
class ClipboardExporter {
public:
    bool CopyImage(HWND owner, const ImageBuffer& image);
};
```

职责：

```text
1. 将 ImageBuffer 转换为 CF_DIB
2. 写入系统剪贴板
```

---

## 11. 项目目录结构

建议目录如下：

```text
ScreenshotMVP/
 ├─ CMakeLists.txt
 │
 ├─ src/
 │   ├─ main.cpp
 │   │
 │   ├─ app/
 │   │   ├─ App.h
 │   │   └─ App.cpp
 │   │
 │   ├─ hotkey/
 │   │   ├─ HotkeyManager.h
 │   │   └─ HotkeyManager.cpp
 │   │
 │   ├─ capture/
 │   │   ├─ ICaptureBackend.h
 │   │   ├─ GdiCaptureBackend.h
 │   │   ├─ GdiCaptureBackend.cpp
 │   │   ├─ CaptureOverlayWindow.h
 │   │   └─ CaptureOverlayWindow.cpp
 │   │
 │   ├─ image/
 │   │   ├─ ImageBuffer.h
 │   │   ├─ ImageBuffer.cpp
 │   │   ├─ ImageConvert.h
 │   │   └─ ImageConvert.cpp
 │   │
 │   ├─ editor/
 │   │   ├─ EditorWindow.h
 │   │   ├─ EditorWindow.cpp
 │   │   ├─ EditorDocument.h
 │   │   ├─ ToolType.h
 │   │   ├─ Shape.h
 │   │   ├─ ArrowShape.h
 │   │   ├─ TextShape.h
 │   │   ├─ HighlightShape.h
 │   │   ├─ MosaicShape.h
 │   │   └─ CropTool.h
 │   │
 │   ├─ export/
 │   │   ├─ ClipboardExporter.h
 │   │   └─ ClipboardExporter.cpp
 │   │
 │   └─ utils/
 │       ├─ RectUtils.h
 │       ├─ DpiUtils.h
 │       └─ Win32Utils.h
 │
 └─ README.md
```

---

## 12. 开发里程碑

### 12.1 Milestone 1：最小截图链路

目标：

```text
按快捷键 → 拖选区域 → 截图 → 复制到剪贴板
```

任务：

```text
1. 创建 Win32 程序入口
2. 创建隐藏主窗口
3. 注册全局快捷键
4. 实现 CaptureOverlayWindow
5. 实现 GdiCaptureBackend
6. 实现 ClipboardExporter
```

完成标准：

```text
用户按下快捷键后，可以拖选区域，并将截图直接复制到剪贴板。
```

---

### 12.2 Milestone 2：编辑器外壳

目标：

```text
截图后进入编辑器，按 Enter 导出到剪贴板。
```

任务：

```text
1. 实现 EditorWindow
2. 显示截图图片
3. 支持 Enter 导出
4. 支持 Esc 取消
```

完成标准：

```text
用户截图后可以看到编辑器窗口，不编辑也可以直接复制到剪贴板。
```

---

### 12.3 Milestone 3：基础编辑能力

目标：

```text
支持箭头、高亮、文字、马赛克、裁剪。
```

任务：

```text
1. 实现 ToolManager
2. 实现 ShapeLayer
3. 实现 ArrowShape
4. 实现 HighlightShape
5. 实现 TextShape
6. 实现 MosaicShape
7. 实现 cropRect
```

完成标准：

```text
用户可以对截图进行基础编辑，并将编辑结果复制到剪贴板。
```

---

### 12.4 Milestone 4：体验完善

目标：

```text
提高 MVP 可用性。
```

任务：

```text
1. 支持 Ctrl + Z 撤销
2. 支持工具快捷键
3. 优化多显示器
4. 优化高 DPI
5. 优化选区层绘制
6. 优化错误处理
```

完成标准：

```text
工具在普通办公、开发场景下可稳定使用。
```

---

## 13. 后续版本规划

### 13.1 长截图

长截图单独作为后续版本实现。

原因：

```text
长截图不是简单截屏 API，而是截图、滚动、重叠匹配、拼接的组合能力。
```

基本流程：

```text
用户选择长截图区域
    ↓
截取当前区域
    ↓
模拟滚轮滚动
    ↓
等待页面稳定
    ↓
再次截取同一区域
    ↓
匹配两张图的重叠区域
    ↓
拼接
    ↓
重复直到滚动结束
```

### 13.2 保存文件

保存文件可以在已有 ImageBuffer 基础上实现。

推荐使用：

```text
WIC PNG Encoder
```

### 13.3 OCR

OCR 可以后续接入：

```text
Windows.Media.Ocr
Tesseract
PaddleOCR
云 OCR 服务
```

### 13.4 其他捕获后端

后续可以扩展：

```text
PrintWindowCaptureBackend
DesktopDuplicationBackend
WindowsGraphicsCaptureBackend
```

其中：

```text
GDI BitBlt：适合普通截图
PrintWindow：适合部分窗口截图
Desktop Duplication：适合录屏和高性能连续捕获
Windows Graphics Capture：适合现代窗口捕获和屏幕共享
```

---

## 14. 风险与注意事项

### 14.1 高 DPI 坐标错位

风险：

```text
用户拖选区域和实际截图区域不一致。
```

解决：

```text
程序启动时设置 Per-Monitor DPI Awareness V2。
```

---

### 14.2 多显示器负坐标

风险：

```text
副屏在主屏左边或上方时，屏幕坐标可能为负数。
```

解决：

```text
统一使用虚拟屏幕坐标。
```

---

### 14.3 截到自身遮罩层

风险：

```text
用户完成选择后，如果立即 BitBlt，可能把选区遮罩截进去。
```

解决：

```text
截图前先隐藏 CaptureOverlayWindow，并等待窗口刷新。
```

---

### 14.4 某些窗口截不到

风险：

```text
视频、游戏、硬件加速窗口、受保护内容可能出现黑屏或空白。
```

解决：

```text
MVP 阶段接受该限制。
后续通过 Windows Graphics Capture 或 Desktop Duplication 扩展。
```

---

### 14.5 剪贴板内存所有权

风险：

```text
SetClipboardData 成功后错误释放 hGlobal，导致剪贴板数据异常。
```

解决：

```text
SetClipboardData 成功后，内存所有权交给系统，程序不能再释放。
```

---

## 15. MVP 验收标准

MVP 版本完成后，需要满足以下标准：

```text
1. 程序启动后可后台运行
2. 全局快捷键可正常触发截图
3. 支持多显示器下拖选区域
4. 支持高 DPI 下区域不偏移
5. 用户可以拖动选择截图区域
6. 截图完成后进入编辑器
7. 编辑器支持箭头、文字、高亮、马赛克、裁剪
8. 支持撤销最后一步编辑
9. 支持 Enter 输出到剪贴板
10. 支持 Esc 取消
11. 粘贴到微信、QQ、浏览器、Office、画图等常见软件中可正常显示
```

---

## 16. 总结

MVP 版本的核心目标不是做一个完整替代 ShareX 或 Snipaste 的工具，而是打通一个稳定、可维护、可扩展的截图链路。

第一版重点是：

```text
快捷键
    ↓
选区
    ↓
GDI 截图
    ↓
编辑器
    ↓
剪贴板
```

推荐先实现：

```text
1. HotkeyManager
2. CaptureOverlayWindow
3. GdiCaptureBackend
4. ImageBuffer
5. ClipboardExporter
6. EditorWindow
```

当这些模块稳定后，再逐步扩展长截图、保存文件、OCR、上传图床、窗口识别和现代捕获后端。

该项目最值得优先保证的不是功能数量，而是：

```text
1. 坐标准确
2. 截图稳定
3. 剪贴板兼容
4. 编辑器操作自然
5. 架构方便扩展
```
