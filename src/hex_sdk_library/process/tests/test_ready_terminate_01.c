// HEX SDK

#include <hex/process.h>
#include <hex/test.h>
#include <hex/pidfile.h>

int main() {
    int status;
    pid_t testpid;
    // TEST - call a daemon process, then try to kill it

    //// Make sure the pid file isn't there
    unlink("./daemontest.pid");

    //// start the test daemon (should create a pid file)
    status = HexSpawn(0, "./daemontest", ZEROCHAR_PTR);
    HEX_TEST(WIFEXITED(status) && WEXITSTATUS(status) == 0);

    // NOTE: Several retries to account for disk pressures on build servers
    HEX_TEST(HexProcPidReady(15, "daemontest.pid") > 0);

    //// kill the dang thing
    testpid = HexPidFileRead("daemontest.pid");
    HEX_TEST_FATAL(testpid > 0);
    HEX_TEST(HexTerminate(testpid) == 0);

    sleep(1);

    //// should return 0 b/c it's no longer running
    HEX_TEST(HexPidFileCheck("daemontest.pid") == 0);

    unlink("./daemontest.pid");

    return HexTestResult;
}

