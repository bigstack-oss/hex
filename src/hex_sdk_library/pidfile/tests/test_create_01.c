// HEX SDK

#include "./test.h"

int main()
{
    unlink(PIDFILE);

    FILE *f;
    HEX_TEST_FATAL((f = fopen(PIDFILE, "w")) != NULL);
    fprintf(f, "99999\n");
    fclose(f);

    // Should overwrite pidfile with our process id since 99999 is not a running process
    // and return 0 for success
    HEX_TEST(HexPidFileCreate(PIDFILE) == 0);

    HEX_TEST_FATAL((f = fopen(PIDFILE, "r")) != NULL);
    int pid;
    HEX_TEST_FATAL(fscanf(f, "%d", &pid) == 1);
    fclose(f);

    HEX_TEST(pid == getpid());

    unlink(PIDFILE);
    return HexTestResult;
}

