
#include <hex/test.h>
#include <hex/cmd.h>

#include <signal.h>
#include <errno.h>

int main()
{
    HEX_TEST(HexCmdInit("test_cmd", 1) == 0);

    sigset_t mask;
    sigemptyset(&mask);
    HEX_TEST(sigsuspend(&mask) == -1);
    HEX_TEST(errno == EINTR);

    HEX_TEST(HexCmdPending() > 0);

    char buf[80];
    HexCmdAddr_t from;
    int n = 0;
    HEX_TEST((n = HexCmdRecv(buf, 80, &from)) > 0);
    printf("HexCmdRecv: got %d bytes: \'%s\'\n", n, buf);
    HEX_TEST(strncmp(buf, "test message", 80) == 0);

    const char ok[] = "OK";
    HEX_TEST(HexCmdResp(ok, strlen(ok)+1, &from) == strlen(ok)+1);

    HEX_TEST(HexCmdPending() == 0);

    HEX_TEST(HexCmdFini() == 0);

    return HexTestResult;
}
