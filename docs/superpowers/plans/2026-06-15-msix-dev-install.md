# MSIX Development Install Workflow Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a one-command local development workflow that creates and trusts a self-signed certificate, signs and installs the MySnipaste MSIX package, launches the packaged app, and exposes diagnostics needed to verify OCR package identity.

**Architecture:** Keep `package_msix.ps1` focused on packaging and optional signing. Add one orchestration script for elevated certificate/install operations, one cleanup script, and a non-destructive PowerShell contract test that validates the manifest and workflow wiring without touching the certificate store or Appx deployment state.

**Tech Stack:** PowerShell 5.1+, CMake custom targets, Windows SDK MakeAppx/SignTool, Appx PowerShell cmdlets, Windows certificate stores.

---

### Task 1: Add the non-destructive workflow contract test

**Files:**
- Create: `tests/test_msix_dev_workflow.ps1`

- [ ] **Step 1: Write the failing test**

Create a PowerShell test that:

```powershell
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

[xml]$manifest = Get-Content -LiteralPath (Join-Path $root "packaging\Package.appxmanifest")
$ns = New-Object System.Xml.XmlNamespaceManager($manifest.NameTable)
$ns.AddNamespace("f", "http://schemas.microsoft.com/appx/manifest/foundation/windows10")

$identity = $manifest.SelectSingleNode("/f:Package/f:Identity", $ns)
$application = $manifest.SelectSingleNode("/f:Package/f:Applications/f:Application", $ns)

if ($identity.Name -ne "MySnipaste.ScreenshotMvp") { throw "Unexpected package identity." }
if ($identity.Publisher -ne "CN=MySnipaste") { throw "Unexpected publisher." }
if ($application.Id -ne "ScreenshotMvp") { throw "Unexpected application id." }

$installScript = Get-Content -Raw (Join-Path $root "scripts\install_dev_msix.ps1")
$uninstallScript = Get-Content -Raw (Join-Path $root "scripts\uninstall_dev_msix.ps1")
$cmake = Get-Content -Raw (Join-Path $root "CMakeLists.txt")
$gitignore = Get-Content -Raw (Join-Path $root ".gitignore")

foreach ($required in @(
    "NoLaunch",
    "New-SelfSignedCertificate",
    "LocalMachine\TrustedPeople",
    "Get-AuthenticodeSignature",
    "Add-AppxPackage",
    "shell:AppsFolder"
)) {
    if ($installScript -notmatch [regex]::Escape($required)) {
        throw "Install workflow is missing: $required"
    }
}

if ($uninstallScript -notmatch "RemoveCertificate") { throw "Cleanup switch missing." }
if ($cmake -notmatch "install-dev-msix") { throw "CMake install target missing." }
foreach ($pattern in @("*.pfx", "*.cer", "*.msix", "dev-signing/")) {
    if ($gitignore -notmatch [regex]::Escape($pattern)) {
        throw ".gitignore is missing: $pattern"
    }
}

Write-Host "MSIX development workflow contract passed."
```

- [ ] **Step 2: Run the test to verify it fails**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\test_msix_dev_workflow.ps1
```

Expected: FAIL because `scripts/install_dev_msix.ps1` and `scripts/uninstall_dev_msix.ps1` do not exist.

- [ ] **Step 3: Commit the red test**

```bat
git add tests/test_msix_dev_workflow.ps1
git commit -m "test: define MSIX development workflow"
```

### Task 2: Harden package signing

**Files:**
- Modify: `scripts/package_msix.ps1`

- [ ] **Step 1: Add strict input validation**

Validate that:

```powershell
if (![string]::IsNullOrWhiteSpace($CertificatePath) -and !(Test-Path -LiteralPath $CertificatePath)) {
    throw "Signing certificate not found: $CertificatePath"
}
```

Read the manifest Publisher and compare it with the PFX certificate Subject before signing.

- [ ] **Step 2: Verify the produced signature**

After SignTool succeeds:

```powershell
$signature = Get-AuthenticodeSignature -LiteralPath $OutputPath
if ($signature.Status -ne [System.Management.Automation.SignatureStatus]::Valid) {
    throw "MSIX signature validation failed: $($signature.Status) $($signature.StatusMessage)"
}
Write-Host "MSIX signature: $($signature.Status)"
```

- [ ] **Step 3: Run the existing C++ tests**

Run:

```bat
cmake --build build-nmake
ctest --test-dir build-nmake --output-on-failure
```

Expected: build succeeds and all tests pass.

- [ ] **Step 4: Commit**

```bat
git add scripts/package_msix.ps1
git commit -m "build: validate MSIX signing inputs"
```

### Task 3: Implement certificate generation, trust, installation, and launch

**Files:**
- Create: `scripts/install_dev_msix.ps1`

- [ ] **Step 1: Implement manifest and path validation**

The script parameters are:

```powershell
param(
    [string]$BuildDir = "build-nmake",
    [switch]$NoLaunch,
    [switch]$Elevated
)
```

Resolve the repository root, build directory, EXE, manifest, package script,
MSIX path, and `dev-signing` directory. Parse Identity Name, Publisher and
Application Id from the manifest and fail if any are empty.

- [ ] **Step 2: Implement controlled elevation**

Before modifying `LocalMachine\TrustedPeople`, detect administrator membership:

```powershell
function Test-IsAdministrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}
```

If not elevated, re-run the same script with `Start-Process -Verb RunAs -Wait`,
preserving `BuildDir` and `NoLaunch`, and propagate the elevated process exit
code.

- [ ] **Step 3: Create or reuse the development certificate**

Use files:

```text
<build-dir>\dev-signing\MySnipaste-Dev.pfx
<build-dir>\dev-signing\MySnipaste-Dev.cer
<build-dir>\dev-signing\password.txt
<build-dir>\dev-signing\thumbprint.txt
```

Reuse a certificate only when its Subject equals the manifest Publisher, it has
a private key, and it is not expired. Otherwise generate:

```powershell
New-SelfSignedCertificate `
  -Type Custom `
  -Subject $publisher `
  -KeyUsage DigitalSignature `
  -FriendlyName "MySnipaste Development Signing" `
  -CertStoreLocation "Cert:\CurrentUser\My" `
  -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3") `
  -HashAlgorithm SHA256 `
  -NotAfter (Get-Date).AddYears(2)
```

Generate a random password with cryptographic random bytes, export PFX and CER,
and store the thumbprint.

- [ ] **Step 4: Trust, package, sign, and install**

Import the CER into:

```text
Cert:\LocalMachine\TrustedPeople
```

Invoke `package_msix.ps1` with the PFX and password. Verify the signature is
`Valid`, uninstall every package returned by:

```powershell
Get-AppxPackage -Name $packageName
```

then install using:

```powershell
Add-AppxPackage -Path $msixPath -ForceApplicationShutdown
```

- [ ] **Step 5: Resolve the AppsFolder launch identity**

After installation, obtain `PackageFamilyName` and build:

```powershell
$appUserModelId = "$($installed.PackageFamilyName)!$applicationId"
```

Unless `-NoLaunch` was supplied, launch:

```powershell
Start-Process "explorer.exe" "shell:AppsFolder\$appUserModelId"
```

Print package name, full name, install location, signature status and AUMID.

- [ ] **Step 6: Run the workflow contract test**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\test_msix_dev_workflow.ps1
```

Expected: still FAIL only because cleanup/CMake/gitignore wiring is incomplete.

- [ ] **Step 7: Commit**

```bat
git add scripts/install_dev_msix.ps1
git commit -m "build: add MSIX development installer"
```

### Task 4: Add safe uninstall and certificate cleanup

**Files:**
- Create: `scripts/uninstall_dev_msix.ps1`

- [ ] **Step 1: Implement uninstall behavior**

Parameters:

```powershell
param(
    [string]$BuildDir = "build-nmake",
    [switch]$RemoveCertificate,
    [switch]$Elevated
)
```

Parse the package identity from the manifest and remove all current-user
packages matching that identity.

- [ ] **Step 2: Implement opt-in certificate cleanup**

When `-RemoveCertificate` is set, elevate if required, read the thumbprint from
`<build-dir>\dev-signing\thumbprint.txt`, and remove only that exact thumbprint
from:

```text
Cert:\LocalMachine\TrustedPeople
Cert:\CurrentUser\My
```

Do not delete certificates by Subject alone.

- [ ] **Step 3: Run the workflow contract test**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\test_msix_dev_workflow.ps1
```

Expected: FAIL because CMake and `.gitignore` are not wired yet.

- [ ] **Step 4: Commit**

```bat
git add scripts/uninstall_dev_msix.ps1
git commit -m "build: add MSIX development cleanup"
```

### Task 5: Wire CMake, ignores, and documentation

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `.gitignore`
- Modify: `README.md`

- [ ] **Step 1: Add CMake targets**

Add:

```cmake
add_custom_target(install-dev-msix
    COMMAND powershell -NoProfile -ExecutionPolicy Bypass
        -File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/install_dev_msix.ps1"
        -BuildDir "${CMAKE_BINARY_DIR}"
    DEPENDS ScreenshotMvp
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Signing, installing, and launching MySnipaste development MSIX"
)

add_custom_target(uninstall-dev-msix
    COMMAND powershell -NoProfile -ExecutionPolicy Bypass
        -File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/uninstall_dev_msix.ps1"
        -BuildDir "${CMAKE_BINARY_DIR}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Uninstalling MySnipaste development MSIX"
)
```

- [ ] **Step 2: Ignore local signing artifacts**

Append:

```gitignore
# MSIX and local development signing artifacts
*.msix
*.pfx
*.cer
dev-signing/
msix-layout/
```

- [ ] **Step 3: Document exact commands**

Update README with:

```bat
cmake --build build-nmake --target install-dev-msix
cmake --build build-nmake --target uninstall-dev-msix
```

Document UAC, self-signed certificate scope, `-NoLaunch`, complete cleanup with
`-RemoveCertificate`, and that OCR must be tested from the installed app rather
than the build-directory EXE.

- [ ] **Step 4: Run the workflow contract test and verify green**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\test_msix_dev_workflow.ps1
```

Expected:

```text
MSIX development workflow contract passed.
```

- [ ] **Step 5: Commit**

```bat
git add CMakeLists.txt .gitignore README.md tests/test_msix_dev_workflow.ps1
git commit -m "build: wire MSIX development deployment"
```

### Task 6: Perform fresh build and local deployment verification

**Files:**
- No source changes unless verification exposes a defect.

- [ ] **Step 1: Run full build and tests**

```bat
cmake --build build-nmake --clean-first
ctest --test-dir build-nmake --output-on-failure
powershell -NoProfile -ExecutionPolicy Bypass -File tests\test_msix_dev_workflow.ps1
```

Expected: all commands exit zero.

- [ ] **Step 2: Execute the real development install**

```bat
cmake --build build-nmake --target install-dev-msix
```

Accept the standard UAC prompt. Expected:

- development certificate is generated or reused;
- certificate is present in `LocalMachine\TrustedPeople`;
- MSIX signature is `Valid`;
- package is installed;
- application launches from AppsFolder.

- [ ] **Step 3: Verify installed package and process identity**

Run:

```powershell
$package = Get-AppxPackage -Name MySnipaste.ScreenshotMvp
$package | Format-List Name,PackageFullName,PackageFamilyName,InstallLocation,Status
Get-AuthenticodeSignature build-nmake\MySnipaste.msix |
    Format-List Status,StatusMessage,SignerCertificate
```

Confirm the running `ScreenshotMvp.exe` path is under the package install
location, not `build-nmake`.

- [ ] **Step 4: Manual OCR acceptance**

Using the packaged process:

1. Capture a region containing clear Chinese text.
2. Click “识别文字”.
3. Confirm the editable OCR dialog opens.
4. Edit and copy the text.
5. Paste into Notepad and confirm Unicode text is preserved.

- [ ] **Step 5: Inspect repository state**

```bat
git status --short
git diff --check
```

Expected: no generated certificate, password, MSIX, or layout files are tracked.
