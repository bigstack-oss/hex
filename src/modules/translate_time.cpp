// HEX SDK

#include <hex/log.h>
#include <hex/yml_util.h>

#include <hex/translate_module.h>

#include "hex/cli_util.h"
#include "include/policy_time.h"

// Translate the time policy
static bool
Translate(const char *policy, FILE *settings)
{
    bool status = true;

    HexLogDebug("translate_time policy: %s", policy);

    GNode *yml = InitYml("time");
    TimeConfigType cfg;

    if (ReadYml(policy, yml) < 0) {
        FiniYml(yml);
        HexLogError("Failed to parse policy file %s", policy);
        return false;
    }

    fprintf(settings, "\n# Time Tuning Params\n");

    HexYmlParseString(cfg.timeZone, yml, "timezone");
    fprintf(settings, "time.timezone=%s\n", cfg.timeZone.c_str());

    FiniYml(yml);

    return status;

}

TRANSLATE_MODULE(time/time1_0, 0, 0, Translate, 0);

