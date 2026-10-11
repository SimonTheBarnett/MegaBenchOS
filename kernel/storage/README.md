# Storage implementation

[MegaBenchOS](../../README.md) / [OS services](../README.md) / Storage

The private storage layer implements the public VFS for a compact session RAM Disk and immutable ROM resources. Applications use SDK file hooks rather than these internal headers.

| File | Responsibility |
| --- | --- |
| [vfs.c](vfs.c) | Path normalisation, drive dispatch, public operations and global handle tokens |
| [ram_adapter.c](ram_adapter.c) | Compact RAM backend adapter |
| [ramdisk.c](ramdisk.c) | Metadata validation and atomic whole-file replacement |
| [rom.c](rom.c) | Read-only bundled files |
| [backend.h](backend.h) | Private driver interface |
| [vfs.h](vfs.h) | Runtime reset and handle-cleanup hooks |

The RAM medium is fixed at 4096 bytes including metadata, with 3592 payload bytes. It uses no heap allocation. VFS handle state is separate. littlefs is an isolated test fixture and is not compiled into the release.

For paths, capacities, token lifetimes and errors, see [VFS](../../docs/VFS.md). Picture encoding is specified in [MBP formats](../../docs/MBP_FORMAT.md). Test commands are in [testing](../../docs/testing.md).

Original storage code uses the [project licence](../../LICENSE); vendored littlefs retains its upstream licence and notices.
