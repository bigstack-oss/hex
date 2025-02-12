// HEX SDK

#include <libgen.h> // basename
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <hex/crash.h>
#include <hex/pidfile.h>
#include <hex/test.h>

static const char PROGRAM[] = "testproc";
static const char PIDFILE[] = "/var/run/testproc.pid";

static volatile sig_atomic_t s_term = 0;

static void 
SignalHandler(int sig)
{
    switch (sig) {
    case SIGTERM:
        s_term = 1;
        break;
    }
}

int 
main(int argc, char **argv)
{
    HEX_TEST(HexCrashInit(PROGRAM) == 0);
    HEX_TEST(HexPidFileCreate(PIDFILE) == 0);

    // Initialize signal handler
    struct sigaction sa;
    sa.sa_handler = SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        printf("failed to initialize signal handler");
        exit(1);
    }

    // Sleep forever
    while (1) {
        if (s_term) {
            printf("SIGTERM received, exiting");
            HexPidFileRelease(PIDFILE);
            exit(0);
        }
        sleep(1);
    }

    return 0;
}

