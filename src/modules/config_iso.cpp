// HEX SDK

#include <cstring> // basename, strcmp
#include <dirent.h> // DIR, ...
#include <unistd.h> // F_OK
#include <sys/stat.h>

#include <hex/process.h>
#include <hex/log.h>

#include <hex/config_module.h>

static const char ISO_MNT_DIR[] = "/mnt/iso";
static const char SUPPORT_DIR[] = "/var/support";

static bool
IsIsoMounted()
{
    struct stat buf;
    if (stat(ISO_MNT_DIR, &buf) != 0)
        HexLogFatal("Failed to stat ISO mount directory: %s", ISO_MNT_DIR);

    // inode will be 1 or 2 when mounted, and > 2 otherwise
    return (buf.st_ino <= 2);
}


static bool
MountIso()
{
    FILE *fp = NULL;
    char isodev[64];

    // FIXME: change to #include<iso.h>
    fp = HexPOpenF(HEX_SDK " WaitForCdrom");
    if (fp != NULL) {
        if (fgets(isodev, sizeof(isodev), fp) == NULL) {
            HexLogError("Unable to retieve the device name of plugged iso");
            pclose(fp);
            return false;
        }
        pclose(fp);

        // remov trailing newline character
        isodev[strcspn(isodev, "\n")] = 0;
    }
    else {
        HexLogError("Unable to scan ISO drive");
        return false;
    }

    if (!IsIsoMounted()) {
        HexLogDebug("Mounting ISO device");
        if (HexSystemF(0, "/bin/mount %s %s >/dev/null 2>&1", isodev, ISO_MNT_DIR) != 0) {
            HexLogError("Failed to mount ISO device");
            return false;
        }
    }

    return true;
}

static bool
UmountIso()
{
    if (IsIsoMounted()) {
        HexLogDebug("Unmounting ISO device");
        if (HexSystemF(0, "/bin/umount %s >/dev/null 2>&1", ISO_MNT_DIR) != 0) {
            HexLogError("Failed to unmount ISO device");
            return false;
        }
    }
    return true;
}

static void
MountIsoUsage(void)
{
    fprintf(stderr, "Usage: %s mount_iso\n", HexLogProgramName());
}

static int
MountIsoMain(int argc, char* argv[])
{
    if (argc != 1) {
        MountIsoUsage();
        return 1;
    }
    return MountIso() ? 0 : 1;
}

static void
UmountIsoUsage(void)
{
    fprintf(stderr, "Usage: %s umount_iso\n", HexLogProgramName());
}

static int
UmountIsoMain(int argc, char* argv[])
{
    if (argc != 1) {
        UmountIsoUsage();
        return 1;
    }
    return UmountIso() ? 0 : 1;
}

static void
ListIsoUsage(void)
{
    fprintf(stderr, "Usage: %s list_iso_files\n", HexLogProgramName());
}

static int
ListIsoMain(int argc, char* argv[])
{
    if (argc != 1) {
        ListIsoUsage();
        return 1;
    }

    bool umountWhenDone = false;
    if (!IsIsoMounted()) {
        if (!MountIso())
            return 1;
        umountWhenDone = true;
    }

    HexLogDebug("Listing ISO device files");

    int status = 0;
    DIR *dir = opendir(ISO_MNT_DIR);
    if (dir) {
        while (1) {
            struct dirent *p = readdir(dir);
            if (p == NULL) break;

            // Echo all non-dot files to stdout
            if (p->d_name[0] != '.') {
                HexLogDebug("ISO device file: %s", p->d_name);
                printf("%s\n", p->d_name);
            }
        }

        closedir(dir);
    } else {
        HexLogError("ISO mount directory not found: %s", ISO_MNT_DIR);
        status = 1;
    }

    if (umountWhenDone)
        UmountIso();

    return status;
}

CONFIG_MODULE(iso, 0, 0, 0, 0, 0);
CONFIG_COMMAND(mount_iso, MountIsoMain, MountIsoUsage);
CONFIG_COMMAND(umount_iso, UmountIsoMain, UmountIsoUsage);
CONFIG_COMMAND(list_iso_files, ListIsoMain, ListIsoUsage);

