#Requires -Version 5.1
# Shared Windows PowerShell 5.1 paths and native-tool invocation. No global environment changes.
$ErrorActionPreference = 'Stop'
$script:ProjectRoot = Split-Path $PSScriptRoot -Parent
function Read-ToolConfig {
    $file = Join-Path $ProjectRoot 'config/toolchain.local.json'
    $cfg = if (Test-Path -LiteralPath $file) { Get-Content -Raw $file | ConvertFrom-Json } else { Get-Content -Raw (Join-Path $ProjectRoot 'config/toolchain.example.json') | ConvertFrom-Json }
    foreach ($pair in @(@('sgdk','MB_SGDK'),@('blastem','MB_BLASTEM'),@('retroarch','MB_RETROARCH'),@('genesis_plus_gx_core','MB_GENPLUS_CORE'))) {
        $value = [Environment]::GetEnvironmentVariable($pair[1])
        if ($value) { $cfg.($pair[0]) = $value }
    }
    return $cfg
}
function Require-Sgdk {
    $cfg = Read-ToolConfig
    if (!$cfg.sgdk) { throw 'Run scripts/setup.ps1 -InstallTools or configure MB_SGDK. See docs/INSTALL.md.' }
    if ($cfg.sgdk -match '\s') { throw 'SGDK must be installed at a path without spaces. See docs/INSTALL.md.' }
    foreach ($entry in @('bin/gcc.exe','bin/make.exe','bin/size.exe','lib/libmd.a','makefile.gen','readme.md')) {
        if (!(Test-Path -LiteralPath (Join-Path $cfg.sgdk $entry))) { throw "Missing SGDK tool: $entry in $($cfg.sgdk)" }
    }
    if (!((Get-Content -Raw (Join-Path $cfg.sgdk 'readme.md')).StartsWith('# SGDK 2.11'))) { throw 'Verified workflow requires SGDK 2.11.' }
    if (!(Get-Command java -ErrorAction SilentlyContinue)) { throw 'Java must be on PATH for SGDK. Install Java 25 and reopen Windows PowerShell; see docs/INSTALL.md.' }
    return $cfg
}
function Write-Utf8([string]$Path, [string]$Content) {
    [IO.Directory]::CreateDirectory((Split-Path $Path)) | Out-Null
    [IO.File]::WriteAllText($Path, $Content.Replace("`r`n","`n"), [Text.UTF8Encoding]::new($false))
}
function Import-HostTools {
    $exe=Join-Path $ProjectRoot 'out/host/megabench-tools.exe'
    if (!(Test-Path -LiteralPath $exe)) {
        $cfg=Require-Sgdk
        if (!$env:MB_HOST_CC) { $env:MB_HOST_CC=Join-Path $env:ProgramFiles 'LLVM/bin/clang.exe' }
        & (Join-Path $cfg.sgdk 'bin/make.exe') -C $ProjectRoot host-tools
        if ($LASTEXITCODE) { throw 'Native host tooling build failed.' }
    }
    return $exe
}
function Invoke-HostTools {
    param([string[]]$Arguments)
    $exe=Import-HostTools
    $result=& $exe @Arguments --root $ProjectRoot
    if ($LASTEXITCODE) { throw ('Native host tool failed: '+($Arguments -join ' ')) }
    return $result
}
function Get-SourceHash([switch]$Contracts,[switch]$LittleFS,[switch]$MusicSmoke) {
    return Invoke-HostTools -Arguments @('source-hash','--contracts',[int][bool]$Contracts,'--littlefs',[int][bool]$LittleFS,'--music-smoke',[int][bool]$MusicSmoke)
}
function Get-RomInfo([string]$Path) {
    $json=Invoke-HostTools -Arguments @('rom-info','--input',$Path) | ConvertFrom-Json
    $result=[ordered]@{}
    foreach ($entry in $json.PSObject.Properties) { $result[$entry.Name]=$entry.Value }
    return $result
}