// HEX SDK

#include <vector>

#include <hex/yml_util.h>
#include <hex/log.h>
#include <hex/process.h>
#include <hex/process_util.h>
#include <hex/filesystem.h>
#include <hex/dryrun.h>

#include <hex/config_module.h>
#include <hex/config_tuning.h>

#define CONF "/etc/dnf/automatic.conf"

struct UpdateRecord {
    std::string image;
    std::string type;
    std::string version;
    std::string variant;
    std::string builtAt;
    std::string createdAt;
};

using UpdateHistory = std::vector<UpdateRecord>;

static const char UPDATE_HISTORY[] = "/var/appliance-db/update.history";

static bool s_bUpdateModified = false;

// public tunings
CONFIG_TUNING_BOOL(UPDATE_SEC_AUTO, "update.security.autoupdate", TUNING_PUB, "Set to true to enable security auto update.", true);

// parse tunings
PARSE_TUNING_BOOL(s_secAuto, UPDATE_SEC_AUTO);

static int
UpdateConfig(bool autoupdate)
{
    FILE *fout = fopen(CONF, "w");
    if (!fout) {
        HexLogError("Could not write checker file: %s", CONF);
        return -1;
    }

    fprintf(fout, "[commands]\n");
    fprintf(fout, "upgrade_type = security\n");
    fprintf(fout, "download_updates = yes\n");
    fprintf(fout, "apply_updates = %s\n", autoupdate ? "yes" : "no");
    fprintf(fout, "[emitters]\n");
    fprintf(fout, "emit_via = stdio\n");

    fclose(fout);

    HexSetFileMode(CONF, "root", "root", 0644);

    return 0;
}

static bool
CommitDnfAutomatic(bool autoupdate)
{
    if (autoupdate) {
        HexSystemF(0, "/usr/bin/systemctl enable dnf-automatic.timer 2>/dev/null");
        HexSystemF(0, "/usr/bin/systemctl start dnf-automatic.timer 2>/dev/null");
    }
    else {
        HexSystemF(0, "/usr/bin/systemctl disable dnf-automatic.timer 2>/dev/null");
        HexSystemF(0, "/usr/bin/systemctl stop dnf-automatic.timer 2>/dev/null");
    }

    return true;
}

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
CommitCheck(bool modified, int dryLevel)
{
    if (IsBootstrap()) {
        s_bUpdateModified = true;
        return true;
    }

    s_bUpdateModified = modified;

    return s_bUpdateModified;
}

static bool
Commit(bool modified, int dryLevel)
{
    // TODO: remove this if support dry run
    HEX_DRYRUN_BARRIER(dryLevel, true);

    if (!CommitCheck(modified, dryLevel))
        return true;

    if (s_bUpdateModified) {
        UpdateConfig(s_secAuto);
        CommitDnfAutomatic(s_secAuto);
    }

    return true;
}

static bool
ParseUpdateHistoryFile(UpdateHistory *history)
{
    GNode *yml = InitYml("history");

    if (ReadYml(UPDATE_HISTORY, yml) < 0) {
        FiniYml(yml);
        HexLogError("Failed to parse %s", UPDATE_HISTORY);
        return false;
    }

    size_t num = SizeOfYmlSeq(yml, "history");
    if (num) {
        for (size_t i = 1 ; i <= num ; i++) {
            UpdateRecord obj;
            HexYmlParseString(obj.image, yml, "history.%d.image", i);
            HexYmlParseString(obj.type, yml, "history.%d.type", i);
            HexYmlParseString(obj.version, yml, "history.%d.version", i);
            HexYmlParseString(obj.variant, yml, "history.%d.variant", i);
            HexYmlParseString(obj.builtAt, yml, "history.%d.built-at", i);
            HexYmlParseString(obj.createdAt, yml, "history.%d.created-at", i);

            history->push_back(obj);
        }
    }

    FiniYml(yml);

    return true;
}

static void
UsageGetUpdateInfo()
{
    fprintf(stderr, "Usage: %s show_update_info \n", HexLogProgramName());
}

static int
MainGetUpdateInfo(int argc, char **argv)
{
    UpdateHistory history;

    if (argc > 1) {
        UsageGetUpdateInfo();
        return EXIT_FAILURE;
    }

    if (!ParseUpdateHistoryFile(&history))
        return EXIT_FAILURE;

    if (history.size()) {
        int currentIdx, rollbackIdx;

        currentIdx = history.size() - 1;
        rollbackIdx = (currentIdx - 1) >= 0 ? (currentIdx - 1) : currentIdx;

        printf("Current: %s\n", history[currentIdx].version.c_str());
        printf("Rollback: %s\n", history[rollbackIdx].version.c_str());
    }

    return EXIT_SUCCESS;
}


static void
UsageGetUpdateHistory()
{
    fprintf(stderr, "Usage: %s view_update_history [ <format> ] \n", HexLogProgramName());
}

static int
MainGetUpdateHistory(int argc, char **argv)
{
    UpdateHistory history;
    std::string format = "pretty";

    if (argc > 2) {
        UsageGetUpdateHistory();
        return EXIT_FAILURE;
    }
    else if (argc == 2) {
        format = argv[1];
        if (format.compare("pretty") != 0) {
            UsageGetUpdateHistory();
            return EXIT_FAILURE;
        }
    }

    if (!ParseUpdateHistoryFile(&history))
        return EXIT_FAILURE;

    char line[97];
    memset(line, '-', sizeof(line));
    line[sizeof(line) - 1] = 0;

#define STATS_H_FMT " %10s  %10s  %10s  %10s  %22s  %22s\n"
#define STATS_FMT " %10s  %10s  %10s  %10s  %22s  %22s\n"

    printf(STATS_H_FMT, "image", "type", "version", "variant", "built at", "created at");
    printf("%s\n", line);

    for (auto& i : history) {
        printf(STATS_FMT, i.image.c_str(), i.type.c_str(), i.version.c_str(),
                          i.variant.c_str(), i.builtAt.c_str(), i.createdAt.c_str());
    }

    return EXIT_SUCCESS;
}

CONFIG_COMMAND(show_update_info, MainGetUpdateInfo, UsageGetUpdateInfo);
CONFIG_COMMAND(get_update_history, MainGetUpdateHistory, UsageGetUpdateHistory);

CONFIG_SUPPORT_FILE(UPDATE_HISTORY);
CONFIG_SUPPORT_COMMAND("ls -l /var/update");

CONFIG_MODULE(update, 0, Parse, 0, 0, Commit);

CONFIG_FIRST(update);

CONFIG_MIGRATE(update, UPDATE_HISTORY);

