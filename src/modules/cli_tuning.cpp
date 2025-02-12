// HEX SDK

#include <hex/log.h>
#include <hex/strict.h>
#include <hex/process.h>

#include <hex/cli_module.h>
#include <hex/cli_util.h>

#include "include/policy_tuning.h"
#include "include/cli_tuning_changer.h"

static bool
ListTunings(const TuningPolicy& policy)
{
    char line[80];
    memset(line, '-', sizeof(line));
    line[sizeof(line) - 1] = 0;

    Tunings cfg;
    policy.getTunings(&cfg);

#define HEADER_FMT " %7s  %30s  %36s"
#define TUNING_FMT " %7s  %30s  %36s"

    CliPrintf(HEADER_FMT, "enabled", "name", "value");
    CliPrintf("%s", line);
    for (auto& t : cfg.tunings)
        CliPrintf(TUNING_FMT, CLI_ENABLE_STR(t.enabled), t.name.c_str(), t.value.c_str());
    CliPrintf("%s", line);

    return true;
}

static int
TuningListMain(int argc, const char** argv)
{
    if (argc > 1) {
        return CLI_INVALID_ARGS;
    }

    HexPolicyManager policyManager;
    TuningPolicy policy;
    if (!policyManager.load(policy)) {
        return CLI_UNEXPECTED_ERROR;
    }

    ListTunings(policy);

    return CLI_SUCCESS;
}

static int
TuningCfgMain(int argc, const char** argv)
{
    if (argc > 1) {
        return CLI_INVALID_ARGS;
    }

    HexPolicyManager policyManager;
    TuningPolicy policy;
    if (!policyManager.load(policy)) {
        return CLI_UNEXPECTED_ERROR;
    }

    CliTuningChanger changer;

    CliList actions;
    int actIdx;
    std::string action;

    enum {
        MODIFY = 0,
        APPLY,
        EXIT
    };

    actions.push_back("modify tunings");
    actions.push_back("apply");
    actions.push_back("exit");

    actIdx = 0;
    while(actIdx != APPLY && actIdx != EXIT) {
        int ret = CLI_SUCCESS;
        ListTunings(policy);
        actIdx = CliReadListIndex(actions);
        switch (actIdx) {
            case MODIFY:
                if (!changer.configure(&policy))
                    ret = CLI_FAILURE;
                if (!policyManager.save(policy))
                    ret = CLI_UNEXPECTED_ERROR;
                break;
        }

        if (ret != CLI_SUCCESS)
            return ret;
    }

    if (actIdx == EXIT)
        return CLI_SUCCESS;

    ListTunings(policy);
    CliPrintf("Apply the changes?");
    if (!CliReadConfirmation())
        return CLI_SUCCESS;

    // hex_config apply (translate + commit)
    if (!policyManager.apply()) {
        return CLI_INVALID_ARGS;
    }

    HexLogEvent("PLC00001I", "%s,category=policy,policy=tuning",
                             CliEventAttrs().c_str());
    return CLI_SUCCESS;
}

static int
TuningDumpMain(int argc, const char** argv)
{
    if (argc > 1) {
        return CLI_INVALID_ARGS;
    }

    HexSpawn(0, "/usr/sbin/hex_config", "-P", NULL);
    CliPrintf("----------------------------------------------------------------\n\n\nNetwork Interfaces");
    HexSpawn(0, HEX_SDK, "-v", "DumpInterface", "0", NULL);

    return CLI_SUCCESS;
}

// This mode is not available in STRICT error state
CLI_MODE(CLI_TOP_MODE, "tuning", "Work with tuning parameters.", !HexStrictIsErrorState() && !FirstTimeSetupRequired());

CLI_MODE_COMMAND("tuning", "list", TuningListMain, NULL,
        "List all configured tuning parameters.",
        "list");

CLI_MODE_COMMAND("tuning", "configure", TuningCfgMain, NULL,
        "Configure tuning parameters.",
        "configure");

CLI_MODE_COMMAND("tuning", "dump", TuningDumpMain, NULL,
        "Display all support tuning parameters.",
        "dump");

