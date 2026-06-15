param(
    [string]$BuildDir = "build-nmake",
    [switch]$RemoveCertificate,
    [switch]$Elevated
)

$ErrorActionPreference = "Stop"

function Test-IsAdministrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Quote-ProcessArgument([string]$value) {
    return '"' + $value.Replace('"', '\"') + '"'
}

$root = Split-Path -Parent $PSScriptRoot
$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) {
    [System.IO.Path]::GetFullPath($BuildDir)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $BuildDir))
}
$manifestPath = Join-Path $root "packaging\Package.appxmanifest"
$thumbprintPath = Join-Path $buildPath "dev-signing\thumbprint.txt"

if (!(Test-Path -LiteralPath $manifestPath)) {
    throw "Package manifest not found: $manifestPath"
}

[xml]$manifest = Get-Content -LiteralPath $manifestPath
$namespaceManager = New-Object System.Xml.XmlNamespaceManager($manifest.NameTable)
$namespaceManager.AddNamespace(
    "foundation",
    "http://schemas.microsoft.com/appx/manifest/foundation/windows10")
$identityNode = $manifest.SelectSingleNode(
    "/foundation:Package/foundation:Identity",
    $namespaceManager)
$packageName = [string]$identityNode.Name
if ([string]::IsNullOrWhiteSpace($packageName)) {
    throw "Package identity could not be read from $manifestPath."
}

Get-AppxPackage -Name $packageName -ErrorAction SilentlyContinue |
    ForEach-Object {
        Write-Host "Removing package: $($_.PackageFullName)"
        Remove-AppxPackage -Package $_.PackageFullName -ErrorAction Stop
    }

if (!$RemoveCertificate) {
    Write-Host "Development package removed. The signing certificate was preserved."
    exit 0
}

if (!(Test-IsAdministrator)) {
    if ($Elevated) {
        throw "Administrative privileges are required to remove the trusted certificate."
    }

    $argumentList = @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Quote-ProcessArgument $PSCommandPath),
        "-BuildDir",
        (Quote-ProcessArgument $buildPath),
        "-RemoveCertificate",
        "-Elevated"
    )
    Write-Host "Requesting administrator permission to remove the development certificate..."
    $process = Start-Process `
        -FilePath "powershell.exe" `
        -ArgumentList $argumentList `
        -Verb RunAs `
        -Wait `
        -PassThru
    if ($process.ExitCode -ne 0) {
        throw "Elevated certificate cleanup failed with exit code $($process.ExitCode)."
    }
    exit 0
}

if (!(Test-Path -LiteralPath $thumbprintPath)) {
    throw "Certificate thumbprint file not found: $thumbprintPath"
}

$thumbprint = (Get-Content -Raw -LiteralPath $thumbprintPath).Trim()
if ($thumbprint -notmatch "^[0-9A-Fa-f]{40}$") {
    throw "Invalid certificate thumbprint in $thumbprintPath."
}

foreach ($storePath in @(
    "Cert:\LocalMachine\TrustedPeople\$thumbprint",
    "Cert:\CurrentUser\My\$thumbprint"
)) {
    $certificate = Get-Item -LiteralPath $storePath -ErrorAction SilentlyContinue
    if ($certificate) {
        Write-Host "Removing development certificate: $storePath"
        Remove-Item -LiteralPath $storePath -Force
    }
}

Write-Host "Development package and project certificate were removed."
