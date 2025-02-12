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
#include <pthread.h>

#include <hex/watchdog.h>

#define HEX_TEST_ABORT exit(1)
#include <hex/test.h>

// Include this last
#include "testfuncs.h"

time_t
GetThreadTime()
{
#if 0
    struct rusage usage;
    getrusage(RUSAGE_SELF /*FIXME:RUSAGE_THREAD*/, &usage);
    return usage.ru_utime.tv_sec;
#else
    /*
     * CLOCK_THREAD_CPUTIME_ID is a better approximation to
     * RUSAGE_THREAD than time(). Without this, we can fail
     * if #CPUs on the system is < number of threads tested.
     */
    struct timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return ts.tv_sec;
#endif
}

// Sleep for appoximately 'n' seconds of cpu (virtual) time
double
TSleep(int n)
{
    double x = 0;
    time_t t0 = GetThreadTime();
    while ((GetThreadTime() - t0) < n)
        x = WasteTime();
    return x;
}

struct Args {
    int fail;
    int real;
};

void*
worker(void* arg)
{
    // Register signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SignalHandler;
    sigemptyset(&sa.sa_mask);
    HEX_TEST_FATAL(sigaction(SIGABRT, &sa, NULL) != -1);

    struct Args* args = (struct Args*)arg;

    HexWatchdogTimerVirt* wd = HexWatchdogTimerVirtCreate();
    HexWatchdogTimerVirtSet(wd, 2);
    if (args->fail) {
        // Loop for 5 seconds
        // Watchdog should kill us 2 seconds into loop
        if (args->real)
            Sleep(5);
        else
            TSleep(5);

        // We should not get here if watchdog is working correctly
        Touch("./test3.out");
    } else {
        int i;
        for (i = 0; i < 5; ++i) {
            TSleep(1);
            HEX_TEST(wd->spurious == 0);
            HexWatchdogTimerVirtReset(wd);
        }
    }

    // Disable watchdog and exit successfully
    HexWatchdogTimerVirtDestroy(wd);

    return 0;
}

int
main(int argc, char **argv)
{
    int fail = 0;
    int real = 0;
    int threads = 0;
    pthread_t* ids = 0;
    struct Args arg;

    static struct option long_options[] = {
        { "fail", no_argument, 0, 'f' },
        { "real", no_argument, 0, 'r' },
        { "num-threads", required_argument, 0, 'n' },
        { 0, 0, 0, 0 }
    };

    // Suppress error messages by getopt_long()
    opterr = 0;

    while (1) {
        int index;
        int c = getopt_long(argc, argv, "frn:", long_options, &index);
        if (c == -1)
            break;
        switch (c) {
        case 'f':
            fail = 1;
            break;
        case 'r':
            real = 1;
            break;
        case 'n':
            threads = atoi(optarg);
            ids = calloc(threads, sizeof(pthread_t));
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

    for (int i = 0; i < threads; i++) {
        arg.real = real;
        arg.fail = fail;
        HEX_TEST_FATAL(pthread_create(&ids[i], 0, worker, &arg) == 0);
    }

    HexWatchdogTimerReal* rwd = 0;
    HexWatchdogTimerVirt* vwd = 0;

    if (real) {
        // Set watchdog real timeout to 2 seconds
        rwd = HexWatchdogTimerRealCreate();
        HexWatchdogTimerRealSet(rwd, 2);
    } else {
        // Set watchdog virtual timeout to 2 seconds
        vwd = HexWatchdogTimerVirtCreate();
        HexWatchdogTimerVirtSet(vwd, 2);
    }
    
    Touch("./test1.out");

    // Loop for 3 seconds (cpu time) and reset watchdog once per second
    int i;
    for (i = 0; i < 3; ++i) {
        if (real) {
            sleep(1);
            HexWatchdogTimerRealReset(rwd);
        } else {
            VSleep(1);
            HexWatchdogTimerVirtReset(vwd);
        }
    }

    Touch("./test2.out");

    if (fail && threads == 0) { // If using threads, let another thread fail
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
    } else if (threads) {
        // Loop for 5 seconds just like the threads do, but always reset wd
        for (i = 0; i < 3; ++i) {
            if (real) {
                sleep(1);
                HexWatchdogTimerRealReset(rwd);
            } else {
                VSleep(1);
                HexWatchdogTimerVirtReset(vwd);
            }
        }
    }

    // Disable watchdog and exit successfully
    if (real)
        HexWatchdogTimerRealDestroy(rwd);
    else
        HexWatchdogTimerVirtDestroy(vwd);

    for (int i = 0; i < threads; i++) {
        HEX_TEST_FATAL(pthread_join(ids[i], 0) == 0);
    }

    free(ids);

    return 0;
}

