
#include <hex/test.h>
#include <hex/process.h>
#include <hex/config_module.h>

static bool
Commit(bool modified, int dryLevel)
{
    if (HexSpawn(0, "./testopenfd", NULL) != 0)
        return false;
    
    return true;
}

CONFIG_MODULE(test, NULL, NULL, NULL, NULL, Commit);
