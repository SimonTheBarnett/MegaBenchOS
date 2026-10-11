param(
    [Parameter(Mandatory=$true)][string[]]$InputPath,
    [string]$OutputDirectory,
    [ValidateSet('xgc','xgm')][string]$Format='xgc',
    [string]$CreditsManifest,
    [string]$Author,
    [string]$License,
    [string]$SourceUrl,
    [switch]$Recurse,
    [switch]$Force
)
# Thin SGDK xgm2tool frontend. Source bytes/credits are never rewritten.
. "$PSScriptRoot/../scripts/common.ps1"
$cfg=Require-Sgdk
$tool=Join-Path $cfg.sgdk 'bin/xgm2tool.jar'
if(!(Test-Path -LiteralPath $tool)){throw 'Installed SGDK has no xgm2tool.jar.'}
if(!(Get-Command java -ErrorAction SilentlyContinue)){throw 'Java must be on PATH.'}
if(!$OutputDirectory){$OutputDirectory=Join-Path $ProjectRoot 'out/music-conversion'}
$destination=[IO.Path]::GetFullPath($OutputDirectory)
$files=@()
foreach($sourceInput in $InputPath){
    foreach($resolved in Resolve-Path -Path $sourceInput){
        $item=Get-Item -LiteralPath $resolved.Path
        if($item.PSIsContainer){$files+=@(Get-ChildItem -LiteralPath $item.FullName -Filter '*.vgm' -File -Recurse:$Recurse)}
        else{$files+=$item}
    }
}
$files=@($files | Sort-Object FullName -Unique)
if(!$files.Count){throw 'No VGM source files found.'}
$credits=$null
if($CreditsManifest){$credits=[IO.File]::ReadAllText((Resolve-Path -LiteralPath $CreditsManifest).Path)|ConvertFrom-Json}
$names=@{}
foreach($file in $files){
    if($destination.TrimEnd('\','/') -eq $file.DirectoryName.TrimEnd('\','/')){throw 'Choose an output directory separate from the source directory.'}
    $key=$file.BaseName.ToLowerInvariant()
    if($names.ContainsKey($key)){throw "Output name collision: $($file.Name). Split this batch into separate output directories."}
    $names[$key]=$true
    if(!$Force -and (Test-Path -LiteralPath (Join-Path $destination ($file.BaseName+'.'+$Format)))){throw "Output exists: $($file.BaseName).$Format. Use -Force to replace generated assets."}
}
# Do not overwrite a previous batch's credits by silently replacing manifest.json.
if(!$Force -and (Test-Path -LiteralPath (Join-Path $destination 'manifest.json'))){throw 'Output manifest exists. Use a fresh output directory or -Force.'}
[IO.Directory]::CreateDirectory($destination)|Out-Null
$results=@();$failures=0
foreach($file in $files){
    $output=Join-Path $destination ($file.BaseName+'.'+$Format)
    $log=Join-Path $destination ($file.BaseName+'.conversion.log')
    $entry=[ordered]@{source_filename=$file.Name;source_sha256=(Get-FileHash $file.FullName).Hash.ToLowerInvariant();output_filename=[IO.Path]::GetFileName($output);author=$Author;license=$License;source_url=$SourceUrl;upstream_commit=$null;status='FAIL';error=$null}
    $match=$null
    if($credits){$match=@($credits.tracks | Where-Object {$_.filename -eq $file.Name})}
    try{
        if($match.Count -gt 1){throw 'Ambiguous source credits.'}
        if($match){
            if($match[0].sha256 -ne $entry.source_sha256){throw 'Source bytes do not match attribution manifest.'}
            foreach($field in 'author','license','source_url'){$entry[$field]=$match[0].$field}
            $entry.title=$match[0].title
            $entry.upstream_commit=$credits.upstream_commit
        }
        if($file.Extension -ine '.vgm'){throw 'Input must be an uncompressed .vgm file.'}
        if($file.Length -gt 16777216){throw 'Source exceeds this wrapper''s 16 MiB validation bound.'}
        $entry.source_bytes=$file.Length
        $entry.source_samples=[long](Invoke-HostTools -Arguments @('vgm-info','--input',$file.FullName))
        # A path-valued native argument is passed individually; never build shell text.
        $messages=& java -jar $tool $file.FullName $output -s 2>&1
        $exit=$LASTEXITCODE
        Write-Utf8 $log (($messages | ForEach-Object {"$_"}) -join "`n")
        if($exit -or !(Test-Path -LiteralPath $output)){throw "xgm2tool failed (exit $exit); see $([IO.Path]::GetFileName($log))."}
        $entry.output_bytes=(Get-Item -LiteralPath $output).Length
        if(!$entry.output_bytes){throw 'xgm2tool produced an empty asset.'}
        $entry.output_sha256=(Get-FileHash $output).Hash.ToLowerInvariant()
        $entry.status='PASS'
        $entry.rights_status=if($entry.author -and $entry.license){'ATTRIBUTED; verify license before redistribution'}else{'UNVERIFIED; conversion grants no redistribution rights'}
        Write-Host "PASS $($file.Name) -> $($entry.output_filename): $($entry.source_bytes) -> $($entry.output_bytes) bytes"
    }catch{
        $entry.error=$_.Exception.Message;$failures++
        Write-Host "FAIL $($file.Name): $($entry.error)"
        # A failed conversion cannot leave a stale generated asset marked as usable.
        if(Test-Path -LiteralPath $output){Remove-Item -LiteralPath $output}
    }
    $results+=$entry
}
$manifest=[ordered]@{format=$Format;converter='SGDK 2.11 xgm2tool 1.05';tool_sha256=(Get-FileHash $tool).Hash.ToLowerInvariant();options=@('-s');license_evidence_url=if($credits){$credits.license_evidence_url}else{$null};license_evidence_sha256=if($credits){$credits.license_evidence_sha256}else{$null};tracks=$results}
if($credits -and $credits.license_evidence_filename){
    $evidence=Join-Path (Split-Path (Resolve-Path -LiteralPath $CreditsManifest).Path) $credits.license_evidence_filename
    if((Get-FileHash -LiteralPath $evidence).Hash.ToLowerInvariant() -ne $credits.license_evidence_sha256){throw 'License evidence hash mismatch.'}
    Copy-Item -LiteralPath $evidence -Destination (Join-Path $destination 'LICENSE_EVIDENCE.md') -Force
    $manifest.license_evidence_file='LICENSE_EVIDENCE.md'
}
if($credits -and $credits.composer_evidence_filename){
    $evidence=Join-Path (Split-Path (Resolve-Path -LiteralPath $CreditsManifest).Path) $credits.composer_evidence_filename
    if((Get-FileHash -LiteralPath $evidence).Hash.ToLowerInvariant() -ne $credits.composer_evidence_sha256){throw 'Composer evidence hash mismatch.'}
    Copy-Item -LiteralPath $evidence -Destination (Join-Path $destination 'COMPOSER_EVIDENCE.txt') -Force
    $manifest.composer_evidence_file='COMPOSER_EVIDENCE.txt'
    $manifest.composer_evidence_sha256=$credits.composer_evidence_sha256
}
Write-Utf8 (Join-Path $destination 'manifest.json') (($manifest|ConvertTo-Json -Depth 8)+"`n")
if($failures){throw "$failures conversion(s) failed; manifest.json records each result."}
