# Files and storage

The public VFS provides one interface for session RAM files and immutable ROM resources. See the [file API reference](api/core.md#fileopen) for parameters, return values and errors.

## Drives and paths

| Drive | Capabilities |
| --- | --- |
| `RAM:` | Writable, volatile, fixed directories, offset reads, same-directory rename and atomic whole-file replacement |
| `ROM:` | Read-only bundled files: `Help/readme.txt` and `Resources/palette.txt` |
| `SAVE:` / `SD:` | Unavailable; descriptors have zero capabilities and operations return `MB_NOT_SUPPORTED` |

Use `mb_fs_drive(index, 1, out)` to enumerate drives and `mb_fs_capacity(drive, 1, out)` to query capabilities. Byte counts distinguish file payload from reserved storage. ROM free space is zero. Versioned queries reject versions other than 1 before accessing outputs.

Paths are case-insensitive ASCII, at most 63 bytes excluding NUL, with forward slashes and a drive prefix. Drive queries use `RAM:`; directory queries use paths such as `RAM:/Documents/`. Trailing directory slashes are accepted. Repeated slashes, backslashes, control/non-ASCII characters and `.`/`..` components are invalid. Unknown drives return `MB_NOT_FOUND`.

RAM files use these fixed directories:

- `RAM:/Documents/name.txt`
- `RAM:/Pictures/name.mbp`
- `RAM:/Music/name.vgm` or `RAM:/Music/name.xgc`

Basenames contain 1-15 ASCII letters, digits, underscores or hyphens. There is no mkdir, nested directory creation or cross-directory move. Cross-drive rename returns `MB_NOT_SUPPORTED`. Music associations open MegaPlayer for inspection; external playback is unsupported.

## Capacity and enumeration

RAM Disk reserves 4096 bytes including metadata and provides **3592 shared payload bytes**, with at most sixteen files. Files survive app exit but disappear on Restart, reset or power-off. Editor workspaces are separate from disk capacity. Query free space before saving and handle capacity errors.

`mb_dir_list_path` returns copied file/directory metadata. Indices are dense and may change after deletion; enumeration ends with `MB_NOT_FOUND`. Stat describes files. Legacy `mb_fs_info` and numeric `mb_dir_list` remain RAM adapters; numeric listing supports Documents and Pictures only.

## Handles and data

Six opaque handles are shared across all drives. CREATE requires WRITE; opening never truncates. One RAM writer excludes other handles to the file. Handle exhaustion is checked before backend conflicts. Handles expire on close, app exit or reset; tokens never wrap. After 65535 lifetime opens, reboot is required.

Reads use explicit offsets into caller-owned buffers. EOF returns success and zero bytes; an offset beyond EOF is invalid. No seek cursor is retained. Writes replace a whole file, with validation before mutation; errors preserve existing data. Empty files are valid. Open files cannot be renamed or deleted.

Check read, write and close results. Close releases ownership even if the backend reports failure. `MB_IO` represents backend I/O failure; corruption returns `MB_CORRUPT` without copying data. Keep unsaved edits on errors, and remove a newly created empty file if its initial save fails.

Filesystem calls are foreground-only. Inputs are borrowed only during the call, metadata is copied, and drive name/backend strings remain immutable for the boot. No heap is allocated by this interface.

## Text and pictures

Text files use ASCII with LF newlines; MegaPad's document limit is 1024 bytes. MegaPaint saves versioned, lossless [MBP2 pictures](MBP_FORMAT.md) and reads MBP1. A picture may be editable but too large for the remaining RAM Disk payload; failed saves do not truncate it.

The private implementation is described in [storage sources](../kernel/storage/README.md). Apps must use the public VFS rather than backend headers. littlefs is an isolated test fixture, not a release filesystem. See [testing](testing.md).
