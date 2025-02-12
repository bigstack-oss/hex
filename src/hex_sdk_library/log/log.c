// HEX SDK

#define _GNU_SOURCE	// GNU asprintf, vasprintf
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include <hex/log.h>

static const char ENABLE_DEBUG_MARKER[] = "/etc/debug.level";

static char *s_programName = 0;

// Free s_programName to avoid errors from valgrind
static void __attribute__ ((destructor))
Fini()
{
    if (s_programName)
        free(s_programName);
}

int HexLogDebugLevel = 0;

static void
SetDebugLevel(const char *file)
{
    // Read debug level from file
    // Default to 1 if level could not be read
    int newDebugLevel = 1;
    FILE *fin = fopen(file, "r");
    if (fin) {
        fscanf(fin, "%d", &newDebugLevel);
        fclose(fin);
    }

    // Command line option has priority
    // Don't lower debug level if previously set on the command line
    if (newDebugLevel > HexLogDebugLevel)
        HexLogDebugLevel = newDebugLevel;
}

void
HexLogInit(const char *programName, int logToStdErr)
{
    if (s_programName)
        free(s_programName);
    s_programName = strdup(programName);

    closelog();
    openlog(programName, ((logToStdErr) ? LOG_PID|LOG_PERROR : LOG_PID), LOG_USER);

    // Enable debugging if special debug file exists
    // Higher value from both files will be used
    if (access(ENABLE_DEBUG_MARKER, F_OK) == 0)
        SetDebugLevel(ENABLE_DEBUG_MARKER);

    char *debugFile;
    if (asprintf(&debugFile, "%s.%s", ENABLE_DEBUG_MARKER, programName) > 0) {
        if (access(debugFile, F_OK) == 0)
            SetDebugLevel(debugFile);
        free(debugFile);
    }
}

void
_HexLogDebugData(const char *label, const unsigned char* data, size_t len)
{
    const unsigned char* p = data;
    HexLogDebug("%s[%d]", label, (int) len);
    char buf[80];
    int n = 0;
    for (size_t i = 0; i < len; i++) {
        if (i % 16 == 0) {
            if (n) {
                HexLogDebug("%s", buf);
                n = 0;
            }
            n += sprintf(&buf[n], "%04zx:", i);
        } else if (i % 8 == 0)
            n += sprintf(&buf[n], "  ");
        n += sprintf(&buf[n], " %02x", *p++);
    }
    HexLogDebug("%s", buf);
}

void
_HexLogDebugTextData(const char *label, const unsigned char* data, size_t len)
{
    const unsigned char* p = data;
    HexLogDebug("%s", label);
    char buf[4096];
    int n = 0;
    if (len > 4096) len = 4096;
    for (size_t i = 0; i < len; i++) {
        n += sprintf(&buf[n], "%c", *p++);
    }
    HexLogDebug("%s", buf);
}

const char *
HexLogProgramName()
{
    if (s_programName)
        return s_programName;
    else
        return "unknown";
}

void HexLogEvent(const char *eventid, const char *fmt, ...)
{
    char *msg;
    int len;
    va_list ap;

    va_start(ap, fmt);
    if (vasprintf(&msg, fmt, ap) < 0) {
        return;
    }
    va_end(ap);

    // Pick up timezone changes
    tzset();

    len = strlen(eventid);
    if (len && eventid[len - 1] == 'E') {
        syslog(LOG_ERR, "%s:: |%s|", eventid, msg);
    }
    else if (len && eventid[len - 1] == 'W') {
        syslog(LOG_WARNING, "%s:: |%s|", eventid, msg);
    }
    else {
        syslog(LOG_INFO, "%s:: |%s|", eventid, msg);
    }

    free(msg);
}

// add escape char '\' before ',' and '\'
void HexLogEscape(const char *orig, char *escaped, size_t escplen)
{
    char *pescaped = escaped;
    const char *porig = orig;

    while (*porig != '\0' && escplen) {
        if (*porig == ',' || *porig == '\\') {
            *pescaped++ = '\\';
            --escplen;
        }
        *pescaped++ = *porig++;
        --escplen;
    }
    *pescaped++ = '\0';
}

// remove escape char
void HexLogUnescape(char *escaped)
{
    char *pescaped = escaped;
    char *porig = escaped;

    while (*pescaped != '\0') {
        if (*pescaped == '\\' &&
            (*(pescaped + 1) == ',' || *(pescaped + 1) == '\\')) {
            *porig++ = *(pescaped + 1);
            pescaped += 2;
        } else {
            *porig++ = *pescaped;
            pescaped += 1;
        }
    }
    *porig = '\0';
}

void HexLogEventNoArg(const char *eventid)
{
    HexLogEvent(eventid, "%s", "");
}

