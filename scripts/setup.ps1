#Requires -Version 5.1
param([switch]$InstallTools, [switch]$InstallEmulatorCore,
      [string]$ToolRoot = (Join-Path $env:LOCALAPPDATA 'MegaBenchOS/tools'))
# Explicit, hash-checked portable installation; never creates a language environment.
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$projectRoot = Split-Path $PSScriptRoot
$toolRoot = [IO.Path]::GetFullPath($ToolRoot)
if ($InstallEmulatorCore -and !$InstallTools) {
    throw 'Use -InstallTools -InstallEmulatorCore to download the optional GX core.'
}
if ($InstallTools) {
    if ($toolRoot -match '\s') {
        throw 'SGDK requires a tool path without spaces. Retry with -ToolRoot C:\MegaBenchTools (or another writable path without spaces).'
    }
    if (!(Get-Command java -ErrorAction SilentlyContinue)) {
        throw 'Install Java 25 and reopen Windows PowerShell. See docs/INSTALL.md.'
    }
    $sevenZip = Join-Path $env:ProgramFiles '7-Zip/7z.exe'
    if (!(Test-Path -LiteralPath $sevenZip)) {
        $sevenZipCommand = Get-Command 7z -ErrorAction SilentlyContinue
        $sevenZip = if ($sevenZipCommand) { $sevenZipCommand.Source } else { '' }
    }
    if (!$sevenZip) { throw 'Install 7-Zip, then retry. See docs/INSTALL.md.' }
    New-Item -ItemType Directory -Force $toolRoot | Out-Null
    $lock = Get-Content -Raw (Join-Path $projectRoot 'config/toolchain.lock.json') | ConvertFrom-Json
    foreach ($item in $lock) {
        if ($item.name -eq 'genesis_plus_gx_core' -and !$InstallEmulatorCore) { continue }
        $archive = Join-Path $toolRoot $item.archive
        $destination = Join-Path $toolRoot $item.destination
        if (!(Test-Path -LiteralPath $archive)) {
            Write-Host "Downloading $($item.name) $($item.version)..."
            # Windows PowerShell 5.1 uses .NET Framework's TLS settings.
            # Enable TLS 1.2 only for this download and restore the process setting.
            $previousTls = [Net.ServicePointManager]::SecurityProtocol
            try {
                [Net.ServicePointManager]::SecurityProtocol = $previousTls -bor [Net.SecurityProtocolType]::Tls12
                Invoke-WebRequest $item.url -OutFile $archive -UseBasicParsing
            } finally {
                [Net.ServicePointManager]::SecurityProtocol = $previousTls
            }
        }
        if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $item.sha256) {
            throw "Checksum mismatch for $($item.name): $archive. The archive is damaged or upstream changed; review the download and lock before retrying."
        }
        $installedEntry = Join-Path $destination $item.installed_entry
        if (!(Test-Path -LiteralPath $installedEntry)) {
            if ($item.kind -eq 'zip') { Expand-Archive -LiteralPath $archive -DestinationPath $destination -Force }
            else {
                & $sevenZip x $archive "-o$destination" -y
                if ($LASTEXITCODE) { throw 'SGDK extraction failed.' }
            }
        }
        if (!(Test-Path -LiteralPath $installedEntry -PathType Leaf)) {
            throw "Installation incomplete: $installedEntry"
        }
        if ($item.runtime_sha256 -and
            (Get-FileHash -LiteralPath $installedEntry).Hash.ToLowerInvariant() -ne $item.runtime_sha256) {
            throw "Installed binary checksum mismatch for $($item.name)."
        }
    }
}
$configFile = Join-Path $projectRoot 'config/toolchain.local.json'
if (!(Test-Path -LiteralPath $configFile)) {
    $cfg = [ordered]@{ sgdk=(Join-Path $toolRoot 'sgdk-2.11'); blastem=(Join-Path $toolRoot 'blastem-1.0.0/blastem-win64-1.0.0/blastem.exe'); retroarch=''; genesis_plus_gx_core=(Join-Path $toolRoot 'genesis-plus-gx/genesis_plus_gx_libretro.dll') }
    $cfg | ConvertTo-Json | Set-Content -Encoding utf8 $configFile
}
& "$PSScriptRoot/doctor.ps1"
Write-Host 'Tool paths configured. Run . ./scripts/env.ps1, then make build.'
Write-Output 'Ready: dot-source .\scripts\env.ps1, then make build and make run (or use VS Code Build/Test/Run).'
Write-Output 'First-time setup and optional test tools: docs/INSTALL.md.'
