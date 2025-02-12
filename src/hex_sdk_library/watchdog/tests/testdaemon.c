// HEX SDK

#include <getopt.h> // should be GNU version
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <hex/log.h>
#include <hex/pidfile.h>
#include <hex/daemon.h>

static const char PROGRAM[] = "testdaemon";
static const char DISPLAY_NAME[] = "Test Daemon";

static volatile sig_atomic_t s_term = 0;
static volatile sig_atomic_t s_restart = 0;
static volatile sig_atomic_t s_restart2 = 0;

static void 
SignalHandler(int sig)
{
    switch (sig) {
    case SIGTERM:
        s_term = 1;
        break;
    case SIGUSR1:
        s_restart = 1;
        break;
    case SIGUSR2:
        s_restart2 = 1;
        break;
    }
}

static int
DaemonCallback(int restart, int childStatus)
{
    FILE *fout = fopen("/tmp/test.out", "w");
    if (fout) {
        fprintf(fout, "RESTART=%d\n", restart);
        fprintf(fout, "EXITED=%d\n", WIFEXITED(childStatus));
        fprintf(fout, "EXITSTATUS=%d\n", WIFEXITED(childStatus) ? WEXITSTATUS(childStatus) : 0);
        fprintf(fout, "SIGNALED=%d\n", WIFSIGNALED(childStatus));
        fprintf(fout, "TERMSIG=%d\n", WIFSIGNALED(childStatus) ? WTERMSIG(childStatus) : 0);
        fclose(fout);
    }

    return 0;
}

int 
main(int argc, char **argv)
{
    int daemonFlags = HEX_LOG_STDERR;
    int crashOnSigterm = 0;
    int ignoreSigterm = 0;

    static struct option long_options[] = {
        { "nodaemon", no_argument, 0, 'D' },
        { "crash_on_sigterm", no_argument, 0, 'C' },
        { "ignore_sigterm", no_argument, 0, 'I' },
        { 0, 0, 0, 0 }
    };

    // Suppress error messages by getopt_long()
    opterr = 0;

    while (1) {
        int index;
        int c = getopt_long(argc, argv, "DCIR", long_options, &index);
        if (c == -1)
            break;

        switch (c) {
        case 'D':
            daemonFlags |= HEX_NO_DAEMON;
            break;
        case 'C':
            crashOnSigterm = 1;
            break;
        case 'I':
            ignoreSigterm = 1;
            break;
        default:
            abort();
        }
    }

    if (optind != argc)
        abort();

    HexWatchdogDaemonSetCallback(DaemonCallback);
    HexWatchdogDaemon(PROGRAM, DISPLAY_NAME, daemonFlags);

    // Initialize signal handler
    struct sigaction sa;
    sa.sa_handler = SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) < 0 ||
        sigaction(SIGUSR1, &sa, NULL) < 0 ||
        sigaction(SIGUSR2, &sa, NULL) < 0)
        HexLogFatal("Failed to initialize signal handler");

    HexLogNotice("Started");

    // Sleep forever
    while (1) {
        if (s_term) {
            if (crashOnSigterm) {
                HexLogNotice("SIGTERM received, raising SIGSEGV");
                raise(SIGSEGV);
            } else if (ignoreSigterm) {
                HexLogNotice("SIGTERM received, ignoring");
            } else {
                HexLogNotice("SIGTERM received, exiting");
                exit(0);
            }
        } else if (s_restart) {
            HexLogNotice("SIGUSR1 received, requesting restart");
            exit(HEX_EXIT_RESTART);
        } else if (s_restart2) {
            HexLogNotice("SIGUSR2 received, requesting restart via parent");
            HexWatchdogDaemonRequestRestart();
        }
        sleep(1);
    }

    return 0;
}

