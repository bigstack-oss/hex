// HEX SDK

#ifndef HEX_LOGROTATE_H
#define HEX_LOGROTATE_H

#ifdef __cplusplus

#include <string>

static const char LOGROTATE_DIR[] = "/etc/logrotate.d";

enum LogRotateFrequency {
    NONE = 0,
    DAILY,
    WEEKLY,
    MONTHLY,
    YEARLY
};

struct LogRotateConf
{
    std::string name;
    std::string logfile;
    LogRotateFrequency frequency;   // rotate frequency
    unsigned int size;              // size in M
    unsigned int retention;         // Log files are rotated "retention" times before being removed
    std::string preRotateCmds;
    std::string postRotateCmds;
    bool copytruncate;
    std::string extraArgs;

    LogRotateConf(const std::string& n, const std::string& l, LogRotateFrequency f,
                  unsigned int s, unsigned int r, bool c = false, const std::string& e = "")
        : name(n), logfile(l), frequency(f), size(s), retention(r), copytruncate(c), extraArgs(e) { }
};

bool WriteDefLogRotateConf(int retention);
bool WriteLogRotateConf(LogRotateConf conf);

#endif  /* endif __cplusplus */

#endif  /* endif HEX_LOGROTATE_H */

