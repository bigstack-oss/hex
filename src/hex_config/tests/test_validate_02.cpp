
#include <hex/test.h>
#include <hex/config_module.h>

static bool
Commit(bool modified, int dryLevel)
{
    return true;
}

CONFIG_MODULE(test, NULL, NULL, NULL, NULL, Commit);
