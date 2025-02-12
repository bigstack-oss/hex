// HEX SDK

#include "hex/loop.h"
#include "hex/test.h"

#include <signal.h>
#include <string.h>
#include <time.h>

static int pipefd[2];

static int usr1Data = 12345;
static int usr1Val = 54321;
static int usr1CbCalled = 0;

int usr1Cb(int signum, void* userData, int value)
{
    HEX_TEST(signum == SIGUSR1);
    HEX_TEST(userData == &usr1Data);
    HEX_TEST(value == usr1Val);
    const char c = 'f';
    write(pipefd[1], &c, 1);
    usr1CbCalled++;
    return 0;
}

static pid_t chldPid = 0;
static int chldCbCalled = 0;

int chldCb(int signum, void* userData, int value)
{
    HEX_TEST(signum == SIGCHLD);
    chldCbCalled++;
    return 0;
}

static int fdData = 51275;
static int fdCbCalled = 0;

int fdCb(int fd, void* userData, int value)
{
    HEX_TEST(fd == pipefd[0]);
    HEX_TEST(userData == &fdData);
    HexLoopQuit();
    fdCbCalled++;
    return 0;
}

static int timerData = 51275;
static int timerCbCalled = 0;
static time_t t = 0;

int timerCb(int, void* userData, int value)
{
    time_t diff = time(0) - t;
    HEX_TEST(diff >= 2 && diff <= 4); // 1 second fuzz
    HEX_TEST(userData == &timerData);

    sigval val;
    memset(&val, 0, sizeof(val));
    val.sival_int = usr1Val;
    HEX_TEST_FATAL(sigqueue(getpid(), SIGUSR1, val) == 0);

    timerCbCalled++;
    return 0;
}

int main()
{
    alarm(10);

    HEX_TEST_FATAL(HexLoopInit(0) == 0);

    HEX_TEST_FATAL(pipe(pipefd) == 0);

    HEX_TEST_FATAL(HexLoopSignalAdd(SIGUSR1, usr1Cb, &usr1Data) == 0);

    HEX_TEST_FATAL(HexLoopSignalAdd(SIGCHLD, chldCb, 0) == 0);

    HEX_TEST_FATAL(HexLoopFdAdd(pipefd[0], fdCb, &fdData) == 0);

    // Fork a child to make sure HexLoop handles SIGCHLD
    chldPid = fork();
    if (chldPid == 0) {
        sleep(1);
        exit(0);
    }

    t = time(0);
    HEX_TEST_FATAL(HexLoopTimerAdd(3, timerCb, &timerData) == 0);
    
    HEX_TEST(HexLoop() == 0);

    HEX_TEST(HexLoopFini() == 0);

    HEX_TEST(usr1CbCalled == 1);
    HEX_TEST(chldCbCalled == 1);
    HEX_TEST(fdCbCalled == 1);
    HEX_TEST(timerCbCalled == 1);

    return HexTestResult;
}
