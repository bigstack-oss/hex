// HEX SDK

#include <cstring> // basename, strcmp
#include <dirent.h> // DIR, ...
#include <unistd.h> // F_OK
#include <sys/stat.h>

#include <hex/process.h>
#include <hex/log.h>

#include <hex/config_module.h>

static const char SUPPORT_DIR[] = "/var/support";

static bool
ResolvePath(const char *arg, const char *dir, std::string& path)
{
    // Always search for file in support directory
    // even if user supplied an absolute path
    std::string tmp = dir;
    tmp += '/';
    tmp += basename(arg);
    if (access(tmp.c_str(), F_OK) == 0) {
        path = tmp;
        return true;
    }

    return false;
}

static bool
IsUsbMounted()
{
    struct stat buf;
    if (stat(USB_MNT_DIR, &buf) != 0)
        HexLogFatal("Failed to stat USB mount directory: %s", USB_MNT_DIR);

    // inode will be 1 or 2 when mounted, and > 2 otherwise
    return (buf.st_ino <= 2);
}

static bool
MountUsb()
{
    FILE *fp = NULL;
    char usbdev[64];

    // FIXME: change to #include<usb.h>
    fp = HexPOpenF(HEX_SDK " WaitForUsb");
    if (fp != NULL) {
        if (fgets(usbdev, sizeof(usbdev), fp) == NULL) {
            HexLogError("Unable to retieve the device name of plugged usb");
            pclose(fp);
            return false;
        }
        pclose(fp);

        // remov trailing newline character
        usbdev[strcspn(usbdev, "\n")] = 0;
    }
    else {
        HexLogError("Unable to scan USB drive");
        return false;
    }

    if (!IsUsbMounted()) {
        HexLogDebug("Mounting USB device");
        if (HexSystemF(0, "/bin/mount %s %s >/dev/null 2>&1", usbdev, USB_MNT_DIR) != 0) {
            HexLogError("Failed to mount USB device");
            return false;
        }
    }

    return true;
}

static bool
UmountUsb()
{
    if (IsUsbMounted()) {
        HexLogDebug("Unmounting USB device");
        if (HexSystemF(0, "/bin/sync ; /bin/umount %s >/dev/null 2>&1", USB_MNT_DIR) != 0) {
            HexLogError("Failed to unmount USB device");
            return false;
        }
    }
    return true;
}

static void
MountUsbUsage(void)
{
    fprintf(stderr, "Usage: %s mount_usb\n", HexLogProgramName());
}

static int
MountUsbMain(int argc, char* argv[])
{
    if (argc != 1) {
        MountUsbUsage();
        return 1;
    }
    return MountUsb() ? 0 : 1;
}

static void
UmountUsbUsage(void)
{
    fprintf(stderr, "Usage: %s umount_usb\n", HexLogProgramName());
}

static int
UmountUsbMain(int argc, char* argv[])
{
    if (argc != 1) {
        UmountUsbUsage();
        return 1;
    }
    return UmountUsb() ? 0 : 1;
}

static void
ListUsbUsage(void)
{
    fprintf(stderr, "Usage: %s list_usb_files\n", HexLogProgramName());
}

static int
ListUsbMain(int argc, char* argv[])
{
    if (argc != 1) {
        ListUsbUsage();
        return 1;
    }

    bool umountWhenDone = false;
    if (!IsUsbMounted()) {
        if (!MountUsb())
            return 1;
        umountWhenDone = true;
    }

    HexLogDebug("Listing USB device files");

    int status = 0;
    DIR *dir = opendir(USB_MNT_DIR);
    if (dir) {
        while (1) {
            struct dirent *p = readdir(dir);
            if (p == NULL) break;

            // Echo all non-dot files to stdout
            if (p->d_name[0] != '.') {
                HexLogDebug("USB device file: %s", p->d_name);
                printf("%s\n", p->d_name);
            }
        }

        closedir(dir);
    } else {
        HexLogError("USB mount directory not found: %s", USB_MNT_DIR);
        status = 1;
    }

    if (umountWhenDone)
        UmountUsb();

    return status;
}

static void
DownloadUsage(void)
{
    // <file>  Name of file in /var/support to download.
    fprintf(stderr, "Usage: %s download_usb_file <file>\n", HexLogProgramName());
}

static int
DownloadMain(int argc, char* argv[])
{
    if (argc != 2) {
        DownloadUsage();
        return 1;
    }

    std::string path;
    if (!ResolvePath(argv[1], SUPPORT_DIR, path)) {
        HexLogFatal("File not found: %s", argv[1]);
    }

    bool umountWhenDone = false;
    if (!IsUsbMounted()) {
        if (!MountUsb())
            return 1;
        umountWhenDone = true;
    }

    HexLogDebug("Downloading to USB: %s", path.c_str());

    int status = 0;
    if (HexSpawn(0, "/bin/cp", path.c_str(), USB_MNT_DIR, NULL) != 0) {
        HexLogError("Failed to download file to USB: %s", path.c_str());
        status = 1;
    }

    if (umountWhenDone)
        UmountUsb();

    return status;
}

static void
UploadUsage(void)
{
    // <file>  Name of file on USB device to upload.
    // <dir>   Target directory for upload. (default is /var/support)
    fprintf(stderr, "Usage: %s upload_usb_file <file> [<dir>]\n", HexLogProgramName());
}

static int
UploadMain(int argc, char* argv[])
{
    const char *file;
    const char *dir = SUPPORT_DIR;

    if (argc == 2) {
        file = argv[1];
    }
    else if (argc == 3) {
        file = argv[1];
        dir = argv[2];
    }
    else {
        UploadUsage();
        return 1;
    }

    std::string path;
    if (!ResolvePath(file, USB_MNT_DIR, path))
        HexLogFatal("USB file not found: %s", argv[1]);

    bool umountWhenDone = false;
    if (!IsUsbMounted()) {
        if (!MountUsb())
            return 1;
        umountWhenDone = true;
    }

    HexLogDebug("Uploading from USB: %s, %s", path.c_str(), dir);

    int status = 0;
    if (HexSpawn(0, "/bin/cp", path.c_str(), dir, NULL) != 0) {
        HexLogError("Failed to upload file from USB: %s", path.c_str());
        status = 1;
    }

    if (umountWhenDone)
        UmountUsb();

    return status;
}

CONFIG_MODULE(usb, 0, 0, 0, 0, 0);

CONFIG_COMMAND(mount_usb, MountUsbMain, MountUsbUsage);
CONFIG_COMMAND(umount_usb, UmountUsbMain, UmountUsbUsage);
CONFIG_COMMAND(list_usb_files, ListUsbMain, ListUsbUsage);
CONFIG_COMMAND(download_usb_file, DownloadMain, DownloadUsage);
CONFIG_COMMAND(upload_usb_file, UploadMain, UploadUsage);
