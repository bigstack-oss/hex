// HEX SDK

#include <hex/process_util.h>
#include <hex/test.h>

int main() {
    std::string output = HexUtilPOpen("%s %s", "echo", "test");
    HEX_TEST(output.compare("test\n") == 0);

    output = HexUtilPOpen("printf \"%s\"", "test\ntest\n");
    HEX_TEST(output.compare("test\ntest\n") == 0);

    int status = HexUtilSystem(0, 0, "touch", "test.out", 0);
    HEX_TEST(status == 0);
    unlink("test.out");

    const char *argv[3] = { "touch", "test.out", 0};
    status = HexUtilSystemV(0, 0, (char* const*)argv);
    HEX_TEST(status == 0);
    unlink("test.out");

    status = HexUtilSystemF(0, 0, "%s %s", "touch", "test.out");
    HEX_TEST(status == 0);
    unlink("test.out");

    status = HexUtilSystem(0, 0, "exit 2", 0);
    HEX_TEST(WIFEXITED(status) && WEXITSTATUS(status) == 2);

    return HexTestResult;
}
