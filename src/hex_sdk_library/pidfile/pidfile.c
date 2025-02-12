// HEX SDK

#define _GNU_SOURCE // GNU asprintf
#include <errno.h> // errno, E...
#include <signal.h> // kill
#include <stdio.h> // fopen, fclose, fscanf
#include <stdlib.h> // mkstemp, free

#include <hex/pidfile.h>
#include <hex/process.h>
#include <hex/process_util.h>
#include <hex/log.h>

int HexPidFileRead(const char *pidFile)
{
    // Try to get the pid from the file if it exists
    pid_t pid = -1;
    {
        FILE *fin = fopen(pidFile, "r");
        if (fin) {
            fscanf(fin, "%d", &pid);
            fclose(fin);
        }
    }
    return pid;
}

int HexPidFileCreate(const char *pidFile)
{
    pid_t pid = HexPidFileCheck(pidFile);
    pid_t mypid = getpid();
    // not existed or to replace with new pidfile
    if (pid <= 0 || pid == mypid) {
        // Create temporary file in same directory as final pid file
        char *tmpFile = NULL;
        if (asprintf(&tmpFile, "%s.XXXXXX", pidFile) > 0) {
            int fd = mkstemp(tmpFile);
            if (fd >= 0) {
                FILE *fout = fdopen(fd, "w");
                if (fout) {
                    fprintf(fout, "%d\n", mypid);
                    fclose(fout);
                    close(fd);
                    // Then atomically rename into position
                    rename(tmpFile, pidFile);
                    free(tmpFile);
                    return 0;
                } else {
                    unlink(tmpFile);
                    free(tmpFile);
                    return -1;
                }
            } else {
                // mkstemp failed
                free(tmpFile);
                return -1;
            }
        }
        else {
            // asprintf failed
            return -1;
        }
    }
    return pid;
}


int HexPidFileCheck(const char *pidFile)
{
    pid_t pid = HexPidFileRead(pidFile);
    if (pid < 0)
        return -1;
    if (pid) {
        // Check if process is still running
        errno = 0;
        if (kill(pid, 0) == 0 || errno == EPERM) {
            // Process is running
            return pid;
        }
    }
    return 0;
}

