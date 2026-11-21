#ifndef OS_LAB4_PROTOCOL_H
#define OS_LAB4_PROTOCOL_H

#include <windows.h>

#define CHUNK_SIZE 4096
#define MESSAGE_SIZE 128

#define MAPPING_NAME "Local\\OSLab4_mapping"
#define REQUEST_NAME "Local\\OSLab4_request"
#define RESPONSE_NAME "Local\\OSLab4_response"

enum { CMD_CHUNK = 1, CMD_END_LINE = 2, CMD_FINISH = 3 };
enum { STATUS_OK = 0, STATUS_INVALID = 1, STATUS_FATAL = 2 };

typedef struct {
    DWORD command;
    DWORD count;
    DWORD status;
    char data[CHUNK_SIZE];
    char message[MESSAGE_SIZE];
} SharedBlock;

#endif
