
#include <hex/test.h>
#include <hex/cmd.h>

#include <signal.h>
#include <errno.h>
#include <sys/poll.h>

int main()
{
    HEX_TEST(HexCmdInit("test_cmd", 1) == 0);

    char buf[80];
    HexCmdAddr_t from;
    int n = 0;
    HEX_TEST((n = HexCmdRecv(buf, 80, &from)) == 0);
    const char* emsg = HexCmdError(n);
    HEX_TEST(emsg != NULL);

    HEX_TEST(HexCmdFini() == 0);

    return HexTestResult;
}
