// HEX SDK

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <hex/lock.h>

#include "./test.h"

int
main(int argc, char **argv)
{
    if (argc != 2)
        return 1;

    if (!HexLockAcquire(LOCKFILE, 1))
        return 1;

    // Sleep for 'n' seconds
    sleep(atoi(argv[1]));

    HexLockRelease(LOCKFILE);
    return 0;
}

