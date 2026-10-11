/**
 * @file tools/native/text.c
 * @brief Builds bounded text output for native SDK and artwork generators.
 *
 * Host-only C99 tooling; excluded from the console ROM.
 */
#include "text.h"
void text_format_c(TextBuffer *text) {
    const char *compiler = getenv("MB_HOST_CC");
    if (!compiler || !*compiler)
        compiler = mb_fmt("%s/LLVM/bin/clang.exe", getenv("ProgramFiles"));
    char *directory = mb_copy(compiler);
    char *slash = strrchr(directory, '/'), *back = strrchr(directory, '\\');
    if (!slash || (back && back > slash))
        slash = back;
    REQUIRE(slash, "Configure an absolute LLVM compiler path");
    *slash = 0;
    const char *tool = mb_fmt("%s/clang-format.exe", directory);
    const char *temporary =
        mb_path(mb_fmt("out/generation/sdk-%lu.h", (unsigned long)GetCurrentProcessId()));
    mb_write(temporary, text->data, text->length);
    const char *arguments[] = {"-i", temporary};
    REQUIRE(mb_process(tool, arguments, 2, 120, mb_path("reports/sdk-format.log")) == 0,
            "Generated SDK formatting failed");
    size_t size;
    char *formatted = (char *)mb_read(temporary, &size);
    free(directory);
    free(text->data);
    *text = (TextBuffer){formatted, size, size + 1};
    /* Exact project-owned temporary filename; never remove user-supplied paths. */
    wchar_t path[4096];
    REQUIRE(MultiByteToWideChar(CP_UTF8, 0, temporary, -1, path, 4096),
            "Temporary path exceeds bound");
    REQUIRE(DeleteFileW(path), "Cannot remove generated SDK temporary file");
}
void text_append(TextBuffer *text, const char *format, ...) {
    va_list args, copy;
    va_start(args, format);
    va_copy(copy, args);
    int count = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    REQUIRE(count >= 0 && text->length + (size_t)count < 16 * 1024 * 1024,
            "Generated text exceeds 16 MiB");
    size_t needed = text->length + (size_t)count + 1;
    if (needed > text->capacity) {
        size_t capacity = text->capacity ? text->capacity : 4096;
        while (capacity < needed)
            capacity *= 2;
        text->data = realloc(text->data, capacity);
        REQUIRE(text->data, "Text allocation failed");
        text->capacity = capacity;
    }
    vsnprintf(text->data + text->length, text->capacity - text->length, format, args);
    va_end(args);
    text->length += (size_t)count;
}
