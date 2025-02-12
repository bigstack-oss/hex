// HEX SDK

#include "./test.h"

int main()
{
    unlink(PIDFILE);

    // Should succeed
    HEX_TEST(HexPidFileCreate(PIDFILE) == 0);

    // Should fail since we couldn't create pid file
    fdopen_enabled = false;
    HEX_TEST(HexPidFileCreate(PIDFILE) == -1);

    // Should succeed since we've reenabled writes
    fdopen_enabled = true;
    HEX_TEST(HexPidFileCreate(PIDFILE) == 0);

    // Should fail since we couldn't create temp file
    mkstemp_enabled = false;
    HEX_TEST(HexPidFileCreate(PIDFILE) == -1);

    // Should succeed since we've reenabled mkstemp
    mkstemp_enabled = true;
    HEX_TEST(HexPidFileCreate(PIDFILE) == 0);

    // FIXME: Should fail since we couldn't create temp file
    //asprintf_enabled = false;
    //HEX_TEST(HexPidFileCreate(PIDFILE) == -1);

    // Should succeed since we've reenabled asprintf
    asprintf_enabled = true;
    HEX_TEST(HexPidFileCreate(PIDFILE) == 0);

    unlink(PIDFILE);
    return HexTestResult;
}

