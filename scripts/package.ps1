param([switch]$Release,[ValidateSet('about','system','calculator','clock','sound','snake','megabounce','colourstorm','ocean','stardrive','megapad','megapaint','megaplayer')][string]$App='calculator')
# Verified ROM release or source SDK package for ROM relinking.
. "$PSScriptRoot/common.ps1"
Invoke-HostTools -Arguments @('check-generated') | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression
$dest=Join-Path $ProjectRoot $(if($Release){'dist/MegaBenchOS-v1.0-release.zip'}else{"dist/$App-rom-linked.zip"});[IO.Directory]::CreateDirectory((Split-Path $dest)) | Out-Null
if($Release){
    $info=Get-RomInfo (Join-Path $ProjectRoot 'dist/MegaBenchOS.bin')
    $report=Get-Content -Raw (Join-Path $ProjectRoot 'dist/build-report.json') | ConvertFrom-Json
    if($report.sha256 -ne $info.sha256 -or $report.source_sha256 -ne (Get-SourceHash)){throw 'Stale ROM: run make build before packaging.'}
    Invoke-HostTools -Arguments @('music-check') | Out-Null
}
if (Test-Path -LiteralPath $dest) { Remove-Item -LiteralPath $dest }
$zip=[IO.Compression.ZipFile]::Open($dest,[IO.Compression.ZipArchiveMode]::Create)
try {
    if($Release){
        $files=@('dist/MegaBenchOS.bin','dist/build-map.txt','dist/build-report.json','LICENSE','THIRD_PARTY_NOTICES.md','music/CREDITS.md','third_party/music/safetystoat/LICENSE','third_party/music/safetystoat/mad_bossa.txt','third_party/music/safetystoat/provenance.json','assets/music/generated/manifest.json' | ForEach-Object {Get-Item -LiteralPath (Join-Path $ProjectRoot $_)})
    } elseif($App -eq 'megapaint') {
        $files=@('apps/creative/megapaint.c','apps/creative/paint_model.c','apps/creative/paint_model.h','apps/creative/megapaint/README.md','docs/MBP_FORMAT.md','docs/MEGAPAINT.md' | ForEach-Object {Get-Item -LiteralPath (Join-Path $ProjectRoot $_)})
    } elseif($App -eq 'megapad') {
        $files=@('apps/creative/editors.c','apps/creative/model.c','apps/creative/model.h',"apps/creative/$App/README.md" | ForEach-Object {Get-Item -LiteralPath (Join-Path $ProjectRoot $_)})
    } elseif($App -in 'megabounce','colourstorm','ocean','stardrive') {
        $files=@("apps/demos/$App.c","apps/demos/${App}_assets.h",'apps/demos/demo_common.c','apps/demos/demo_common.h',"apps/demos/$App/README.md" | ForEach-Object {Get-Item -LiteralPath (Join-Path $ProjectRoot $_)})
    } else { $files=@(Get-ChildItem -LiteralPath (Join-Path $ProjectRoot "apps/$App") -File | Where-Object { $_.Extension -in '.c','.h','.md' }) }
    if(!$Release){$files+=@('sdk/include/megabenchos.h','docs/api/core.md','sdk/README.md','LICENSE','THIRD_PARTY_NOTICES.md' | ForEach-Object { Get-Item -LiteralPath (Join-Path $ProjectRoot $_) })}
    foreach ($file in $files) {
        $rel=$file.FullName.Substring($ProjectRoot.Length+1).Replace('\','/')
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip,$file.FullName,$rel,[IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
    if($Release){
        $entry=$zip.CreateEntry('README.txt');$writer=[IO.StreamWriter]::new($entry.Open(),[Text.UTF8Encoding]::new($false))
        try { $writer.Write("MegaBenchOS v1.0`n`nOpen dist/MegaBenchOS.bin in BlastEm or Genesis Plus GX.`nPAL and NTSC are supported. RAM Disk files disappear on restart.`n`nArrows/A: select. S: back. Enter+S held for one second: exit app.`nSource, setup and guides: https://github.com/SimonTheBarnett/MegaBenchOS`n`nSee LICENSE, THIRD_PARTY_NOTICES.md and music/CREDITS.md for licences.`n") } finally { $writer.Dispose() }
    }
    $entry=$zip.CreateEntry('manifest.json');$writer=[IO.StreamWriter]::new($entry.Open(),[Text.UTF8Encoding]::new($false))
    try {
        $manifest=if($Release){[ordered]@{name='MegaBenchOS';version='1.0';mode='rom';rom_sha256=$info.sha256}}else{[ordered]@{name=$App;api_version=1;mode='rom-linked-source';independent_installation=$false}}
        $writer.Write(($manifest | ConvertTo-Json))
    } finally { $writer.Dispose() }
} finally { $zip.Dispose() }
Write-Host $dest
