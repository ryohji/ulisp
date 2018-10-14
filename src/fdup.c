#include <stdio.h>
#include <unistd.h>

FILE* fdup(FILE* stream, const char* mode) {
    int fd = fileno(stream);
    return fdopen(dup(fd), mode);
}
