#include "options.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char *program)
{
    fprintf(stderr,
            "Usage: %s --input FILE --clusters K --threads P\n"
            "Input file: N on the first line, then N pairs of coordinates x y.\n",
            program);
}

static int positive_number(const char *text, size_t *number)
{
    char *end;
    unsigned long long value;
    if (!*text || *text == '-') return 0;
    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno || *end || value == 0 || value > SIZE_MAX) return 0;
    *number = (size_t)value;
    return 1;
}

int parse_options(int argc, char **argv, Options *options)
{
    int i;
    memset(options, 0, sizeof(*options));
    if (argc != 7) return 0;
    for (i = 1; i < argc; i += 2) {
        const char *key = argv[i];
        const char *value = argv[i + 1];
        if (strncmp(value, "--", 2) == 0) return 0;
        if (strcmp(key, "--input") == 0) options->input_path = value;
        else if (strcmp(key, "--clusters") == 0) {
            if (!positive_number(value, &options->clusters)) return 0;
        } else if (strcmp(key, "--threads") == 0) {
            if (!positive_number(value, &options->threads)) return 0;
        } else return 0;
    }
    return options->input_path && options->clusters && options->threads;
}
