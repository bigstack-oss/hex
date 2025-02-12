// HEX SDK

#define _GNU_SOURCE // GNU vasprintf
#include <string.h>
#include <unistd.h> // fork, daemon, alarm, getpid, ...
#include <stdarg.h> // va_...
#include <errno.h> // errno, E... defines
#include <time.h> // nanosleep
#include <sys/stat.h> // open
#include <fcntl.h>

#include <hex/process.h>

//#define TRACE_EXECUTION 1
#ifdef TRACE_EXECUTION
#define Trace(fmt, ...) do { printf("%d:%d: " fmt, getpid(), __LINE__, ## __VA_ARGS__); fflush(stdout); } while (0)
#else
#define Trace(fmt, ...)
#endif

int HexSystem(int timeout, const char *arg, ...)
{
    char *argv[4];
    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = NULL;
    argv[3] = NULL;

    // include room for space between arguments
    int len = strlen(arg) + 1;
    va_list ap;
    va_start(ap, arg);
    char *p;
    while ((p = va_arg(ap, char *)) != NULL) {
        // include room for space between arguments
        len += strlen(p) + 1;
    }
    va_end(ap);
    // include room for terminating null character
    len += 1;

    if ((argv[2] = malloc(len)) == NULL) {
        errno = ENOMEM;
        return -1;
    }

    strcpy(argv[2], arg);
    strcat(argv[2], " ");
    va_start(ap, arg);
    while ((p = va_arg(ap, char *)) != NULL) {
        strcat(argv[2], p);
        strcat(argv[2], " ");
    }
    va_end(ap);

    int rc = HexSpawnV(timeout, argv);
    free(argv[2]);
    return rc;
}

int HexSystemV(int timeout, char *const origArgv[])
{
    char *argv[4];
    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = NULL;
    argv[3] = NULL;

    int i, len = 0;
    char *p;

    if (!origArgv[0]) {
        errno = EINVAL;
        return -1;
    }

    for (i = 0; (p = origArgv[i]) != NULL; ++i) {
        // include room for space between arguments
        len += strlen(p) + 1;
    }
    // include room for terminating null character
    len += 1;

    if ((argv[2] = alloca(len)) == NULL) {
        errno = ENOMEM;
        return -1;
    }

    strcpy(argv[2], origArgv[0]);
    strcat(argv[2], " ");
    for (i = 1; origArgv[i] != NULL; ++i) {
        strcat(argv[2], origArgv[i]);
        strcat(argv[2], " ");
    }

    return HexSpawnV(timeout, argv);
}


int HexSystemF(int timeout, const char *fmt, ...)
{
    char *argv[4];
    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = NULL;
    argv[3] = NULL;

    va_list ap;
    va_start(ap, fmt);
    int status = vasprintf(&argv[2], fmt, ap);
    if (status < 0)
        return status;
    va_end(ap);

    status = HexSpawnV(timeout, argv);
    free(argv[2]);
    return status;
}


int HexSystemNoSig(shandler sighandlerfunc, int isChildLeader, int timeout, const char *arg, ...)
{
    char *argv[4];
    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = NULL;
    argv[3] = NULL;

    // include room for space between arguments
    int len = strlen(arg) + 1;
    va_list ap;
    va_start(ap, arg);
    char *p;
    while ((p = va_arg(ap, char *)) != NULL) {
        // include room for space between arguments
        len += strlen(p) + 1;
    }
    va_end(ap);
    // include room for terminating null character
    len += 1;

    if ((argv[2] = malloc(len)) == NULL) {
        errno = ENOMEM;
        return -1;
    }

    strcpy(argv[2], arg);
    strcat(argv[2], " ");
    va_start(ap, arg);
    while ((p = va_arg(ap, char *)) != NULL) {
        strcat(argv[2], p);
        strcat(argv[2], " ");
    }
    va_end(ap);

    int rc = HexSpawnNoSigV(sighandlerfunc, isChildLeader, timeout, argv);

    free(argv[2]);
    return rc;
}

int
HexSpawn(int timeout, const char *arg, ...)
{
    va_list ap;
    char **argv;
    int i;

    va_start(ap, arg);
    for (i = 2; va_arg(ap, char *) != NULL; i++)
        continue;
    va_end(ap);

    if ((argv = alloca(i * sizeof (char *))) == NULL) {
        errno = ENOMEM;
        return -1;
    }

    va_start(ap, arg);
    argv[0] = (char *)arg;
    for (i = 1; (argv[i] = va_arg(ap, char *)) != NULL; i++)
        continue;
    va_end(ap);
    return HexSpawnV(timeout, argv);
}

int HexSpawnVQ(int timeout, int flag, char *const argv[])
{
    int saved_stdout = dup(STDOUT_FILENO),
        saved_stderr = dup(STDERR_FILENO),
        dev_null     = open("/dev/null", O_WRONLY);

    if (flag & NO_STDOUT) {
        dup2(dev_null, STDOUT_FILENO);
    }
    if (flag & NO_STDERR) {
        dup2(dev_null, STDERR_FILENO);
    }

    int ret = HexSpawnV(timeout, argv);

    dup2(saved_stdout, STDOUT_FILENO);
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stdout);
    close(saved_stderr);
    return ret;
}

int
HexSpawnV(int timeout, char *const argv[])
{
    return  HexSpawnNoSigV(SIG_IGN, 0, timeout, argv);
}

int
HexSpawnNoSig(shandler sighandlerfunc, int isChildLeader, int timeout, const char *arg, ...)
{
    va_list ap;
    char **argv;
    int i;

    va_start(ap, arg);
    for (i = 2; va_arg(ap, char *) != NULL; i++)
        continue;
    va_end(ap);

    if ((argv = alloca(i * sizeof (char *))) == NULL) {
        errno = ENOMEM;
        return -1;
    }

    va_start(ap, arg);
    argv[0] = (char *)arg;
    for (i = 1; (argv[i] = va_arg(ap, char *)) != NULL; i++)
        continue;
    va_end(ap);

    return HexSpawnNoSigV(sighandlerfunc, isChildLeader, timeout, argv);
}

int
HexSpawnNoSigV(shandler sighandlerfunc, int isChildLeader, int timeout, char *const argv[])
{
    int status = 0;

    struct sigaction intsa, quitsa, sa;
    sigset_t omask;

    // Configure SIGINT(ctrl-c) and SIGQUIT(dump core)
    sa.sa_handler = sighandlerfunc;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    // save oldact for restore
    sigaction(SIGINT, &sa, &intsa);
    sigaction(SIGQUIT, &sa, &quitsa);

    // Block SIGCHLD
    // blocking is different from ignoring
    // blocked signal leaves the signal pending in kernel until it is unblocked
    sigaddset(&sa.sa_mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &sa.sa_mask, &omask);

    Trace("parent: forking child\n");
    pid_t pid = fork();
    if (pid < 0) {
        Trace("parent: fork failed\n");
        status = -1;
    }
    else if (pid == 0) {
        // Child
        Trace("child: timeout=%d\n", timeout);
        if (timeout < 0) {
            // Detach child from original parent process (e.g. daemonize)
            // Don't chdir
            daemon(1, 0);
        }
        else if (timeout > 0) {
            // Set uncaught alarm that will terminate child if it runs longer than timeout seconds
            struct sigaction sa;
            sa.sa_handler = SIG_DFL;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGALRM, &sa, (struct sigaction *)0);
            alarm(timeout);
        }
        sa.sa_handler = sighandlerfunc;
        if(SIG_IGN==sa.sa_handler) {
            // Restore signals
            sigaction(SIGINT, &intsa, (struct sigaction *)0);
            sigaction(SIGQUIT, &quitsa, (struct sigaction *)0);
        }
        else {
            sigaction(SIGINT, &sa, (struct sigaction *)0);
            sigaction(SIGQUIT, &sa, (struct sigaction *)0);
        }
        // Clear signal mask to avoid child process starts with some signal blocked unexpectedly.
        sigemptyset(&omask);
        sigprocmask(SIG_SETMASK, &omask, (sigset_t *)0);

        //Make child the process group leader to avaoid session signals reaching it
        if(isChildLeader != 0) {
            pid_t mypid = getpid();
            if(mypid != getpgrp()) {
                if(setpgid(0, 0)==-1) {
                    int err = errno;
                    err = err;
                    Trace("System error %d while running setpgid: %s", err, strerror(err));
                }
            }
        }
        Trace("child: execing new process\n");
        if (execv(argv[0], argv) == -1)
            _exit(127);
    }
    else {
        // pid > 0, Parent
        // Reap child (or second parent of daemonized child) process
        Trace("parent: waiting on child pid=%d\n", pid);
        while (waitpid(pid, &status, 0) == -1) {
            if (errno != EINTR) {
                break;
            }
            status=0;
        }

    }

    // Restore signals
    sigaction(SIGINT, &intsa, (struct sigaction *)0);
    sigaction(SIGQUIT, &quitsa, (struct sigaction *)0);
    sigprocmask(SIG_SETMASK, &omask, (sigset_t *)0);

    Trace("parent: wait status=%d\n", status);
    return status;
}

FILE *
HexPOpenF(const char *fmt, ...)
{
    char *argv;
    va_list ap;
    va_start(ap, fmt);
    if (vasprintf(&argv, fmt, ap) < 0)
        return NULL;
    va_end(ap);
    FILE *fp = popen(argv, "r");
    free(argv);
    return fp;
}

