# Interactive GX uses RetroArch; the automated test harness loads the core directly.
. "$PSScriptRoot/common.ps1"
$cfg=Read-ToolConfig; $rom=Join-Path $ProjectRoot 'dist/MegaBenchOS.bin'; Get-RomInfo $rom | Out-Null
foreach ($key in @('retroarch','genesis_plus_gx_core')) {
    if (!$cfg.$key -or !(Test-Path -LiteralPath $cfg.$key)) { throw "Configure $key in local config (MB_RETROARCH / MB_GENPLUS_CORE). Automated GX tests do not require RetroArch." }
}
# Interactive frontend needs a visible window; automated tests load the core directly.
Start-Process -FilePath $cfg.retroarch -ArgumentList @('-L',('"'+$cfg.genesis_plus_gx_core+'"'),('"'+$rom+'"')) -WorkingDirectory (Split-Path $cfg.retroarch) -WindowStyle Normal
