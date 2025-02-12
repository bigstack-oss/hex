// HEX SDK

#ifndef HEX_DAEMON_H
#define HEX_DAEMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* Watchdog Daemon APIs */

// Detach and run in the background as a child process of a watchdog daemon unless the
// HEX_NO_DAEMON flag is set. The watchdog daemon will restart the child process (e.g. return
// from this function) whenever SIGCHLD is caught and the wait status indicated that the process
// was terminated by a signal.
//
// This function will call HexLogInit() before returning to the child process. The parent
// (watchdog) process will emit log messages during its normal operation. If the HEX_NO_LOG_INIT
// flag is set the file descriptor used by HexLogInit() will be close before returning to the
// child process (used by hex_syslogd).
//
// This function will call HexPidFileCreate("/var/run/" + program + ".pid) before
// returning to the child process unless the HEX_NO_PID_FILE flag is set. The PID file will
// be removed automatically when the child process exists.
//
// The parent (watchdog) process will also call these functions with the process name
// <program>_watchdog" so that its log messages and PID files can be distinguished
// from those of the child process. The process name will appear twice in top and ps output,
// once for the child and once for the parent (watchdog) process. These processes can be
// distinguished by examining the corresponding PID files.
//
// This function will call HexCrashInit() before returning to the child process unless
// the HEX_NO_CRASH_INIT flag is set.
//
// program
//      Pathname of calling process. Basename of this value will be used to initialize
//      crash API, log API, and to create PID filename.
//
// displayName
//      A "user friendly" disply name for this process.
//
// flags
//      Logically OR-ed collection of flags to control the behavior of the parent (watchdog) process.
//
void HexWatchdogDaemon(const char *program, const char* displayName, int flags);

// HexWatchdogDaemon flags
enum {
    HEX_NO_DAEMON     = 1 << 0,    // Do not run in background as a daemon. Redirect log messages to stderr.
    HEX_NO_PID_FILE   = 1 << 1,    // Do not manage child PID file (HexPidFileCreate).
    HEX_NO_CRASH_INIT = 1 << 2,    // Do not initialize crash handler (HexCrashInit).
    HEX_NO_LOG_INIT   = 1 << 3,    // Close file descriptor used by HexLogInit before returning to caller.
    HEX_NO_LOG_STDERR = 1 << 4,    // When HEX_NO_DAEMON is specified, logs will be redirected to stderr
                                    // unless HEX_NO_LOG_STDERR or HEX_NO_LOG_INIT is specified
    HEX_LOG_STDERR    = 1 << 5,    // When HEX_NO_DAEMON is *not* specified, logs will *not* be redirected to stderr
                                    // unless HEX_LOG_STDERR is specified
};

// Exit codes for communicating status from child process to parent watchdog process:
enum {
    HEX_EXIT_SUCCESS = 0,          // Child is exiting with success, watchdog will also exit with success.
    HEX_EXIT_FAILURE = 1,          // Child is exiting with failure, watchdog will also exit with failure.
    HEX_EXIT_RESTART = 2,          // Child is exiting and needs to be restarted. Watchdog will restart it.
};

// Process types for watchdog daemons
enum {
    HEX_WATCHDOG_DAEMON_NONE,      // Calling process is not running as a daemon
    HEX_WATCHDOG_DAEMON_PARENT,    // Calling process is the watchdog/parent
    HEX_WATCHDOG_DAEMON_CHILD      // Calling process is the watched/child
};

// Determine what type of process the caller is
int HexWatchdogDaemonType();

// Send a restart signal from child to parent
// If the caller is not HEX_WATCHDOG_DAEMON_CHILD, this returns -1 and sets errno to ESRCH
int HexWatchdogDaemonRequestRestart();

// Register a callback function anytime the child process exits.
// Function must be registered before calling HexWatchdogDaemon().
//
// The callback function will be passed the following:
//
// restart
//      Non-zero if child will be restart and zero if watchdog will exit.
//
// childStatus
//      Child status as returned by waitpid(). The WIFEXITED and WIFSIGNALLED macros should be used to examine
//      this status. See the waitpid man page for more details.
//
// The callback function should return zero to continue to restart the child, and non-zero to abort the restart.
// The return value of the callback will be ignored if the watchdog is exiting.
//
typedef int (*HexWatchdogDaemonCallback)(int restart, int childStatus);
void HexWatchdogDaemonSetCallback(HexWatchdogDaemonCallback callback);

/* Watchdog daemon control APIs */

enum {
    HEX_DAEMON_NO_SIGHUP  = (1 << 0), // Do not send SIGHUP. Restart process even when arguments do not change.
};

// Start the daemon process "program" using the specified command line.
// If command is NULL, start the process by its program name, which must be in the current path.
// If the process is already started and command line arguments have changed since it was started, then
// the process will be stopped and restarted.
// If the process is already started and command line arguments have not changed, then
// process will be sent a SIGHUP signal, unless HEX_DAEMON_NO_SIGHUP flag is specified,
// in which case the process will be stopped and restarted.
// Return zero if process is started or restarted successfully.
// Otherwise return -1 and set errno to one of the following values:
int HexDaemonStart(const char *program, const char *command, int flags);

// Stop the daemon process "program".
// Return 0 if process was stopped successfully (or was already stopped).
// Otherwise return -1 and set errno to one of the following values:
int HexDaemonStop(const char *program);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* endif HEX_DAEMON_H */

