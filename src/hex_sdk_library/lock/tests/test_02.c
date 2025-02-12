// HEX SDK

#include <time.h>
#include <hex/test.h>
#include <hex/lock.h>

#include "./test.h"

int main()
{
    // Lock file should already exist
    HEX_TEST_FATAL(access(LOCKFILE, F_OK) == 0);

    // Acquire lock - should succeed after 10 seconds
    HEX_TEST_FATAL(HexLockAcquire(LOCKFILE, 30));

    // Lock file should exist and contain my pid
    HEX_TEST_FATAL(access(LOCKFILE, F_OK) == 0);
    FILE *f;
    HEX_TEST_FATAL((f = fopen(LOCKFILE, "r")) != NULL);
    int pid;
    HEX_TEST_FATAL(fscanf(f, "%d", &pid) == 1);
    fclose(f);
    HEX_TEST(pid == getpid());

    // Release lock
    HexLockRelease(LOCKFILE);

    // Lock file should no longer exist
    HEX_TEST(access(LOCKFILE, F_OK) < 0);

    return HexTestResult;
}

