#ifndef MB_NATIVE_TEXT_H
#define MB_NATIVE_TEXT_H
#include "host.h"
/** Growable host text output, capped at 16 MiB. The caller frees data. */
typedef struct {
    char *data;
    size_t length, capacity;
} TextBuffer;
void text_append(TextBuffer *text, const char *format, ...);
/** Format generated C declarations/comments with the installed LLVM formatter.
 * Replaces the caller-owned buffer; temporary files stay under project out/.
 */
void text_format_c(TextBuffer *text);
#endif
