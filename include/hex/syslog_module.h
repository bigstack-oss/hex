// HEX SDK

#ifndef HEX_SYSLOGMODULE_H
#define HEX_SYSLOGMODULE_H

#ifdef __cplusplus

#include <hex/syslog_impl.h>
#include <hex/hex_impl.h>

// Special system event nvp name to set a specific response for an event
static const char SYSLOG_RESPONSE[] = "syslog_response";

// event regex are executed prior to parse func. parse func will run
// if none of event regex matched and successfully executed
#define SYSLOG_MODULE(module_name, init, set_param, parse)  \
    static hex_syslog::Module HEX_CAT(s_module_,__LINE__)(#module_name, init, set_param, parse)

#define SYSLOG_MODULE_EVENTREGEX(module_name, eventid, eventregex)  \
    static hex_syslog::ModuleEventRegex HEX_CAT(eventid,__LINE__)(#eventid, #module_name, eventregex)

#define SYSLOG_MODULE_EVENTREGEX_ONMATCH(module_name, eventid, eventregex, onmatch) \
    static hex_syslog::ModuleEventRegex HEX_CAT(eventid,__LINE__)(#eventid, #module_name, eventregex, onmatch)

#endif // !__cplusplus

#ifdef __cplusplus
extern "C" {
#endif

// Syslog modules should use this Log API since hex/log.h calls syslog(3)
// The internal implementation will append a newline.
void Log(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));

#define LogFatal(fmt, ...) do { Log("Fatal error: " fmt, ## __VA_ARGS__); abort(); } while (0)

#define LogError(fmt, ...) Log("Error: " fmt, ## __VA_ARGS__)

#define LogWarning(fmt, ...) Log("Warning: " fmt, ## __VA_ARGS__)

#define LogNotice(fmt, ...) Log(fmt, ## __VA_ARGS__)

#define LogInfo(fmt, ...) Log(fmt, ## __VA_ARGS__)

extern int LogDebugLevel;

#define LogDebug(fmt, ...) \
  do { if (LogDebugLevel >= 1) Log("Debug: %s: " fmt, __func__, ## __VA_ARGS__); } while (0)

#define LogDebugN(n, fmt, ...) \
  do { if (LogDebugLevel >= n) Log("Debug: %s: " fmt, __func__, ## __VA_ARGS__); } while (0)

#ifdef NDEBUG
#define LogTrace(fmt, ...)
#define LogTraceN(n, fmt, ...)
#else
#define LogTrace(fmt, ...) \
  do { if (LogDebugLevel >= 1) Log("Trace: %s: " fmt, __func__, ## __VA_ARGS__); } while (0)
#define LogTraceN(n, fmt, ...) \
  do { if (LogDebugLevel >= n) Log("Trace: %s: " fmt, __func__, ## __VA_ARGS__); } while (0)
#endif

#include <sys/uio.h>    // iovec
#include <stdbool.h>    // size_t

bool EventsdSend(int mid, const char* msg, size_t len);

bool EventsdSendV(int mid, const struct iovec *vec, int count);

#ifdef __cplusplus
}
#endif

#endif /* ndef HEX_SYSLOGMODULE_H */

