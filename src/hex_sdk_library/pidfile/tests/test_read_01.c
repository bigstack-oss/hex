// HEX SDK

#include "./test.h"

int main()
{
    unlink(PIDFILE);
    HEX_TEST(HexPidFileCreate(PIDFILE) == 0);

    // Should return our process id from the pidfile
    HEX_TEST(HexPidFileRead(PIDFILE) == getpid());

    // Should return -1 because pid file does not exist
    unlink(PIDFILE);
    HEX_TEST(HexPidFileRead(PIDFILE) == -1);

    return HexTestResult;
}

