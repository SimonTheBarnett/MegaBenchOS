param([ValidateSet('Prepare','Publish','Clean')][string]$Action='Prepare',[switch]$Contracts,[switch]$LittleFS,[switch]$MusicSmoke,[ValidateSet('','snake','megabounce','colourstorm','ocean','stardrive','megaplayer','megapaint')][string]$PreviewDemo='',[string]$CompilerFlags='')
# Windows file staging and checked ROM publication; SGDK compilation belongs to Make.
. "$PSScriptRoot/common.ps1"
if ($Action -eq 'Clean') {
    foreach ($name in @('out','dist')) {
        $target = [IO.Path]::GetFullPath((Join-Path $ProjectRoot $name))
        if ((Split-Path $target) -ne [IO.Path]::GetFullPath($ProjectRoot)) { throw "Unsafe clean path: $target" }
        if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force }
    }
    return
}
$cfg = Require-Sgdk
if($MusicSmoke -and $PreviewDemo -ne 'megaplayer'){throw 'MusicSmoke requires -PreviewDemo megaplayer.'}
if (($Contracts -and ($PreviewDemo -or $LittleFS)) -or ($LittleFS -and $PreviewDemo)) { throw 'Choose one of contracts, littlefs experiment or preview.' }
$project = Join-Path $ProjectRoot $(if ($Contracts) { 'out/contracts' } elseif($LittleFS){'out/littlefs-project'} elseif($MusicSmoke){'out/preview/megaplayer-smoke'} elseif($PreviewDemo){"out/preview/$PreviewDemo"} else { 'out/project' })
if ($Action -eq 'Prepare') {
Invoke-HostTools -Arguments @('generate') | Out-Null
Invoke-HostTools -Arguments @('music-check') | Out-Null
$staged=@{}
function Stage([string]$Source,[string]$Destination) {
    $Destination=[IO.Path]::GetFullPath($Destination)
    $staged[$Destination]=$true
    [IO.Directory]::CreateDirectory((Split-Path $Destination)) | Out-Null
    if (!(Test-Path -LiteralPath $Destination) -or (Get-FileHash -LiteralPath $Source).Hash -ne (Get-FileHash -LiteralPath $Destination).Hash) { Copy-Item -LiteralPath $Source -Destination $Destination -Force }
}
foreach ($name in (@('src','kernel','desktop','apps') + $(if ($Contracts) { @('tests/target') } elseif($LittleFS){@('third_party/littlefs','tests/littlefs')} else { @() }))) {
    $base = Join-Path $ProjectRoot $name
    foreach ($file in Get-ChildItem -LiteralPath $base -Recurse -File | Where-Object { $_.Extension -in '.c','.h' }) {
        if($LittleFS -and $name -eq 'tests/littlefs' -and $file.Name -eq 'host.c'){continue}
        $rel = $file.FullName.Substring($base.Length+1)
        if ($name -ne 'src') { $rel = Join-Path $name $rel }
        $dest = Join-Path $project "src/$rel"
        Stage $file.FullName $dest
    }
}
if($MusicSmoke){Stage (Join-Path $ProjectRoot 'tests/target/music_native.c') (Join-Path $project 'src/tests/target/music_native.c')}
foreach ($base in @('inc','sdk/include')) {
    [IO.Directory]::CreateDirectory((Join-Path $project 'inc')) | Out-Null
    foreach ($file in Get-ChildItem -LiteralPath (Join-Path $ProjectRoot $base) -Filter '*.h') { Stage $file.FullName (Join-Path $project "inc/$($file.Name)") }
}
Stage (Join-Path $ProjectRoot 'assets/music/music.res') (Join-Path $project 'res/music.res')
foreach($file in Get-ChildItem (Join-Path $ProjectRoot 'assets/music/generated') -Filter '*.xgc') {
    Stage $file.FullName (Join-Path $project "res/generated/$($file.Name)")
}
foreach($file in Get-ChildItem (Join-Path $project 'res') -Recurse -File | Where-Object {$_.Extension -in '.vgm','.xgc'}) {
    if (!$staged.ContainsKey($file.FullName)) {
        Remove-Item -LiteralPath $file.FullName
        # Old rescomp dependency lists refer to removed source VGMs.
        foreach($extension in '.o','.d') {
            $artifact=Join-Path $project "out/res/music$extension"
            if(Test-Path -LiteralPath $artifact){Remove-Item -LiteralPath $artifact}
        }
    }
}
# Prune renamed/deleted staged sources and corresponding objects so Make cannot link ghosts.
foreach ($folder in @('src','inc')) {
    foreach ($file in Get-ChildItem -LiteralPath (Join-Path $project $folder) -Recurse -File | Where-Object { $_.Extension -in '.c','.h' }) {
        if (!$staged.ContainsKey($file.FullName)) {
            $rel=$file.FullName.Substring($project.Length+1)
            Remove-Item -LiteralPath $file.FullName
            if ($file.Extension -eq '.c') {
                foreach ($extension in @('.o','.d')) { $artifact=Join-Path $project ('out/'+[IO.Path]::ChangeExtension($rel,$extension));if (Test-Path -LiteralPath $artifact) { Remove-Item -LiteralPath $artifact } }
            }
        }
    }
}
$upstream = Get-Content -Raw (Join-Path $cfg.sgdk 'makefile.gen')
if ([regex]::Matches($upstream,'-Wl,--gc-sections').Count -ne 1) { throw 'Unexpected SGDK link recipe.' }
$makefile=Join-Path $project 'makefile.gen';$recipe=$upstream.Replace('-Wl,--gc-sections','-Wl,--gc-sections,-Map,out/build.map').Replace('-flto=auto','-flto=1 -flto-partition=one')
$recipe += "`n`n" + '$(OBJS) $(OUT)/sega.o $(OUT)/rom_head.o: makefile.gen' + "`n"
$recipe += "# Project compiler options: $CompilerFlags`n"
if (!(Test-Path -LiteralPath $makefile) -or [IO.File]::ReadAllText($makefile).Replace("`r`n","`n") -cne $recipe.Replace("`r`n","`n")) { Write-Utf8 $makefile $recipe }
return
}
$dist = Join-Path $ProjectRoot 'dist'; [IO.Directory]::CreateDirectory($dist) | Out-Null
$prefix = if ($Contracts) { 'contracts-' } elseif($LittleFS){'littlefs-'} elseif($MusicSmoke){'preview-megaplayer-smoke-'} elseif($PreviewDemo){"preview-$PreviewDemo-"} else { '' }
$rom = Join-Path $dist $(if ($Contracts) { 'MegaBenchOS-contracts.bin' } elseif($LittleFS){'MegaBenchOS-littlefs.bin'} elseif($MusicSmoke){'MegaBenchOS-preview-megaplayer-smoke.bin'} elseif($PreviewDemo){"MegaBenchOS-preview-$PreviewDemo.bin"} else { 'MegaBenchOS.bin' })
Invoke-HostTools -Arguments @('publish-rom','--input',(Join-Path $project 'out/rom.bin'),'--output',$rom) | Out-Null
$info = Get-RomInfo $rom; $info.source_sha256 = Get-SourceHash -Contracts:$Contracts -LittleFS:$LittleFS -MusicSmoke:$MusicSmoke
$info.preview_app = $PreviewDemo
$info.music_smoke = [bool]$MusicSmoke
$size = & (Join-Path $cfg.sgdk 'bin/size.exe') (Join-Path $project 'out/rom.out')
if ($LASTEXITCODE) { throw 'Linker size inspection failed.' }
$numbers = ($size[-1].Trim() -split '\s+')
$info.linker_size = ($size -join "`n").Trim(); $info.ram_static_bytes = [int]$numbers[1]+[int]$numbers[2]
$info.vram_custom_patterns_bytes = (Get-Content -Raw (Join-Path $ProjectRoot 'assets/generated/manifest.json') | ConvertFrom-Json).pattern_bytes
$info.vram_demo_max_pattern_bytes = (Get-Content -Raw (Join-Path $ProjectRoot 'assets/generated/demos-manifest.json') | ConvertFrom-Json).max_leased_pattern_bytes
$info.frame_performance = 'SGDK V-counter subticks; emulator diagnostics are not CPU-cycle accurate.'
$info.vdp_layout = 'SGDK default tables; 64x32 A/B planes; OS atlas/font reservation; 79 exclusive app sprite slots plus hidden anchor'
$budgets = Get-Content -Raw (Join-Path $ProjectRoot 'config/budgets.json') | ConvertFrom-Json
$info.budget_warnings = @()
foreach ($item in @(@('rom_bytes',$info.rom_bytes),@('ram_static_bytes',$info.ram_static_bytes),@('custom_tiles',($info.vram_custom_patterns_bytes/32)))) {
    if ($item[1] -gt $budgets.($item[0])) { $info.budget_warnings += "$($item[0]): $($item[1]) exceeds advisory $($budgets.($item[0]))" }
}
Copy-Item -LiteralPath (Join-Path $project 'out/build.map') -Destination (Join-Path $dist "${prefix}build-map.txt") -Force
Write-Utf8 (Join-Path $dist "${prefix}build-report.json") (($info | ConvertTo-Json -Depth 8)+"`n")
Write-Host "Built $rom ($($info.rom_bytes) bytes)."
foreach ($warning in $info.budget_warnings) { Write-Warning $warning }
