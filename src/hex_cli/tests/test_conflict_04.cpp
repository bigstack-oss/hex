// HEX SDK

#include <cstring>

#include <hex/test.h>
#include <hex/cli_module.h>

static int
BarMain(int argc, const char** argv) {
    HEX_TEST_FATAL(argc == 4);
    HEX_TEST_FATAL(strcmp(argv[1], "a") == 0);
    HEX_TEST_FATAL(strcmp(argv[2], "b") == 0);
    HEX_TEST_FATAL(strcmp(argv[3], "c") == 0);

    printf("BarMain\n");

    return HexTestResult;
}

CLI_MODE_COMMAND("foo", "bar", BarMain, 0,
    "FooBarDescription.",
    "FooBarUsage");

CLI_MODE_COMMAND("baz", "bar", BarMain, 0,
    "BazBarDescription.",
    "BazBarUsage");

