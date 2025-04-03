// HEX SDK

#include <cstring>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <hex/log.h>
#include <hex/process.h>
#include <hex/process_util.h>

#include <hex/config_module.h>
#include <hex/config_tuning.h>
#include <hex/dryrun.h>

// public tunings
CONFIG_TUNING_STR(TIME_TZ, "time.timezone", TUNING_PUB, "Set system timzeon.", "America/New_York", ValidateRegex, "^[a-zA-Z/]+$");

// parse tunings
PARSE_TUNING_STR(s_tz, TIME_TZ);

static bool s_bTimeModified = false;

static int
SetTimezone(const char* timezone)
{
    std::string timezoneFile = "/usr/share/zoneinfo/";
    timezoneFile += timezone;

    if (access(timezoneFile.c_str(), F_OK) != 0) {
        HexLogError("Timezone %s is unknown.\n", timezone);
        return 1;
    }

    const char* sysTimezoneFile = "/etc/localtime";
    if (access(sysTimezoneFile, F_OK) == 0) {
        if (unlink(sysTimezoneFile) != 0) {
            int rc = errno;
            HexLogError("Could not remove old timezone file: %s\n", strerror(rc));
            return rc;
        }
    }

    HexLogInfo("Set timezone to %s", timezone);
    if (symlink(timezoneFile.c_str(), sysTimezoneFile) != 0) {
        int rc = errno;
        HexLogError("Could not set new timezone file: %s\n", strerror(rc));
        return rc;
    }

    return 0;
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
        s_bTimeModified = true;
        return true;
    }

    s_bTimeModified = modified;

    return s_bTimeModified;
}

static bool
Commit(bool modified, int dryLevel)
{
    // TODO: remove this if support dry run
    HEX_DRYRUN_BARRIER(dryLevel, true);

    if (!CommitCheck(modified, dryLevel))
        return true;

    if (s_bTimeModified) {
        SetTimezone(s_tz.c_str());
    }

    return true;
}

static void
TimezoneUsage(void)
{
    fprintf(stderr, "Usage: %s timezone <timezone>\n", HexLogProgramName());
}

static int
TimezoneMain(int argc, char* argv[])
{
    /* Check we've been called with correct number of arguments */
    if (argc != 2) {
        HexLogError("Incorrect number of arguments supplied to hex_config timezone");
        TimezoneUsage();
        return 1;
    }

    return SetTimezone(argv[1]);
}

static void
TimezoneShowUsage(void)
{
    fprintf(stderr, "Usage: %s show_timezone\n", HexLogProgramName());
}

static int
TimezoneShowMain(int argc, char* argv[])
{
    /* Check we've been called with correct number of arguments */
    if (argc != 1) {
        HexLogError("Incorrect number of arguments supplied to hex_config show_timezone");
        TimezoneShowUsage();
        return 1;
    }

    std::string timezoneFile = "/usr/share/zoneinfo/";
    const char* sysTimezoneFile = "/etc/localtime";

    struct stat sb;
    char *timezone;
    ssize_t r;

    if (lstat(sysTimezoneFile, &sb) == -1) {
        HexLogError("Could not get the file status");
        return 1;
    }


    timezone = new char[sb.st_size+1];
    if (timezone == NULL) {
        HexLogError("Could not allocate memory");
        return 1;
    }

    r = readlink(sysTimezoneFile, timezone, sb.st_size+1);

    if (r < 0) {
        int rc = errno;
        HexLogError("Could not get the timezone: %s\n", strerror(rc));
        delete[] timezone;
        timezone = NULL;
        return rc;
    }

    if (r > sb.st_size) {
        HexLogError("timezone size is inconsistent:%zu %zu", r, sb.st_size);
        delete[] timezone;
        timezone = NULL;
        return 1;
    }

    timezone[sb.st_size] = '\0';
    printf("%s", (timezone+timezoneFile.size()));

    delete[] timezone;
    timezone = NULL;

    return 0;
}

static void
DatetimeUsage(void)
{
    fprintf(stderr, "Usage: %s datetime <YYYY-MM-DD> <hh:mm:ss>\n", HexLogProgramName());
}

static int
DatetimeMain(int argc, char* argv[])
{
    /* Check we've been called with correct number of arguments */
    if (argc != 3) {
        HexLogError("Incorrect number of arguments supplied to hex_config datetime");
        DatetimeUsage();
        return 1;
    }

    const char* strDate = argv[1];
    const char* strTime = argv[2];

    std::string datetime = "\"" + std::string(strDate) + " " + std::string(strTime) + "\"";

    /* Check we've been called with date in correct format */
    struct tm tmTime;
    char* ret = strptime(datetime.c_str(), "\"%Y-%m-%d %H:%M:%S\"", &tmTime);

    if (ret == NULL || *ret != '\0') {
        // The time is in an invalid format.
        HexLogError("Time value %s is not in the correct format.\n", datetime.c_str());
        DatetimeUsage();
        return 1;
    }

    int rc = HexUtilSystem(0, 0, "/bin/date", "-s", datetime.c_str(), ZEROCHAR_PTR);

    // Sync to hardware clock in case we lose power and don't shutdown gracefully
    HexUtilSystem(0, 0, "/sbin/hwclock -w -u", ZEROCHAR_PTR);

    // Tell other modules that date/time has changed
    HexSystemF(0, HEX_CFG " trigger date_time_changed");

    return rc;
}

static void
DateUsage(void)
{
    fprintf(stderr, "Usage: %s date '\"<YYYY-MM-DD hh:mm:ss>\"'\n", HexLogProgramName());
}

static int
DateMain(int argc, char* argv[])
{
    /* Check we've been called with correct number of arguments */
    if (argc != 2) {
        HexLogError("Incorrect number of arguments supplied to hex_config date");
        DateUsage();
        return 1;
    }
    const char* strTime = argv[1];

    /* Check we've been called with date in correct format */
    struct tm tmTime;
    char* ret = strptime(strTime, "\"%Y-%m-%d %H:%M:%S\"", &tmTime);

    if (ret == NULL || *ret != '\0') {
        // The time is in an invalid format.
        HexLogError("Time value %s is not in the correct format.\n", strTime);
        DateUsage();
        return 1;
    }

    int rc = HexUtilSystem(0, 0, "/bin/date", "-s", strTime, ZEROCHAR_PTR);

    // Sync to hardware clock in case we lose power and don't shutdown gracefully
    HexUtilSystem(0, 0, "/sbin/hwclock -w -u", ZEROCHAR_PTR);

    // Tell other modules that date/time has changed
    HexSystemF(0, HEX_CFG " trigger date_time_changed");

    return rc;
}

CONFIG_COMMAND(date,          DateMain,         DateUsage);
CONFIG_COMMAND(datetime,      DatetimeMain,     DatetimeUsage);
CONFIG_COMMAND(timezone,      TimezoneMain,     TimezoneUsage);
CONFIG_COMMAND(show_timezone, TimezoneShowMain, TimezoneShowUsage);

CONFIG_MODULE(time, 0, Parse, 0, 0, Commit);

