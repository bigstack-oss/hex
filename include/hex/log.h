// HEX SDK

#ifndef HEX_LOG_H
#define HEX_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <syslog.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <hex/hex_impl.h>

enum DebugLogLevel {
    AWY = 0,    // show anyway
    FWD,        // forward
    RRA,        // recurring, repeat, annoying
    DMP         // packet/data dump
};

void HexLogInit(const char *programName, int logToStdErr);

const char *HexLogProgramName();

void _HexLogDebugData(const char *label, const unsigned char *data, size_t len);
void _HexLogDebugTextData(const char *label, const unsigned char *data, size_t len);

#define HexLogDebugData(label, data, len) \
    do { if (unlikely(HexLogDebugLevel >= 1)) { _HexLogDebugData(label, data, len); } } while (0)
#define HexLogDebugTextData(label, data, len) \
    do { if (unlikely(HexLogDebugLevel >= 1)) { _HexLogDebugTextData(label, data, len); } } while (0)

// Event logging interface.
// eventid : unique message id for the event
// fmt : printf style format string (comma separated name=value pairs for event params)
// ... : arguments to format string
// fmt and all character string arguments to it must be UTF-8 encoded.
// Message is logged to the syslog which generates a corresponding 'eventid'
// event.
void HexLogEvent(const char *eventid, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
void HexLogEventNoArg(const char *eventid);

// Escape a string that may contain characters that conflict with internal event
// processing (,)
// Caller must provide buffer large enough to hold the entire escaped string:
// orig : string to be escaped
// escaped : storage for escaped string
// escplen : length of the escaped buffer
//
// returns : null terminated escaped string in the escaped
void HexLogEscape(const char *orig, char *escaped, size_t escplen);

// Macro to get the required size for a buffer based on the length of a string
// in the worst case i.e. all characters in the 'orig' must be escaped,
// escaped would be 2 * strlen(orig) + 1 bytes long
//
#define HEX_LOG_ESCAPE_BUFF_LEN(size) ((2 * size) + 1)

// Unescape a string escaped by HexLogEvent
//
// escaped : string to be unescaped
//
// returns: unescaped string : modifies the passed string 'escaped' in place
void HexLogUnescape(char *escaped);

// These depend on GNU CPP extensions (## and __func__)
// putting ## in front of __VA_ARGS__ for getting rid of the trailing comma when no argument given

#define HexLogFatal(fmt, ...) do { tzset(); syslog(LOG_CRIT, "Fatal error: " fmt, ## __VA_ARGS__); exit(1); } while (0)

#define HexLogError(fmt, ...) do { tzset(); syslog(LOG_ERR, "Error: " fmt, ## __VA_ARGS__); } while (0)

#define HexLogWarning(fmt, ...) do { tzset(); syslog(LOG_WARNING, "Warning: " fmt, ## __VA_ARGS__); } while (0)

#define HexLogNotice(fmt, ...) do { tzset(); syslog(LOG_NOTICE, fmt, ## __VA_ARGS__); } while (0)

#define HexLogInfo(fmt, ...) do { tzset(); syslog(LOG_INFO, fmt, ## __VA_ARGS__); } while (0)

extern int HexLogDebugLevel;

#ifndef likely
#define likely(cond)            __builtin_expect(!!(cond), 1)
#define unlikely(cond)          __builtin_expect(!!(cond), 0)
#endif
// Debug output for release code
// Does not include function names
#define HexLogDebug(fmt, ...) \
  do { if (unlikely(HexLogDebugLevel >= 1)) { tzset(); syslog(LOG_DEBUG, "Debug: " fmt, ## __VA_ARGS__); } } while (0)

#define HexLogDebugN(n, fmt, ...) \
  do { if (unlikely(HexLogDebugLevel >= n)) { tzset(); syslog(LOG_DEBUG, "Debug: " fmt, ## __VA_ARGS__); } } while (0)

// Trace output for non-release code
// Does include function names
// Used same HexLogDebugLevel
#ifdef HEX_PROD
#define HexLogTrace(fmt, ...)
#define HexLogTraceN(n, fmt, ...)
#define HexLogTraceData(label, data, len)
#else
#define HexLogTrace(fmt, ...) \
  do { if (HexLogDebugLevel >= 1) { tzset(); syslog(LOG_DEBUG, "Trace: %s: " fmt, __func__, ## __VA_ARGS__); } } while (0)
#define HexLogTraceN(n, fmt, ...) \
  do { if (HexLogDebugLevel >= n) { tzset(); syslog(LOG_DEBUG, "Trace: %s: " fmt, __func__, ## __VA_ARGS__); } } while (0)
#define HexLogTraceData(label, data, len) \
  do { if (HexLogDebugLevel >= 1) HexLogDebugData(label, data, len); } while (0)
#endif

#define HexLogClose closelog

#ifdef __cplusplus
}
#endif

#endif /* endif HEX_LOG_H */
