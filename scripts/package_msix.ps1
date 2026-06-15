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
$manifestPath = Join-Path $root "packaging\Package.appxmanifest"
if (!(Test-Path $exePath)) {
    throw "ScreenshotMvp.exe not found at $exePath. Build the app before packaging."
}
if (!(Test-Path -LiteralPath $manifestPath)) {
    throw "Package manifest not found at $manifestPath."
}
if (![string]::IsNullOrWhiteSpace($CertificatePath) -and !(Test-Path -LiteralPath $CertificatePath)) {
    throw "Signing certificate not found: $CertificatePath"
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
Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $layout "AppxManifest.xml")
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
    [xml]$manifest = Get-Content -LiteralPath $manifestPath
    $namespaceManager = New-Object System.Xml.XmlNamespaceManager($manifest.NameTable)
    $namespaceManager.AddNamespace(
        "foundation",
        "http://schemas.microsoft.com/appx/manifest/foundation/windows10")
    $identity = $manifest.SelectSingleNode(
        "/foundation:Package/foundation:Identity",
        $namespaceManager)
    if (!$identity -or [string]::IsNullOrWhiteSpace($identity.Publisher)) {
        throw "Package publisher could not be read from $manifestPath."
    }

    $securePassword = if ([string]::IsNullOrWhiteSpace($CertificatePassword)) {
        New-Object System.Security.SecureString
    } else {
        ConvertTo-SecureString -String $CertificatePassword -AsPlainText -Force
    }
    $certificate = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2
    $certificate.Import(
        $CertificatePath,
        $securePassword,
        [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::DefaultKeySet)
    if ($certificate.Subject -ne $identity.Publisher) {
        throw "Certificate subject '$($certificate.Subject)' does not match manifest publisher '$($identity.Publisher)'."
    }

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

    $signature = Get-AuthenticodeSignature -LiteralPath $OutputPath
    if ($signature.Status -ne [System.Management.Automation.SignatureStatus]::Valid) {
        throw "MSIX signature validation failed: $($signature.Status) $($signature.StatusMessage)"
    }
    Write-Host "MSIX signature: $($signature.Status)"
}

Write-Host "MSIX package created: $OutputPath"
