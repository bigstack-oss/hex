
#include <string>

#include <hex/test.h>
#include <hex/config_module.h>

static int
Main(int argc, char **argv)
{
    // Verify that we're running as root
    HEX_TEST_FATAL(getuid() == 0);

    // Run child script that just outputs the current user id
    system("./test.sh >test.out 2>test.err");
    return 0;
}

static void
Usage()
{
}

CONFIG_COMMAND(foo, Main, Usage);
