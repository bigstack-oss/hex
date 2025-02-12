// HEX SDK

#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h> // should be GNU version
#include <errno.h>

#include <hex/watchdog.h>

#define HEX_TEST_ABORT exit(1)
#include <hex/test.h>

// Include this last
#include "testfuncs.h"

int
main(int argc, char **argv)
{
    int fail = 0;
    int real = 0;

    static struct option long_options[] = {
        { "fail", no_argument, 0, 'f' },
        { "real", no_argument, 0, 'r' },
        { 0, 0, 0, 0 }
    };

    // Suppress error messages by getopt_long()
    opterr = 0;

    while (1) {
        int index;
        int c = getopt_long(argc, argv, "fr", long_options, &index);
        if (c == -1)
            break;
        switch (c) {
        case 'f':
            fail = 1;
            break;
        case 'r':
            real = 1;
            break;
        case '?':
        default:
            return 127;
        }
    }

    // Register signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SignalHandler;
    sigemptyset(&sa.sa_mask);
    HEX_TEST_FATAL(sigaction(SIGABRT, &sa, NULL) != -1);

    HexWatchdogTimerReal* rwd = 0;
    HexWatchdogTimerVirt* vwd = 0;

    if (real) {
        // Set watchdog real timeout to 3 seconds
        rwd = HexWatchdogTimerRealCreate();
        HexWatchdogTimerRealSet(rwd, 3);
        HexWatchdogTimerRealReset(rwd);
    } else {
        // Set watchdog virtual timeout to 2 seconds
        vwd = HexWatchdogTimerVirtCreate();
        HexWatchdogTimerVirtSet(vwd, 2);
        HexWatchdogTimerVirtReset(vwd);
    }

    // Reset wd before and after Touch, in case of heavy I/O on build machine
    Touch("./test1.out");

    // Loop for 5 seconds and reset watchdog once per second
    int i;
    for (i = 0; i < 5; ++i) {
        if (real) {
            HexWatchdogTimerRealReset(rwd);
            sleep(1);
        } else {
            HexWatchdogTimerVirtReset(vwd);
            VSleep(1);
        }
    }

    if (real)
        HexWatchdogTimerRealReset(rwd);
    else
        HexWatchdogTimerVirtReset(vwd);

    Touch("./test2.out");

    if (fail) {
        // Loop for 5 seconds
        // Watchdog should kill us 2 seconds into loop
        if (real)
            Sleep(5);
        else
            VSleep(5);

        // We should not get here if watchdog is working correctly
        Touch("./test3.out");

        // Return failure
        return 1;
    }

    // Disable watchdog and exit successfully
    if (real)
        HexWatchdogTimerRealDestroy(rwd);
    else
        HexWatchdogTimerVirtDestroy(vwd);

    return 0;
}

