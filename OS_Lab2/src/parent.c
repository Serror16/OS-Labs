#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
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


static int write_all(HANDLE handle, const char *data, size_t length) {
    while (length) {
        DWORD part = length > 65536 ? 65536 : (DWORD)length;
        DWORD written;
        if (!WriteFile(handle, data, part, &written, NULL) || written == 0)
            return 0;
        data += written;
        length -= written;
    }
    return 1;
}


static DWORD WINAPI print_errors(LPVOID argument) {
    HANDLE pipe_read = (HANDLE)argument;
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    char buffer[4096];
    DWORD count, error = 0;
    for (;;) {
        if (!ReadFile(pipe_read, buffer, sizeof(buffer), &count, NULL)) {
            error = GetLastError();
            if (error == ERROR_BROKEN_PIPE) error = 0;
            break;
        }
        if (count == 0) break;
        if (!write_all(output, buffer, count)) {
            error = GetLastError();
            break;
        }
    }
    CloseHandle(pipe_read);
    return error;
}

int main(void) {
    HANDLE pipe1_read = NULL, pipe1_write = NULL;
    HANDLE pipe2_read = NULL, pipe2_write = NULL;
    HANDLE file = INVALID_HANDLE_VALUE, reader_thread = NULL;
    PROCESS_INFORMATION child = {0};
    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
    STARTUPINFOW start = {0};
    DWORD child_code, reader_code;
    UINT old_codepage = GetConsoleCP();
    int result = 1, read_error = 0;
    size_t length;
    char *line = NULL;
    wchar_t *wide_name = NULL;


    if (old_codepage && !SetConsoleCP(CP_UTF8)) {
        fprintf(stderr, "Cannot set console input to UTF-8\n");
        goto cleanup;
    }
    printf("Output file name: ");
    fflush(stdout);
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

    if (!CreatePipe(&pipe1_read, &pipe1_write, &sa, 0) ||
        !CreatePipe(&pipe2_read, &pipe2_write, &sa, 0) ||
        !SetHandleInformation(pipe1_write, HANDLE_FLAG_INHERIT, 0) ||
        !SetHandleInformation(pipe2_read, HANDLE_FLAG_INHERIT, 0)) {
        fprintf(stderr, "Cannot prepare pipes (error %lu)\n", GetLastError());
        goto cleanup;
    }

    start.cb = sizeof(start);
    start.dwFlags = STARTF_USESTDHANDLES;
    start.hStdInput = pipe1_read;
    start.hStdOutput = file;
    start.hStdError = pipe2_write;
    wchar_t command[] = L"child.exe";
    if (!CreateProcessW(NULL, command, NULL, NULL, TRUE, 0,
                        NULL, NULL, &start, &child)) {
        fprintf(stderr, "Cannot start child.exe (error %lu)\n", GetLastError());
        goto cleanup;
    }

    CloseHandle(pipe1_read); pipe1_read = NULL;
    CloseHandle(pipe2_write); pipe2_write = NULL;
    CloseHandle(file); file = INVALID_HANDLE_VALUE;

    reader_thread = CreateThread(NULL, 0, print_errors, pipe2_read, 0, NULL);
    if (!reader_thread) {
        fprintf(stderr, "Cannot start error reader (error %lu)\n", GetLastError());
        CloseHandle(pipe1_write); pipe1_write = NULL;
        print_errors(pipe2_read); pipe2_read = NULL;
        goto wait_child;
    }
    pipe2_read = NULL;

    printf("Enter lines; Ctrl+Z then Enter to finish.\n");
    fflush(stdout);
    while ((line = read_line(&length, &read_error)) != NULL) {
        if (!write_all(pipe1_write, line, length) ||
            !write_all(pipe1_write, "\n", 1)) {
            fprintf(stderr, "Cannot send a line to child.exe\n");
            free(line); line = NULL;
            goto wait_child;
        }
        free(line); line = NULL;
    }
    if (read_error) {
        fprintf(stderr, "Input error or not enough memory\n");
        goto wait_child;
    }
    result = 0;

wait_child:
    if (pipe1_write) { CloseHandle(pipe1_write); pipe1_write = NULL; }
    if (WaitForSingleObject(child.hProcess, INFINITE) != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(child.hProcess, &child_code) || child_code != 0) {
        fprintf(stderr, "child.exe ended with an error\n");
        result = 1;
    }
    if (reader_thread) {
        if (WaitForSingleObject(reader_thread, INFINITE) != WAIT_OBJECT_0 ||
            !GetExitCodeThread(reader_thread, &reader_code) || reader_code != 0) {
            fprintf(stderr, "Cannot read child errors\n");
            result = 1;
        }
    }

cleanup:
    free(line);
    free(wide_name);
    if (pipe1_read) CloseHandle(pipe1_read);
    if (pipe1_write) CloseHandle(pipe1_write);
    if (pipe2_read) CloseHandle(pipe2_read);
    if (pipe2_write) CloseHandle(pipe2_write);
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (reader_thread) CloseHandle(reader_thread);
    if (child.hThread) CloseHandle(child.hThread);
    if (child.hProcess) CloseHandle(child.hProcess);
    if (old_codepage) SetConsoleCP(old_codepage);
    return result;
}
