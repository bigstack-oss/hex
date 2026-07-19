
#include <hex/log.h>
#include <hex/test.h>
#include <hex/config_module.h>

// A scoped commit of "leaf" must still commit "glob" (globals provider), but
// not "mid", which sits between them in commit order.

static bool
CommitGlob(bool modified, int dryLevel)
{
    HexLogDebugN(FWD, "Commit glob");
    return true;
}

CONFIG_MODULE(glob, NULL, NULL, NULL, NULL, CommitGlob);
CONFIG_PROVIDES_GLOBALS(glob);

static bool
CommitMid(bool modified, int dryLevel)
{
    HexLogDebugN(FWD, "Commit mid");
    return true;
}

CONFIG_MODULE(mid, NULL, NULL, NULL, NULL, CommitMid);
CONFIG_REQUIRES(mid, glob);

static bool
CommitLeaf(bool modified, int dryLevel)
{
    HexLogDebugN(FWD, "Commit leaf");
    return true;
}

CONFIG_MODULE(leaf, NULL, NULL, NULL, NULL, CommitLeaf);
CONFIG_REQUIRES(leaf, mid);
