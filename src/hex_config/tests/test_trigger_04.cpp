// HEX SDK

#include <cstdlib>
#include <hex/config_module.h>

static int
Trigger(int argc, char **argv)
{
    if (argc != 2)
        return -1;

    if (strcmp(argv[0], "foo") == 0)
        system("/bin/touch test.foo_triggered");

    if (strcmp(argv[1], "bar") == 0)
        system("/bin/touch test.bar_triggered");

    return 0;
}

static bool
Commit(bool modified, int dryLevel)
{
    ArgVec argv;
    argv.push_back("pkg_updated");
    argv.push_back("foo");
    argv.push_back("bar");
    if (ApplyTrigger(argv) != EXIT_SUCCESS)
        return false;

    return true;
}

CONFIG_MODULE(foo, 0, 0, 0, 0, Commit);
CONFIG_TRIGGER(foo, "pkg_updated", Trigger);