# Third-party notices

MegaBenchOS includes SGDK library code and its shared font under this notice. Development tools are separately licensed; see docs/dependencies.md.

Native host tests use [Unity v2.6.1](https://github.com/ThrowTheSwitch/Unity/releases/tag/v2.6.1),
commit cbcd08fa7de711053a3deec6339ee89cad5d2697, under MIT.
Copyright (c) 2007–2026 Unity Project by Mike Karlesky, Mark VanderVoord,
and Greg Williams. Its full licence and pinned source hashes are retained in
[third_party/unity/LICENSE.txt](third_party/unity/LICENSE.txt) and
[upstream.json](third_party/unity/upstream.json). Unity is not included in the ROM.

Native host tools use [cJSON v1.7.19](https://github.com/DaveGamble/cJSON/tree/v1.7.19), MIT, Copyright (c) 2009-2017 Dave Gamble and cJSON contributors. The unmodified sources, full [licence](third_party/cjson/LICENSE) and [pinned hashes](third_party/cjson/upstream.json) are retained. cJSON is excluded from the ROM.

The isolated filesystem benchmark vendors littlefs v2.11.3, commit
6cb4e86540eca0d9ba62500a298385c9d863c8be, Copyright (c) 2022 the littlefs authors,
Copyright (c) 2017 Arm Limited. BSD-3-Clause; full license in
third_party/littlefs/LICENSE.md. Vendored sources are unmodified and hash recorded
in upstream.json. SGDK test utility/CRC adaptations retain that attribution and
license. These sources are excluded from the release ROM.

MIT License - Copyright (c) 2025 Stephane Dallongeville

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.

Bundled music: Safety Stoat Studios and Spring, CC0-1.0, free_vgms commit 2e376d0abfa5709f6c0e648095672cd6c1654f93. Mad Bossa VGM transcription by Zachary at Safety Stoat Studios. Exact sources, hashes, attribution and licence evidence: [music/CREDITS.md](music/CREDITS.md) and third_party/music/safetystoat/LICENSE. Beatscribe Charge remains a CC0 conversion fixture with its original dedication in third_party/music/beatscribe/UPSTREAM_README.md. SGDK XGM2 retains the SGDK MIT notice above; conversion tools are documented in docs/dependencies.md.
