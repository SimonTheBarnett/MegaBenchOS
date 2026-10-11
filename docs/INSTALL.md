# Get MegaBenchOS running on Windows 11

Get the desktop onto your screen, then start exploring. This guide uses **Windows PowerShell 5.1**, already included with Windows 11, and the project's Windows x64 tools.

## Just want to play?

Download the ROM from the [latest release](https://github.com/SimonTheBarnett/MegaBenchOS/releases/latest), then download [BlastEm 1.0.0 for 64-bit Windows](https://www.retrodev.com/blastem/downloads.html), extract its ZIP and drag the ROM onto `blastem.exe`. You only need the emulator and ROM to play.

If you have the project source instead, follow the steps below to build your ROM. The source repository does not include generated ROMs or emulator binaries.

## 1. Get the project and open PowerShell

Download and extract the project ZIP, or clone it using [Git for Windows](https://git-scm.com/install/windows). Open the folder containing `README.md`, `apps` and `scripts`.

Open **Windows PowerShell** from Start, or press Win+R and enter `powershell.exe`. In that window, change to your project folder:

```powershell
Set-Location 'C:\Projects\MegaBenchOS'
$PSVersionTable.PSVersion
```

Use your actual project path. The version should start with `5.1`. Project paths can contain spaces; the SGDK tool path must not. [Windows PowerShell](https://learn.microsoft.com/en-us/powershell/scripting/what-is-windows-powershell) ships with Windows, so there is no separate shell to install.

If downloaded scripts are blocked, allow local scripts for this PowerShell session and unblock the project's script files:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy RemoteSigned
Get-ChildItem .\scripts, .\tools -Recurse -Filter *.ps1 | Unblock-File
```

The [process-scope execution-policy setting](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.security/set-executionpolicy?view=powershell-5.1) expires when this window closes. It does not change the machine-wide policy. If your organisation enforces a policy, use its approved development workflow.

## 2. Install the build prerequisites

| Tool | What to install | Why |
| --- | --- | --- |
| Java | [Eclipse Temurin 25 JDK](https://adoptium.net/installation/windows), Windows x64; keep the installer option to add Java to PATH. | SGDK's resource and music tools run on Java. |
| 7-Zip | [64-bit Windows installer](https://www.7-zip.org/download.html). | Extracts the SGDK archive during setup. |
| LLVM | [Windows x64 LLVM 22.1.8](https://github.com/llvm/llvm-project/releases/tag/llvmorg-22.1.8). | Compiles native C build helpers and Unity tests; includes clang-format and clang-tidy. |
| C++ Build Tools and Windows SDK | [Microsoft installer](https://learn.microsoft.com/en-us/cpp/overview/acquire-msvc?view=msvc-170), Desktop development with C++, including MSVC x64/x86 tools and a Windows SDK. | Supplies C runtime headers/libraries for native tools; the Visual Studio editor is optional. |

Temurin also offers this [WinGet installation command](https://adoptium.net/installation):

```powershell
winget install --id EclipseAdoptium.Temurin.25.JDK --exact
```

After installation, close and reopen PowerShell or VS Code so it picks up Java. Return to the project folder and check:

```powershell
java --version
```

An existing working Java 25 installation is fine. SGDK supplies its own GCC and Make; you do not need to install those separately.

## 3. Let setup fetch SGDK and BlastEm

From the project folder:

```powershell
.\scripts\setup.ps1 -InstallTools
```

Setup downloads the project's pinned **SGDK 2.11** and **BlastEm 1.0.0**, checks their archive hashes and extracts them into `%LOCALAPPDATA%\MegaBenchOS\tools`. It writes machine-local paths to `config/toolchain.local.json`, inspects the tools; `make build` generates and validates SDK files. It does not install another PowerShell or change system PATH.

If your Windows profile path contains spaces, choose a writable tool folder without spaces:

```powershell
.\scripts\setup.ps1 -InstallTools -ToolRoot C:\MegaBenchTools
```

The tool folder is separate from the project. Existing local configuration is preserved; if you move tools later, edit that JSON or use the [documented path overrides](BUILD.md).

Already have the tools? Copy `config/toolchain.example.json` to `config/toolchain.local.json`, fill in `sgdk` and `blastem`, and run `scripts/setup.ps1` without `-InstallTools`. Use forward slashes or doubled backslashes in JSON paths. Leave optional emulator fields empty if unused. [SGDK's official 2.11 release](https://github.com/Stephane-D/SGDK/releases/tag/v2.11) and [BlastEm downloads](https://www.retrodev.com/blastem/downloads.html) are the manual download sources.

## 4. Build, launch and have a look around

```powershell
. .\scripts\env.ps1
make build
make run
```

The build creates `dist\MegaBenchOS.bin`. BlastEm opens with the ROM: let the logo fade, then choose a desktop folder.

**Arrows** move, **A** opens, **S** returns, **D** switches pointer/focus, and **Enter** opens System from the desktop. Hold **Enter+S** for one second to exit an app. Try MegaPaint, a song in MegaPlayer, Snake or a fullscreen demo. Run one emulator window at a time so two audio streams do not overlap.

For PAL, use `make run REGION=pal`; the default is NTSC. Close the emulator before launching another instance. RAM Disk files belong to the running session and disappear on restart.

## Developing and testing

[VS Code](https://code.visualstudio.com/download) is optional. Open the project folder; its terminal and tasks select Windows PowerShell 5.1. Use **Terminal > Run Task** for **Build**, **Test** and **Run**. Each task prepares the tool environment automatically. Restart an already-open terminal after changing profiles or installing tools.

The LLVM and Windows SDK prerequisites above serve both native build helpers and tests. The standard LLVM path is `C:\Program Files\LLVM`; use `MB_HOST_CC` for another clang path. Unity and cJSON are included as C sources, so no package restore or .NET project toolchain is required.

```powershell
.\scripts\doctor.ps1
make test
```

For automated PAL/NTSC checks, setup can also install the pinned Genesis Plus GX libretro core:

```powershell
.\scripts\setup.ps1 -InstallTools -InstallEmulatorCore
. .\scripts\env.ps1
make build
make compatibility
```

These tests load the core directly; they do not need RetroArch. The core download is an upstream nightly snapshot: if its hash has changed, setup stops. Keep using BlastEm and host tests while reviewing a lock update; do not bypass the checksum. Configure a verified local core through `genesis_plus_gx_core` or `MB_GENPLUS_CORE` when available.

For interactive Genesis Plus GX, install [RetroArch for Windows x64](https://www.retroarch.com/index.php?page=platforms), set `retroarch` in local config and use `scripts/run-genplus.ps1` with a configured GX core. This is optional for building and playing in BlastEm.

Next: [your first app](FIRST_APP.md), [build commands](BUILD.md), [public API](API.md) and [testing](testing.md).

## If something gets stuck

| Symptom | Fix |
| --- | --- |
| `java` is not recognised | Reopen PowerShell after installing Java with its PATH option, then check `java --version`. |
| Setup cannot find 7-Zip | Use its standard install folder or put `7z.exe` on PATH. |
| Tool path contains spaces | Use `-ToolRoot` with a path without spaces; update existing local config if moving an install. |
| BlastEm shows a usage dialog | Launch with `run-blastem.ps1` or pass the ROM to `blastem.exe`; opening the emulator alone may show usage. |
| A tool is missing | Run `doctor.ps1` and check `config/toolchain.local.json` against the [path table](BUILD.md). |
| Tests report a stale ROM | Rebuild after source, contract or tool-lock changes. |
| Core checksum mismatch | The nightly changed. Retain the existing lock until a new core is reviewed and tested. |

## Licensing

See [project licensing](../README.md#licensing) and [dependency notices](dependencies.md). Emulators and development tools keep their own licences; they are not bundled into the project source archive.
