# Repository instructions

- Keep work within this repository and preserve unrelated changes.
- Use C99 and SGDK 2.11. Host scripts must work in Windows PowerShell 5.1; do not introduce Python or require PowerShell 7.
- Dot-source `./scripts/env.ps1`, then build with `make build`. Run relevant checks from [testing](docs/testing.md); avoid repeating unchanged emulator suites.
- Use Unity and native C for host tests/tools; do not introduce C#/.NET project dependencies. Add readable `@file` comments and interface contracts. Format handwritten C with `make format` and check with `make lint`; leave vendored/generated sources untouched.
- Apps include only the public SDK and app-local headers. Keep SGDK and hardware access in OS modules.
- Edit `api/core.json` for public contracts, then run `make generate`. Preserve descriptor compatibility and document parameters, errors, buffer sizes and ownership.
- Use bounded storage and shared services. Preserve atomic file replacement, session-only RAM Disk behaviour, and graphics/audio cleanup on exit.
- Keep public documentation about the current software. Test reports and captures belong in ignored `reports/`; retain the README splash screenshot and tour GIF in Git.
- Preserve licences, attribution and music provenance. Do not modify vendored code or install/change global tools without a specific need.
- Keep `plans/` empty and ignored. Ask before major architecture, hardware or licensing changes.
