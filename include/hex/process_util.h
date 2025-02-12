// HEX SDK

#ifndef HEX_PROCESS_UTIL_H
#define HEX_PROCESS_UTIL_H

// Process extended API requires C++
#ifdef __cplusplus

#include <string>
#include <cstdio> // NULL
#include <hex/hex_impl.h>

// Return command output as string
std::string HexUtilPOpen(const char *arg, ...);

// The following function is used to allow us to 'page' the output of a
// shell command.  We do this by piping the command to 'more'.  Unfortunately
// we cannot do this directly from the HexSpawn command as it doesn't like
// pipes (i.e. '|'). The easiest way around this is to embed the command in
// a tempory shell script.
//
int HexUtilSystemWithPage(const char* command);

// Versions of System[VF] with the following enhancements:
// 1. When HexLogDebugLevel < 'debugLevel', stdout and stderr will be redirected to /dev/null
// 2. When HexLogDebugLevel >= 'debugLevel', "Executing: ..." will be written to syslog
// 2. When HexLogDebugLevel >= 'debugLevel', stdout and stderr will be piped to "logger -t hex_xxx"
//
// Recommendations:
// When its important to be able to diagnose customer problems after the fact, its recommended that the
// debug level be set to 0, unless the command is run too frequently in which case 1 can be used.
// If the command failures are expected (e.g. deleting a resource on bootstrap that doesn't exist), then
// the debug level should be set to 2.
//
// Notes:
// 1. These functions should only be used inside hex_xxx modules.
// 2. These functions should NOT be used with shell commands that require their own redirection of
//    stdout and/or stderr.
//
int HexUtilSystem_(int debugLevel, int timeout, const char *location, const char *arg0, ...);
int HexUtilSystemV_(int debugLevel, int timeout, const char *location, char *const argv[]);
int HexUtilSystemF_(int debugLevel, int timeout, const char *location, const char *fmt, ...) __attribute__ ((format (printf, 4, 5)));

#ifdef NDEBUG
#define HexUtilSystem(debugLevel, timeout, arg0, ...)    HexUtilSystem_(debugLevel, timeout, NULL, arg0, ## __VA_ARGS__)
#define HexUtilSystemV(debugLevel, timeout, argv)        HexUtilSystemV_(debugLevel, timeout, NULL, argv)
#define HexUtilSystemF(debugLevel, timeout, fmt, ...)    HexUtilSystemF_(debugLevel, timeout, NULL, fmt, ## __VA_ARGS__)
#else
// When compiling in debug mode we'll include the location in the source so its easier to track problems back to the code
#define HexUtilSystem(debugLevel, timeout, arg0, ...)    HexUtilSystem_(debugLevel, timeout, __FILE__ ":" HEX_STR(__LINE__), arg0, ## __VA_ARGS__)
#define HexUtilSystemV(debugLevel, timeout, argv)        HexUtilSystemV_(debugLevel, timeout, __FILE__ ":" HEX_STR(__LINE__), argv)
#define HexUtilSystemF(debugLevel, timeout, fmt, ...)    HexUtilSystemF_(debugLevel, timeout, __FILE__ ":" HEX_STR(__LINE__), fmt, ## __VA_ARGS__)
#endif

#endif // __cplusplus

#endif /* endif HEX_PROCESS_UTIL_H */

