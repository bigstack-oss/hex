// HEX SDK

#include <cstdio>
#include <string>
#include <map>
#include <set>
#include <string.h>
#include <hex/log.h>
#include <hex/process.h>

#include <hex/config_module.h>
#include <hex/config_tuning.h>
#include <hex/dryrun.h>

// private tunings
CONFIG_TUNING_INT(DBG_LVL, "debug.level", TUNING_UNPUB, "Set global debug level", 0, 0, 2);

// public tunigns
CONFIG_TUNING_INT(DBG_LVL_PROC, "debug.level.%s", TUNING_PUB, "Set debug level for process %s", 0, 0, 9);
CONFIG_TUNING_BOOL(DBG_CORE_PROC, "debug.enable_core_dump.%s", TUNING_PUB, "Enable core dump for process %s", false);
CONFIG_TUNING_INT(DBG_CORE_MAX, "debug.max_core_dump", TUNING_PUB, "Set the total number of core files before oldest are removed", 0, 0, 999);
CONFIG_TUNING_BOOL(DBG_KERNEL_DUMP, "debug.enable_kdump", TUNING_PUB, "Enable kdump to collect dump from kernel panic", false);

// parse tunings
PARSE_TUNING_INT(s_globalDebugLevel, DBG_LVL);
PARSE_TUNING_INT_MAP(s_debugLevelMap, DBG_LVL_PROC);
PARSE_TUNING_BOOL_MAP(s_enableCoreMap, DBG_CORE_PROC);
PARSE_TUNING_INT(s_maxCore, DBG_CORE_MAX);
PARSE_TUNING_BOOL(s_enableKdump, DBG_KERNEL_DUMP);

static const char ENABLE_DEBUG_MARKER[] = "/etc/debug.level";
static const char ENABLE_CORE_FILES_MARKER[] = "/etc/debug.enable_core_files";
static const char MAX_CORE_FILE[] = "/etc/debug.max_core_files";

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
Commit(bool unused, int dryLevel)
{
    // TODO: remove this if support dry run
    HEX_DRYRUN_BARRIER(dryLevel, true);

    // Delete files before creating new ones
    HexSystemF(0, "rm -f %s* %s* %s", ENABLE_DEBUG_MARKER, ENABLE_CORE_FILES_MARKER, MAX_CORE_FILE);

    if (s_globalDebugLevel > 0)
        HexSystemF(0, "echo %d > %s", (int)s_globalDebugLevel, ENABLE_DEBUG_MARKER);

    for (auto it = s_debugLevelMap.begin(); it != s_debugLevelMap.end(); ++it)
        HexSystemF(0, "echo %d > %s.%s", (int)it->second, ENABLE_DEBUG_MARKER, it->first.c_str());

    for (auto it = s_enableCoreMap.begin(); it != s_enableCoreMap.end(); ++it)
        if (it->second)
            HexSystemF(0, "touch %s.%s", ENABLE_CORE_FILES_MARKER, it->first.c_str());

    if (s_enableKdump) {
        if (HexSystemF(0, "hex_kdump enable")==0)
            HexLogInfo("Kdump enabled");
        else
            HexLogWarning("Failed to enable kdump");
    }
    else {
        if (HexSystemF(0, "hex_kdump disable")==0)
            HexLogInfo("Kdump disabled");
        else
            HexLogWarning("Failed to disable kdump");
    }

    FILE *maxCoreFile = fopen(MAX_CORE_FILE, "w");
    if (maxCoreFile) {
        HexLogDebugN(FWD, "Setting maximum number of core files: %d", (int)s_maxCore);
        fprintf(maxCoreFile, "%d\n", (int)s_maxCore);
        fclose(maxCoreFile);
    }

    return true;
}

CONFIG_MODULE(debug, NULL, Parse, NULL, NULL, Commit);

// Make best effort to run before other modules
CONFIG_FIRST(debug);

