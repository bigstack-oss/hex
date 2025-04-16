// HEX SDK

#include <hex/log.h>

#include <hex/translate_module.h>

#include "include/policy_tuning.h"

static bool
ProcessTuning(Tunings &cfg, FILE* settings)
{
    HexLogDebug("Processing Tuning config");

    if (!cfg.enabled)
        return true;

    for (auto& t : cfg.tunings) {
        if (!t.enabled)
            continue;

        fprintf(settings, "%s = %s\n", t.name.c_str(), t.value.c_str());
    }

    return true;
}

// Translate the tuning policy
static bool
Translate(const char *policy, FILE *settings)
{
    bool status = true;

    HexLogDebug("translate_tuning policy: %s", policy);

    GNode *yml = InitYml("tuning");
    Tunings cfg;

    if (ReadYml(policy, yml) < 0) {
        FiniYml(yml);
        yml = NULL;
        HexLogError("Failed to parse policy file %s", policy);
        return false;
    }

    HexYmlParseBool(&cfg.enabled, yml, "enabled");

    size_t num = SizeOfYmlSeq(yml, "tunings");
    for (size_t i = 1 ; i <= num ; i++) {
        Tuning obj;

        HexYmlParseBool(&obj.enabled, yml, "tunings.%d.enabled", i);
        HexYmlParseString(obj.name, yml, "tunings.%d.name", i);
        HexYmlParseString(obj.value, yml, "tunings.%d.value", i);

        cfg.tunings.push_back(obj);
    }

    fprintf(settings, "\n# Tuning Parameters\n");
    ProcessTuning(cfg, settings);

    FiniYml(yml);
    yml = NULL;

    return status;
}

TRANSLATE_MODULE(tuning/tuning1_0, 0, 0, Translate, 0);

