// HEX SDK

#include <hex/daemon.h>
#include <hex/process.h>
#include <hex/pidfile.h>
#include <hex/test.h>

#include "test.h"

int 
main() 
{
    pid_t pid;

    // Should return false because program name is NULL
    HEX_TEST_FATAL(HexDaemonStart(NULL, NULL, 0) == -1);

    // Should return true and start program
    HEX_TEST_FATAL(HexDaemonStart(PROGRAM, NULL, 0) == 0);
    WaitForStart();

    // Cmd line file should contain "daemontest"
    HEX_TEST_FATAL(HexSystemF(0, "grep '^daemontest$' %s", CMD_LINE_FILE) == 0);

    // Should return true because program is already started
    // Pid should not change, SIGHUP signal should be sent
    pid = HexPidFileRead(PID_FILE);
    HEX_TEST_FATAL(pid > 0);
    HEX_TEST_FATAL(access(SIGHUP_TEST_FILE, F_OK) != 0);
    HEX_TEST_FATAL(HexDaemonStart(PROGRAM, NULL, 0) == 0);
    WaitForSigHupTestFile();
    unlink(SIGHUP_TEST_FILE);
    HEX_TEST_FATAL(HexPidFileRead(PID_FILE) == pid);

    // Cmd line file should contain "./daemontest"
    HEX_TEST_FATAL(HexSystemF(0, "grep '^daemontest$' %s", CMD_LINE_FILE) == 0);

    // Should return true because program was restarted (args have not changed, but told not to send SIGHUP)
    // Pid should change, SIGHUP signal should not be sent
    pid = HexPidFileRead(PID_FILE);
    HEX_TEST_FATAL(pid > 0);
    HEX_TEST_FATAL(access(SIGHUP_TEST_FILE, F_OK) != 0);
    HEX_TEST_FATAL(HexDaemonStart(PROGRAM, NULL, HEX_DAEMON_NO_SIGHUP) == 0);
    WaitForPidChange(pid);
    HEX_TEST_FATAL(access(SIGHUP_TEST_FILE, F_OK) != 0);

    // Cmd line file should contain "./daemontest"
    HEX_TEST_FATAL(HexSystemF(0, "grep '^daemontest$' %s", CMD_LINE_FILE) == 0);

    return HexTestResult;
}

