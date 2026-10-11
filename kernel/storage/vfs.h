/* OS-only VFS lifecycle. Applications use the public SDK file API. */
#ifndef MB_VFS_H
#define MB_VFS_H
void mb_fs_reset(void);
void mb_fs_close_handles(void);
#endif
