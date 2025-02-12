// HEX SDK

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>

#include <hex/log.h>
#include <hex/process.h>
#include <hex/pidfile.h>
#include <hex/daemon.h>

// Save command line used to start program
// Return nonzero if command line changed since last time, and zero otherwise
static int
SaveCommandLine(const char *program, const char *command)
{
    char *tmpfile, *cmdfile;
    int size = 0, ret = 0;

    size = asprintf(&tmpfile, "/var/run/%s.cmdline.new", program);
    if (size == -1)
        return ret;

    size = asprintf(&cmdfile, "/var/run/%s.cmdline", program);
    if (size == -1) {
        free(tmpfile);
        return ret;
    }

    // Write command line to temporary file
    FILE *fout = fopen(tmpfile, "w");
    if (fout) {
        fprintf(fout, "%s\n", command);
        fclose(fout);
    }

    // Compare temporary file against previous file (which may not exist)
    if (HexSystemF(0, "cmp %s %s" OUT_NULL, tmpfile, cmdfile) == 0) {
        // Unchanged
        unlink(tmpfile);
    }
    else {
        // Changed: replace command line file with new one
        HexSystemF(0, "mv -f %s %s" OUT_NULL, tmpfile, cmdfile);
        ret = 1;
    }

    free(tmpfile);
    free(cmdfile);
    return ret;
}

//   0 if process was running and stopped
//   1 if process was not running
//  -1 and set errno if an error occurred
static int
StopProcess(const char *program)
{
    HexLogDebug("Trying to stop %s", program);

    char *pidfile;
    int size = asprintf(&pidfile, "/var/run/%s.pid", program);
    if (size == -1)
        return -1;

    pid_t pid = HexPidFileCheck(pidfile);
    free(pidfile);

    if (pid > 0) {
        HexLogInfo("Sending SIGTERM to %s (pid %d)", program, pid);
        return HexTerminateTimeout(pid, 30);
    }

    // Process not running
    HexLogDebug("%s already stopped", program);
    return 1;
}

int
HexDaemonStart(const char *program, const char *command, int flags)
{
    if (program == NULL)
        return -1;

    if (command == NULL)
        command = program;

    char *pidfile;
    int size = asprintf(&pidfile, "/var/run/%s.pid", program);
    if (size == -1)
        return -1;

    pid_t pid = HexPidFileCheck(pidfile);
    free(pidfile);

    int modified = SaveCommandLine(program, command);
    if (pid > 0) {
        if (modified || (flags & HEX_DAEMON_NO_SIGHUP) != 0) {
            // already started but command line changed or no SIGHUP support
            HexLogInfo("Restart daemon: %s", command);

            // Must stop and restart program
            if (HexDaemonStop(program) != 0) {
                HexLogError("Failed to stop %s: %s (errno %d)", program, strerror(errno), errno);
                return -1;
            }

            if (HexSystemF(0, "%s", command) != 0) {
                HexLogError("Failed to start %s: %s (errno %d)", program, strerror(errno), errno);
                return -1;
            }
        }
        else {
            // already started but has no command line change and SIGHUP support
            HexLogInfo("Restart daemon by sending SIGHUP: %s (pid %d)", command, pid);

            if (kill(pid, SIGHUP) != 0) {
                HexLogError("Failed to send SIGHUP to daemon: %s: %s (errno %d)", program, strerror(errno), errno);
                return -1;
            }
        }
    }
    else {
        // not yet start
        HexLogInfo("Starting daemon: %s", command);

        if (HexSystemF(0, "%s", command) != 0) {
            HexLogError("Failed to start daemon: %s: %s (errno %d)", program, strerror(errno), errno);
            return -1;
        }
    }

    return 0;
}

int
HexDaemonStop(const char *program)
{
    if (program == NULL)
        return -1;

    // Try to stop watchdog first if its running
    // Watchdog will stop child process
    // If child process crashes during shutdown it should/will not be restarted

    char *watchdog;
    int size = asprintf(&watchdog, "%s_watchdog", program);
    if (size == -1)
        return -1;

    int status = StopProcess(watchdog);
    free(watchdog);

    if (status == 0)
        return 0;

    // If watchdog could not be stopped, it might not be running
    // Ignore errors, continue, and try to stop child process
    if (StopProcess(program) < 0)
        return -1;

    return 0;
}

