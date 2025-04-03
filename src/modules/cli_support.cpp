// HEX SDK

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <string>
#include <vector>
#include <unistd.h>

#include <hex/process.h>
#include <hex/parse.h>
#include <hex/log.h>
#include <hex/tempfile.h>

#include <hex/cli_module.h>
#include <hex/cli_util.h>

// Get list of support info files
static int
GetList(CliList& list)
{
    if (CliPopulateList(list, HEX_CFG " list_support_files | sort") != 0)
        return CLI_UNEXPECTED_ERROR;

    if (list.size() == 0)
        CliPrintf("No support information files available\n");

    return CLI_SUCCESS;
}

static int
CreateMain(int argc, const char** argv)
{
    char filename[256];
    FILE *fp = NULL;

    HexTempFile tmpfile;
    if (tmpfile.fd() < 0) {
        HexLogError("Could not create temporary file");
        return CLI_UNEXPECTED_ERROR;
    }

    if (argc == 1) {
        // no comment given
        fp = HexPOpenF(HEX_CFG " create_support_file");
    }
    else {
        // Concatenate arguments into comment string
        std::string comment;
        comment += argv[1];
        for (int i = 2; i < argc; ++i) {
            comment += ' ';
            comment += argv[i];
        }

        write(tmpfile.fd(), comment.c_str(), comment.size());
        tmpfile.close();

        fp = HexPOpenF(HEX_CFG " create_support_file %s", tmpfile.path());
    }

    int status = CLI_SUCCESS;
    if (fp != NULL) {
        if (fgets(filename, sizeof(filename), fp) == NULL) {
            HexLogError("Unable to retieve the name of the file created");
            status = CLI_UNEXPECTED_ERROR;
            pclose(fp);
        }
        else if (pclose(fp) == 0) {
            char username[256];
            CliGetUserName(username, sizeof(username));

            //TODO: HexLogEvent("create support info [filename] by [username] via [CLI]");

            CliPrintf("%s\n", filename);
        }
        else {
            HexLogError("Could not create support info file");
            status = CLI_UNEXPECTED_ERROR;
        }
    }
    else {
        HexLogError("Could not create support info file");
        status = CLI_UNEXPECTED_ERROR;
    }

    return status;
}

static int
ListMain(int argc, const char** argv)
{
    if (argc == 1) {
        CliList list;
        int rc = GetList(list);
        if (rc != 0)
            return rc;

        for (size_t i = 0; i < list.size(); ++i)
            CliPrintf("%zu: %s\n", i + 1, list[i].c_str());

        return CLI_SUCCESS;
    }

    return CLI_INVALID_ARGS;
}

static int
DeleteMain(int argc, const char** argv)
{
    if (argc > 2) {
        return CLI_INVALID_ARGS;
    } else {
        CliList list;
        int rc = GetList(list);
        if (rc != 0)
            return rc;
        if (list.size() == 0)
            return CLI_SUCCESS;

        int index;
        if (argc == 1) {
            index = CliReadListIndex(list);
            if (index < 0)
                return CLI_SUCCESS;
        } else {
            int64_t tmp;
            if (!HexParseInt(argv[1], 1, list.size(), &tmp)) {
                CliPrintf("Invalid index\n");
                return CLI_SUCCESS;
            }
            index = tmp - 1;
        }

        if (HexSpawn(0, HEX_CFG, "delete_support_file", list[index].c_str(), NULL) == 0) {
            CliPrintf("Support information file deleted\n");
        } else {
            HexLogError("Could not delete support info file: %s", list[index].c_str());
            return CLI_UNEXPECTED_ERROR;
        }
    }
    return CLI_SUCCESS;
}

static int
GetCommentMain(int argc, const char** argv)
{
    if (argc > 2) {
        return CLI_INVALID_ARGS;
    } else {
        CliList list;
        int rc = GetList(list);
        if (rc != 0)
            return rc;
        if (list.size() == 0)
            return CLI_SUCCESS;

        int index;
        if (argc == 1) {
            index = CliReadListIndex(list);
            if (index < 0)
                return CLI_SUCCESS;
        } else {
            int64_t tmp;
            if (!HexParseInt(argv[1], 1, list.size(), &tmp)) {
                CliPrintf("Invalid index\n");
                return CLI_SUCCESS;
            }
            index = tmp - 1;
        }

        if (HexSpawn(0, HEX_CFG, "get_support_file_comment", list[index].c_str(), NULL) != 0)
            return CLI_UNEXPECTED_ERROR;
    }

    return CLI_SUCCESS;
}

static int
SetCommentMain(int argc, const char** argv)
{
    CliList list;
    int rc = GetList(list);
    if (rc != 0)
        return rc;
    if (list.size() == 0)
        return CLI_SUCCESS;

    int index;
    std::string comment;
    if (argc == 1) {
        index = CliReadListIndex(list);
        if (index < 0)
            return CLI_SUCCESS;
    } else {
        // argc >= 2
        int64_t tmp;
        if (!HexParseInt(argv[1], 1, list.size(), &tmp)) {
            CliPrintf("Invalid index\n");
            return CLI_SUCCESS;
        }
        index = tmp - 1;

        if (argc > 2) {
            comment = argv[2];
            for (int i = 3; i < argc; ++i) {
                comment += ' ';
                comment += argv[i];
            }
            comment += '\n';
        }
    }

    if (comment.empty()) {
        if (!CliReadMultipleLines("Enter comment: ", comment)) {
            return CLI_SUCCESS;
        }
    }

    HexTempFile tmpfile;
    if (tmpfile.fd() < 0) {
        HexLogError("Could not create temporary file");
        return CLI_UNEXPECTED_ERROR;
    }
    write(tmpfile.fd(), comment.c_str(), comment.size());
    tmpfile.close();

    int status = CLI_SUCCESS;
    if (HexSystemF(0, "cat %s | /usr/sbin/hex_config set_support_file_comment %s", tmpfile.path(), list[index].c_str()) == 0) {
        CliPrintf("Comment updated\n");
    }
    else {
        status = CLI_UNEXPECTED_ERROR;
    }

    return status;
}

static int
DownloadMain(int argc, const char** argv)
{
    if (argc > 2)
        return CLI_INVALID_ARGS;

    CliList list;
    int rc = GetList(list);
    if (rc != 0)
        return rc;
    if (list.size() == 0)
        return CLI_SUCCESS;

    int index;
    if (argc == 1) {
        index = CliReadListIndex(list);
        if (index < 0)
            return CLI_SUCCESS;
    } else {
        int64_t tmp;
        if (!HexParseInt(argv[1], 1, list.size(), &tmp)) {
            CliPrintf("Invalid index\n");
            return CLI_SUCCESS;
        }
        index = tmp - 1;
    }

    CliPrintf("Insert a USB drive into the USB port on the appliance.\n");
    if (!CliReadConfirmation())
        return CLI_SUCCESS;

    if (HexSpawn(0, HEX_CFG, "mount_usb", NULL) != 0) {
        CliPrintf("Could not write to the USB drive. Please check the USB drive and retry the command.\n");
        return CLI_SUCCESS;
    }

    CliPrintf("Copying...\n");
    if (HexSpawn(0, HEX_CFG, "download_usb_file", list[index].c_str(), NULL) != 0) {
        HexSpawn(0, HEX_CFG, "umount_usb", NULL);
        CliPrintf("Copy failed. Please check the USB drive and retry the command.\n");
    }
    else {
        HexSpawn(0, HEX_CFG, "umount_usb", NULL);
        CliPrintf("Copy complete. It is safe to remove the USB drive.\n");
    }

    return CLI_SUCCESS;
}

CLI_MODE(CLI_TOP_MODE, "support",
         "Work with support information files and will be stored in /var/support.",
         !FirstTimeSetupRequired());

CLI_MODE_COMMAND("support", "create", CreateMain, 0,
    "Create a support information file.",
    "create [ <comment> ... ]");

CLI_MODE_COMMAND("support", "list", ListMain, 0,
    "List the support information files.",
    "list");

CLI_MODE_COMMAND("support", "delete", DeleteMain, 0,
    "Delete a support information file.",
    "delete [ <index> ]");

CLI_MODE_COMMAND("support", "get_comment", GetCommentMain, 0,
    "View the comment associated with a support information file.",
    "get_comment [ <index> ]");

CLI_MODE_COMMAND("support", "set_comment", SetCommentMain, 0,
    "Replace the comment associated with a support information file.",
    "set_comment [ <index> [ <comment> ... ]]");

CLI_MODE_COMMAND("support", "download", DownloadMain, 0,
    "Download a support information file to a USB flash drive.",
    "download [ <index> ]");

