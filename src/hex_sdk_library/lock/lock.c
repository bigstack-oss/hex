// HEX SDK

#include <time.h>
#include <unistd.h>

#include <hex/lock.h>
#include <hex/log.h>
#include <hex/pidfile.h>

// Try to grab lock file so that only one instance can run at a time
// Exit with error if unable to grab the lock after 'timeoutSecs' seconds
bool
HexLockAcquire(const char *lockfile, int timeoutSecs)
{
    HexLogDebug("Grabbing lock");
    time_t limit = time(0) + timeoutSecs;
    while (true) {
        pid_t pid = HexPidFileCreate(lockfile);
        if (pid == 0) {
            HexLogDebug("Successfully grabbed lock");
            return true;
        }
        sleep(1);
        time_t now = time(0);
        if (now > limit) {
            HexLogError("Unable to grab lock after %d seconds. Another instance is running (pid %d).", timeoutSecs, pid);
            return false;
        }
    }
    // Not reached
    return false;
}

