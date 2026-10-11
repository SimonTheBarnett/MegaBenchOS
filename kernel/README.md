# OS services

[MegaBenchOS](../README.md) / OS services

Private implementations behind the [public SDK](../docs/API.md). Apps must not include these internal headers.

| Source area | Responsibility |
| --- | --- |
| [Application runtime](app_runtime/) | Foreground lifecycle and ownership |
| [Shared services](services/) | System services and hardware integration |
| [Storage](storage/README.md) | VFS, RAM Disk and ROM files |
| [Media](media/) | Bundled music and audio integration |
| [Time](time/) | Runtime and frame timebase |

See [architecture](../docs/ARCHITECTURE.md) for the full source map and [testing](../docs/testing.md) for verification.

Original OS code uses the [project licence](../LICENSE).
