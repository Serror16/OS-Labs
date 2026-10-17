#include <stdio.h>
#include <stdlib.h>


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

int main(void) {
    size_t length;
    unsigned long long line_number = 0;
    int read_error;
    char *line;

    while ((line = read_line(&length, &read_error)) != NULL) {
        ++line_number;
        if (length && (line[length - 1] == '.' || line[length - 1] == ';')) {
            if (fwrite(line, 1, length, stdout) != length || putchar('\n') == EOF) {
                free(line);
                return 1;
            }
        } else if (fprintf(stderr, "Line %llu: expected '.' or ';' at the end.\n", line_number) < 0) {
            free(line);
            return 1;
        }
        free(line);
    }
    if (read_error || fflush(stdout) == EOF || fflush(stderr) == EOF)
        return 1;
    return 0;
}
