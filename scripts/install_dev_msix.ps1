param(
    [string]$BuildDir = "build-nmake",
    [switch]$NoLaunch,
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

function Test-UsableSigningCertificate(
    [System.Security.Cryptography.X509Certificates.X509Certificate2]$certificate,
    [string]$publisher) {
    return $null -ne $certificate `
        -and $certificate.Subject -eq $publisher `
        -and $certificate.HasPrivateKey `
        -and $certificate.NotAfter -gt (Get-Date).AddDays(1)
}

function New-RandomPassword {
    $bytes = New-Object byte[] 32
    $generator = [System.Security.Cryptography.RandomNumberGenerator]::Create()
    try {
        $generator.GetBytes($bytes)
    } finally {
        $generator.Dispose()
    }
    return [Convert]::ToBase64String($bytes)
}

$root = Split-Path -Parent $PSScriptRoot
$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) {
    [System.IO.Path]::GetFullPath($BuildDir)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $BuildDir))
}
$manifestPath = Join-Path $root "packaging\Package.appxmanifest"
$packageScriptPath = Join-Path $root "scripts\package_msix.ps1"
$exePath = Join-Path $buildPath "ScreenshotMvp.exe"
$msixPath = Join-Path $buildPath "MySnipaste.msix"
$signingDirectory = Join-Path $buildPath "dev-signing"
$pfxPath = Join-Path $signingDirectory "MySnipaste-Dev.pfx"
$cerPath = Join-Path $signingDirectory "MySnipaste-Dev.cer"
$passwordPath = Join-Path $signingDirectory "password.txt"
$thumbprintPath = Join-Path $signingDirectory "thumbprint.txt"

foreach ($requiredFile in @($manifestPath, $packageScriptPath, $exePath)) {
    if (!(Test-Path -LiteralPath $requiredFile)) {
        throw "Required file not found: $requiredFile"
    }
}

[xml]$manifest = Get-Content -LiteralPath $manifestPath
$namespaceManager = New-Object System.Xml.XmlNamespaceManager($manifest.NameTable)
$namespaceManager.AddNamespace(
    "foundation",
    "http://schemas.microsoft.com/appx/manifest/foundation/windows10")
$identityNode = $manifest.SelectSingleNode(
    "/foundation:Package/foundation:Identity",
    $namespaceManager)
$applicationNode = $manifest.SelectSingleNode(
    "/foundation:Package/foundation:Applications/foundation:Application",
    $namespaceManager)

$packageName = [string]$identityNode.Name
$publisher = [string]$identityNode.Publisher
$applicationId = [string]$applicationNode.Id
if ([string]::IsNullOrWhiteSpace($packageName) `
    -or [string]::IsNullOrWhiteSpace($publisher) `
    -or [string]::IsNullOrWhiteSpace($applicationId)) {
    throw "Package identity, publisher, or application id is missing from $manifestPath."
}

New-Item -ItemType Directory -Path $signingDirectory -Force | Out-Null

$certificate = $null
if (Test-Path -LiteralPath $thumbprintPath) {
    $storedThumbprint = (Get-Content -Raw -LiteralPath $thumbprintPath).Trim()
    if (![string]::IsNullOrWhiteSpace($storedThumbprint)) {
        $certificate = Get-Item `
            -LiteralPath "Cert:\CurrentUser\My\$storedThumbprint" `
            -ErrorAction SilentlyContinue
    }
}

if (!(Test-UsableSigningCertificate $certificate $publisher) `
    -and (Test-Path -LiteralPath $pfxPath) `
    -and (Test-Path -LiteralPath $passwordPath)) {
    $certificate = $null
    if ((Test-Path -LiteralPath $pfxPath) -and (Test-Path -LiteralPath $passwordPath)) {
        $existingPassword = Get-Content -Raw -LiteralPath $passwordPath
        $existingSecurePassword = ConvertTo-SecureString `
            -String $existingPassword `
            -AsPlainText `
            -Force
        try {
            $imported = Import-PfxCertificate `
                -FilePath $pfxPath `
                -CertStoreLocation "Cert:\CurrentUser\My" `
                -Password $existingSecurePassword `
                -Exportable
            if (Test-UsableSigningCertificate $imported $publisher) {
                $certificate = $imported
            }
        } catch {
            Write-Warning "Existing development certificate could not be reused and will be replaced."
        }
    }
}

if (!(Test-UsableSigningCertificate $certificate $publisher)) {
    Write-Host "Creating a self-signed MySnipaste development certificate..."
    $password = New-RandomPassword
    $securePassword = ConvertTo-SecureString -String $password -AsPlainText -Force
    $certificate = New-SelfSignedCertificate `
        -Type Custom `
        -Subject $publisher `
        -KeyUsage DigitalSignature `
        -FriendlyName "MySnipaste Development Signing" `
        -CertStoreLocation "Cert:\CurrentUser\My" `
        -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3") `
        -HashAlgorithm SHA256 `
        -NotAfter (Get-Date).AddYears(2)

    Export-PfxCertificate `
        -Cert $certificate `
        -FilePath $pfxPath `
        -Password $securePassword `
        -Force | Out-Null
    Export-Certificate `
        -Cert $certificate `
        -FilePath $cerPath `
        -Type CERT `
        -Force | Out-Null
    Set-Content -LiteralPath $passwordPath -Value $password -NoNewline
    Set-Content -LiteralPath $thumbprintPath -Value $certificate.Thumbprint -NoNewline
} else {
    $password = Get-Content -Raw -LiteralPath $passwordPath
    if (!(Test-Path -LiteralPath $cerPath)) {
        Export-Certificate `
            -Cert $certificate `
            -FilePath $cerPath `
            -Type CERT `
            -Force | Out-Null
    }
}

if ($certificate.Subject -ne $publisher) {
    throw "Development certificate subject '$($certificate.Subject)' does not match publisher '$publisher'."
}

$trustedCertificate = Get-Item `
    -LiteralPath "Cert:\LocalMachine\TrustedPeople\$($certificate.Thumbprint)" `
    -ErrorAction SilentlyContinue
if (!$trustedCertificate -and !(Test-IsAdministrator)) {
    if ($Elevated) {
        throw "Administrative privileges are required to trust the development certificate."
    }

    $argumentList = @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Quote-ProcessArgument $PSCommandPath),
        "-BuildDir",
        (Quote-ProcessArgument $buildPath),
        "-Elevated"
    )
    if ($NoLaunch) {
        $argumentList += "-NoLaunch"
    }

    Write-Host "Requesting administrator permission to trust the development certificate..."
    $process = Start-Process `
        -FilePath "powershell.exe" `
        -ArgumentList $argumentList `
        -Verb RunAs `
        -Wait `
        -PassThru
    if ($process.ExitCode -ne 0) {
        throw "Elevated MSIX installation failed with exit code $($process.ExitCode)."
    }
    exit 0
}

if (!$trustedCertificate) {
    Write-Host "Trusting the development certificate in LocalMachine\TrustedPeople..."
    Import-Certificate `
        -FilePath $cerPath `
        -CertStoreLocation "Cert:\LocalMachine\TrustedPeople" | Out-Null
}

& $packageScriptPath `
    -BuildDir $buildPath `
    -OutputPath $msixPath `
    -CertificatePath $pfxPath `
    -CertificatePassword $password
if ($LASTEXITCODE -ne 0) {
    throw "MSIX packaging failed with exit code $LASTEXITCODE."
}

$signature = Get-AuthenticodeSignature -LiteralPath $msixPath
if ($signature.Status -ne [System.Management.Automation.SignatureStatus]::Valid) {
    throw "MSIX signature validation failed: $($signature.Status) $($signature.StatusMessage)"
}

Get-AppxPackage -Name $packageName -ErrorAction SilentlyContinue |
    ForEach-Object {
        Write-Host "Removing existing package: $($_.PackageFullName)"
        Remove-AppxPackage -Package $_.PackageFullName -ErrorAction Stop
    }

Write-Host "Installing development MSIX..."
Add-AppxPackage -Path $msixPath -ForceApplicationShutdown -ErrorAction Stop

$installedPackage = Get-AppxPackage -Name $packageName |
    Sort-Object Version -Descending |
    Select-Object -First 1
if (!$installedPackage) {
    throw "Package installation completed but $packageName could not be found."
}

$appUserModelId = "$($installedPackage.PackageFamilyName)!$applicationId"
Write-Host ""
Write-Host "Package name:       $packageName"
Write-Host "Package full name:  $($installedPackage.PackageFullName)"
Write-Host "Install location:   $($installedPackage.InstallLocation)"
Write-Host "Signature status:   $($signature.Status)"
Write-Host "Application AUMID:  $appUserModelId"

if (!$NoLaunch) {
    Write-Host "Launching the packaged MySnipaste application..."
    Start-Process `
        -FilePath "explorer.exe" `
        -ArgumentList "shell:AppsFolder\$appUserModelId"
}
