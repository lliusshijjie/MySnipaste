$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$manifestPath = Join-Path $root "packaging\Package.appxmanifest"
[xml]$manifest = Get-Content -LiteralPath $manifestPath

$namespaceManager = New-Object System.Xml.XmlNamespaceManager($manifest.NameTable)
$namespaceManager.AddNamespace(
    "foundation",
    "http://schemas.microsoft.com/appx/manifest/foundation/windows10")

$identity = $manifest.SelectSingleNode(
    "/foundation:Package/foundation:Identity",
    $namespaceManager)
$application = $manifest.SelectSingleNode(
    "/foundation:Package/foundation:Applications/foundation:Application",
    $namespaceManager)

if (!$identity -or $identity.Name -ne "MySnipaste.ScreenshotMvp") {
    throw "Unexpected package identity."
}
if ($identity.Publisher -ne "CN=MySnipaste") {
    throw "Unexpected package publisher."
}
if (!$application -or $application.Id -ne "ScreenshotMvp") {
    throw "Unexpected application id."
}

$installScriptPath = Join-Path $root "scripts\install_dev_msix.ps1"
$uninstallScriptPath = Join-Path $root "scripts\uninstall_dev_msix.ps1"
if (!(Test-Path -LiteralPath $installScriptPath)) {
    throw "Development install script is missing."
}
if (!(Test-Path -LiteralPath $uninstallScriptPath)) {
    throw "Development uninstall script is missing."
}

$installScript = Get-Content -Raw -LiteralPath $installScriptPath
$uninstallScript = Get-Content -Raw -LiteralPath $uninstallScriptPath
$cmake = Get-Content -Raw -LiteralPath (Join-Path $root "CMakeLists.txt")
$gitignore = Get-Content -Raw -LiteralPath (Join-Path $root ".gitignore")

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

if ($uninstallScript -notmatch "RemoveCertificate") {
    throw "Cleanup switch is missing."
}
if ($cmake -notmatch "install-dev-msix") {
    throw "CMake install target is missing."
}
foreach ($pattern in @("*.pfx", "*.cer", "*.msix", "dev-signing/")) {
    if ($gitignore -notmatch [regex]::Escape($pattern)) {
        throw ".gitignore is missing: $pattern"
    }
}

Write-Host "MSIX development workflow contract passed."
