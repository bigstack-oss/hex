// HEX SDK

#include <vector>

#include <hex/log.h>
#include <hex/process.h>
#include <hex/process_util.h>
#include <hex/pidfile.h>

#include <hex/config_module.h>
#include <hex/config_tuning.h>
#include <hex/dryrun.h>

CONFIG_TUNING_BOOL(CRON_ENABLED, "cron.enabled", TUNING_UNPUB, "Set to true to enable the cron daemon.", true);
CONFIG_TUNING_INT(CRON_DEBUG, "cron.debug", TUNING_UNPUB, "Set cron daemon log level. Set higher value for more verbose", 8, 0, 8);

PARSE_TUNING_BOOL(s_enabled, CRON_ENABLED);
PARSE_TUNING_INT(s_debug_level, CRON_DEBUG);

static const char CRND_NAME[] = "crond";

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

static bool
Commit(bool modified, int dryLevel)
{
    // TODO: remove this if support dry run
    HEX_DRYRUN_BARRIER(dryLevel, true);

    if (!modified)
        return true;

    HexUtilSystemF(FWD, 0, "systemctl stop %s", CRND_NAME);

    if (s_enabled) {
        HexLogDebug("starting %s service", CRND_NAME);

        int status = HexUtilSystemF(0, 0, "systemctl start %s", CRND_NAME);
        if (status != 0) {
            HexLogError("failed to start %s", CRND_NAME);
        }

        HexLogInfo("%s is running", CRND_NAME);
    }
    else {
        HexLogInfo("%s has been stopped", CRND_NAME);
    }

    return true;
}

CONFIG_MODULE(cron, 0, Parse, 0, 0, Commit);

// the first batch of startup service
CONFIG_FIRST(cron);

// add this in any module that will write a crontab file.
// CONFIG_REQUIRES(crond, <module that needs crond>);

CONFIG_SUPPORT_FILE("/etc/crontab");
CONFIG_SUPPORT_FILE("/etc/cron.d");
CONFIG_SUPPORT_FILE("/etc/cron.daily");
CONFIG_SUPPORT_FILE("/etc/cron.hourly");
CONFIG_SUPPORT_FILE("/etc/cron.monthly");
CONFIG_SUPPORT_FILE("/etc/cron.weekly");

