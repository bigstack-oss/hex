// HEX SDK

#include <string>
#include <signal.h>
#include <vector>
#include <sys/statvfs.h>
#include <errno.h>
#include <unistd.h>

#include <hex/log.h>
#include <hex/process.h>
#include <hex/logrotate.h>
#include <hex/process_util.h>
#include <hex/tuning.h>

#include <hex/config_module.h>
#include <hex/config_tuning.h>
#include <hex/dryrun.h>

static const char RSYSLOG[] = "rsyslog";
static const char RSYSLOG_CONF[] = "/etc/rsyslog.conf";

static const char LOGDIR[] = "/var/log";

// rotate daily and enable copytruncate
static LogRotateConf log_conf("syslog", "/var/log/messages /var/log/secure /var/log/cron", DAILY, 128, 0, true);

static bool s_bLogrotateChanged = false;

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

    char buf[64];
    uint64_t upperlimit = GetDiskSizeKB(LOGDIR) / 100 * percent;

    snprintf(buf, 64, "/usr/sbin/hex_trim_syslog %lu", upperlimit);
    log_conf.postRotateCmds = buf;
    WriteLogRotateConf(log_conf);

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

    if (s_bLogrotateChanged) {
        UpdateLogrotateConfig((unsigned)s_diskPercentage);
        HexUtilSystemF(FWD, 0, "systemctl restart %s", RSYSLOG);
    }

    return true;
}

CONFIG_MODULE(syslog, 0, Parse, NULL, Prepare, Commit);

// Start syslogd as early as possible and after eventsd so that config modules can log events
// CONFIG_REQUIRES(syslog, events);
CONFIG_FIRST(syslog);

CONFIG_SUPPORT_FILE("/etc/rsyslogd.conf");

