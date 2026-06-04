# MySnipaste

MySnipaste 是一个 Windows 截屏工具项目，当前目标是实现类似飞书截图/Snipaste 的本地截屏、编辑、复制、保存和贴图体验。

## 当前功能

- 后台托盘运行，支持托盘菜单截图和退出。
- `Ctrl + Shift + A` 触发截图。
- 支持多显示器虚拟屏幕坐标和高 DPI 初始化。
- 拖选区域后进入原位编辑器。
- 编辑器支持箭头、矩形、椭圆、直线、序号、文字、高亮、马赛克、裁剪、画笔、标签、模糊。
- 支持 Undo/Redo、选中、移动、删除、样式调整。
- `Enter` 或确认按钮复制最终图片到剪贴板。
- `Ctrl + S` 或保存按钮通过 WIC 导出 PNG。
- 贴图功能支持生成独立置顶图片窗口、拖动、缩放和右上角关闭。

## 项目结构

```text
src/
  app/       应用生命周期、主窗口、状态流转
  capture/   截图选区层和 GDI 截图后端
  editor/    截图编辑器、工具栏、图形、渲染
  export/    剪贴板和 PNG 导出
  hotkey/    全局快捷键
  image/     ImageBuffer 和 DIB 构建
  pin/       贴图窗口
  tray/      托盘图标和通知
  utils/     DPI、日志等工具
tests/       轻量级 C++ 自动化测试
docs/        设计文档和里程碑方案
images/      图标资源
```

## 构建环境

- Windows
- Visual Studio 2022 MSVC
- CMake 3.24 或更高版本

## 构建

使用 NMake 生成目录：

```bat
cmake -S . -B build-nmake -G "NMake Makefiles"
cmake --build build-nmake
```

如果使用 Visual Studio 生成器：

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

## 运行

构建成功后运行：

```bat
build-nmake\ScreenshotMvp.exe
```

程序启动后不会显示主窗口，会在系统托盘中运行。

## 说明

本项目不引入第三方库，主要基于 Win32、GDI、WIC、CMake 和 CTest 实现。
