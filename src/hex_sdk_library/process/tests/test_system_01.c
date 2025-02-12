// HEX SDK

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <hex/process.h>
#include <hex/test.h>

int main() {

    int status = 0;

    // TEST - /bin/sleep 3, timelimit 1, return status
    unlink("test.out");
    status = HexSystem(1, "sleep", "3", ";", "touch", "test.out", NULL);
    HEX_TEST(WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM);
    HEX_TEST(access("test.out", F_OK) == -1);

    // TEST - /bin/sleep 3, no timelimit, return status
    unlink("test.out");
    status = HexSystem(0, "sleep", "3", ";", "touch", "test.out", NULL);
    HEX_TEST(status == 0);
    HEX_TEST(access("test.out", F_OK) == 0);

    return HexTestResult;
}
