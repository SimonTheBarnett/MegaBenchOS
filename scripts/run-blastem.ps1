param([ValidateSet('ntsc','pal')][string]$Region='ntsc')
# Launch installed BlastEm. Process launch alone is not a successful visual test.
. "$PSScriptRoot/common.ps1"
$cfg=Read-ToolConfig; $rom=Join-Path $ProjectRoot 'dist/MegaBenchOS.bin'; Get-RomInfo $rom | Out-Null
if (!$cfg.blastem -or !(Test-Path -LiteralPath $cfg.blastem)) { throw 'Set MB_BLASTEM or blastem in local config.' }
$flag=if ($Region -eq 'pal') { 'E' } else { 'U' }
# This is an interactive emulator, so expose its window for the user to inspect/control.
Start-Process -FilePath $cfg.blastem -ArgumentList @(('"'+$rom+'"'),'-r',$flag) -WorkingDirectory (Split-Path $cfg.blastem) -WindowStyle Normal
Write-Host 'Launched BlastEm; visual and controller verification requires observation.'
