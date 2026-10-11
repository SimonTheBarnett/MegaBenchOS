/**
 * @file tools/native/process.c
 * @brief Starts native child tools with Windows argv quoting, timeouts and job cleanup.
 *
 * Host-only C99 tooling; excluded from the console ROM.
 */
#include "host.h"
#include <wchar.h>
/* Native argv quoting and bounded child lifetime; never invoke a shell. */
static void argument(char *out, size_t *at, const char *s) {
    REQUIRE(*at + strlen(s) * 2 + 4 < 32768, "Process command exceeds Windows limit");
    out[(*at)++] = '"';
    while (*s) {
        size_t slashes = 0;
        while (*s == '\\') {
            slashes++;
            s++;
        }
        if (*s == '"' || !*s)
            slashes *= 2;
        while (slashes--)
            out[(*at)++] = '\\';
        if (*s == '"')
            out[(*at)++] = '\\';
        if (*s)
            out[(*at)++] = *s++;
    }
    out[(*at)++] = '"';
    out[(*at)++] = ' ';
    out[*at] = 0;
}
int mb_process(const char *exe, const char *const *args, size_t argc, unsigned timeout,
               const char *log) {
    char line[32768] = {0};
    size_t at = 0;
    argument(line, &at, exe);
    for (size_t i = 0; i < argc; i++)
        argument(line, &at, args[i]);
    wchar_t command[32768], directory[4096];
    REQUIRE(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, line, -1, command, 32768),
            "Invalid process UTF-8 arguments");
    REQUIRE(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, mb_root, -1, directory, 4096),
            "Invalid working directory");
    HANDLE output = INVALID_HANDLE_VALUE;
    STARTUPINFOW start = {0};
    PROCESS_INFORMATION process = {0};
    start.cb = sizeof start;
    if (log) {
        mb_write(log, "", 0);
        wchar_t path[4096];
        REQUIRE(MultiByteToWideChar(CP_UTF8, 0, log, -1, path, 4096), "Log path exceeds limit");
        SECURITY_ATTRIBUTES sa = {sizeof sa, NULL, TRUE};
        output = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, NULL);
        REQUIRE(output != INVALID_HANDLE_VALUE, "Cannot open process log");
        start.dwFlags = STARTF_USESTDHANDLES;
        start.hStdOutput = output;
        start.hStdError = output;
        start.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    }
    HANDLE job = CreateJobObjectW(NULL, NULL);
    REQUIRE(job, "Cannot create child job");
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {0};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    REQUIRE(SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof limits),
            "Cannot bound child lifetime");
    BOOL started =
        CreateProcessW(NULL, command, NULL, NULL, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED, NULL,
                       directory, &start, &process);
    if (!started) {
        DWORD error = GetLastError();
        if (output != INVALID_HANDLE_VALUE)
            CloseHandle(output);
        CloseHandle(job);
        REQUIRE(false, "Cannot launch %s (Windows error %lu)", exe, (unsigned long)error);
    }
    if (!AssignProcessToJobObject(job, process.hProcess)) {
        TerminateProcess(process.hProcess, 1);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        CloseHandle(job);
        if (output != INVALID_HANDLE_VALUE)
            CloseHandle(output);
        REQUIRE(false, "Cannot assign child process to bounded job");
    }
    ResumeThread(process.hThread);
    CloseHandle(process.hThread);
    DWORD wait = WaitForSingleObject(process.hProcess, timeout * 1000), code = 1;
    if (wait == WAIT_TIMEOUT) {
        TerminateJobObject(job, 1);
        WaitForSingleObject(process.hProcess, 3000);
    } else if (wait == WAIT_OBJECT_0)
        GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hProcess);
    CloseHandle(job);
    if (output != INVALID_HANDLE_VALUE)
        CloseHandle(output);
    REQUIRE(wait == WAIT_OBJECT_0, "Process %s exceeded %u seconds or wait failed", exe, timeout);
    return (int)code;
}
