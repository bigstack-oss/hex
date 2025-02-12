
#include <hex/config_module.h>

static int
FooMain(int argc, char **argv)
{
    return 0;
}

static void
FooUsage()
{
}

CONFIG_COMMAND(foo, FooMain, FooUsage);
