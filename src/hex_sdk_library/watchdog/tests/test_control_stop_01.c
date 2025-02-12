// HEX SDK

#include <hex/daemon.h>
#include <hex/process.h>
#include <hex/pidfile.h>
#include <hex/test.h>

#include "test.h"

int 
main() 
{
    // Should return false because program name is NULL
    HEX_TEST(HexDaemonStop(NULL) == -1);

    // Should return true because program is already stopped
    HEX_TEST(HexPidFileCheck(PID_FILE) <= 0);
    HEX_TEST(HexDaemonStop(PROGRAM) == 0);

    // Should return true and stop program
    HEX_TEST(system("./daemontest") == 0);
    WaitForStart();
    HEX_TEST(HexDaemonStop(PROGRAM) == 0);
    WaitForStop();

    return HexTestResult;
}

