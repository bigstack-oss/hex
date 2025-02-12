
#include <hex/test.h>
#include <hex/cmd.h>

#include <sys/stat.h>
#include <unistd.h>

int main()
{
    HEX_TEST(HexCmdInit("test_cmd", 0) == 0);

    struct stat s;
    HEX_TEST(stat("/var/run/test_cmd.sock", &s) == 0);
    HEX_TEST(S_ISSOCK(s.st_mode));

    HEX_TEST(HexCmdFini() == 0);

    return HexTestResult;
}
