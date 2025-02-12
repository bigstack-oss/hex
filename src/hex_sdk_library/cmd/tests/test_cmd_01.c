
#include <hex/test.h>
#include <hex/cmd.h>

#include <signal.h>
#include <errno.h>
#include <sys/poll.h>

int main()
{
    HEX_TEST(HexCmdInit("test_cmd", 0) == 0);

    int fd = -1;
    HEX_TEST((fd = HexCmdFd()) >= 0);

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    HEX_TEST(poll(&pfd, 1, 5000) == 1);

    char buf[80];
    HexCmdAddr_t from;
    int n = 0;
    HEX_TEST((n = HexCmdRecv(buf, 80, &from)) > 0);
    printf("HexCmdRecv: got %d bytes: \'%s\'\n", n, buf);
    HEX_TEST(strncmp(buf, "test message", 80) == 0);

    const char ok[] = "OK";
    HEX_TEST(HexCmdResp(ok, strlen(ok)+1, &from) == strlen(ok)+1);

    HEX_TEST(HexCmdFini() == 0);

    return HexTestResult;
}
