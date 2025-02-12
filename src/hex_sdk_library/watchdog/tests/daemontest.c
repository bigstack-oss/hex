
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

#include <hex/pidfile.h>
#include <hex/log.h>

static const char PIDFILE[] = "/var/run/daemontest.pid";

static const char PROGRAM[] = "daemontest";

static volatile sig_atomic_t s_term = 0;
static volatile sig_atomic_t s_hup = 0;

static void 
SignalHandler(int sig)
{
    switch (sig) {
    case SIGTERM:
        s_term = 1;
        break;
    case SIGHUP:
        s_hup = 1;
        break;
    }
}

int 
main(int argc, char **argv) 
{
    int pid = HexPidFileCheck(PIDFILE);
    if (pid > 0) {
        perror("Process already running!");
        exit(0);
    } 

    daemon(1 /*nochdir*/, 0);
    HexPidFileCreate(PIDFILE);
    HexLogInit(PROGRAM, 1 /*logToStdErr*/);
 
    // Initialize signal handler
    struct sigaction sa;
    sa.sa_handler = SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) == -1 ||
        sigaction(SIGHUP, &sa, NULL) == -1)
        HexLogFatal("failed to initialize signal handler");

    HexLogNotice("Started");

    // Sleep forever
    while (1) {
        if (s_term) {
            HexLogNotice("SIGTERM received, exiting");
            unlink(PIDFILE);
            exit(0);
        } else if (s_hup) {
            s_hup = 0;
            system("touch test.sighup");
        }
        sleep(1);
    }

    // Not reached
    return 0;
}
