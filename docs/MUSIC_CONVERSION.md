# MegaBenchOS music conversion

MegaPlayer uses SGDK 2.11's XGM2 Z80 driver. A VGM describes timed chip writes;
the driver expects compiled XGM2 data instead. Conversion happens on the host,
using the installed `bin/xgm2tool.jar` and Java. No additional converter or
dependency is required. Original VGM files are never rewritten.

## Convert files with PowerShell

Configure SGDK as described in [BUILD.md](BUILD.md). From the project root:

```powershell
# One verified source, producing compiled mad_bossa.xgc and manifest.json.
./tools/convert-music.ps1 -InputPath third_party/music/safetystoat/mad_bossa.vgm `
    -OutputDirectory out/music-conversion/mad-bossa `
    -CreditsManifest third_party/music/safetystoat/provenance.json

# All five bundled sources; directories, wildcards and multiple paths work.
./tools/convert-music.ps1 -InputPath third_party/music/safetystoat `
    -OutputDirectory out/music-conversion/safetystoat `
    -CreditsManifest third_party/music/safetystoat/provenance.json

# A separately licensed collection, including subdirectories.
./tools/convert-music.ps1 -InputPath D:/Music/Sources -Recurse `
    -OutputDirectory out/music-conversion/custom `
    -Author 'Author name' -License 'CC-BY-4.0' -SourceUrl 'https://example.org/music'
```

Use a fresh output directory per collection. `-Force` explicitly replaces
generated assets and the batch manifest; it does not merge old manifest entries
or remove old unrelated outputs. Duplicate basenames in one batch are rejected.
Input and output directories must differ. Errors are reported per file, valid
files in the batch continue, and any failure makes the command fail. The manifest
records each result; converter diagnostics are saved in `.conversion.log` files.
Successful results print source and output byte sizes.

The default `-Format xgc` produces compiled, packed single-track data for the
driver. `-Format xgm` produces the tool's XGM2 container/intermediate, with its
`XGM2` signature; it is not interchangeable with the compiled `.xgc` blob.
The tool's `-s` option silences console chatter, not the generated music.
Do not use the older XGM1 `xgmtool`.

## Input limits and rights

The wrapper conservatively accepts uncompressed `.vgm`, versions 1.50 through
1.71, at most 16 MiB, with YM2612 and/or SN76489. It checks header/data/tag bounds,
declared duration, command lengths and an end command. Supported commands are
YM2612/PSG writes, ordinary waits, and type-00 PCM blocks with DAC seek/wait
commands and bounded DAC streams (0x90-0x95) configured for YM2612 port 0, register 0x2a. Other chips, dual-chip variants and extra extended-header fields are rejected. VGZ/gzip, MP3, MOD and generic game-rip
formats are unsupported. OGG/MP3/WAV renders cannot reconstruct timed YM2612/PSG writes and are not inputs to this workflow. This conservative subset is narrower than every format
the upstream tool might accept. A successful conversion does not establish
perfect fidelity: XGM2 frame timing, PCM conversion and unusual effects need
listening checks on both PAL and NTSC. Three current tracks contain PCM/DAC streams. SGDK handles their sample conversion and resampling.

Keep original sources and their credits in `third_party/music/` (or a separate
source collection). Trial conversions go in ignored `out/`; the release library is preconverted and versioned separately in `assets/music/generated/`. The optional credits
JSON follows [the bundled provenance schema](../third_party/music/safetystoat/provenance.json):
top-level upstream commit and license-evidence URL/hash/filename, and a `tracks`
array with filename, SHA-256, author, license and source URL. Matching source
hashes are mandatory when using that manifest. License evidence is hash checked
and copied as `LICENSE_EVIDENCE.md` alongside generated assets.

Generated `manifest.json` retains source and output hashes, byte sizes, author,
license, source URL, revision, converter hash/options and conversion status.
Distribute it and license evidence with the outputs; compiled blobs do not embed
credits. Supplying author/license strings does not verify reuse rights. Missing
credits are explicitly marked UNVERIFIED. Respect the original author's license
and preserve any required attribution/license text separately.

## Bundle music into the ROM

The release build verifies source/licence hashes and the conversion manifest, then stages the five **already converted** `.xgc` files from `assets/music/generated/`. `assets/music/music.res` uses raw `BIN` resources; a normal build never reconverts the library. To regenerate the library deliberately:

```powershell
./tools/convert-music.ps1 -InputPath third_party/music/safetystoat `
    -OutputDirectory assets/music/generated `
    -CreditsManifest third_party/music/safetystoat/provenance.json -Force
```

Review generated hashes and licence/composer evidence before committing assets. To add another preconverted track, stage it into the resource directory and use:

```text
BIN music_custom "generated/track.xgc" 256 256 0 NONE
```

The first 256 aligns the address; the second pads its size. Do not compress it.
Add the generated resource symbol, verified title and credit to the private
catalog in `kernel/media/music.c`, and extend source verification/provenance.
Apps select catalog IDs through `mb_music_*`; they never pass raw driver pointers.
Run `make build` and the focused music checks described in [testing](testing.md).
Retain the generated manifest and license evidence when shipping custom assets.

## Prepare for future external storage

Keep `.xgc`, manifest and license evidence together in a separate distribution
directory. Preserve `.vgm` as source, not as a runtime music format. Music file
paths use the public VFS; future mounted storage can use the same file API.

External playback is currently **unsupported**, including otherwise valid tiny
`.xgc` files. RAM Disk's entire payload is only 3592 bytes shared with documents
and pictures. `RAM:/Music/` supports browsing, associations, rename and delete;
opening `.vgm` reports unsupported and malformed `.xgc` reports a format error.
No hidden song buffer or untrusted driver pointer is allocated.

Enabling external playback later requires full packed-stream/sample/loop bounds
validation, a stable 256-byte-aligned driver-addressable buffer, ownership for the
whole playback lifetime, and review of Z80 bus access and bank/address limits.
ROM resources above the driver's direct bank range need SGDK's appropriate far
playback path. Storage capacity alone does not satisfy these requirements.

## Bundled sources

The five bundled Safety Stoat tracks use SGDK 2.11 / xgm2tool 1.05. Source sample counts, exact filenames and attribution are in [music credits](../music/CREDITS.md); converter hashes/options and asset hashes are in the [conversion manifest](../assets/music/generated/manifest.json). All five have zero source loop fields and play once. Charge is a separate CC0 converter test fixture with its own retained licence and provenance.
