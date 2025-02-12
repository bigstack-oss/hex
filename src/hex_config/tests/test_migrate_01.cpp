
#include <string.h>

#include <hex/test.h>
#include <hex/process.h>
#include <hex/tuning.h>
#include <hex/config_module.h>

static bool
Migrate(const char *prevVersion, const char *prevRootDir)
{
    // Copy contents of /etc/test/test.txt (previous root) to current dir
    HexSystemF(0, "cp %s/etc/test/test.txt .", prevRootDir);

    // Save previous version
    HexSystemF(0, "echo %s > test.version", prevVersion);

    return true;
}

CONFIG_MODULE(test, 0, 0, 0, 0, 0);
CONFIG_MIGRATE(test, Migrate);
