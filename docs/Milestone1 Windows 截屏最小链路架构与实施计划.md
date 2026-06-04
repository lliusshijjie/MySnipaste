# Milestone 1：Windows 截屏工具最小链路开发文档

## 1. 阶段目标

Milestone 1 只实现 Windows 截屏工具的最小可用闭环。

本阶段目标是：

```text
后台运行
    ↓
全局快捷键触发
    ↓
显示全屏截图选区层
    ↓
用户拖动选择截图区域
    ↓
基于 GDI BitBlt 截取屏幕区域
    ↓
转换为内部 ImageBuffer
    ↓
构造 CF_DIB
    ↓
写入系统剪贴板
```

第一阶段完成后，用户可以：

```text
1. 启动程序后在后台运行
2. 通过托盘菜单或全局快捷键进入截图模式
3. 鼠标拖动选择屏幕区域
4. 截图结果自动复制到系统剪贴板
5. 粘贴到画图、微信、QQ、Office、浏览器输入框等常见应用中
```

---

## 2. 本阶段范围

### 2.1 本阶段实现内容

Milestone 1 实现以下能力：

```text
1. C++20 + Win32 + CMake 工程骨架
2. 后台运行
3. 普通隐藏主窗口
4. 系统托盘图标
5. 托盘菜单：截图、退出
6. 全局快捷键：Ctrl + Shift + A
7. 全屏截图选区层
8. 鼠标拖动选择区域
9. Esc 取消截图
10. GDI BitBlt 截图
11. ImageBuffer 图像结构
12. CF_DIB 剪贴板输出
13. DPI awareness 初始化
14. 多显示器虚拟屏幕坐标支持
15. 基础错误处理
16. 基础日志输出
17. 核心纯逻辑单元测试
```

### 2.2 本阶段不实现内容

以下能力不属于 Milestone 1：

```text
1. 编辑器窗口
2. 箭头、文字、高亮、马赛克、裁剪
3. 撤销和重做
4. 截长屏
5. OCR
6. 保存图片文件
7. 上传图床
8. 录屏
9. GIF
10. 窗口自动识别
11. PrintWindow 截窗口
12. Desktop Duplication API
13. Windows Graphics Capture API
14. 快捷键配置界面
15. 截图历史管理
```

Milestone 1 的核心原则是：

```text
先把截图链路做稳，再进入编辑器阶段。
```

---

## 3. 技术选型

### 3.1 开发语言与平台

```text
语言：C++20
平台：Windows 10 / Windows 11
构建：CMake
编译器：MSVC / Visual Studio 2022
UI：Win32 API
截图：GDI BitBlt
剪贴板：CF_DIB
```

### 3.2 依赖库

Milestone 1 不引入第三方库。

使用的系统库：

```text
user32
gdi32
shell32
comctl32
```

CMake 链接示例：

```cmake
target_link_libraries(ScreenshotMvp
    PRIVATE
        user32
        gdi32
        shell32
        comctl32
)
```

---

## 4. 核心链路

### 4.1 主流程

```text
WinMain
    ↓
DpiUtils::InitializeDpiAwareness()
    ↓
App::Initialize()
    ↓
创建普通隐藏主窗口
    ↓
注册全局热键
    ↓
创建托盘图标
    ↓
进入消息循环
    ↓
用户按 Ctrl + Shift + A 或点击托盘截图
    ↓
App::StartCapture()
    ↓
CaptureOverlayWindow::SelectRegion()
    ↓
用户拖动选区
    ↓
返回 SelectionResult
    ↓
隐藏 Overlay 并刷新窗口状态
    ↓
GdiCaptureBackend::CaptureRect()
    ↓
得到 CaptureResult
    ↓
ClipboardExporter::CopyImage()
    ↓
成功复制到剪贴板
    ↓
程序回到后台 Idle 状态
```

### 4.2 截图完成后的行为

Milestone 1 截图完成后直接复制到剪贴板。

```text
不打开编辑器。
不弹出图片预览。
不保存文件。
```

成功后可以选择：

```text
1. 静默成功
2. 托盘气泡提示“已复制到剪贴板”
```

建议 MVP 阶段使用托盘轻提示，但不要打断用户。

---

## 5. 程序状态机

为了避免截图流程重入，App 需要维护状态机。

```cpp
enum class AppState {
    Idle,
    Selecting,
    Capturing,
    CopyingToClipboard,
    Exiting
};
```

状态规则：

```text
Idle：
    允许开始截图。

Selecting：
    正在显示选区层。
    再次触发热键或托盘截图时直接忽略。

Capturing：
    正在调用 GDI 截图。
    不允许再次进入截图。

CopyingToClipboard：
    正在写剪贴板。
    不允许再次进入截图。

Exiting：
    正在退出。
    不处理新的截图请求。
```

状态迁移：

```text
Idle
    ↓ StartCapture
Selecting
    ↓ 用户完成选区
Capturing
    ↓ 截图成功
CopyingToClipboard
    ↓ 剪贴板写入完成
Idle
```

取消路径：

```text
Selecting
    ↓ Esc / 选区太小 / 失败
Idle
```

失败路径：

```text
Capturing / CopyingToClipboard
    ↓ 失败提示
Idle
```

---

## 6. 模块划分

项目目录结构建议如下：

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
 │   ├─ tray/
 │   │   ├─ TrayIcon.h
 │   │   └─ TrayIcon.cpp
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
 │   │   ├─ DibBuilder.h
 │   │   └─ DibBuilder.cpp
 │   │
 │   ├─ export/
 │   │   ├─ ClipboardExporter.h
 │   │   └─ ClipboardExporter.cpp
 │   │
 │   └─ utils/
 │       ├─ RectUtils.h
 │       ├─ DpiUtils.h
 │       ├─ Win32Utils.h
 │       └─ LogUtils.h
 │
 ├─ tests/
 │   ├─ test_rect_utils.cpp
 │   ├─ test_image_buffer.cpp
 │   └─ test_dib_builder.cpp
 │
 └─ docs/
     └─ Milestone1_Windows截屏最小链路开发文档.md
```

---

## 7. 模块职责

### 7.1 App

职责：

```text
1. 初始化 DPI awareness
2. 创建隐藏主窗口
3. 管理消息循环
4. 注册和注销全局热键
5. 创建和销毁托盘图标
6. 维护 AppState
7. 调度截图流程
8. 处理退出清理
```

App 需要处理的主要消息：

```text
WM_CREATE
WM_DESTROY
WM_HOTKEY
WM_COMMAND
WM_TRAYICON
```

---

### 7.2 Hidden Main Window

Milestone 1 使用普通隐藏窗口，不使用 message-only window。

原因：

```text
1. 需要接收 WM_HOTKEY
2. 需要接收托盘图标回调消息
3. 需要作为托盘菜单 owner
4. 需要作为 OpenClipboard 的 owner
5. 普通隐藏窗口在 Shell 交互上更稳妥
```

隐藏窗口创建后不调用 `ShowWindow` 即可。

窗口职责：

```text
1. 接收全局热键消息
2. 接收托盘回调消息
3. 处理托盘菜单命令
4. 作为程序生命周期入口
```

---

### 7.3 HotkeyManager

职责：

```text
1. 封装 RegisterHotKey
2. 封装 UnregisterHotKey
3. 管理默认热键 Ctrl + Shift + A
4. 热键冲突时返回失败
```

接口草案：

```cpp
class HotkeyManager {
public:
    bool Register(HWND hwnd, int id, UINT modifiers, UINT vk);
    void Unregister(HWND hwnd, int id);

private:
    bool registered_ = false;
};
```

默认热键：

```cpp
Ctrl + Shift + A
```

对应参数：

```cpp
MOD_CONTROL | MOD_SHIFT, 'A'
```

热键注册失败时：

```text
1. MessageBox 明确提示用户
2. 程序继续后台运行
3. 用户仍可通过托盘菜单触发截图
```

---

### 7.4 TrayIcon

职责：

```text
1. 创建系统托盘图标
2. 响应左键双击截图
3. 响应右键菜单
4. 提供菜单项：截图、退出
5. 退出时删除托盘图标
```

托盘图标使用：

```cpp
NOTIFYICONDATA
Shell_NotifyIcon
```

建议定义：

```cpp
constexpr UINT WM_TRAYICON = WM_APP + 1;
```

托盘消息处理：

```text
WM_LBUTTONDBLCLK：
    触发截图

WM_RBUTTONUP：
    弹出菜单
```

右键菜单弹出前需要调用：

```cpp
SetForegroundWindow(hwnd);
```

否则托盘菜单可能无法正常自动关闭。

退出时必须调用：

```cpp
Shell_NotifyIcon(NIM_DELETE, ...);
```

---

### 7.5 CaptureOverlayWindow

职责：

```text
1. 覆盖整个虚拟屏幕
2. 显示截图选区层
3. 处理鼠标拖动
4. 实时绘制选区矩形
5. 支持 Esc 取消
6. 返回最终截图区域
```

对外接口：

```cpp
enum class SelectionStatus {
    Completed,
    Cancelled,
    TooSmall,
    Failed
};

struct SelectionResult {
    SelectionStatus status;
    RECT screenRect{};
};

class CaptureOverlayWindow {
public:
    SelectionResult SelectRegion(HWND owner);
};
```

本阶段采用 Modal 局部消息循环模型。

也就是说：

```text
SelectRegion 内部创建 Overlay 窗口，并进入局部消息循环。
用户完成选择或取消后，局部消息循环结束，函数返回 SelectionResult。
```

需要注意：

```text
1. App 必须在调用 SelectRegion 前把状态改为 Selecting
2. Selecting 状态下禁止再次进入截图
3. Overlay 销毁后必须恢复 App 状态
```

---

## 8. CaptureOverlayWindow 设计细节

### 8.1 窗口范围

Overlay 需要覆盖整个虚拟屏幕。

不能只用主屏幕尺寸。

应使用：

```cpp
int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
```

虚拟屏幕可能存在负坐标：

```text
副屏在主屏左侧时：
副屏 x 可能为 -1920
主屏 x 为 0
```

所以所有选区坐标都应使用虚拟屏幕物理坐标。

---

### 8.2 窗口样式

Overlay 普通窗口样式：

```cpp
WS_POPUP
```

Overlay 扩展窗口样式：

```cpp
WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED
```

不要使用：

```cpp
WS_EX_TRANSPARENT
```

原因：

```text
WS_EX_TRANSPARENT 会让鼠标事件穿透窗口，导致 Overlay 收不到拖选事件。
```

---

### 8.3 鼠标事件

Overlay 需要处理：

```text
WM_LBUTTONDOWN
WM_MOUSEMOVE
WM_LBUTTONUP
WM_CAPTURECHANGED
WM_KEYDOWN
WM_PAINT
```

拖选流程：

```text
WM_LBUTTONDOWN：
    记录起点
    selecting = true
    SetCapture(hwnd)

WM_MOUSEMOVE：
    如果 selecting = true
        更新 current 点
        InvalidateRect 触发重绘

WM_LBUTTONUP：
    ReleaseCapture
    selecting = false
    归一化矩形
    判断尺寸
    返回 SelectionResult

WM_CAPTURECHANGED：
    如果正在选择
        取消本次选择

WM_KEYDOWN：
    如果按下 Esc
        返回 Cancelled
```

必须使用：

```cpp
SetCapture(hwnd);
ReleaseCapture();
```

原因：

```text
用户拖动时鼠标可能移出窗口边界或跨屏幕边缘。
如果不捕获鼠标，可能丢失 WM_LBUTTONUP，导致 Overlay 卡住。
```

---

### 8.4 选区矩形规则

选区矩形需要归一化。

用户可能从任意方向拖动：

```text
左上 → 右下
右下 → 左上
右上 → 左下
左下 → 右上
```

归一化后保证：

```text
left <= right
top <= bottom
```

小于 `3 x 3` 像素的选区视为无效。

```text
宽度 < 3 或高度 < 3：
    返回 TooSmall
```

TooSmall 可以静默处理，不弹窗。

---

### 8.5 Overlay 绘制

MVP 推荐绘制方式：

```text
1. 进入截图模式前，先截取整个虚拟屏幕作为背景预览图
2. Overlay 显示该背景图
3. 在背景图上绘制半透明暗色遮罩
4. 选区区域显示原始亮度
5. 绘制选区边框
```

注意：

```text
Overlay 背景截图只用于选区预览。
最终复制到剪贴板的图片，必须在 Overlay 隐藏后重新截取选区区域。
```

也就是说，流程是两次截图：

```text
第一次：全屏截图，用于 Overlay 背景预览
第二次：选区截图，用于最终剪贴板输出
```

如果为了减少第一阶段复杂度，也可以采用更简单的 Overlay 绘制方式：

```text
只绘制半透明遮罩和选区边框，不显示背景截图。
```

但推荐第一阶段保留背景预览，用户体验更自然。

---

## 9. 截图后端设计

### 9.1 ICaptureBackend

为了后续扩展 PrintWindow、Desktop Duplication、Windows Graphics Capture，第一阶段保留截图后端抽象。

```cpp
class ICaptureBackend {
public:
    virtual ~ICaptureBackend() = default;
    virtual std::optional<CaptureResult> CaptureRect(const RECT& rect) = 0;
};
```

---

### 9.2 GdiCaptureBackend

Milestone 1 只实现 GDI 截图后端。

输入：

```text
虚拟屏幕物理坐标 RECT
```

输出：

```text
CaptureResult
```

核心流程：

```text
1. GetDC(nullptr) 获取屏幕 DC
2. CreateCompatibleDC 创建内存 DC
3. CreateCompatibleBitmap 创建位图
4. SelectObject 选入 bitmap
5. BitBlt 拷贝屏幕区域
6. GetDIBits 读取像素
7. 转换为 ImageBuffer
8. 释放所有 GDI 资源
```

BitBlt 调用方式：

```cpp
BitBlt(
    memDc,
    0,
    0,
    width,
    height,
    screenDc,
    rect.left,
    rect.top,
    SRCCOPY | CAPTUREBLT
);
```

注意：

```text
这里是 BitBlt 的 raster operation 参数使用 SRCCOPY | CAPTUREBLT。
不是 BitBlt 函数本身与 CAPTUREBLT 做或运算。
```

---

### 9.3 GDI 截图限制

MVP 接受以下限制：

```text
1. 某些 GPU 加速窗口可能截取异常
2. 某些视频或 DRM 内容可能黑屏
3. UAC 安全桌面不能截
4. 锁屏界面不能截
5. 管理员权限窗口在某些情况下可能受限制
6. CAPTUREBLT 不能保证捕获所有 layered window
```

后续版本可以通过其他后端扩展：

```text
PrintWindowCaptureBackend
DesktopDuplicationBackend
WindowsGraphicsCaptureBackend
```

但 Milestone 1 不实现。

---

## 10. Overlay 隐藏与截图时序

用户完成选区后，不能立即截图。

否则可能截到自己的 Overlay 遮罩层。

推荐流程：

```text
1. 用户松开鼠标
2. Overlay 得到选区 rect
3. 隐藏 Overlay
4. 刷新窗口状态
5. 处理必要的窗口消息
6. 调用 GdiCaptureBackend 截图
```

建议封装：

```cpp
void HideOverlayBeforeCapture(HWND overlayHwnd);
```

参考逻辑：

```cpp
ShowWindow(overlayHwnd, SW_HIDE);
UpdateWindow(overlayHwnd);

MSG msg;
while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}
```

MVP 可以在必要时增加短暂等待：

```cpp
Sleep(30);
```

但不要依赖长时间 Sleep 作为主要同步机制。

---

## 11. 图像数据结构

### 11.1 ImageBuffer

程序内部统一使用 ImageBuffer，不在各模块之间传递 HBITMAP。

```cpp
struct ImageBuffer {
    int width = 0;
    int height = 0;
    int stride = 0;
    std::vector<std::uint8_t> pixels;
};
```

内部规则：

```text
像素格式：BGRA 8-bit
内存布局：top-down
每个像素：4 字节
stride：width * 4
pixels 大小：stride * height
```

top-down 含义：

```text
pixels 第 0 行表示图片顶部。
```

### 11.2 CaptureResult

```cpp
struct CaptureResult {
    ImageBuffer image;
    RECT screenRect{};
};
```

字段说明：

```text
image：
    截图像素数据

screenRect：
    截图区域在虚拟屏幕中的物理坐标
```

---

## 12. DIB 构造与剪贴板输出

### 12.1 为什么拆分 DibBuilder

为了方便测试，剪贴板输出拆成两层：

```text
DibBuilder：
    只负责把 ImageBuffer 构造成 CF_DIB 字节数据。
    可单元测试。

ClipboardExporter：
    只负责 OpenClipboard / EmptyClipboard / SetClipboardData。
    主要手动验收。
```

接口草案：

```cpp
class DibBuilder {
public:
    std::vector<std::uint8_t> BuildCfDib(const ImageBuffer& image);
};

class ClipboardExporter {
public:
    bool CopyImage(HWND owner, const ImageBuffer& image);
};
```

---

### 12.2 CF_DIB 格式

Milestone 1 使用：

```text
剪贴板格式：CF_DIB
位深：32bpp
压缩：BI_RGB
```

CF_DIB 内容不包含：

```text
BITMAPFILEHEADER
```

CF_DIB 内容包含：

```text
BITMAPINFOHEADER
像素数据
```

---

### 12.3 top-down 到 bottom-up 转换

内部 ImageBuffer 使用 top-down。

为了兼容性，CF_DIB 导出使用 bottom-up。

转换规则：

```text
ImageBuffer 第 0 行是图片顶部。
CF_DIB 第 0 行是图片底部。
```

导出时需要翻转行顺序：

```cpp
for (int y = 0; y < image.height; ++y) {
    int srcY = image.height - 1 - y;

    std::memcpy(
        dibPixels + y * dibStride,
        image.pixels.data() + srcY * image.stride,
        image.stride
    );
}
```

---

### 12.4 alpha 通道处理

为了保证粘贴到常见软件时不出现透明异常，写入 CF_DIB 前需要强制 alpha 为不透明。

规则：

```text
所有像素 alpha = 255
```

即：

```cpp
pixel.a = 0xFF;
```

原因：

```text
32bpp BI_RGB DIB 的 alpha 通道在不同应用中的解释并不完全一致。
普通屏幕截图不需要透明度。
强制 alpha = 255 可以提高兼容性。
```

---

### 12.5 ClipboardExporter 流程

写入剪贴板流程：

```text
1. DibBuilder 构造 CF_DIB 字节数据
2. GlobalAlloc 分配 HGLOBAL
3. GlobalLock 写入数据
4. GlobalUnlock
5. OpenClipboard(owner)
6. EmptyClipboard()
7. SetClipboardData(CF_DIB, hGlobal)
8. CloseClipboard()
```

重要所有权规则：

```text
SetClipboardData 成功后，hGlobal 所有权转移给系统。
程序不能再调用 GlobalFree(hGlobal)。

SetClipboardData 失败时，程序需要 GlobalFree(hGlobal)。
```

OpenClipboard 失败时：

```text
返回 false
不崩溃
提示用户或输出日志
```

---

## 13. DPI 与多显示器

### 13.1 DPI 初始化

DPI 初始化必须发生在创建任何窗口之前。

启动顺序必须是：

```text
WinMain
    ↓
DpiUtils::InitializeDpiAwareness()
    ↓
创建任何窗口
```

推荐策略：

```text
1. 优先调用 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)
2. 如果失败，降级调用 SetProcessDPIAware()
3. 如果仍失败，继续运行，但记录日志
```

Milestone 1 不要求支持很老的 Windows 版本，但不能因为 DPI 初始化失败直接退出。

---

### 13.2 坐标系统

Milestone 1 统一使用：

```text
虚拟屏幕物理坐标
```

规则：

```text
1. Overlay 覆盖整个虚拟屏幕
2. 用户选区返回虚拟屏幕物理坐标
3. GDI BitBlt 使用虚拟屏幕物理坐标
4. RECT 允许 left/top 为负数
```

不要假设屏幕左上角是：

```text
(0, 0)
```

副屏在主屏左侧或上方时，坐标可能为负。

---

## 14. Win32 资源管理规则

Milestone 1 所有 Win32 资源必须遵守所有权规则。

建议使用 RAII 包装。

| 资源          | 创建方式                     | 释放方式                              |
| ----------- | ------------------------ | --------------------------------- |
| HDC 屏幕 DC   | GetDC                    | ReleaseDC                         |
| HDC 内存 DC   | CreateCompatibleDC       | DeleteDC                          |
| HBITMAP     | CreateCompatibleBitmap   | DeleteObject                      |
| HGDIOBJ 旧对象 | SelectObject 返回          | 退出前 SelectObject 还原               |
| HWND        | CreateWindowEx           | DestroyWindow                     |
| HGLOBAL     | GlobalAlloc              | GlobalFree，除非 SetClipboardData 成功 |
| 托盘图标        | Shell_NotifyIcon NIM_ADD | Shell_NotifyIcon NIM_DELETE       |
| 全局热键        | RegisterHotKey           | UnregisterHotKey                  |

GDI 截图部分必须保证：

```text
1. 任意失败路径都不泄漏 HDC
2. 任意失败路径都不泄漏 HBITMAP
3. SelectObject 后必须恢复旧对象
4. 连续截图时 GDI 对象数量不应持续增长
```

---

## 15. 错误处理策略

### 15.1 错误分级

| 场景           | 处理方式                      |
| ------------ | ------------------------- |
| DPI 初始化失败    | OutputDebugString 记录，继续运行 |
| 热键注册失败       | MessageBox 提示，继续托盘运行      |
| 托盘图标创建失败     | MessageBox 提示，可继续运行       |
| 用户 Esc 取消    | 静默                        |
| 选区太小         | 静默                        |
| Overlay 创建失败 | MessageBox 或托盘提示          |
| GDI 截图失败     | 托盘气泡或 MessageBox          |
| 剪贴板打开失败      | 托盘气泡                      |
| 内存分配失败       | MessageBox                |
| 退出清理失败       | OutputDebugString 记录      |

### 15.2 日志策略

Milestone 1 不引入日志库。

Debug 构建使用：

```cpp
OutputDebugString
```

建议封装：

```cpp
void LogInfo(std::wstring_view message);
void LogError(std::wstring_view action);
void LogLastError(std::wstring_view action);
```

`LogLastError` 输出格式：

```text
[Module] Action failed. GetLastError=xxx
```

例如：

```text
[GdiCapture] BitBlt failed. GetLastError=5
[Clipboard] OpenClipboard failed. GetLastError=1418
```

---

## 16. 类与接口草案

### 16.1 App

```cpp
class App {
public:
    bool Initialize(HINSTANCE instance);
    int Run();
    void Shutdown();

private:
    void StartCapture();
    void OnSelectionFinished(const SelectionResult& result);
    void SetState(AppState state);

private:
    HINSTANCE instance_ = nullptr;
    HWND mainWindow_ = nullptr;
    AppState state_ = AppState::Idle;
};
```

职责：

```text
1. 管理全局生命周期
2. 维护状态机
3. 串联热键、托盘、Overlay、截图、剪贴板
```

---

### 16.2 HotkeyManager

```cpp
class HotkeyManager {
public:
    bool Register(HWND hwnd, int id, UINT modifiers, UINT vk);
    void Unregister(HWND hwnd, int id);

private:
    bool registered_ = false;
    int id_ = 0;
};
```

---

### 16.3 TrayIcon

```cpp
class TrayIcon {
public:
    bool Add(HWND hwnd, UINT callbackMessage);
    void Remove();
    void ShowMenu(HWND hwnd);
    void ShowBalloon(std::wstring_view title, std::wstring_view message);

private:
    NOTIFYICONDATA nid_{};
    bool added_ = false;
};
```

---

### 16.4 CaptureOverlayWindow

```cpp
enum class SelectionStatus {
    Completed,
    Cancelled,
    TooSmall,
    Failed
};

struct SelectionResult {
    SelectionStatus status = SelectionStatus::Failed;
    RECT screenRect{};
};

class CaptureOverlayWindow {
public:
    SelectionResult SelectRegion(HWND owner);

private:
    bool CreateOverlayWindow(HWND owner);
    void DestroyOverlayWindow();

    void OnMouseDown(POINT pt);
    void OnMouseMove(POINT pt);
    void OnMouseUp(POINT pt);
    void OnCaptureChanged();
    void OnKeyDown(WPARAM key);
    void OnPaint();

private:
    HWND hwnd_ = nullptr;
    bool selecting_ = false;
    bool done_ = false;
    POINT start_{};
    POINT current_{};
    SelectionResult result_{};
};
```

---

### 16.5 ICaptureBackend 与 GdiCaptureBackend

```cpp
struct ImageBuffer {
    int width = 0;
    int height = 0;
    int stride = 0;
    std::vector<std::uint8_t> pixels;
};

struct CaptureResult {
    ImageBuffer image;
    RECT screenRect{};
};

class ICaptureBackend {
public:
    virtual ~ICaptureBackend() = default;
    virtual std::optional<CaptureResult> CaptureRect(const RECT& rect) = 0;
};

class GdiCaptureBackend : public ICaptureBackend {
public:
    std::optional<CaptureResult> CaptureRect(const RECT& rect) override;
};
```

---

### 16.6 DibBuilder 与 ClipboardExporter

```cpp
class DibBuilder {
public:
    std::vector<std::uint8_t> BuildCfDib(const ImageBuffer& image);
};

class ClipboardExporter {
public:
    bool CopyImage(HWND owner, const ImageBuffer& image);

private:
    DibBuilder dibBuilder_;
};
```

---

## 17. 测试计划

### 17.1 自动化测试范围

自动化测试只覆盖纯逻辑模块。

不对以下模块做自动化测试：

```text
1. RegisterHotKey
2. Shell_NotifyIcon
3. BitBlt
4. OpenClipboard
5. Overlay Window
6. DPI API
```

这些模块以手动验收为主。

---

### 17.2 RectUtils 测试

测试项：

```text
RectUtils_NormalizesDraggedRect
RectUtils_AllowsNegativeVirtualCoordinates
RectUtils_RejectsTooSmallSelection
RectUtils_ClipsRectToVirtualScreen
RectUtils_ComputesEmptyRect
RectUtils_NormalizesZeroWidthOrHeight
```

覆盖场景：

```text
1. 从左上拖到右下
2. 从右下拖到左上
3. 从右上拖到左下
4. 从左下拖到右上
5. 负坐标矩形
6. 0 宽度矩形
7. 0 高度矩形
8. 小于 3x3 的矩形
```

---

### 17.3 ImageBuffer 测试

测试项：

```text
ImageBuffer_ComputesBgraStride
ImageBuffer_ValidatesCapacity
ImageBuffer_RejectsInvalidSize
```

验证：

```text
stride = width * 4
pixels.size = stride * height
width / height 必须大于 0
```

---

### 17.4 DibBuilder 测试

测试项：

```text
DibBuilder_BuildsValidBitmapInfoHeader
DibBuilder_DoesNotIncludeBitmapFileHeader
DibBuilder_ConvertsTopDownToBottomUp
DibBuilder_ForcesAlphaToOpaque
DibBuilder_ComputesDibStrideAlignedTo4Bytes
```

验证：

```text
1. BITMAPINFOHEADER 字段正确
2. biBitCount = 32
3. biCompression = BI_RGB
4. biHeight 为正数，表示 bottom-up DIB
5. CF_DIB 不包含 BITMAPFILEHEADER
6. alpha 被强制设置为 255
7. 行顺序完成上下翻转
```

---

## 18. 手动验收计划

### 18.1 基础功能验收

```text
1. 启动程序后不显示主窗口
2. 系统托盘图标存在
3. 托盘右键菜单包含“截图”和“退出”
4. 点击托盘“截图”可以进入截图模式
5. 按 Ctrl + Shift + A 可以进入截图模式
6. 鼠标拖选区域后截图自动复制到剪贴板
7. 截图后程序继续后台运行
8. 点击托盘“退出”后程序正常退出
```

---

### 18.2 剪贴板验收

截图后应能粘贴到：

```text
1. Windows 画图
2. 微信聊天输入框
3. QQ 聊天输入框
4. Word
5. PowerPoint
6. 浏览器富文本输入框
```

验证点：

```text
1. 图片方向正确，没有上下颠倒
2. 图片没有透明异常
3. 图片区域与拖选区域一致
4. 图片没有包含 Overlay 遮罩层
```

---

### 18.3 取消与异常流程验收

```text
1. 进入截图模式后按 Esc，截图取消
2. Esc 取消后剪贴板不应被修改
3. 拖选小于 3x3 的区域，截图取消
4. 取消后可以再次截图
5. 截图过程中再次按 Ctrl + Shift + A 不会创建第二个 Overlay
6. 截图过程中通过托盘点击截图不会重入
```

---

### 18.4 多显示器验收

测试场景：

```text
1. 单显示器
2. 双显示器，副屏在主屏右侧
3. 双显示器，副屏在主屏左侧
4. 双显示器，副屏在主屏上方
```

验证点：

```text
1. Overlay 覆盖所有显示器
2. 副屏负坐标区域可以正确拖选
3. 截图区域不偏移
4. 截图结果尺寸正确
```

---

### 18.5 高 DPI 验收

测试缩放比例：

```text
100%
125%
150%
200%
```

验证点：

```text
1. 鼠标拖选区域与截图结果一致
2. 没有明显偏移
3. 没有被系统缩放导致尺寸错误
4. 多屏不同 DPI 下基本可用
```

---

### 18.6 资源泄漏验收

使用任务管理器观察：

```text
任务管理器
    ↓
详细信息
    ↓
选择列
    ↓
GDI 对象
```

验收方式：

```text
1. 连续截图 20 次
2. 连续取消截图 20 次
3. 重复托盘截图 20 次
```

通过标准：

```text
GDI 对象数量不应持续增长。
内存不应出现明显持续增长。
程序不崩溃。
```

---

## 19. 构建与运行

### 19.1 构建环境

```text
Windows 10 / 11
Visual Studio 2022 Community
CMake
MSVC
```

### 19.2 构建命令

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug

cmake --build build

ctest --test-dir build --output-on-failure
```

### 19.3 子系统

MVP 程序使用 Windows GUI 子系统，不显示控制台窗口。

CMake 中应使用：

```cmake
add_executable(ScreenshotMvp WIN32
    ...
)
```

---

## 20. 实施顺序

建议按以下顺序开发。

### Step 1：创建 CMake 工程

完成内容：

```text
1. 创建 CMakeLists.txt
2. 创建 src/main.cpp
3. 创建空 App
4. 成功编译 Windows GUI 程序
```

验收：

```text
程序可以启动和退出。
```

---

### Step 2：实现纯逻辑工具

完成内容：

```text
1. RectUtils
2. ImageBuffer
3. DibBuilder
4. 对应单元测试
```

验收：

```text
ctest 全部通过。
```

---

### Step 3：实现 DPI 初始化

完成内容：

```text
1. DpiUtils::InitializeDpiAwareness()
2. 启动最早期调用
3. 失败时 OutputDebugString
```

验收：

```text
程序能在普通 DPI 和高 DPI 环境启动。
```

---

### Step 4：实现隐藏主窗口

完成内容：

```text
1. 注册窗口类
2. 创建普通隐藏窗口
3. 实现消息过程
4. 实现 App::Run()
```

验收：

```text
程序后台运行，没有主窗口。
```

---

### Step 5：实现托盘图标

完成内容：

```text
1. 添加托盘图标
2. 右键菜单
3. 菜单项：截图、退出
4. 删除托盘图标
```

验收：

```text
托盘图标可见。
右键菜单正常。
退出后托盘图标消失。
```

---

### Step 6：实现全局热键

完成内容：

```text
1. 注册 Ctrl + Shift + A
2. 处理 WM_HOTKEY
3. 热键冲突提示
4. 退出时注销热键
```

验收：

```text
按 Ctrl + Shift + A 可以触发日志或占位截图流程。
```

---

### Step 7：实现 CaptureOverlayWindow

完成内容：

```text
1. 覆盖虚拟屏幕
2. 绘制遮罩
3. 鼠标拖选
4. Esc 取消
5. SetCapture / ReleaseCapture
6. 返回 SelectionResult
```

验收：

```text
用户可以拖选区域。
四个方向拖选均正确。
Esc 可以取消。
多显示器下 Overlay 覆盖完整。
```

---

### Step 8：实现 GdiCaptureBackend

完成内容：

```text
1. GetDC(nullptr)
2. CreateCompatibleDC
3. CreateCompatibleBitmap
4. BitBlt with SRCCOPY | CAPTUREBLT
5. GetDIBits
6. 转换为 top-down BGRA ImageBuffer
7. RAII 释放 GDI 资源
```

验收：

```text
可以截取指定 RECT 并得到 ImageBuffer。
```

---

### Step 9：实现 ClipboardExporter

完成内容：

```text
1. DibBuilder 构造 CF_DIB
2. GlobalAlloc
3. OpenClipboard
4. EmptyClipboard
5. SetClipboardData
6. CloseClipboard
```

验收：

```text
指定 ImageBuffer 可以复制到剪贴板。
```

---

### Step 10：串联完整链路

完成内容：

```text
1. 热键触发 App::StartCapture
2. Overlay 返回选区
3. 隐藏 Overlay
4. GDI 截图
5. 写入剪贴板
6. 回到 Idle 状态
```

验收：

```text
Ctrl + Shift + A → 拖选 → 自动复制 → 粘贴成功。
```

---

## 21. Milestone 1 完成标准

Milestone 1 完成时，需要满足：

```text
1. 程序启动后后台运行
2. 程序有托盘图标
3. 托盘菜单支持截图和退出
4. Ctrl + Shift + A 可以触发截图
5. 全屏 Overlay 覆盖完整虚拟屏幕
6. 鼠标拖选区域准确
7. Esc 可以取消截图
8. 小选区不会生成异常图片
9. GDI 截图成功生成 ImageBuffer
10. CF_DIB 成功写入剪贴板
11. 图片可以粘贴到常见软件
12. 高 DPI 下截图区域不明显偏移
13. 多显示器负坐标下截图正确
14. 截图过程中不会重入
15. 连续截图不出现明显 GDI 资源泄漏
16. 退出时释放托盘图标和全局热键
```

---

## 22. 后续阶段衔接

Milestone 1 需要为 Milestone 2 保留扩展点。

### 22.1 当前流程

```text
CaptureResult
    ↓
ClipboardExporter
```

### 22.2 Milestone 2 目标流程

```text
CaptureResult
    ↓
EditorWindow
    ↓
用户按 Enter
    ↓
ClipboardExporter
```

所以 Milestone 1 中必须保留：

```text
1. CaptureResult
2. ImageBuffer
3. ICaptureBackend
4. ClipboardExporter
```

后续接入编辑器时，不应重写截图链路。

---

## 23. 总结

Milestone 1 的目标不是做完整截图工具，而是打通稳定的最小截图链路。

本阶段最重要的工程点是：

```text
1. Win32 消息循环清晰
2. AppState 防止重入
3. DPI 初始化时机正确
4. 多显示器虚拟坐标正确
5. Overlay 不被截进最终图片
6. GDI 资源不泄漏
7. ImageBuffer 格式统一
8. CF_DIB 上下方向正确
9. alpha 强制不透明
10. 剪贴板所有权处理正确
```

完成 Milestone 1 后，项目就具备了一个稳定的底层截图能力。后续 Milestone 2 只需要把 `CaptureResult` 交给 `EditorWindow`，即可在不破坏第一阶段链路的基础上进入编辑器开发。
