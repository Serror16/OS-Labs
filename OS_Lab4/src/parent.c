#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

static char *read_line(size_t *length, int *error) {
    size_t size = 128, used = 0;
    char *line = malloc(size);
    int ch;
    *error = 0;
    if (!line) {
        *error = 1;
        return NULL;
    }
    while ((ch = getchar()) != EOF && ch != '\n') {
        if (used + 1 == size) {
            size_t bigger = size * 2;
            char *new_line = bigger > size ? realloc(line, bigger) : NULL;
            if (!new_line) {
                free(line);
                *error = 1;
                return NULL;
            }
            line = new_line;
            size = bigger;
        }
        line[used++] = (char)ch;
    }
    if (ferror(stdin)) {
        free(line);
        *error = 1;
        return NULL;
    }
    if (ch == EOF && used == 0) {
        free(line);
        return NULL;
    }
    if (used && line[used - 1] == '\r') --used;
    line[used] = '\0';
    *length = used;
    return line;
}

static int exchange(HANDLE request, HANDLE response, HANDLE child_process,
                    SharedBlock *shared, DWORD command,
                    const char *data, DWORD count) {
    HANDLE waits[2] = {response, child_process};
    DWORD state;

    if (count > CHUNK_SIZE) return 0;
    shared->command = command;
    shared->count = count;
    shared->status = STATUS_OK;
    shared->message[0] = '\0';
    if (count) memcpy(shared->data, data, count);

    if (!SetEvent(request)) {
        fprintf(stderr, "Cannot signal child (error %lu)\n", GetLastError());
        return 0;
    }
    state = WaitForMultipleObjects(2, waits, FALSE, INFINITE);
    if (state == WAIT_OBJECT_0) return 1;
    if (state == WAIT_OBJECT_0 + 1)
        fprintf(stderr, "child.exe ended before replying\n");
    else
        fprintf(stderr, "Cannot wait for child reply (error %lu)\n", GetLastError());
    return 0;
}

int main(void) {
    HANDLE file = INVALID_HANDLE_VALUE;
    HANDLE mapping = NULL, request = NULL, response = NULL;
    HANDLE child_input = NULL, child_error = NULL;
    SharedBlock *shared = NULL;
    PROCESS_INFORMATION child = {0};
    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
    STARTUPINFOW start = {0};
    UINT old_codepage = GetConsoleCP();
    DWORD child_code;
    int result = 1, read_error = 0, communication_failed = 0;
    size_t length;
    char *line = NULL;
    wchar_t *wide_name = NULL;

    if (old_codepage && !SetConsoleCP(CP_UTF8)) {
        fprintf(stderr, "Cannot set console input to UTF-8\n");
        goto cleanup;
    }
    printf("Output file name: ");
    if (fflush(stdout) == EOF) goto cleanup;
    line = read_line(&length, &read_error);
    if (!line || length == 0) {
        fprintf(stderr, "Cannot read a nonempty output file name\n");
        goto cleanup;
    }

    int name_size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                        line, -1, NULL, 0);
    if (!name_size || !(wide_name = malloc((size_t)name_size * sizeof(wchar_t))) ||
        !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                             line, -1, wide_name, name_size)) {
        fprintf(stderr, "Invalid UTF-8 file name or not enough memory\n");
        goto cleanup;
    }
    file = CreateFileW(wide_name, GENERIC_WRITE, FILE_SHARE_READ, &sa,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Cannot open output file (error %lu)\n", GetLastError());
        goto cleanup;
    }
    free(line); line = NULL;
    free(wide_name); wide_name = NULL;

    DWORD parent_pid = GetCurrentProcessId();

    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                 0, sizeof(SharedBlock), MAPPING_NAME);
    if (!mapping) {
        fprintf(stderr, "Cannot create file mapping (error %lu)\n", GetLastError());
        goto cleanup;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        fprintf(stderr, "File mapping name is already in use\n");
        goto cleanup;
    }
    shared = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedBlock));
    if (!shared) {
        fprintf(stderr, "Cannot map shared memory (error %lu)\n", GetLastError());
        goto cleanup;
    }
    ZeroMemory(shared, sizeof(SharedBlock));

    request = CreateEventA(NULL, FALSE, FALSE, REQUEST_NAME);
    if (!request) {
        fprintf(stderr, "Cannot create request event (error %lu)\n", GetLastError());
        goto cleanup;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        fprintf(stderr, "Request event name is already in use\n");
        goto cleanup;
    }
    response = CreateEventA(NULL, FALSE, FALSE, RESPONSE_NAME);
    if (!response) {
        fprintf(stderr, "Cannot create response event (error %lu)\n", GetLastError());
        goto cleanup;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        fprintf(stderr, "Response event name is already in use\n");
        goto cleanup;
    }

    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE error_output = GetStdHandle(STD_ERROR_HANDLE);
    if (!input || input == INVALID_HANDLE_VALUE ||
        !error_output || error_output == INVALID_HANDLE_VALUE ||
        !DuplicateHandle(GetCurrentProcess(), input, GetCurrentProcess(),
                         &child_input, 0, TRUE, DUPLICATE_SAME_ACCESS) ||
        !DuplicateHandle(GetCurrentProcess(), error_output, GetCurrentProcess(),
                         &child_error, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
        fprintf(stderr, "Cannot prepare child standard handles (error %lu)\n", GetLastError());
        goto cleanup;
    }

    start.cb = sizeof(start);
    start.dwFlags = STARTF_USESTDHANDLES;
    start.hStdInput = child_input;
    start.hStdOutput = file;
    start.hStdError = child_error;
    wchar_t command[64];
    if (swprintf(command, sizeof(command) / sizeof(command[0]),
                 L"child.exe %lu", parent_pid) < 0 ||
        !CreateProcessW(NULL, command, NULL, NULL, TRUE, 0,
                        NULL, NULL, &start, &child)) {
        fprintf(stderr, "Cannot start child.exe (error %lu)\n", GetLastError());
        goto cleanup;
    }
    CloseHandle(child_input); child_input = NULL;
    CloseHandle(child_error); child_error = NULL;
    CloseHandle(file); file = INVALID_HANDLE_VALUE;

    printf("Enter lines; Ctrl+Z then Enter to finish.\n");
    if (fflush(stdout) == EOF) goto finish_child;

    while ((line = read_line(&length, &read_error)) != NULL) {
        size_t offset = 0;
        while (offset < length) {
            size_t remaining = length - offset;
            DWORD part = (DWORD)(remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining);
            if (!exchange(request, response, child.hProcess, shared,
                          CMD_CHUNK, line + offset, part)) {
                communication_failed = 1;
                goto wait_child;
            }
            if (shared->status != STATUS_OK) {
                fprintf(stderr, "Child error: %s\n", shared->message);
                communication_failed = 1;
                goto wait_child;
            }
            offset += part;
        }
        if (!exchange(request, response, child.hProcess, shared,
                      CMD_END_LINE, NULL, 0)) {
            communication_failed = 1;
            goto wait_child;
        }
        if (shared->status == STATUS_INVALID) {
            if (fputs(shared->message, stdout) == EOF || fflush(stdout) == EOF)
                goto finish_child;
        } else if (shared->status != STATUS_OK) {
            fprintf(stderr, "Child error: %s\n", shared->message);
            communication_failed = 1;
            goto wait_child;
        }
        free(line); line = NULL;
    }
    if (read_error)
        fprintf(stderr, "Input error or not enough memory\n");
    else
        result = 0;

finish_child:
    if (!exchange(request, response, child.hProcess, shared,
                  CMD_FINISH, NULL, 0)) {
        result = 1;
        communication_failed = 1;
    } else if (shared->status != STATUS_OK) {
        fprintf(stderr, "Child error: %s\n", shared->message);
        result = 1;
    }

wait_child:
    free(line); line = NULL;
    if (communication_failed) {
        DWORD state = WaitForSingleObject(child.hProcess, 1000);
        if (state == WAIT_TIMEOUT) {
            if (!TerminateProcess(child.hProcess, 1)) {
                fprintf(stderr, "Cannot stop child.exe (error %lu)\n", GetLastError());
                goto cleanup;
            }
        } else if (state == WAIT_FAILED) {
            fprintf(stderr, "Cannot wait for child.exe (error %lu)\n", GetLastError());
            goto cleanup;
        }
    }
    if (WaitForSingleObject(child.hProcess, INFINITE) != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(child.hProcess, &child_code) || child_code != 0) {
        fprintf(stderr, "child.exe ended with an error\n");
        result = 1;
    }

cleanup:
    free(line);
    free(wide_name);
    if (shared) UnmapViewOfFile(shared);
    if (mapping) CloseHandle(mapping);
    if (request) CloseHandle(request);
    if (response) CloseHandle(response);
    if (child_input) CloseHandle(child_input);
    if (child_error) CloseHandle(child_error);
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (child.hThread) CloseHandle(child.hThread);
    if (child.hProcess) CloseHandle(child.hProcess);
    if (old_codepage) SetConsoleCP(old_codepage);
    return result;
}
