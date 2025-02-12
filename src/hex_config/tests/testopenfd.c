
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
    int openmax = sysconf(_SC_OPEN_MAX);
    if (openmax < 0)
        openmax = 256; // Guess

    fprintf(stderr, "openmax = %d\n", openmax);

    struct stat buf;
    int fd;
    for (fd = 0; fd < openmax; ++fd) {
        switch (fd) {
        case STDIN_FILENO:
        case STDOUT_FILENO:
        case STDERR_FILENO:
            break;
        default:
            // Return failure if fd is open
            if (fstat(fd, &buf) == 0) {
                fprintf(stderr, "fd %d is open\n", fd);
                return 1;
            }
        }
    }

    return 0;
}
