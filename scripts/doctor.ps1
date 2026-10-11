# Inspect verified tools without changing PATH or installing anything.
. "$PSScriptRoot/common.ps1"
$cfg=Require-Sgdk
& (Join-Path $cfg.sgdk 'bin/gcc.exe') --version
& (Join-Path $cfg.sgdk 'bin/make.exe') --version
Write-Host "Windows PowerShell target: 5.1; running $($PSVersionTable.PSVersion) ($($PSVersionTable.PSEdition))"
& java --version
if ($LASTEXITCODE) { throw 'Java version inspection failed.' }
$hostCompiler = $env:MB_HOST_CC
if (!$hostCompiler) { $hostCompiler = Join-Path $env:ProgramFiles 'LLVM/bin/clang.exe' }
if (Test-Path -LiteralPath $hostCompiler) {
    Write-Host "Native C compiler: $hostCompiler"
} else {
    Write-Host 'Native C compiler: required LLVM compiler not configured; see docs/INSTALL.md.'
}
foreach ($key in @('blastem','retroarch','genesis_plus_gx_core')) {
    $status=if ($cfg.$key -and (Test-Path -LiteralPath $cfg.$key)) { $cfg.$key } else { 'Unavailable (optional); configure local path or environment override.' }
    Write-Host "${key}: $status"
}
