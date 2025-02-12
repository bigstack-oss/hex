// HEX SDK

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h> // errno, E... defines
#include <time.h> // nanosleep

#include <hex/process.h>
#include <hex/pidfile.h>

int
HexTerminateTimeout(pid_t pid, int max_Secs)
{
    // check to see if process is running
    if (kill(pid, 0) == -1) {
        if (errno == ESRCH)
            return 0;
        else
            return -1;
    }

    // ask process to exit gracefully
    kill(pid, SIGTERM);

    // nanosleep wait timeout
    // after kill(pid,SIGTERM), future calls to kill(pid,0) will return 0 until it is reaped via waitpid()
    const int tenMs = 10000000;
    struct timespec ts = { 0, tenMs};
    int maxLoops = max_Secs * 100;

    // wait for it to exit on its own
    int i;
    for (i = 0; i < maxLoops; i++) {
        ts.tv_sec = 0;
        ts.tv_nsec = tenMs;
        nanosleep(&ts, 0);
        if (kill(pid, 0) == -1 && errno == ESRCH) return 0;
    }

    // just kill it
    if (kill(pid, SIGKILL) == -1 && errno != ESRCH) return -1;

    // wait for it to get killed
    for (i = 0; i < maxLoops; i++) {
        ts.tv_sec = 0;
        ts.tv_nsec = tenMs;
        nanosleep(&ts, 0);
        if (kill(pid, 0) == -1 && errno == ESRCH) return 0;
    }

    return -1;
}

int
HexTerminate(pid_t pid)
{
    return HexTerminateTimeout(pid, 3);
}

int
HexProcPidReady(int timeout, const char* pidfile)
{
    int i = timeout;
    pid_t pid = -1;

    while (1) {
        FILE *fin = fopen(pidfile, "r");

        if (fin) {
           fscanf(fin, "%d", &pid);
           fclose(fin);
        }

        if (pid > 0) {
            // Check if process is still running
            errno = 0;
            if (kill(pid, 0) == 0 || errno == EPERM) {
                // Process is running
                break;
            }
        }

        if (--i <= 0) {
            return -1;
        }
        sleep(1);
    }

    return pid;
}

int
HexSocketReady(int timeout, const char* sockfile)
{
    int i = timeout;
    struct stat s;

    while (1) {
        if(stat(sockfile, &s) == 0 && S_ISSOCK(s.st_mode))
            break;

        if (--i <= 0) {
            return -1;
        }
        sleep(1);
    }

    return 0;
}

int
HexStopProcessByPid(const char *name, const char *pidFile)
{
    pid_t pid = HexPidFileCheck(pidFile);

    if (pid > 0) {
        if (HexTerminate(pid) < 0) {
            return -1;
        }
    }

    return 0;
}

