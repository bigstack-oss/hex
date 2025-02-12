// HEX SDK

#include "./test.h"

int main()
{
    unlink(PIDFILE);

    FILE *f;
    HEX_TEST_FATAL((f = fopen(PIDFILE, "w")) != NULL);
    fprintf(f, "1\n");
    fclose(f);

    // Should return pid of 1 since pidfile contains pid of a running process (init)
    HEX_TEST(HexPidFileCreate(PIDFILE) == 1);

    unlink(PIDFILE);
    return HexTestResult;
}

