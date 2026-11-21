#include "protocol.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int append_chunk(char **line, size_t *used, size_t *capacity,
                        const char *data, DWORD count) {
    if (*used > SIZE_MAX - count) return 0;
    size_t needed = *used + count;
    if (needed > *capacity) {
        size_t bigger = *capacity ? *capacity : CHUNK_SIZE;
        while (bigger < needed) {
            if (bigger > SIZE_MAX / 2) {
                bigger = needed;
                break;
            }
            bigger *= 2;
        }
        char *new_line = realloc(*line, bigger);
        if (!new_line) return 0;
        *line = new_line;
        *capacity = bigger;
    }
    memcpy(*line + *used, data, count);
    *used = needed;
    return 1;
}

static int signal_response(HANDLE response) {
    if (SetEvent(response)) return 1;
    fprintf(stderr, "Cannot signal parent (error %lu)\n", GetLastError());
    return 0;
}

int main(int argc, char **argv) {
    HANDLE mapping = NULL, request = NULL, response = NULL, parent = NULL;
    SharedBlock *shared = NULL;
    char *line = NULL;
    size_t used = 0, capacity = 0;
    unsigned long long line_number = 0;
    int result = 1;

    if (argc != 2) {
        fprintf(stderr, "Usage: child.exe <parent-pid>\n");
        return 1;
    }
    errno = 0;
    char *end;
    unsigned long value = strtoul(argv[1], &end, 10);
    if (errno == ERANGE || value == 0 || *end != '\0' || value > UINT32_MAX) {
        fprintf(stderr, "Invalid parent process ID\n");
        return 1;
    }

    mapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MAPPING_NAME);
    if (!mapping) {
        fprintf(stderr, "Cannot open file mapping (error %lu)\n", GetLastError());
        goto cleanup;
    }
    shared = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedBlock));
    if (!shared) {
        fprintf(stderr, "Cannot map shared memory (error %lu)\n", GetLastError());
        goto cleanup;
    }
    request = OpenEventA(SYNCHRONIZE, FALSE, REQUEST_NAME);
    response = OpenEventA(EVENT_MODIFY_STATE, FALSE, RESPONSE_NAME);
    parent = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)value);
    if (!request || !response || !parent) {
        fprintf(stderr, "Cannot open shared events or parent process (error %lu)\n", GetLastError());
        goto cleanup;
    }

    for (;;) {
        HANDLE waits[2] = {request, parent};
        DWORD state = WaitForMultipleObjects(2, waits, FALSE, INFINITE);
        if (state == WAIT_OBJECT_0 + 1) {
            fprintf(stderr, "Parent process ended unexpectedly\n");
            break;
        }
        if (state != WAIT_OBJECT_0) {
            fprintf(stderr, "Cannot wait for request (error %lu)\n", GetLastError());
            break;
        }

        shared->status = STATUS_OK;
        shared->message[0] = '\0';
        if (shared->command == CMD_CHUNK) {
            if (shared->count == 0 || shared->count > CHUNK_SIZE) {
                shared->status = STATUS_FATAL;
                snprintf(shared->message, MESSAGE_SIZE, "Invalid chunk size");
            } else if (!append_chunk(&line, &used, &capacity,
                                     shared->data, shared->count)) {
                shared->status = STATUS_FATAL;
                snprintf(shared->message, MESSAGE_SIZE, "Not enough memory for input line");
            }
        } else if (shared->command == CMD_END_LINE) {
            if (shared->count != 0) {
                shared->status = STATUS_FATAL;
                snprintf(shared->message, MESSAGE_SIZE, "Invalid end-of-line request");
            } else {
                ++line_number;
                if (used && (line[used - 1] == '.' || line[used - 1] == ';')) {
                    if (fwrite(line, 1, used, stdout) != used ||
                        putchar('\n') == EOF || fflush(stdout) == EOF) {
                        shared->status = STATUS_FATAL;
                        snprintf(shared->message, MESSAGE_SIZE, "Cannot write output file");
                    }
                } else {
                    shared->status = STATUS_INVALID;
                    snprintf(shared->message, MESSAGE_SIZE,
                             "Line %llu: expected '.' or ';' at the end.\n", line_number);
                }
                used = 0;
            }
        } else if (shared->command == CMD_FINISH) {
            if (shared->count != 0 || used != 0 || fflush(stdout) == EOF) {
                shared->status = STATUS_FATAL;
                snprintf(shared->message, MESSAGE_SIZE, "Cannot finish output cleanly");
            }
        } else {
            shared->status = STATUS_FATAL;
            snprintf(shared->message, MESSAGE_SIZE, "Unknown request command");
        }

        int fatal = shared->status == STATUS_FATAL;
        int finish = shared->command == CMD_FINISH;
        if (!signal_response(response)) break;
        if (fatal || finish) {
            result = fatal ? 1 : 0;
            break;
        }
    }

cleanup:
    free(line);
    if (shared) UnmapViewOfFile(shared);
    if (mapping) CloseHandle(mapping);
    if (request) CloseHandle(request);
    if (response) CloseHandle(response);
    if (parent) CloseHandle(parent);
    return result;
}
