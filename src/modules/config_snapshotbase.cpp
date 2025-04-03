// HEX SDK

#include <sys/types.h>
#include <unistd.h>

#include <hex/process.h>
#include <hex/log.h>
#include <hex/string_util.h>

#include <hex/config_tuning.h>
#include <hex/config_module.h>

static const char SLA_ACCPETED_FILE[] = "/etc/appliance/state/sla_accepted";
static const char CONFIGURED_FILE[] = "/etc/appliance/state/configured";

// public tunings
CONFIG_TUNING_STR(SNAPSHOT_APPLY_ACTION, "snapshot.apply.action", TUNING_PUB, "Action in applying a snapshot (apply|load)", "apply", ValidateRegex, DFT_REGEX_STR);
CONFIG_TUNING_STR(SNAPSHOT_APPLY_POLICY_IGNORE, "snapshot.apply.policy.ignore", TUNING_PUB, "Comma separated list for ignored policies in applying a snapshot", "", ValidateRegex, DFT_REGEX_STR);

// parse tunings
PARSE_TUNING_STR(s_applyAct, SNAPSHOT_APPLY_ACTION);
PARSE_TUNING_STR(s_policyIgnore, SNAPSHOT_APPLY_POLICY_IGNORE);

static bool
Parse(const char *name, const char *value, bool isNew)
{
    bool r = true;

    TuneStatus s = ParseTune(name, value, isNew);
    if (s == TUNE_INVALID_NAME) {
        HexLogWarning("Unknown settings name \"%s\" = \"%s\" ignored", name, value);
    }
    else if (s == TUNE_INVALID_VALUE) {
        HexLogError("Invalid settings value \"%s\" = \"%s\"", name, value);
        r = false;
    }
    return r;
}

static int
ApplyPolicy(const char* backupBaseDir, const char* snapshotBaseDir)
{
    HexLogInfo("%sing snapshot with ignored policies: %s", s_applyAct.c_str(), s_policyIgnore.c_str());

    std::string cmd = "adapt_and_" + s_applyAct.newValue();
    std::string policyDir = snapshotBaseDir;
    policyDir += "/etc/policies";

    auto policyIgnoreVec = hex_string_util::split(s_policyIgnore, ',');

    for (auto it = policyIgnoreVec.begin(); it != policyIgnoreVec.end(); ++it) {
        // Only honor policy names that consist only of alphanumeric or underscore characters
        if (it->find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_") != std::string::npos)
            continue;

        HexLogDebugN(FWD, "Ignoring policy: %s", it->c_str());

        // Remove ignored policy from set of files to commit
        if (HexLogDebugLevel >= RRA) {
            HexLogDebugN(RRA, "Deleting policy from snapshot commit:");
            HexSystemF(0, "find %s -type f -name \"%s*.*\" | logger -t '%s[%d]'",
                          policyDir.c_str(), it->c_str(), HexLogProgramName(), getpid());
        }

        HexSystemF(0, "find %s -type f -name \"%s*.*\" | xargs rm -f", policyDir.c_str(), it->c_str());

        // Restore original policy
        if (HexLogDebugLevel >= RRA) {
            HexLogDebugN(RRA, "Restoring current policy to snapshot commit:");
            HexSystemF(0, "cd / && find ./etc/policies -depth -type f -name \"%s*.*\" | logger -t '%s[%d]'",
                          it->c_str(), HexLogProgramName(), getpid());
        }

        HexSystemF(0, "cd / && find ./etc/policies -depth -type f -name \"%s*.*\" | cpio --quiet -pumd %s",
                      it->c_str(), snapshotBaseDir);
    }

    HexLogDebugN(FWD, "Commit policies in %s", policyDir.c_str());
    int status = HexExitStatus(HexSpawn(0, HEX_CFG, "-p", cmd.c_str(), policyDir.c_str(), ZEROCHAR_PTR));
    HexLogDebugN(FWD, "Commit returned status %d", status);

    return status;
}

// The policies
CONFIG_SNAPSHOT_FILE("/etc/policies");

// appliance state (first-time setup is done)
CONFIG_SNAPSHOT_MANAGED_FILE(SLA_ACCPETED_FILE, "root", "www-data", 0644);
CONFIG_SNAPSHOT_MANAGED_FILE(CONFIGURED_FILE, "root", "www-data", 0644);

CONFIG_SNAPSHOT_COMMAND_WITH_SETTINGS(policy, 0, ApplyPolicy, 0);
CONFIG_SNAPSHOT_COMMAND_LAST(policy);

CONFIG_MODULE(snapshot, 0, Parse, 0, 0, 0);

