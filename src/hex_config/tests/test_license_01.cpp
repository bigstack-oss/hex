
#include <string>

#include <hex/test.h>
#include <hex/config_module.h>

static int
Main(int argc, char **argv)
{
    return 0;
}

static void
Usage()
{
}

CONFIG_COMMAND(foo, Main, Usage);