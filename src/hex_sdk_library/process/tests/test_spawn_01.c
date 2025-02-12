// HEX SDK

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <hex/process.h>
#include <hex/test.h>

int main() {

    int status = 0;

    // TEST - /bin/true, return status should be 0
    status = HexSpawn(0, "/bin/true", NULL);
    HEX_TEST(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    HEX_TEST(status == 0);

    // TEST - /bin/false, return status should be non-zero
    status = HexSpawn(0, "/bin/false", NULL);
    HEX_TEST(WIFEXITED(status) && WEXITSTATUS(status) != 0);

    // TEST - /bin/fakeproggie, return status should be 127 as from system()
    status = HexSpawn(0, "/bin/fakeproggie", NULL);
    HEX_TEST(WIFEXITED(status) && WEXITSTATUS(status) == 127);

    // TEST - /bin/sleep 3, timelimit 5, return status 0
    status = HexSpawn(5, "/bin/sleep", "3", NULL);
    HEX_TEST(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    HEX_TEST(status == 0);

    // TEST - /bin/sleep 3, timelimit 1, return status
    unlink("test.out");
    status = HexSpawn(1, "/bin/sh", "-c", "sleep 3; touch test.out", NULL);
    HEX_TEST(WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM);
    HEX_TEST(access("test.out", F_OK) == -1);

    return HexTestResult;
}
