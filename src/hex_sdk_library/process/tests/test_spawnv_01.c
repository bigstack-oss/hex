// HEX SDK

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <hex/process.h>
#include <hex/test.h>

int main() {

    int status = 0;
    char *argv[5];

    // TEST - /bin/true, return status should be 0
    argv[0] = "/bin/true";
    argv[1] = NULL;
    status = HexSpawnV(0, argv);
    HEX_TEST(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    HEX_TEST(status == 0);

    // TEST - /bin/false, return status should be non-zero
    argv[0] = "/bin/false";
    argv[1] = NULL;
    status = HexSpawnV(0, argv);
    HEX_TEST(WIFEXITED(status) && WEXITSTATUS(status) != 0);

    // TEST - /bin/fakeproggie, return status should be 127 as from system()
    argv[0] = "/bin/fakeproggie";
    argv[1] = NULL;
    status = HexSpawnV(0, argv);
    HEX_TEST(WIFEXITED(status) && WEXITSTATUS(status) == 127);

    // TEST - /bin/sleep 3, timelimit 5, return status 0
    argv[0] = "/bin/sh";
    argv[1] = "-cx";
    argv[2] = "sleep 3; echo creating marker file; touch test.out; exit 0";
    argv[3] = NULL;
    unlink("test.out");
    status = HexSpawnV(5, argv);
    HEX_TEST(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    // marker file should exist
    HEX_TEST(access("test.out", F_OK) == 0);

    // TEST - /bin/sleep 3, timelimit 1, return status should be non-zero
    argv[0] = "/bin/sh";
    argv[1] = "-cx";
    argv[2] = "sleep 3; echo creating marker file; touch test.out; exit 0";
    argv[3] = NULL;
    unlink("test.out");
    status = HexSpawnV(1, argv);
    HEX_TEST(WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM);
    // marker file should not exist
    HEX_TEST(access("test.out", F_OK) == -1);

    // TEST - touch marker and fail, timeout -1 (no wait), return status should be 0
    argv[0] = "/bin/sh";
    argv[1] = "-cx";
    argv[2] = "echo creating marker file; touch test.out; exit 1";
    argv[3] = NULL;
    unlink("test.out");
    status = HexSpawnV(-1, argv);
    // return status should be zero even though child process returned 1
    HEX_TEST(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    // marker file should exist (maybe after a couple seconds)
    for (int i = 0; i < 5; ++i) {
        if (access("test.out", F_OK) == 0)
            break;
        sleep(1);
    }
    HEX_TEST(access("test.out", F_OK) == 0);

    return HexTestResult;
}
