# Contributing

Set up the project using [installation](INSTALL.md), then [build](BUILD.md) and run the relevant [tests](testing.md). For a new app, start with [Creating Your First MegaBench App](FIRST_APP.md).

## Coding standards

- Use C99 and SGDK 2.11. Host tools use native C; keep Windows integration compatible with PowerShell 5.1.
- Use Unity for native tests and C emulator fixtures. Exercise production code and keep hardware assertions in the emulator or target harness; see [testing](testing.md#adding-tests).
- Run `make format` and `make lint` for handwritten C. Leave vendored sources and generated assets to their maintainers/generators.
- Start each C file with a Doxygen-style `@file` and `@brief` explaining its responsibility and layer. Document ownership, bounds, errors and non-obvious behaviour at interfaces; use short comments for hardware assumptions and test intent. Prefer named limits and explicit bounds checks.
- Apps use `megabenchos.h` and app-local headers. Hardware, allocation, palettes and storage backends belong to OS modules.
- Keep callbacks responsive, buffers bounded and shared-resource ownership explicit. Check file read, write and close results; failed saves must preserve existing files and unsaved edits.
- Query capabilities and window geometry instead of assuming hardware or coordinates.

Apps are trusted, cooperative and linked into the ROM. Register new descriptors and desktop entries as described in the first-app tutorial; there is no external executable loader.

## Public API changes

Edit [api/core.json](../api/core.json), then run `make generate` in the configured build environment. Commit the generated header, metadata and reference together. Document purpose, parameters, results, errors, buffer sizes, ownership, callback context and hardware limits. Preserve existing descriptor layout and source compatibility.

Use [Clock](../apps/clock/clock.c) for lifecycle callbacks, [Calculator](../apps/calculator/calculator.c) for widgets, and the creative apps for file and canvas examples. See [API](API.md) for shared-service rules.

## Pull requests

Keep changes focused. Explain the problem, the resulting behaviour and how it was tested. Update the relevant guide when controls, formats or contracts change. Distinguish automated passes from manual or skipped checks.

Do not commit local configuration, tool binaries, ROMs, reports or test captures. Generated SDK and artwork files are tracked; test output stays under ignored `reports/`.

Original code and artwork use [MIT](../LICENSE). Preserve [third-party notices](../THIRD_PARTY_NOTICES.md), asset licences and music provenance when adding or changing resources.
