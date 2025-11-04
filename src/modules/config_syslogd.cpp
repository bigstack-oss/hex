// HEX SDK

#include <errno.h>
#include <signal.h>
#include <string>
#include <sys/statvfs.h>
#include <vector>

#include <hex/log.h>
#include <hex/logrotate.h>
#include <hex/process.h>
#include <hex/process_util.h>
#include <hex/tuning.h>

#include <hex/config_module.h>
#include <hex/config_tuning.h>
#include <hex/dryrun.h>

static const char RSYSLOG[] = "rsyslog";
static const char RSYSLOG_CONF[] = "/etc/rsyslog.conf";
static const char HEX_RSYSLOG_CONF[] = "/etc/rsyslog.d/hex.conf";

// private tunings
CONFIG_TUNING_UINT(SYSLOG_DISK_PERC, "syslog.disk_percentage", TUNING_UNPUB, "Set max (all) log file size to a percentage of the partition space.", 5, 1, 50);

// parse tunings
PARSE_TUNING_UINT(s_diskPercentage, SYSLOG_DISK_PERC);

static bool
UpdateConfig(const char* filepath)
{
    FILE* fout = fopen(filepath, "w");
    if (!fout)
        return false;

    fprintf(fout, "module(load=\"imuxsock\" SysSock.Use=\"off\")\n");
    fprintf(fout, "module(load=\"imjournal\" StateFile=\"imjournal.state\")\n");
    fprintf(fout, "\n");
    fprintf(fout, "global(workDirectory=\"/var/lib/rsyslog\")\n");
    fprintf(fout, "\n");
    fprintf(fout, "$template TraditionalFileFormat,\"%%TIMESTAMP%% %%syslogtag%%%%msg:::sp-if-no-1st-sp%%%%msg:::drop-last-lf%%\\n\"\n");
    fprintf(fout, "\n");
    fprintf(fout, "module(load=\"builtin:omfile\" Template=\"TraditionalFileFormat\")\n");
    fprintf(fout, "\n");
    fprintf(fout, "include(file=\"/etc/rsyslog.d/*.conf\" mode=\"optional\")\n");
    fprintf(fout, "*.info;authpriv.none;cron.none                /var/log/messages\n");
    fprintf(fout, "authpriv.*                                    /var/log/secure\n");
    fprintf(fout, "cron.*                                        /var/log/cron\n");
    fprintf(fout, "*.emerg                                       :omusrmsg:*\n");
    fprintf(fout, "uucp,news.crit                                /var/log/spooler\n");
    fprintf(fout, "local7.*                                      /var/log/boot.log\n");
    fclose(fout);

    return true;
}

static bool
updateHexLogConfig()
{
    FILE* fout = fopen(HEX_RSYSLOG_CONF, "w");
    if (!fout) {
        return false;
    }

    fprintf(fout, ":programname, isequal, \"hex_cli\"  /var/log/hex_cli.log\n");
    fprintf(fout, ":programname, isequal, \"hex_config\"  /var/log/hex_config.log\n");
    fprintf(fout, ":programname, isequal, \"hex_firsttime\"  /var/log/hex_firsttime.log\n");
    fprintf(fout, ":programname, isequal, \"hex_sdk\"  /var/log/hex_sdk.log\n");
    fprintf(fout, ":programname, isequal, \"hex_translate\"  /var/log/hex_translate.log\n");

    fclose(fout);
    return true;
}

static const char LOGDIR[] = "/var/log";

// rotate daily and enable copytruncate
static LogRotateConf logConf(
    "syslog",
    "/var/log/messages\n"
    "/var/log/secure\n"
    "/var/log/cron\n"
    "/var/log/maillog\n"
    "/var/log/spooler",
    DAILY,
    128,
    0,
    true);

static bool s_bLogrotateChanged = false;

// Determine the total disk size in KB
static uint64_t
GetDiskSizeKB(const char* absoluteFilePath)
{
    struct statvfs fs;
    uint64_t diskSize = 5242880; // 5GB = 5 * 1024 * 1024 KB (minimum)

    if (statvfs(absoluteFilePath, &fs) == -1) {
        HexLogDebug("Failed to get disk size (errno=%d) using default", errno);
        return diskSize;
    }

    diskSize = fs.f_bsize * fs.f_blocks / 1024;
    HexLogDebug("Disk size: %ld (KB) (%ld,%ld)", diskSize, fs.f_bsize, fs.f_blocks);
    return diskSize;
}

static bool
UpdateLogrotateConfig(unsigned percent)
{
    // We should not have any problem of underflow becuase of the minimum disk size,
    // but we need to prevent integer overflow. So we do division first
    std::uint64_t upperlimit = GetDiskSizeKB(LOGDIR) / 100 * percent;

    std::stringstream cmd;
    cmd << "/usr/sbin/hex_trim_syslog "
        << upperlimit
        << " || true; /usr/bin/systemctl -s HUP kill rsyslog.service >/dev/null 2>&1 || true";
    logConf.postRotateCmds = cmd.str();
    WriteLogRotateConf(logConf);

    return true;
}

static LogRotateConf hexLogrotateConf(
    "hex",
    "/var/log/hex_cli.log\n"
    "/var/log/hex_config.log\n"
    "/var/log/hex_firsttime.log\n"
    "/var/log/hex_sdk.log\n"
    "/var/log/hex_translate.log",
    DAILY,
    128,
    0,
    true);

static bool
writeHexLogrotateConfig()
{
    return WriteLogRotateConf(hexLogrotateConf);
}

static bool
Parse(const char* name, const char* value, bool isNew)
{
    bool r = true;

    TuneStatus s = ParseTune(name, value, isNew);
    if (s == TUNE_INVALID_NAME) {
        HexLogWarning("Unknown settings name \"%s\" = \"%s\" ignored", name, value);
    } else if (s == TUNE_INVALID_VALUE) {
        HexLogError("Invalid settings value \"%s\" = \"%s\"", name, value);
        r = false;
    }
    return r;
}

static bool
Prepare(bool modified, int dryLevel)
{
    if (IsBootstrap()) {
        s_bLogrotateChanged = true;
        return true;
    }

    if (s_diskPercentage.modified())
        s_bLogrotateChanged = true;

    return true;
}

static bool
Commit(bool modified, int dryLevel)
{
    // TODO: remove this if support dry run
    HEX_DRYRUN_BARRIER(dryLevel, true);

    UpdateConfig(RSYSLOG_CONF);
    updateHexLogConfig();

    if (s_bLogrotateChanged) {
        UpdateLogrotateConfig((unsigned)s_diskPercentage);
        writeHexLogrotateConfig();
        HexUtilSystemF(FWD, 0, "systemctl restart %s", RSYSLOG);
    }

    return true;
}

CONFIG_MODULE(syslog, 0, Parse, NULL, Prepare, Commit);

// Start syslogd as early as possible and after eventsd so that config modules can log events
// CONFIG_REQUIRES(syslog, events);
CONFIG_FIRST(syslog);

CONFIG_SUPPORT_FILE("/etc/rsyslogd.conf");
