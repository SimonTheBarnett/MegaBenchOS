# Development tools and licences

Use [installation](INSTALL.md) for setup and [build](BUILD.md) for configuration. Toolchains and emulators are not included in the source repository or ROM.

| Tool | Version | Licence/source |
| --- | --- | --- |
| SGDK | 2.11 | [MIT](https://github.com/Stephane-D/SGDK/tree/v2.11) |
| SGDK's GCC / Make | 13.2.0 / 3.81 | GNU tooling; libgcc uses GPLv3 with the GCC runtime exception |
| Windows PowerShell | 5.1 | [Windows component](https://learn.microsoft.com/en-us/powershell/scripting/what-is-windows-powershell) |
| Java JDK | 25 | [Temurin installation](https://adoptium.net/installation/windows); no JRE is distributed |
| 7-Zip | Archive extraction | [Official downloads](https://www.7-zip.org/download.html) |
| BlastEm | 1.0.0 | [GPLv3](https://www.retrodev.com/blastem/downloads.html) |
| LLVM clang/lld/clang-format/clang-tidy | 22.1.8 | [Apache 2.0 with LLVM exception](https://llvm.org/LICENSE.txt); native host tools and tests |
| cJSON | 1.7.19 | [MIT](../third_party/cjson/LICENSE); native host JSON parser, excluded from the ROM |
| Unity | 2.6.1 | [MIT](../third_party/unity/LICENSE.txt); vendored C test framework, excluded from the ROM |
| C++ Build Tools / Windows SDK | Installed Windows x64 toolset | [Microsoft installation](https://learn.microsoft.com/en-us/cpp/overview/acquire-msvc?view=msvc-170); C runtime headers/libraries for native host tests |
| Genesis Plus GX | v1.7.4, commit 393f70c | [Noncommercial core licence and component licences](https://github.com/ekeeke/Genesis-Plus-GX/blob/master/LICENSE.txt); optional test tool |

Download URLs, archive hashes and the GX runtime hash are in [toolchain.lock.json](../config/toolchain.lock.json). Setup validates them before extraction. The GX URL is a mutable nightly; changed downloads fail the pinned hash check.

Unity's exact commit and source hashes are in [third_party/unity/upstream.json](../third_party/unity/upstream.json). Only its C sources, headers and licence are vendored; no test-generator scripting dependencies are used.

Genesis Plus GX has noncommercial restrictions. Review its upstream licence before commercial development/testing. It is not part of MegaBenchOS or its source packages.

Project licensing, SGDK/font notices and music attribution are retained in [LICENSE](../LICENSE), [third-party notices](../THIRD_PARTY_NOTICES.md) and [music credits](../music/CREDITS.md). Development tools keep their own licences.
