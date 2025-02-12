
#include <hex/log.h>
#include <hex/test.h>
#include <hex/config_module.h>

static bool
Commit1(bool modified, int dryLevel)
{
    HexLogDebugN(FWD, "Commit test1");
    return true;
}

CONFIG_MODULE(test1, NULL, NULL, NULL, NULL, Commit1);

static bool
Commit2(bool modified, int dryLevel)
{
    HexLogDebugN(FWD, "Commit test2");
    return true;
}

CONFIG_MODULE(test2, NULL, NULL, NULL, NULL, Commit2);

static bool
Commit3(bool modified, int dryLevel)
{
    HexLogDebugN(FWD, "Commit test3");
    return true;
}

CONFIG_MODULE(test3, NULL, NULL, NULL, NULL, Commit3);

static bool
Commit4(bool modified, int dryLevel)
{
    HexLogDebugN(FWD, "Commit test4");
    return true;
}

CONFIG_MODULE(test4, NULL, NULL, NULL, NULL, Commit4);

static bool
Commit5(bool modified, int dryLevel)
{
    HexLogDebugN(FWD, "Commit test5");
    return true;
}

CONFIG_MODULE(test5, NULL, NULL, NULL, NULL, Commit5);