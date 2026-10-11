#Requires -Version 5.1
# Dot-source once per terminal; configure only this PowerShell session.
. "$PSScriptRoot/common.ps1"
$cfg = Require-Sgdk
$env:MB_SGDK = $cfg.sgdk.Replace('\','/')
if (!$env:MB_HOST_CC) {
    $clang=Get-Command clang -ErrorAction SilentlyContinue
    $env:MB_HOST_CC=if ($clang) {$clang.Source} else {Join-Path $env:ProgramFiles 'LLVM/bin/clang.exe'}
}
# Keep SGDK's Unix helper executables out of PATH: they affect GCC's LTO cleanup.
Set-Alias -Name make -Value (Join-Path $cfg.sgdk 'bin/make.exe') -Scope Global
