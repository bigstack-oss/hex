// HEX SDK

#ifndef TEST_H
#define TEST_H

static const char PROGRAM[] = "daemontest";
static const char PID_FILE[] = "/var/run/daemontest.pid";
static const char CMD_LINE_FILE[] = "/var/run/daemontest.cmdline";
static const char SIGHUP_TEST_FILE[] = "test.sighup";

static inline void
WaitForStart()
{
    // Throw assertion
    HEX_TEST_FATAL(HexProcPidReady(3, PID_FILE) > 0);
}

static inline void
WaitForStop()
{
    // Throw assertion
    HEX_TEST_FATAL(HexProcPidReady(3, PID_FILE) < 0);
}

static inline void
WaitForSigHupTestFile()
{
    for (int i = 0; i < 3; ++i) {
        if (access(SIGHUP_TEST_FILE, F_OK) == 0)
            return;
        sleep(1);
    }
    // Throw assertion
    HEX_TEST_FATAL(access(SIGHUP_TEST_FILE, F_OK) == 0);
}

static inline void
WaitForPidChange(pid_t pid)
{
    for (int i = 0; i < 3; ++i) {
        pid_t pid2 = HexPidFileRead(PID_FILE);
        if (pid != pid2)
            return;
        sleep(1);
    }
    // Throw assertion
    HEX_TEST_FATAL(HexPidFileRead(PID_FILE) != pid);
}

#endif
