// HEX SDK

#include <getopt.h> // should be GNU version
#include <errno.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <unistd.h>

#include <hex/log.h>
#include <hex/process.h>
#include <hex/daemon.h>
#include <hex/watchdog.h>

// Hidden SDK func
int HexCrashInfo(const char* filename, siginfo_t* info, const char** reason, void** addrs, size_t* naddrs, greg_t* regs, size_t* nregs);

static const char PROGRAM[] = "hex_crashd";
static const char DISPLAY_NAME[] = "crash logger";

// Guess at inotify buffer length
static const size_t BUF_LEN = 16 * (sizeof (struct inotify_event) + 16);

// Main loop timeout value
static const int TIMEOUT = 10;

static const char CRASH_DIR[] = "/var/support";
static const char MAX_CORE_FILE[] = "/etc/debug.max_core_files";

// Default number of cores before the oldest is removed.
static const int MAX_CORES = 3;

static volatile sig_atomic_t s_term = 0;

static void
SignalHandler(int sig)
{
    switch (sig)
    {
        case SIGTERM:
            s_term = 1;
            break;
    }
}

static void
Usage()
{
    fprintf(stderr, "Usage: %s [-f] [--foreground] [-v] [--verbose]\n", PROGRAM);
}

// When debug.enable_core_files.<process name> has been enabled we
// need to manage how many core files end up being written to disk to prevent it
// filling up. The maximum number of cores is configurable via advanced
// tuning parameter debug.max_core_files
static void
coreHousekeep()
{
    static const char corePattern[] = "core_*.*";
    int maxCores = MAX_CORES;
    int numCores = 0;

    // Determine the maximum number of cores from the max core file
    FILE *maxCoreFile = fopen(MAX_CORE_FILE, "r");
    if (!maxCoreFile) {
        // so make it debug
        HexLogDebug("Unable to open %s; using default %d", MAX_CORE_FILE, maxCores);
    }
    else {
        int bufSize = 5; // accommodate a 5 digit number plus terminator
        char buf[bufSize];
        char *res = fgets(buf, bufSize, maxCoreFile);
        if (res) {
            maxCores = strtol(buf, NULL, 10);
        }
        fclose(maxCoreFile);
    }

    // Count existing cores
    FILE *fp = HexPOpenF("ls %s/%s | wc -l", CRASH_DIR, corePattern);
    if (fp) {
        int bufSize = 5; // accommodate a 5 digit number plus terminator
        char buf[bufSize];
        char *res = fgets(buf, bufSize, fp);
        if (res) {
            numCores = strtol(buf, NULL, 10);
        }
        pclose(fp);
        fp = 0;
    }
    else {
        HexLogInfo("Failed to get number of cores");
    }

    // Remove the oldest core files from the filesystem if required
    int numCoresToClean = 0;
    if (numCores > maxCores) {
        numCoresToClean = numCores - maxCores;
        HexSystemF(0, "ls -tr %s/%s | head -%d | xargs rm -f", CRASH_DIR, corePattern, numCoresToClean);
        // Crashmaps intentionally not removed to provide indication to how
        // many times a deamon has crashed
    }

    // Note: maybe off by 1 in the case where core hasn't been sync'ed to
    // disk, however, one over our maximum isn't a bug deal
    HexLogDebug("Core Files: found: %d, max: %d, removed: %d", numCores, maxCores, numCoresToClean);
}

// Wait until a crash file is done being written
static int
WaitUntilFileClosed(const char* filename, int timeout)
{
    off_t fsize = 0;
    int decrement = timeout / 10;
    int r = 1;
    while (timeout > 0) {
        struct stat s;
        if (stat(filename, &s) == -1) {
            HexLogError("Unable to get size of crash file %s: %s", filename, strerror(errno));
            return -1;
        }
        if (s.st_size > 0 && s.st_size == fsize) {
            r = 0;
            break; // no file size changed. assume the file done
        }
        fsize = s.st_size;
        timeout -= decrement;
        usleep(decrement * 1000); // Sleep 1/10 of the timeout.  Kinda arbitrary.
    }

    if (r != 0)
        HexLogError("Timed out waiting for crash file %s to close", filename);

    return r;
}

static int
LogCrash(const char* filename)
{
    size_t namelen = strlen(filename) + 1;
    char* fn = alloca(namelen);

    // make a copy for the following modification
    memcpy(fn, filename, namelen);

    // filename doesn't start with crash_
    char* prog = strstr(fn, "crash_");
    if (!prog)
        return 1;

    // find pid
    prog += strlen("crash_");
    char* pid = strrchr(fn, '.');
    if (!pid)
        return 1;
    *pid++ = '\0';

    // used for HexCrashInfo()
    siginfo_t info;
    const char* reason;
    void* addrs[10];
    size_t naddrs = 10;
    greg_t regs[23];
    size_t nregs = 23;

    memset(&info, 0, sizeof(info));

    int r = HexCrashInfo(filename, &info, &reason, addrs, &naddrs, regs, &nregs);
    HexLogDebug("signal=%d code=%d addr=%p naddrs=%zu", info.si_signo, info.si_code, info.si_addr, naddrs);

    if (r == 0) {
        char buffer[1024];
        int n = snprintf(buffer, 1024, "CRASH: program=%s pid=%s reason=\"%s\" signal=%d code=%d addr=%p stack=\'",
                         prog, pid, reason, info.si_signo, info.si_code, info.si_addr);
        size_t j;
        for (j = 0; j < naddrs && n < 1000; j++) { //FIXME: need a better way to check for room in buffer
            n += snprintf(&buffer[n], 1024 - n, "%p ", addrs[j]);
        }

        n += snprintf(&buffer[n], 1024 - n, "\' regs=\'");
        for (j = 0; j < nregs && n < 1000; j++) { //FIXME: need a better way to check for room in buffer
            n += snprintf(&buffer[n], 1024 - n, "0x%lx ", (unsigned long)regs[j]);
        }

        n += snprintf(&buffer[n], 1024 - n, "\'");
        HexLogError("%s", buffer); // FIXME: needs to be event

        coreHousekeep();
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    int daemonFlags = HEX_NO_CRASH_INIT;

    static struct option long_options[] = {
        { "verbose", no_argument, 0, 'v' },
        { "foreground", no_argument, 0, 'f' },
        { 0, 0, 0, 0 }
    };

    // Suppress error messages by getopt_long()
    opterr = 0;

    while (1) {
        int index;
        int c = getopt_long(argc, argv, "vf", long_options, &index);
        if (c == -1)
            break;

        switch (c) {
            case 'v':
                ++HexLogDebugLevel;
                break;
            case 'f':
                daemonFlags |= HEX_NO_DAEMON;
                break;
            case '?':
                Usage();
                return 1;
            default:
                abort();
        }
    }

    if (optind != argc) {
        Usage();
        return 1;
    }

    HexWatchdogDaemon(PROGRAM, DISPLAY_NAME, daemonFlags);

    HexLogInfo("Started");

    chdir(CRASH_DIR);

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SignalHandler;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGTERM, &act, 0) < 0)
        HexLogFatal("Unable to set up signal handlers");

    HexWatchdogTimerReal* wdtimer = HexWatchdogTimerRealCreate();

    int fd = inotify_init();
    if (fd < 0)
        HexLogFatal("Failed to initialize file system monitor");

    int wd = inotify_add_watch(fd, CRASH_DIR, IN_CREATE);
    if (wd < 0)
        HexLogFatal("Failed to add watch for %s: %s", CRASH_DIR, strerror(errno));

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN|POLLERR|POLLHUP;
    pfd.revents = 0;

    char buf[BUF_LEN];

    // Start watchdog
    HexWatchdogTimerRealSet(wdtimer, TIMEOUT * 2);

    while (!s_term) {
        HexWatchdogTimerRealReset(wdtimer);
        int r = poll(&pfd, 1, TIMEOUT);
        if (r > 0) {
            ssize_t len = read(fd, buf, BUF_LEN);
            if (len < 0) {
                if (errno == EINTR)
                    continue;
                else
                    HexLogError("Failed to receive file system event");
            }
            else if (!len) {
                // BUF_LEN too small?
                HexLogFatal("Internal error (event too large for buffer)");
            }

            ssize_t i = 0;
            while (i < len) {
                struct inotify_event *event = (struct inotify_event *) &buf[i];
                if (event->mask & IN_CREATE && event->len) {
                    // Ignore all files except those that start with "crash_"
                    if (strncmp(event->name, "crash_", 6) == 0) {
                        HexLogDebug("Detected new file %s", event->name);
                        if (WaitUntilFileClosed(event->name, 1000) == 0) { //TODO: make timeout configurable?
                            LogCrash(event->name);
                            unlink(event->name);
                        }
                    }
                }
                else {
                    HexLogDebug("Unexpected file system event (mask=%d)", event->mask);
                }
                i += sizeof(struct inotify_event) + event->len;
            }
        }
        else if (r == -1 && errno != EINTR) {
            HexLogFatal("Internal error (poll: %s)", strerror(errno));
        }

        {
            static time_t lastHousekeep = 0;
            time_t now = time(0);
            if (now - lastHousekeep > 1) {
                coreHousekeep();
                lastHousekeep = now;
            }
        }
    }

    HexLogInfo("Shutting down");
    HexWatchdogTimerRealDestroy(wdtimer);
    return 0;
}

