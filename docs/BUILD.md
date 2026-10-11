# Build

For a new machine, follow [Windows 11 installation](INSTALL.md). Use Windows PowerShell 5.1 from the repository root.

## Prerequisites and environment

- SGDK 2.11 supplies GCC, Make and the resource tools.
- Java 25 must be on PATH. Setup also needs 7-Zip to extract SGDK.
- BlastEm 1.0.0 is the primary interactive emulator.
- Native build helpers and tests need LLVM clang/lld/clang-format/clang-tidy and the Windows C runtime headers/libraries from the C++ Build Tools and Windows SDK. All project host utilities are C99.
- Automated compatibility checks need the pinned Genesis Plus GX x64 libretro core; RetroArch is only for interactive GX.

```powershell
./scripts/setup.ps1 -InstallTools  # First-time setup
. ./scripts/env.ps1               # Once per terminal session
```

The environment helper reads ignored `config/toolchain.local.json`, validates SGDK and makes its bundled Make available through a PowerShell session alias. It does not alter PATH or global settings. VS Code's **Build**, **Test** and **Run** tasks prepare the same environment automatically.

To use existing tools, copy [the example configuration](../config/toolchain.example.json), fill in the paths and run setup without `-InstallTools`. Run `./scripts/doctor.ps1` for diagnostics. Environment variables override configuration:

| Variable | Path |
| --- | --- |
| `MB_SGDK` | SGDK root with `bin/` and `lib/` |
| `MB_BLASTEM` | BlastEm executable |
| `MB_GENPLUS_CORE` | Genesis Plus GX x64 libretro DLL |
| `MB_RETROARCH` | RetroArch executable |
| `MB_HOST_CC` | LLVM clang executable |

The SGDK path must not contain spaces. Project paths may contain spaces. Setup's `-ToolRoot` selects a different installation directory. See [tool licences](dependencies.md).

## Everyday commands

| Command | Operation |
| --- | --- |
| `make build` (or `make`) | Compile resources and C, then publish the checked ROM |
| `make test` | Build and run host/source regression checks |
| `make test-unit` | Compile and run Unity tests against the production C models |
| `make test-emulator` | Build release/contract ROMs and run every GX scenario in PAL/NTSC |
| `make run` | Build and launch BlastEm in NTSC; use `REGION=pal` for PAL |
| `make compatibility` | Build and run the existing host and PAL/NTSC emulator suites, one cycle each |
| `make clean` | Remove only repository `out/` and `dist/`; retain test reports |
| `make package` | Build and create `dist/MegaBenchOS-v1.0-release.zip` |

The ROM is `dist/MegaBenchOS.bin`; the map and build report are alongside it. Warnings fail compilation. Publication validates ROM size, header, reset vectors and additive checksum, and records a source stamp used to reject stale test inputs.

Make invokes SGDK's own compilation and resource recipes in `out/project/`. The Windows build helper stages the source layout into SGDK's expected `src/inc/res` directories and publishes checked outputs. SGDK itself is not modified. The pinned SGDK Make is 32-bit; the root Makefile uses Windows' Sysnative shell path for 64-bit PowerShell 5.1. Make also compiles the Unity executable and native C host utility incrementally into `out/host/`.

## Other supported operations

```powershell
make generate                         # SDK/UI and demo artwork generators
make lint                             # Check handwritten C formatting
make format                           # Apply .clang-format
make verify-clean                     # Isolated source-copy build/test/package
make check-generated                  # Check generated files and music provenance
make contracts                        # dist/MegaBenchOS-contracts.bin
make build PREVIEW=megapaint           # App preview ROM
make build PREVIEW=megaplayer MUSIC_SMOKE=1
make package-app APP=calculator        # Existing ROM-linked source SDK package
make run-genplus                      # Interactive RetroArch, if configured
```

Preview names: snake, megabounce, colourstorm, ocean, stardrive, megaplayer and megapaint. Select only one build variant. The optional storage experiment uses `make littlefs`; it does not enable persistent storage in the release ROM. Source packages are for ROM relinking, not independently loadable apps.

Music conversion uses SGDK's existing xgm2tool through the [conversion utility](MUSIC_CONVERSION.md). Normal builds verify and bundle committed XGM2 assets without reconverting them. Focused emulator and optional storage commands remain in [testing](testing.md).

## Helper responsibilities

Contributors normally use Make. PowerShell handles Windows setup, file staging, emulator launch and packaging; C owns generation and test validation:

| Helpers | Purpose |
| --- | --- |
| `setup.ps1`, `env.ps1`, `doctor.ps1`, `common.ps1` | Pinned installation, session environment, diagnostics and native-tool invocation |
| `build-support.ps1`, `package.ps1` | SGDK file staging/cleanup, checked publication and ZIP packaging |
| `tools/native/generate.c`, `demos.c` | Native SDK, UI and original artwork generators |
| `run-blastem.ps1`, `run-genplus.ps1` | Windows emulator launching |
| `tools/native/workflow.c` | Formatting and isolated clean-source reproducibility checks |
| `tools/convert-music.ps1` | Attributed SGDK xgm2tool conversion |

The [native C host utility](../tools/native/main.c) owns ROM/VGM validation, source stamps, SDK isolation, music provenance and emulator orchestration. Scenario definitions live in [tests/emulator/](../tests/emulator/). Native model tests live in [tests/unit/](../tests/unit/) and reuse [tests/host/](../tests/host/) fixtures. See [testing](testing.md) for focused commands and reports.

## Troubleshooting

- **Make missing:** dot-source `scripts/env.ps1` in the current terminal. Use the bundled SGDK Make, not a separate installation.
- **Missing tool:** run `doctor.ps1`; check Java and configured paths.
- **Host compilation cannot find `stdio.h` or CRT libraries:** install the C++ Build Tools/Windows SDK listed in [installation](INSTALL.md). LLVM detects their standard installation paths.
- **Blocked scripts:** follow the process-scoped execution-policy instructions in [installation](INSTALL.md).
- **Generated drift:** run `make generate` and review changes.
- **Stale ROM:** run `make build` after source, recipe, contract or tool configuration changes.
- **Core checksum mismatch:** review and test a dependency-lock update; do not bypass hash checks.
- **ROM size or resource warning:** review the build report and [config/budgets.json](../config/budgets.json). The unbanked cartridge limit is 4 MiB; budgets are advisory, while invalid ROMs and failed contracts are errors.
