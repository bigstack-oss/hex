
#include <hex/log.h>
#include <hex/config_module.h>
#include <hex/config_tuning.h>

// The other half of the cycle: defines BETA_NAME and parses alpha's spec.

CONFIG_TUNING_STR(BETA_NAME, "beta.name", TUNING_UNPUB, "beta tuning", "b",
                  ValidateRegex, DFT_REGEX_STR);

PARSE_TUNING_X_STR(s_alphaFromBeta, ALPHA_NAME, 2);

static bool
CommitBeta(bool modified, int dryLevel)
{
    HexLogDebugN(FWD, "Commit beta");
    return true;
}

CONFIG_MODULE(beta, NULL, NULL, NULL, NULL, CommitBeta);
