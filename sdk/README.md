# MegaBenchOS SDK

[MegaBenchOS](../README.md) / SDK

Build ROM-linked C apps using shared window, input, graphics, audio and file services.

- [Creating your first app](../docs/FIRST_APP.md): a complete starting example and registration steps.
- [SDK guide](../docs/API.md): lifecycle, coordinates, ownership and resource rules.
- [Function reference](../docs/api/core.md): every public signature, parameter, result and error.
- [Public header](include/megabenchos.h) and [contract source](../api/core.json).
- [Build](../docs/BUILD.md), [contributing](../docs/CONTRIBUTING.md) and [testing](../docs/testing.md).

Apps are trusted and cooperative, use only public SDK and app-local headers, and link into the host ROM. Source packages are for relinking, not independent installation. API additions through 1.7 retain the v1 descriptor layout; no external binary ABI is promised.

Original code follows the [project licence](../LICENSE). Packages retain relevant notices; music and third-party components keep their separate licences.
