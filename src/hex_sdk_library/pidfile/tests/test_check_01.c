// HEX SDK

#include "./test.h"

int main()
{
    int mypid = getpid();

    FILE *f;
    HEX_TEST_FATAL((f = fopen(PIDFILE, "w")) != NULL);
    fprintf(f, "%d\n", mypid);
    fclose(f);

    // Should return my process id
    HEX_TEST(HexPidFileCheck(PIDFILE) == mypid);

    HEX_TEST_FATAL((f = fopen(PIDFILE, "w")) != NULL);
    fprintf(f, "999999\n");
    fclose(f);

    // Should return 0 because pid is not associated with a running process
    HEX_TEST(HexPidFileCheck(PIDFILE) == 0);

    unlink(PIDFILE);

    // Should return -1 because pid files does not exist
    HEX_TEST(HexPidFileCheck(PIDFILE) == -1);

    return HexTestResult;
}

