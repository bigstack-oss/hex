// HEX SDK

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <hex/log.h>
#include <hex/process.h>
#include <hex/strict.h>

static const char HEX_STRICT_MARKER[] = "/etc/appliance/state/strict_mode";
static const char HEX_STRICT_ERROR_MARKER[] = "/etc/appliance/state/strict_mode_error";

int
HexStrictIsEnabled(void)
{
    return access(HEX_STRICT_MARKER, F_OK) == 0 ? 1 : 0;
}

void
HexStrictSetEnabled(void)
{
    HexLogInfo("Enabling STRICT mode. Rebooting...");

    // Touch STRICT failure marker file
    int fd = open(HEX_STRICT_MARKER, O_WRONLY | O_CREAT, 0644);
    if (fd) {
        close(fd);
    }

    // HexSpawn waits for the process to finish. Instead fork out and return;
    int pid = fork();
    if (!pid) {
        HexSpawn(0, "/usr/sbin/hex_config reboot", "10", NULL);
    }
}

int
HexStrictIsErrorState(void)
{
    return access(HEX_STRICT_ERROR_MARKER, F_OK) == 0 ? 1 : 0;
}

void
HexStrictSetErrorState(void)
{
    HexLogError("STRICT failure detected. Rebooting...");

    // Touch STRICT failure marker file
    int fd = open(HEX_STRICT_ERROR_MARKER, O_WRONLY | O_CREAT, 0644);
    if (fd) {
        close(fd);
    }

    // HexSpawn waits for the process to finish. Instead fork out and return;
    int pid = fork();
    if (!pid) {
        HexSpawn(0, "/usr/sbin/hex_config reboot", "10", NULL);
    }
}


