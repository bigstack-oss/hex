// HEX SDK

#define _GNU_SOURCE 1
#include <string.h> // GNU basename, asprintf
#include <errno.h>

#include <hex/crash.h>
#include <hex/log.h>
#include <hex/pidfile.h>
#include <hex/process.h>
#include <hex/daemon.h>

static volatile sig_atomic_t s_term = 0;
static volatile sig_atomic_t s_forceRestart = 0;
static volatile pid_t s_signallingPid = 0;
static pid_t s_childPid = 0;
static int s_procType = HEX_WATCHDOG_DAEMON_NONE;

static char *s_displayName = NULL;
static char *s_childProgram = NULL;
static char *s_childPidFile = NULL;
static int s_removeChildPidFile = 0;

static char *s_watchdogProgram = NULL;
static char *s_watchdogPidFile = NULL;
static int s_removeWatchdogPidFile = 1;

static HexWatchdogDaemonCallback s_callback = NULL;

static void
SignalHandler(int sig, siginfo_t *info, void *context)
{
    if (sig == SIGTERM) {
        s_term = 1;
        if (info->si_code <= 0) {
            s_signallingPid = info->si_pid;
        }
    } else if (sig == SIGUSR1) {
        s_forceRestart = 1;
        if (info->si_code <= 0) {
            s_signallingPid = info->si_pid;
        }
    } else if (sig == SIGALRM) {
        // Alarm was received, child must not have exited after sending SIGTERM, so send SIGKILL
        kill(s_childPid, SIGKILL);
    }
}

static void
TerminateChild()
{
    // Set alarm to go off in 30 seconds in case child doesn't respond to SIGTERM
    alarm(30);
    kill(s_childPid, SIGTERM);
}

// Destructor to remove pid files on exit and free memory to make valgrind happy
static void __attribute__ ((destructor))
Fini()
{
    if (s_childProgram) {
        free(s_childProgram);
        s_childProgram = NULL;
    }

    if (s_displayName) {
        free(s_displayName);
        s_displayName = NULL;
    }

    if (s_childPidFile) {
        if (s_removeChildPidFile)
            unlink(s_childPidFile);
        free(s_childPidFile);
        s_childPidFile = NULL;
    }

    if (s_watchdogProgram) {
        free(s_watchdogProgram);
        s_watchdogProgram = NULL;
    }

    if (s_watchdogPidFile) {
        if (s_removeWatchdogPidFile)
            unlink(s_watchdogPidFile);
        free(s_watchdogPidFile);
        s_watchdogPidFile = NULL;
    }
}

void
HexWatchdogDaemonSetCallback(HexWatchdogDaemonCallback callback)
{
    s_callback = callback;
}

static void
DoCallback(int restart, int childStatus)
{
    if (s_callback) {
        int ret = s_callback(restart, childStatus);
        // Did callback cancel restart?
        if (restart && ret != 0) restart = 0;
    }
    if (restart == 0)
        exit(0);
}

void
HexWatchdogDaemon(const char *fullPathname, const char* displayName, int flags)
{
    int nodaemon = (flags & HEX_NO_DAEMON);
    int nopidfile = (flags & HEX_NO_PID_FILE);
    int nocrashinit = (flags & HEX_NO_CRASH_INIT);
    int nologinit = (flags & HEX_NO_LOG_INIT);

    // Daemon mode: log to syslog only unless HEX_LOG_STDERR is specified
    // Non-daemon mode: log to stderr unless HEX_NO_LOG_STDERR is specified
    int logstderr = nodaemon ? 1 : 0;
    if ((flags & HEX_LOG_STDERR) != 0) logstderr = 1;
    if ((flags & HEX_NO_LOG_STDERR) != 0) logstderr = 0;

    HexLogInit(fullPathname, logstderr);

    if ((s_childProgram = strdup(basename(fullPathname))) == NULL)
        HexLogFatal("Memory allocation failed");

    if ((s_displayName = strdup(displayName)) == NULL)
        HexLogFatal("Memory allocation failed");

    if (!nopidfile) {
        if (asprintf(&s_childPidFile, "/var/run/%s.pid", s_childProgram) < 0)
            HexLogFatal("Memory allocation failed");

        pid_t pid = HexPidFileCheck(s_childPidFile);
        if (pid > 0) {
            HexLogError("Another instance of %s (pid %d) is already running", s_childProgram, pid);
            fprintf(stderr, "Error: Another instance of %s (pid %d) is already running\n", s_childProgram, pid);
            exit(1);
        }
    }

    if (nodaemon) {
        // Do not daemonize
        // Just initialize crash/log, create pid file and return to control back to caller
        HexLogInit(s_childProgram, logstderr);

        if (!nocrashinit)
            HexCrashInit(s_childProgram);

        if (!nopidfile) {
            if (HexPidFileCreate(s_childPidFile) != 0)
                HexLogFatal("Failed to create pid file");

            // We've created the pid file, so make sure we remove it on exit
            s_removeChildPidFile = 1;
        }

        if (nologinit)
            HexLogClose();

        return;
    }

    // Append unique suffix to child program so we can identify watchdog pid files, and log entires
    if (asprintf(&s_watchdogProgram, "%s_watchdog", s_childProgram) < 0)
        HexLogFatal("Memory allocation failed");

    HexLogInit(s_watchdogProgram, logstderr);

    if (!nocrashinit)
        HexCrashInit(s_watchdogProgram);

    if (asprintf(&s_watchdogPidFile, "/var/run/%s.pid", s_watchdogProgram) < 0)
        HexLogFatal("Memory allocation failed");

    pid_t watchdogPid = HexPidFileCheck(s_watchdogPidFile);
    if (watchdogPid > 0) {
        HexLogError("Another instance of %s watchdog (pid %d) is already running", s_childProgram, watchdogPid);
        fprintf(stderr, "Error: Another instance of %s watchdog (pid %d) is already running\n", s_childProgram, watchdogPid);
        exit(1);
    }

    // attach to init and detach from caller
    if (daemon(0, 0) < 0)
        HexLogFatal("Failed to daemonize");

    HexLogDebugN(RRA, "Creating watchdog pid file");
    if (HexPidFileCreate(s_watchdogPidFile) != 0)
        HexLogFatal("Failed to create watchdog pid file");

    // We've created the pid file, so make sure we remove it on exit
    s_removeWatchdogPidFile = 1;

    HexLogInfo("Watchdog started");

    // Initialize signal handler
    struct sigaction sa;
    sa.sa_sigaction = SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGTERM, &sa, NULL) < 0 ||
        sigaction(SIGUSR1, &sa, NULL) < 0 ||
        sigaction(SIGALRM, &sa, NULL) < 0)
        HexLogFatal("Failed to initialize signal handler");

    // Main loop
    while (1) {
        time_t startTime = time(NULL);

        // Start child process
        HexLogDebug("Starting child process");
        s_childPid = fork();
        if (s_childPid == 0) {

            HexLogInit(s_childProgram, logstderr);

            // Child process
            if (!nocrashinit)
                HexCrashInit(s_childProgram);

            if (!nopidfile) {
                if (HexPidFileCreate(s_childPidFile) != 0)
                    HexLogFatal("Failed to create pid file");

                // We've created the pid file, so make sure we remove it on exit
                s_removeChildPidFile = 1;
            }

            // Restore SIGTERM to default behavior
            if (signal(SIGTERM, SIG_DFL) < 0)
                HexLogWarning("Failed to reset SIGTERM signal handler");

            if (nologinit)
                HexLogClose();

            // Record what type of process this is
            s_procType = HEX_WATCHDOG_DAEMON_CHILD;

            // Return to calling code
            return;

        }
        else if (s_childPid < 0) {
            HexLogFatal("fork failed");
        }

        HexLogInfo("Child process started (pid=%d)", s_childPid);

        // Record what type of process this is
        s_procType = HEX_WATCHDOG_DAEMON_PARENT;

        // Wait for child to exit or for waitpid to be terminated by signal
        int term = 0, forceRestart = 0;
        int childStatus;
        while (waitpid(s_childPid, &childStatus, 0) < 0 && errno == EINTR) {
            // waitpid() exits because an interrupt is sent to watchgod process
            // Block signals before checking them
            sigset_t mask;
            sigemptyset(&mask);
            sigaddset(&mask, SIGTERM);
            sigaddset(&mask, SIGUSR1);
            sigprocmask(SIG_BLOCK, &mask, NULL);

            // term/s_term & forceRestart/s_forceRestart
            // is used to avoid s_value to be overrided by next interrupt
            if (s_term) {
                term = 1;
                s_term = 0;
                HexLogInfo("Watchdog exiting on SIGTERM (sent by %d), terminating child", s_signallingPid);
                TerminateChild();
            }
            else if (s_forceRestart) {
                forceRestart = 1;
                s_forceRestart = 0;
                HexLogInfo("Watchdog received SIGUSR1 (sent by %d), restarting child", s_signallingPid);
                TerminateChild();
            }

            // Unblock signals afterwards
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
        }

        // Cancel alarm
        alarm(0);

        // Child has been reaped, invoke the callback function
        if (term) {
            // Already logged something for this condition
            DoCallback(0, childStatus);
        }
        else if (forceRestart) {
            // Already logged something for this condition
            DoCallback(1, childStatus);
        }
        else if (WIFEXITED(childStatus)) {
            int status = WEXITSTATUS(childStatus);
            if (status == HEX_EXIT_RESTART) {
                HexLogInfo("Child exited and requested restart");
                DoCallback(1, childStatus);
            }
            else {
                HexLogInfo("Child exited with status %d, watchdog exiting", status);
                DoCallback(0, childStatus);
            }
        }
        else if (WIFSIGNALED(childStatus)) {
            int sig = WTERMSIG(childStatus);
            if (sig == SIGTERM) {
                // This is unlikely since most processes catch SIGTERM and exit gracefully
                HexLogInfo("Child killed by SIGTERM, watchdog exiting");
                DoCallback(0, childStatus);
            }
            else {
                //HexLogEvent(GLGSY0000W, "GLG_events", "service=%s", s_displayName);
                HexLogError("Child %s killed by signal %d, restarting", s_childProgram, sig);
                DoCallback(1, childStatus);
            }
        }

        // Check to see whether we crashed too quickly.  If we crashed
        // inside of 5 seconds we put a delay in before attempting the
        // restart.  This will stop us from respawning too quickly and
        // consuming all of the CPU.
        time_t endTime = time(NULL);
        if ((endTime - startTime) < 5)
            sleep(5);
    }

    // Not reached
}

int
HexWatchdogDaemonType()
{
    return s_procType;
}

int
HexWatchdogDaemonRequestRestart()
{
    int r = 0;

    pid_t parent = getppid();
    if (s_procType == HEX_WATCHDOG_DAEMON_CHILD && parent > 1) {
        r = kill(parent, SIGUSR1);
    }
    else {
        errno = ESRCH;
        r = -1;
    }

    return r;
}

