// HEX SDK

#include <unistd.h>

#include <hex/log.h>
#include <hex/filesystem.h>
#include <hex/logrotate.h>

#define DEF_CONF "/etc/logrotate.conf"

bool
WriteDefLogRotateConf(int retention = 4)
{
    HexLogDebug("Writing %s", DEF_CONF);
    FILE *fout = fopen(DEF_CONF, "w");
    if (!fout) {
        HexLogError("Could not write file: %s", DEF_CONF);
        return false;
    }

    fprintf(fout, "daily\n");
    fprintf(fout, "su root syslog\n");
    fprintf(fout, "rotate %d\n", retention);
    fprintf(fout, "create\n");
    fprintf(fout, "include /etc/logrotate.d\n");

    fclose(fout);

    HexSetFileMode(DEF_CONF, "root", "root", 0644);

    return true;
}

bool
WriteLogRotateConf(LogRotateConf conf)
{
    std::string conffile = LOGROTATE_DIR;
    conffile += "/";
    conffile += conf.name;

    if (access(LOGROTATE_DIR, F_OK) != 0) {
        HexLogError("%s doesn't exist", LOGROTATE_DIR);
        return false;
    }

    HexLogDebug("Writing %s", conffile.c_str());
    FILE *fout = fopen(conffile.c_str(), "w");
    if (!fout) {
        HexLogError("Could not write file: %s", conffile.c_str());
        return false;
    }

    fprintf(fout, "%s {\n", conf.logfile.c_str());
    switch(conf.frequency)
    {
        case DAILY:
            fprintf(fout, "  daily\n");
            break;
        case WEEKLY:
            fprintf(fout, "  weekly\n");
            break;
        case MONTHLY:
            fprintf(fout, "  monthly\n");
            break;
        case YEARLY:
            fprintf(fout, "  yearly\n");
            break;
        default: //use global value
            break;
    }

    if (conf.copytruncate)
        fprintf(fout, "  copytruncate\n");

    /*
     * minsize: rotates only when the file has reached an appropriate size AND the set time period has passed.
     * maxsize: will rotate when the log reaches a set size OR the appropriate time has passed
     * size: will rotate when the log > size. REGARDLESS of whether hourly/daily/weekly/monthly is specified
     */
    if (conf.size > 0)
        fprintf(fout, "  maxsize %uM\n", conf.size);

    if (conf.retention > 0)
        fprintf(fout, "  rotate %u\n", conf.retention);

    if (!conf.preRotateCmds.empty()) {
        fprintf(fout, "  prerotate\n");
        fprintf(fout, "    %s\n", conf.preRotateCmds.c_str());
        fprintf(fout, "  endscript\n");
    }

    if (!conf.postRotateCmds.empty()) {
        fprintf(fout, "  postrotate\n");
        fprintf(fout, "    %s\n", conf.postRotateCmds.c_str());
        fprintf(fout, "  endscript\n");
    }

    if (!conf.extraArgs.empty())
        fprintf(fout, "  %s\n", conf.extraArgs.c_str());

    // common configs
    fprintf(fout, "  missingok\n");
    fprintf(fout, "  compress\n");
    fprintf(fout, "  delaycompress\n");
    fprintf(fout, "  notifempty\n");

    fprintf(fout, "}\n");
    fclose(fout);

    HexSetFileMode(conffile.c_str(), "root", "root", 0644);

    return true;
}

