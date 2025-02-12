// HEX SDK

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <hex/process.h>
#include <hex/test.h>

int main() {

    int status = 0;
    char *argv[6] = { "sleep", "3", ";", "touch", "test.out", NULL };

    unlink("test.out");

    // TEST - /bin/sleep 3, timelimit 1, return status
    status = HexSystemV(1, argv);
    HEX_TEST(WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM);
    HEX_TEST(access("test.out", F_OK) == -1);

    unlink("test.out");

    // TEST - /bin/sleep 3, no timelimit, return status
    status = HexSystemV(0, argv);
    HEX_TEST(status == 0);
    HEX_TEST(access("test.out", F_OK) == 0);

    // TEST - NULL argv
    char *null_argv[1] = { NULL };
    status = HexSystemV(0, null_argv);
    HEX_TEST(status == -1);
    
    return HexTestResult;
}
