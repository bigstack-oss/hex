// HEX SDK

// Define _GNU_SOURCE for RTLD_NEXT
#define _GNU_SOURCE 1
#include <dlfcn.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <hex/pidfile.h>
#include <hex/test.h>

const char PIDFILE[] = "/var/run/test.pid";

FILE *(*fdopen_ptr)(int, const char *) = 0;

bool fdopen_enabled = true;

// If fdopen_enabled is false and called with mode "w" then return failure (NULL)
// Otherwise forward to real fdopen()
FILE *fdopen(int fd, const char *mode)
{
    if (!fdopen_ptr)
        fdopen_ptr = (FILE *(*)(int, const char *)) dlsym(RTLD_NEXT, "fdopen");

    HEX_TEST_FATAL(fdopen_ptr != NULL);

    if (!fdopen_enabled && strcmp(mode, "w") == 0)
        return NULL;
    else
        return fdopen_ptr(fd, mode);
}

int(*mkstemp_ptr)(char *) = 0;

bool mkstemp_enabled = true;

// If mkstemp_enabled is false then return failure (-1)
// Otherwise forward to real mkstemp()
int mkstemp(char *template)
{
    if (!mkstemp_ptr)
        mkstemp_ptr = (int(*)(char *)) dlsym(RTLD_NEXT, "mkstemp");

    HEX_TEST_FATAL(mkstemp_ptr != NULL);

    if (!mkstemp_enabled)
        return -1;
    else
        return mkstemp_ptr(template);
}

int(*asprintf_ptr)(char **, const char *, ...) = 0;

bool asprintf_enabled = true;

// If asprintf_enabled is false then return failure (-1)
// Otherwise forward to real asprintf()
int asprintf(char **strp, const char *fmt, ...)
{
    int status = -1;
    va_list ap;
    va_start(ap, fmt);
    if (asprintf_enabled)
        status = vasprintf(strp, fmt, ap);
    va_end(ap);
    return status;
}

