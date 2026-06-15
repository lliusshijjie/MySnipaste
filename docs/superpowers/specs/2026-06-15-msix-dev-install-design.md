# MSIX 本机开发安装链路设计

## 目标

为 MySnipaste 提供一条可重复执行的本机开发部署链路：

```text
构建 EXE -> 生成开发证书 -> 信任证书 -> 打包并签名 MSIX
-> 安装或更新包 -> 从已安装应用启动 -> 验证 package identity
```

该链路只面向开发和本机测试，不替代 Microsoft Store 或可信 CA
证书的正式分发方案。

## 使用方式

新增 CMake 目标：

```bat
cmake --build build-nmake --target install-dev-msix
```

该目标调用 `scripts/install_dev_msix.ps1`。脚本在需要写入
`LocalMachine\TrustedPeople` 时请求管理员权限，完成后安装并启动
MSIX 应用。

同时保留现有目标：

```bat
cmake --build build-nmake --target package-msix
```

该目标只生成 MSIX，不负责创建证书、信任、安装或启动。

## 组件设计

### `scripts/install_dev_msix.ps1`

脚本负责编排完整流程：

1. 解析 `packaging/Package.appxmanifest`，读取包名、Publisher 和应用 ID。
2. 检查 `ScreenshotMvp.exe` 是否存在；不存在时给出明确错误。
3. 在 `build-nmake/dev-signing/` 中复用或创建开发证书。
4. 证书 Subject 必须与 manifest Publisher 完全一致。
5. 将公钥证书安装到 `Cert:\LocalMachine\TrustedPeople`。
6. 调用 `scripts/package_msix.ps1` 生成并签名 MSIX。
7. 验证 MSIX Authenticode 签名状态为 `Valid`。
8. 删除同包名的旧开发安装，再使用 `Add-AppxPackage` 安装新包。
9. 通过 `shell:AppsFolder\<PackageFamilyName>!<ApplicationId>` 启动应用。
10. 输出包全名、安装目录、签名状态和启动入口，便于诊断。

脚本支持 `-NoLaunch`，用于只安装、不启动的自动化或故障排查场景。

### `scripts/package_msix.ps1`

保留其“打包 + 可选签名”职责。开发安装脚本向它传入 PFX 路径和密码，
避免复制 MakeAppx 和 SignTool 查找逻辑。

签名后脚本必须检查 `$LASTEXITCODE`，并验证输出文件签名有效。

### `scripts/uninstall_dev_msix.ps1`

提供可重复的开发环境清理：

- 卸载 `MySnipaste.ScreenshotMvp`。
- 默认保留开发证书，避免每次重建信任链。
- 传入 `-RemoveCertificate` 时，从 `LocalMachine\TrustedPeople` 删除由该
  项目创建的匹配证书。
- 不删除其他 Subject 相同但指纹不同且不属于项目开发目录的证书。

## 证书与安全

- 开发证书使用 `New-SelfSignedCertificate` 创建，代码签名用途，
  SHA-256，有限有效期。
- PFX、CER 和密码文件位于构建目录：

  ```text
  build-nmake/dev-signing/
  ```

- 随机 PFX 密码保存在该目录的本地文件中，仅供脚本复用。
- 构建目录和证书产物不得提交到 Git。
- 证书只安装到 `LocalMachine\TrustedPeople`，不安装到
  `Trusted Root Certification Authorities`。
- 脚本不会关闭系统安全策略，也不会绕过签名验证。

## 提权行为

脚本先在普通权限下完成参数和文件检查。需要安装证书时，如果当前进程
不是管理员，则使用相同 PowerShell 可执行文件和原始参数重新启动自身，
并显示标准 UAC 提示。

用户拒绝 UAC 时，脚本失败退出，不生成“安装成功”的误导结果。

## 更新与重复执行

- 已存在有效且 Subject 匹配的开发证书时直接复用。
- 证书过期、缺少私钥或 Subject 不匹配时重新生成。
- 安装前卸载同 Identity Name 的旧包，避免版本号不变导致部署失败。
- 每次都重新打包和签名，确保安装内容与当前构建产物一致。

## OCR 验证

部署完成后，应用必须从已安装包入口启动，不能直接运行构建目录中的
`ScreenshotMvp.exe`。

脚本验证包已安装并输出启动入口。应用内 OCR 验收步骤：

1. 从安装后的 MySnipaste 启动截图。
2. 截取包含清晰中文或英文的区域。
3. 点击“识别文字”或按 `Ctrl+O`。
4. 应显示可编辑 OCR 文本窗口。
5. 修改文本并复制，验证剪贴板为 Unicode 文本。

OCR 语言仍由 `Windows.Media.Ocr` 的用户语言策略决定。若系统未安装
对应 OCR 语言能力，应用应报告 OCR 不受支持，而不是误报 package
identity 缺失。

## 错误处理

所有失败必须终止当前部署流程，并输出具体阶段：

- manifest 缺失或字段无法解析；
- EXE 未构建；
- Windows SDK 的 MakeAppx 或 SignTool 缺失；
- 证书生成、导出或信任失败；
- Publisher 与证书 Subject 不一致；
- MSIX 打包或签名失败；
- 签名状态不是 `Valid`；
- Appx 卸载、安装或启动入口解析失败。

脚本使用非零退出码，便于 CMake 和 CI 判断失败。

## 测试策略

自动化测试不修改机器证书库或 Appx 安装状态。纯逻辑检查放入
PowerShell 测试脚本，覆盖：

- manifest Identity Name、Publisher、Application Id 可读取；
- Publisher 与预期开发证书 Subject 一致；
- 开发产物路径位于构建目录；
- 安装脚本声明 `-NoLaunch` 和清理脚本声明 `-RemoveCertificate`；
- CMake 暴露 `install-dev-msix` 目标；
- `.gitignore` 排除 PFX、CER、MSIX 和 `dev-signing`。

手动集成验收覆盖：

- 首次执行出现 UAC 并完成证书信任；
- MSIX 签名状态为 `Valid`；
- `Get-AppxPackage MySnipaste.ScreenshotMvp` 能找到安装包；
- 应用从 AppsFolder 启动；
- OCR 可打开结果窗口并复制文字；
- 重复执行可更新安装；
- 清理脚本可卸载包，并可选择删除项目开发证书。

## 非目标

- 不申请或购买正式代码签名证书。
- 不提交证书、私钥或密码。
- 不实现 Microsoft Store 发布。
- 不在 CI 中安装本机证书或部署 Appx。
- 不为普通未打包 EXE 绕过 `Windows.Media.Ocr` 的 package identity
  要求。
