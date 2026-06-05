# MySnipaste

MySnipaste 是一个 Windows 本地截屏工具，目标是实现接近飞书截图和 Snipaste 的截屏、编辑、复制、保存、贴图、长截图和文字提取体验。

## 当前功能

- 后台托盘运行，支持托盘菜单截图、长截图和退出。
- `Ctrl + Shift + A` 触发普通截图。
- 支持多显示器虚拟屏幕坐标和高 DPI 初始化。
- 拖选区域后进入原位编辑器，未选区域不遮盖编辑背景。
- 编辑器工具栏支持拖动、tooltip、Undo/Redo 和快捷键。
- 支持矩形、椭圆、直线、箭头、序号、文字、高亮、马赛克、裁剪、画笔、标签、模糊。
- 支持图形选中、移动、删除、样式调整；移动时可见外框限制在截图边界内。
- 支持文字多行输入和再次编辑。
- `Enter` 或确认按钮复制最终图片到剪贴板。
- `Ctrl + S` 或保存按钮通过 WIC 导出 PNG。
- 支持贴图：生成独立置顶图片窗口，可拖动、缩放，并通过右上角关闭按钮退出。
- 支持长截图：托盘 `Long Capture` 入口，选区外半透明遮罩，选区内可直接滚动页面，右下角独立控制窗提供取消和确认；确认后进入固定视口编辑器。
- 支持文字提取入口：编辑器内点击 OCR 按钮或 `Ctrl + O`，识别当前最终图后打开可编辑文本窗口，并可复制到剪贴板。

## OCR 说明

OCR 使用 Windows 本地 `Windows.Media.Ocr`，默认使用系统用户语言。该能力需要应用具备 package identity，因此：

- 普通 `ScreenshotMvp.exe` 可以正常使用截屏、编辑、保存、贴图和长截图。
- 未以 MSIX/package identity 方式运行时，OCR 会提示当前环境不可用。
- 使用 `package-msix` 目标可生成 MSIX 包；如需安装运行，还需要按 Windows 要求签名。

## 项目结构

```text
src/
  app/          应用生命周期、主窗口、状态流转
  capture/      截图选区层和 GDI 截图后端
  editor/       截图编辑器、工具栏、视口、图形、渲染
  export/       剪贴板和 PNG 导出
  hotkey/       全局快捷键
  image/        ImageBuffer 和 DIB 构建
  longcapture/  长截图遮罩、控制窗、拼接逻辑
  ocr/          Windows OCR、文本结果窗口和文本剪贴板导出
  pin/          贴图窗口
  tray/         托盘图标、菜单和通知
  utils/        DPI、日志、虚拟屏幕等工具
tests/          轻量级 C++ 自动化测试
docs/           设计文档和里程碑方案
images/         图标资源
packaging/      MSIX manifest
scripts/        打包脚本
```

## 构建环境

- Windows
- Visual Studio 2022 MSVC
- Windows 10/11 SDK
- CMake 3.24 或更高版本

## 构建

NMake 构建：

```bat
cmake -S . -B build-nmake -G "NMake Makefiles"
cmake --build build-nmake
```

Visual Studio 生成器：

```bat
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug
```

## 测试

NMake 构建目录：

```bat
ctest --test-dir build-nmake --output-on-failure
```

Visual Studio 构建目录：

```bat
ctest --test-dir build -C Debug --output-on-failure
```

## 打包 MSIX

生成未签名 MSIX：

```bat
cmake --build build-nmake --target package-msix
```

输出路径：

```text
build-nmake\MySnipaste.msix
```

如需签名，可直接运行脚本并传入证书：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\package_msix.ps1 `
  -BuildDir build-nmake `
  -CertificatePath path\to\cert.pfx `
  -CertificatePassword your_password
```

## 运行

构建成功后运行：

```bat
build-nmake\ScreenshotMvp.exe
```

程序启动后不会显示主窗口，会在系统托盘中运行。右键托盘图标可选择普通截图、长截图或退出。

## 说明

项目当前不引入第三方库，主要基于 Win32、GDI、WIC、Windows OCR、CMake 和 CTest 实现。长截图第一版以标准纵向滚动区域为主；复杂虚拟列表、横向滚动、窗口识别和长截图自动去重优化仍可继续迭代。
