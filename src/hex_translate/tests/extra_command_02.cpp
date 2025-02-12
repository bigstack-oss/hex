
#include <hex/translate_module.h>

static int
FooMain(int argc, char **argv)
{
    return 0;
}

static void
FooUsage()
{
}

TRANSLATE_COMMAND(foo, FooMain, FooUsage);
