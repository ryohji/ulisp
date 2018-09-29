#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

bool freadable(FILE* fp) {
    fd_set fds;
    struct timeval timeout = {0};
    FD_ZERO(&fds);
    FD_SET(fileno(stdin), &fds);
    
    return select(fileno(stdin) + 1, &fds, NULL, NULL, &timeout);
}
