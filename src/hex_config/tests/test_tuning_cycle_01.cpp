
#include <hex/log.h>
#include <hex/test.h>
#include <hex/config_module.h>
#include <hex/config_tuning.h>

// "alpha" parses "beta"'s spec and vice versa (extra_tuning_cycle_01.cpp) --
// a cross-TU spec cycle that no link order can satisfy eagerly.

CONFIG_TUNING_STR(ALPHA_NAME, "alpha.name", TUNING_UNPUB, "alpha tuning", "a",
                  ValidateRegex, DFT_REGEX_STR);

// reach across to the other translation unit's spec
PARSE_TUNING_X_STR(s_betaFromAlpha, BETA_NAME, 1);

static bool
CommitAlpha(bool modified, int dryLevel)
{
    HexLogDebugN(FWD, "Commit alpha");
    return true;
}

CONFIG_MODULE(alpha, NULL, NULL, NULL, NULL, CommitAlpha);
