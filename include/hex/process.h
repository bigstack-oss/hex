// HEX SDK

#ifndef HEX_PROCESS_H
#define HEX_PROCESS_H

#include <stdlib.h> // EXIT_FAILURE
#include <sys/types.h>
#include <sys/wait.h> // WIFEXITED(), ...
#include <stdio.h>  //FILE *

#define HEX_SDK "/usr/sbin/hex_sdk"
#define HEX_CFG "/usr/sbin/hex_config"
#define HEX_CLI "/usr/sbin/hex_cli"

#define ZEROCHAR_PTR ((char*)0)

#define OUT_NULL  " >/dev/null 2>&1"

#define NO_STDOUT 0x1
#define NO_STDERR 0x2
#define QUIET   (NO_STDOUT | NO_STDERR)

#ifdef __cplusplus
extern "C" {
#endif

//signal handler decl
typedef void  (*shandler)(int);

// Spawn a child process through the system shell (/bin/sh), wait for it to complete,
// and return the status from waitpid(2) on the child process.
// If timeout is zero (0), wait indefinitely for the child process to complete.
// If timeout is positive (>0), wait up to timeout seconds for the child process
// to complete. After timeout seconds the child process will be terminated by
// SIGALRM.
// If timeout is negative (<0), do not wait for the child process to complete, but
// instead return immediately.
int HexSystem(int timeout, const char *arg0, ...) __attribute__ ((sentinel));
int HexSystemV(int timeout, char *const argv[]);
int HexSystemF(int timeout, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
int HexSystemNoSig(shandler sighandlerfunc, int isChildLeader, int timeout, const char *arg, ...) __attribute__ ((sentinel));

// Spawn a child process directly (no shell), wait for it to complete, and return the
// status from waitpid(2) on the child process.
// If timeout is zero (0), wait indefinitely for the child process to complete.
// If timeout is positive (>0), wait up to timeout seconds for the child process
// to complete. After timeout seconds the child process will be terminated by
// SIGALRM.
// If timeout is negative (<0), do not wait for the child process to complete, but
// instead return immediately.

int HexSpawn(int timeout, const char *arg0, ...) __attribute__ ((sentinel));
int HexSpawnV(int timeout, char *const argv[]);
int HexSpawnVQ(int timeout, int flag, char *const argv[]);


int HexSpawnNoSig(shandler sighandlerfunc, int isChildLeader, int timeout, const char *arg0, ...) __attribute__ ((sentinel));
int HexSpawnNoSigV(shandler sighandlerfunc, int isChildLeader, int timeout, char *const argv[]);

// Open a read pipe and run subcommand
FILE *HexPOpenF(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

// Return WEXITSTATUS if process exited otherwise return EXIT_FAILURE
static inline
int HexExitStatus(int rc) {
    // WIFEXITED(rc) returns true if the child terminated normally
    return WIFEXITED(rc) ? WEXITSTATUS(rc) : EXIT_FAILURE;
}

// Send SIGTERM to a non-child process.
// If the process has not exited after 3 seconds then send SIGKILL.
// NOTE: This function should never be used on a child process. It does not call waitpid().
//       Calling this on a child process will cause the parent to hang for the full 3 seconds.
int HexTerminate(pid_t pid);

// Send SIGTERM to a non-child process.
// If the process has not exited after 'maxSecs' seconds then send SIGKILL.
// NOTE: This function should never be used on a child process. It does not call waitpid().
//       Calling this on a child process will cause the parent to hang for the full 'maxSecs' seconds.
int HexTerminateTimeout(pid_t pid, int maxSecs);

// Keep check the content of pidfile until timeout reached or pidfile has >0 value
// return -1: timeout reach, >0: pid of process
int HexProcPidReady(int timeout, const char* pidfile);

// Keep check sockfile until timeout reached
// return -1: timeout reach, 0: sock file created
int HexSocketReady(int timeout, const char* sockfile);


#ifdef __cplusplus
}
#endif

/* C++ only helper functions */

#ifdef __cplusplus
#include <stdarg.h> // va_xxx
#include <sstream>
/**
 * Build a single shell argument from input string.
 * Input - An arbitrary string.
 * Output - An escaped string that can pass into HexSystemF as a single
 *          argument of command line.
 */
static inline const std::string
HexBuildShellArg(const std::string &arg)
{
    // 1. Enclose input string with single quoats
    // 2. Escape each single quoat(') to '\''
    std::stringstream str;
    str << '\'';
    for (std::string::const_iterator it = arg.begin(); it != arg.end(); it++) {
        if (*it == '\'')
            str << "'\\''";
        else
            str << *it;
    }
    str << '\'';
    return str.str();
}

/* Run a command, get its output and exit status */
static inline bool
HexRunCommand(int &exitStatus, std::string &output, const char *fmt, ...)
{
    output = "";
    exitStatus = -1;
    bool success = false;

    // construct argument list
    char *argv;
    va_list ap;
    va_start(ap, fmt);
    if (vasprintf(&argv, fmt, ap) < 0)
        return false;
    va_end(ap);

    FILE *fp = popen(argv, "r");
    if (!fp) {
        free(argv);
        return false;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        output += buffer;
    }

    int waitStatus = pclose(fp);
    if (WIFEXITED(waitStatus)) {
        exitStatus = WEXITSTATUS(waitStatus);
        success = true;
    }

    free(argv);
    return success;
}

#endif /* endif __cplusplus */

#endif /* endif HEX_PROCESS_H */

