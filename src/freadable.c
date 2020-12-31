#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

bool freadable(FILE* fp) {
    fd_set fds;
    struct timeval timeout = {0};
    FD_ZERO(&fds);
    FD_SET(fileno(fp), &fds);

    return select(fileno(fp) + 1, &fds, NULL, NULL, &timeout);
}
