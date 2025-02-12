// HEX SDK

// An example/test daemon for the HexLoop API

#include "hex/loop.h"
#include "hex/pidfile.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LogFatal(fmt, ...) do { fprintf(stderr, "Fatal error: " fmt, ## __VA_ARGS__); exit(1); } while (0)

#define PIPENAME "loopd.pipe"
#define PIDFILE "/var/run/loopd.pid"

struct Foo {
    int i;
};

static int s_fd = -1;

int termCb(int signum, void* userData, int value)
{
    HexLoopQuit();
    return 0;
}

int usr1Cb(int signum, void* userData, int value)
{
    fprintf(stderr, "Received signal SIGUSR1\n");
    return 0;
}

int fdCb(int fd, void* userData, int value)
{
    int r = 0;
    char buf[81];
    memset(buf,0,81);
    ssize_t s = read(fd, buf, 80);
    if (s > 0) {
        fprintf(stderr, "Received message \"%.*s\"\n", (int)s, buf);
    } else if (s < 0) {
        fprintf(stderr, "Error reading from pipe: %s", strerror(errno));
        r = -1; // This should cause HexLoop to return failure
    } else {
        r = -1; // This should cause HexLoop to return failure
    }
    return r;
}

int timerCb(int, void* userData, int value)
{
    fprintf(stderr, "Timer expired\n");
    return 0;
}

int wdCb(int, void* userData, int value)
{
    alarm(3); // Resets the watchdog
    return 0;
}

int main()
{
    pid_t pid = HexPidFileCheck(PIDFILE);
    if (pid > 0) {
        fprintf(stderr, "Error: Another instance of loopd (pid %d) is already running\n", pid);
        exit(EXIT_FAILURE);
    }

    if (HexPidFileCreate(PIDFILE) != 0) {
        fprintf(stderr, "Failed to create pid file\n"); // COV_IGNORE
        exit(EXIT_FAILURE);
    }

    int r = EXIT_SUCCESS;

    Foo foo;
    HexLoopInit(0);

    if (HexLoopSignalAdd(SIGTERM, termCb, 0) != 0)
        LogFatal("Failed to add SIGTERM signal callback");

    if (HexLoopSignalAdd(SIGUSR1, usr1Cb, &foo) != 0)
        LogFatal("Failed to add SIGUSR1 signal callback");

    if ((s_fd = open(PIPENAME, O_RDWR)) == -1)
        LogFatal("open(\"%s\"): %s", PIPENAME, strerror(errno));

    if (HexLoopFdAdd(s_fd, fdCb, &foo) != 0)
        LogFatal("Failed to add fd callback");

    if (HexLoopTimerAdd(5, timerCb, &foo) != 0)
        LogFatal("Failed to add timer callback");

    // Simulate a watchdog timer
    if (HexLoopTimerAdd(1, wdCb, 0) != 0)
        LogFatal("Failed to add wd timer callback");

    fprintf(stderr, "%d\n", getpid());

    alarm(3); // Arm the watchdog
    if (HexLoop() != 0)
        r = EXIT_FAILURE;

    HexLoopFini();

    return r;
}
