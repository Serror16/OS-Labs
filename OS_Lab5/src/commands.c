#include "commands.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static char *read_line(void)
{
    size_t capacity = 128;
    size_t length = 0;
    char *line = malloc(capacity);
    if (line == NULL) {
        return NULL;
    }

    int character;
    while ((character = fgetc(stdin)) != EOF && character != '\n') {
        if (length + 1 >= capacity) {
            if (capacity > SIZE_MAX / 2) {
                free(line);
                return NULL;
            }
            capacity *= 2;
            char *larger = realloc(line, capacity);
            if (larger == NULL) {
                free(line);
                return NULL;
            }
            line = larger;
        }
        line[length++] = (char)character;
    }

    if (character == EOF && length == 0) {
        free(line);
        return NULL;
    }
    line[length] = '\0';
    return line;
}

/* 1: integer read, 0: end of line, -1: malformed or out of range. */
static int next_int(char **cursor, int *value)
{
    while (isspace((unsigned char)**cursor)) {
        ++*cursor;
    }
    if (**cursor == '\0') {
        return 0;
    }

    errno = 0;
    char *end;
    long parsed = strtol(*cursor, &end, 10);
    if (end == *cursor || errno == ERANGE || parsed < INT_MIN || parsed > INT_MAX ||
        (*end != '\0' && !isspace((unsigned char)*end))) {
        return -1;
    }

    *cursor = end;
    *value = (int)parsed;
    return 1;
}

static int no_more_arguments(char **cursor)
{
    int unused;
    return next_int(cursor, &unused) == 0;
}

static void run_gcf(char *cursor, const Lab5Functions *functions)
{
    int a;
    int b;
    if (next_int(&cursor, &a) != 1 || next_int(&cursor, &b) != 1 ||
        !no_more_arguments(&cursor) || a <= 0 || b <= 0) {
        puts("Usage: 1 A B (A and B must be positive integers).");
        return;
    }

    printf("GCF(%d, %d) = %d\n", a, b, functions->gcf(a, b));
}

static void run_sort(char *cursor, const Lab5Functions *functions)
{
    int count;
    if (next_int(&cursor, &count) != 1 || count < 0) {
        puts("Usage: 2 N x1 ... xN (N must be nonnegative).");
        return;
    }
    if ((size_t)count > SIZE_MAX / sizeof(int) - 1) {
        puts("Array is too large.");
        return;
    }

    int *array = malloc(((size_t)count + 1) * sizeof(*array));
    if (array == NULL) {
        puts("Could not allocate the array.");
        return;
    }
    array[0] = count;
    for (size_t i = 1; i <= (size_t)count; ++i) {
        if (next_int(&cursor, &array[i]) != 1) {
            puts("Usage: 2 N x1 ... xN (exactly N values are required).");
            free(array);
            return;
        }
    }
    if (!no_more_arguments(&cursor)) {
        puts("Usage: 2 N x1 ... xN (exactly N values are required).");
        free(array);
        return;
    }

    int *result = functions->sort(array);
    if (result != array || array[0] != count) {
        puts("The library violated the Sort contract.");
        free(array);
        return;
    }

    printf("Sorted:");
    for (size_t i = 1; i <= (size_t)count; ++i) {
        printf(" %d", array[i]);
    }
    putchar('\n');
    free(array);
}

int lab5_run(Lab5Functions *functions, Lab5SwitchFn switch_function, void *context)
{
    if (functions == NULL || functions->gcf == NULL || functions->sort == NULL) {
        return 1;
    }

    puts("Commands: 1 A B | 2 N x1 ... xN | 0 (switch, runtime program only)");
    puts("End input with Ctrl+Z then Enter on Windows.");
    for (;;) {
        fputs("> ", stdout);
        fflush(stdout);
        char *line = read_line();
        if (line == NULL) {
            if (!feof(stdin)) {
                fputs("Input error.\n", stderr);
                return 1;
            }
            putchar('\n');
            return 0;
        }

        char *cursor = line;
        int command;
        int result = next_int(&cursor, &command);
        if (result == 1) {
            switch (command) {
                case 0:
                    if (!no_more_arguments(&cursor)) {
                        puts("Usage: 0");
                    } else if (switch_function == NULL) {
                        puts("Switching is available only in the runtime program.");
                    } else {
                        switch_function(functions, context);
                    }
                    break;
                case 1:
                    run_gcf(cursor, functions);
                    break;
                case 2:
                    run_sort(cursor, functions);
                    break;
                default:
                    puts("Unknown command. Use 0, 1 or 2.");
                    break;
            }
        } else if (result < 0) {
            puts("Command must be an integer: 0, 1 or 2.");
        }
        free(line);
    }
}
