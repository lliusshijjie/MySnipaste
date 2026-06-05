param(
    [string]$BuildDir = "build-nmake",
    [string]$OutputPath = "",
    [string]$CertificatePath = "",
    [string]$CertificatePassword = ""
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) { $BuildDir } else { Join-Path $root $BuildDir }
$exePath = Join-Path $buildPath "ScreenshotMvp.exe"
if (!(Test-Path $exePath)) {
    throw "ScreenshotMvp.exe not found at $exePath. Build the app before packaging."
}

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $buildPath "MySnipaste.msix"
}

$layout = Join-Path $buildPath "msix-layout"
if (Test-Path $layout) {
    Remove-Item -LiteralPath $layout -Recurse -Force
}
New-Item -ItemType Directory -Path $layout | Out-Null
New-Item -ItemType Directory -Path (Join-Path $layout "Assets") | Out-Null

Copy-Item -LiteralPath $exePath -Destination (Join-Path $layout "ScreenshotMvp.exe")
Copy-Item -LiteralPath (Join-Path $root "packaging\Package.appxmanifest") -Destination (Join-Path $layout "AppxManifest.xml")
Copy-Item -LiteralPath (Join-Path $root "images\icon.png") -Destination (Join-Path $layout "Assets\icon.png")

$kitsRoot = "${env:ProgramFiles(x86)}\Windows Kits\10\bin"
$makeAppx = Get-ChildItem -Path $kitsRoot -Filter makeappx.exe -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -like "*\x64\makeappx.exe" } |
    Sort-Object FullName -Descending |
    Select-Object -First 1
if (!$makeAppx) {
    throw "makeappx.exe was not found under $kitsRoot."
}

& $makeAppx.FullName pack /d $layout /p $OutputPath /overwrite
if ($LASTEXITCODE -ne 0) {
    throw "makeappx failed with exit code $LASTEXITCODE."
}

if (![string]::IsNullOrWhiteSpace($CertificatePath)) {
    $signTool = Get-ChildItem -Path $kitsRoot -Filter signtool.exe -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -like "*\x64\signtool.exe" } |
        Sort-Object FullName -Descending |
        Select-Object -First 1
    if (!$signTool) {
        throw "signtool.exe was not found under $kitsRoot."
    }

    $signArgs = @("sign", "/fd", "SHA256", "/f", $CertificatePath)
    if (![string]::IsNullOrWhiteSpace($CertificatePassword)) {
        $signArgs += @("/p", $CertificatePassword)
    }
    $signArgs += $OutputPath
    & $signTool.FullName @signArgs
    if ($LASTEXITCODE -ne 0) {
        throw "signtool failed with exit code $LASTEXITCODE."
    }
}

Write-Host "MSIX package created: $OutputPath"
