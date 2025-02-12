// HEX SDK

#ifndef CLI_SUPPORT_HELPER_H
#define CLI_SUPPORT_HELPER_H

#include <hex/cli_util.h>

#define SUPPORT_DIR "/var/support/"
#define HEX_CFG "/usr/sbin/hex_config"

static const char* MSG_INSERT_USB = "Insert a USB drive into the USB port on the appliance.";
static const char* MSG_MOUNT_FAIL = "Could not write to the USB drive. Please check the USB drive and retry the command.";
static const char* MSG_COPYING = "Copying...";
static const char* MSG_COPY_FAIL = "Copy failed. Please check the USB drive and retry the command.";
static const char* MSG_COPY_DONE = "Copy complete. It is safe to remove the USB drive.";
static const char* MSG_LIST_EMPTY = "No file available.";
static const char* MSG_SELECT_FAIL = "Invalid selection.";

class CliSupportHelper
{
public:
    static int pick(int argc, const char** argv, int argidx)
    {
        return argc > argidx ? atoi(argv[argidx]) : -1;
    }

    static bool list(const std::string &cmd, CliList *list)
    {
        if (CliPopulateList(*list, cmd.c_str()) != 0) {
            return false;
        }

        if (list->size() == 0)
            CliPrintf(MSG_LIST_EMPTY);

        return true;
    }

    static bool select(const std::string &cmd, const int picked, std::string *value)
    {
        CliList l;
        int index;

        if (!list(cmd, &l))
            return false;

        if (picked == -1) {
            index = CliReadListIndex(l);
            if (index < 0)
                return false;
        }
        else {
            if (picked < 0 || picked >= (int)l.size()) {
                CliPrintf(MSG_SELECT_FAIL);
                return false;
            }
            index = picked;
        }

        *value = l[index];
        return true;
    }

    static bool upload(const std::string &ext)
    {
        CliPrintf(MSG_INSERT_USB);
        if (!CliReadConfirmation())
            return true;

        AutoSignalHandlerMgt hdr(UnInterruptibleHdr);
        if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, HEX_CFG, "mount_usb", NULL) != 0) {
            CliPrintf(MSG_MOUNT_FAIL);
            return true;
        }

        char cmd[256];
        std::string filename, value;

        snprintf(cmd, sizeof(cmd), HEX_CFG " list_usb_files | grep '\\.%s$' | sort", ext.c_str());
        if (!select(cmd, -1, &value))
            goto cleanup;

        CliPrintf(MSG_COPYING);
        if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, HEX_CFG, "upload_usb_file", value.c_str(), NULL) != 0) {
            CliPrintf(MSG_COPY_FAIL);
            goto cleanup;
        }

        filename = SUPPORT_DIR + value;

        CliPrintf(MSG_COPY_DONE);

        // char username[256];
        // CliGetUserName(username, sizeof(username));
        // HexLogEvent("User [username] successfully copy [filename] from USB drive via [CLI]");
    cleanup:
        HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, HEX_CFG, "umount_usb", NULL);

        return true;
    }

    static bool download(const std::string& file)
    {
        CliPrintf(MSG_INSERT_USB);
        if (!CliReadConfirmation())
            return true;

        AutoSignalHandlerMgt hdr(UnInterruptibleHdr);
        if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, HEX_CFG, "mount_usb", NULL) != 0) {
            CliPrintf(MSG_MOUNT_FAIL);
            return true;
        }

        CliPrintf(MSG_COPYING);
        if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, HEX_CFG, "download_usb_file", file.c_str(), NULL) != 0)
            CliPrintf(MSG_COPY_FAIL);
        else
            CliPrintf(MSG_COPY_DONE);

        HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, HEX_CFG, "umount_usb", NULL);

        return true;
    }
};

#endif /* endif CLI_SUPPORT_HELPER_H */

