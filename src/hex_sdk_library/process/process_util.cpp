// HEX SDK

#include <string>
#include <cstring> // strcpy
#include <cerrno>

#include <hex/log.h>
#include <hex/process.h>
#include <hex/tempfile.h>

std::string
HexUtilPOpen(const char *arg, ...)
{
    char cmd[512], buffer[1024];
    std::string result;

    va_list ap;
    va_start(ap, arg);
    vsnprintf(cmd, sizeof(cmd), arg, ap);
    va_end (ap);
    buffer[0] = '\0';
    FILE *fd = HexPOpenF("%s", cmd);
    if (fd) {
        while (fgets(buffer, sizeof(buffer), fd)) {
            result.append(buffer);
        }
        fclose(fd);
    }

    return result;
}

int
HexUtilSystemWithPage(const char* command)
{
    HexTempFile shellFile;

    std::string fullCommand;

    fullCommand  = command;
    fullCommand += " | more\n";

    write(shellFile.fd(), fullCommand.data(), fullCommand.length());

    shellFile.close();

    int rc = HexSpawn(0, "/bin/sh", shellFile.path(), 0x00, NULL);

    return HexExitStatus(rc);
}

int HexUtilRunSystem_(int debugLevel, int timeout, const char *location, char *buf, const char** argv)
{
    int rc;

    if (HexLogDebugLevel >= debugLevel) {
        if (location)
            HexLogInfo("%s: Executing: %s", location, buf);
        else
            HexLogInfo("Executing: %s", buf);

        // Set pipefail so that we get the true exit status of the command
        std::string cmd = "set -o pipefail && ";
        cmd += buf;
        cmd += " 2>&1 | logger -t ";
        cmd += HexLogProgramName();
        argv[2] = cmd.c_str();
        rc = HexSpawnV(timeout, (char* const*)argv);
    }
    else {
        std::string cmd = buf;
        cmd += " >/dev/null 2>&1";
        argv[2] = cmd.c_str();
        rc = HexSpawnV(timeout, (char* const*)argv);
    }

    return rc;
}

int HexUtilSystem_(int debugLevel, int timeout, const char *location, const char *arg, ...)
{
    const char *argv[4];
    argv[0] = "/bin/bash";
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

    char *buf = NULL;
    if ((buf = (char *)malloc(len)) == NULL) {
        errno = ENOMEM;
        return -1;
    }

    int n = strlen(arg) + 1;
    snprintf(buf, len, "%s ", arg);
    va_start(ap, arg);
    while ((p = va_arg(ap, char *)) != NULL) {
        snprintf(buf + n, len - n, "%s ", p);
        n += strlen(p) + 1;
    }
    va_end(ap);

    int rc = HexUtilRunSystem_(debugLevel, timeout, location, buf, argv);

    free(buf);
    return rc;
}

int HexUtilSystemV_(int debugLevel, int timeout, const char *location, char* const origArgv[])
{
    const char *argv[4];
    argv[0] = "/bin/bash";
    argv[1] = "-c";
    argv[2] = NULL;
    argv[3] = NULL;

    int i, len = 0;
    for (i = 0; origArgv[i] != NULL; ++i) {
        // include room for space between arguments
        len += strlen(origArgv[i]) + 1;
    }

    // include room for terminating null character
    len += 1;

    char *buf = NULL;
    if ((buf = (char *)malloc(len)) == NULL) {
        errno = ENOMEM;
        return -1;
    }

    int n = 0;
    for (i = 0; origArgv[i] != NULL; ++i) {
        snprintf(buf + n, len - n, "%s ", origArgv[i]);
        n += strlen(origArgv[i]) + 1;
    }

    int rc = HexUtilRunSystem_(debugLevel, timeout, location, buf, argv);

    free(buf);
    return rc;
}

int HexUtilSystemF_(int debugLevel, int timeout, const char *location, const char *fmt, ...)
{
    const char *argv[4];
    argv[0] = "/bin/bash";
    argv[1] = "-c";
    argv[2] = NULL;
    argv[3] = NULL;

    char *buf = NULL;
    va_list ap;
    va_start(ap, fmt);
    int status = vasprintf(&buf, fmt, ap);
    if (status < 0)
        return status;
    va_end(ap);

    int rc = HexUtilRunSystem_(debugLevel, timeout, location, buf, argv);

    free(buf);
    return rc;
}

