// HEX SDK
#include <filesystem>

#include <hex/process.h>
#include <hex/log.h>
#include <hex/tempfile.h>
#include <hex/strict.h>

#include <hex/config_module.h>  // CONFIG_EXIT_NEED_REBOOT, ...
#include <hex/cli_module.h>
#include <hex/cli_util.h>

#define CONFIG_CMD "/usr/sbin/hex_config"

namespace fs = std::filesystem;

static int
GetList(CliList& list)
{
    if (CliPopulateList(list, CONFIG_CMD " snapshot_list | sort") != 0)
        return CLI_UNEXPECTED_ERROR;

    if (list.size() == 0)
        CliPrintf("No snapshot files available\n");

    return CLI_SUCCESS;
}

static int
SelectSnapshot(std::string& snapshot, const char* picked)
{
    CliList list;
    int rc = GetList(list);
    if (rc != CLI_SUCCESS)
        return rc;
    if (list.size() == 0)
        return CLI_FAILURE;

    int index;
    if (picked == NULL) {
        index = CliReadListIndex(list);
        if (index < 0)
            return CLI_FAILURE;
    }
    else {
        int64_t tmp;
        if (!HexParseInt(picked, 1, list.size(), &tmp)) {
            CliPrintf("Invalid index\n");
            return CLI_FAILURE;
        }
        index = tmp - 1;
    }

    snapshot = list[index];
    return CLI_SUCCESS;
}

static int
CreateMain(int argc, const char** argv)
{
    char filename[256];
    FILE *fp = NULL;

    AutoSignalHandlerMgt autoSignalHandlerMgt(UnInterruptibleHdr);

    HexTempFile tmpfile;
    if (tmpfile.fd() < 0) {
        HexLogError("Could not create temporary file");
        return CLI_UNEXPECTED_ERROR;
    }

    if (argc == 1) {
        fp = HexPOpenF(CONFIG_CMD " snapshot_create");
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

        fp = HexPOpenF(CONFIG_CMD " snapshot_create %s", tmpfile.path());
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
            HexLogInfo("%s created a snapshot %s via %s", username, filename, "CLI");
            CliPrintf("%s", filename);
        }
        else {
            HexLogError("Could not create snapshot file");
            status = CLI_UNEXPECTED_ERROR;
        }
    }
    else {
        HexLogError("Could not create snapshot file");
        status = CLI_UNEXPECTED_ERROR;
    }

    return status;
}

static int
ApplyMain(int argc, const char** argv)
{
    if (argc > 2)
        return CLI_INVALID_ARGS;

    std::string snapshot;
    AutoSignalHandlerMgt autoSignalHandlerMgt(UnInterruptibleHdr);
    bool onlyLoad = false;

    if (strcmp(argv[0], "load_network") == 0) {
        onlyLoad = true;
    }

    do {
        int rc = SelectSnapshot(snapshot, argc == 1 ? NULL : argv[1]);
        if (rc != CLI_SUCCESS)
            return rc;

        if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0,
                          CONFIG_CMD, "snapshot_get_comment",
                          snapshot.c_str(), NULL) == 0)
            break;

        CliPrintf("Unable to get comment from snapshot file. The snapshot file is invalid.\n\nPlease select a different file.\n\n");
        //force argc to 1 so that we prompt for an available file if the user came in with index as an arg
        argc = 1;
    } while (1);

    if (FirstTimeSetupRequired() && !onlyLoad) {
        char buf[255];

        time_t currentTime;
        struct tm currentTm;

        // Get the current time
        if (time(&currentTime) == -1) {
            return CLI_UNEXPECTED_ERROR;
        }

        // Convert it into a localtime
        if (localtime_r(&currentTime, &currentTm) == NULL) {
            return CLI_UNEXPECTED_ERROR;
        }

        // Format the current time into the CLI presented format
        if (strftime(buf, 255, "%m/%d/%Y %H:%M:%S %Z", &currentTm) == 0) {
            return CLI_UNEXPECTED_ERROR;
        }

        CliPrintf("Date/Time is important for applying changes to an unconfigured box.");
        CliPrintf("Please confirm the current time is good.");
        printf("\n");
        CliPrintf("  * Local Time: %s", buf);
        printf("\n");
        if (!CliReadConfirmation())
            return CLI_SUCCESS;
    }

    if (onlyLoad) {
        CliPrintf("Load snapshot is for debugging and won't persist system changes.");
        CliPrintf("Please confirm this is on purpose.");
        if (!CliReadConfirmation())
            return CLI_SUCCESS;
    }

    int status = CLI_UNEXPECTED_ERROR;

    if (onlyLoad) {
        /* set /etc/settings.txt
         * snapshot.apply.action = load
         * snapshot.apply.policy.ignore = [ignore policies]
         */
        HexSystemF(0, HEX_SDK " snapshot_load_network_config");
    }

    int ret = HexExitStatus(HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0,
                                          CONFIG_CMD, "snapshot_apply",
                                          snapshot.c_str(), NULL));
    if ((ret & EXIT_FAILURE) == 0) {
        char username[256];
        CliGetUserName(username, sizeof(username));
        //HexLogEvent("User [username] successfully apply snapshot [snapshot] via [CLI]");
        if (onlyLoad)
            CliPrintf("Policy snapshot file loaded\n");
        else
            CliPrintf("Policy snapshot file applied\n");
        status = CLI_SUCCESS;
    }
    else {
        CliPrintf("Policy snapshot file could not be applied.\n");
        HexLogError("Could not apply snapshot file: %s", snapshot.c_str());
    }

    if (onlyLoad)
        HexSystemF(0, HEX_SDK " snapshot_load_network_undo");

    if ((ret & CONFIG_EXIT_NEED_REBOOT) != 0) {
        CliPrintf("Snapshot requires reboot\nRebooting...\n");
        HexSystemF(0, "/usr/sbin/hex_config reboot");
    }
    else if ((ret & CONFIG_EXIT_NEED_LMI_RESTART) != 0) {
        CliPrintf("Snapshot requires LMI restart\nRestarting LMI... ");
        HexSystemF(0, CONFIG_CMD " restart_lmi");
        CliPrintf("Done\n");
    }

    return status;
}

static int
ListMain(int argc, const char** argv)
{
    if (argc != 1)
        return CLI_INVALID_ARGS;

    CliList list;
    int rc = GetList(list);
    if (rc != 0)
        return rc;

    for (size_t i = 0; i < list.size(); ++i)
        CliPrintf("%zu: %s\n", i + 1, list[i].c_str());

    return CLI_SUCCESS;
}

static int
DeleteMain(int argc, const char** argv)
{
    if (argc > 2) {
        return CLI_INVALID_ARGS;
    }

    std::string snapshot;
    int rc = SelectSnapshot(snapshot, argc == 1 ? NULL : argv[1]);
    if (rc != CLI_SUCCESS)
        return rc;

    AutoSignalHandlerMgt autoSignalHandlerMgt(UnInterruptibleHdr);
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0,
                      CONFIG_CMD, "snapshot_delete",
                      snapshot.c_str(), NULL) == 0) {
        char username[256];
        CliGetUserName(username, sizeof(username));
        //HexLogEvent("User [username] successfully delete snapshot [snapshot] via [CLI]");
        CliPrintf("Snapshot file %s deleted\n", snapshot.c_str());
    }
    else {
        HexLogError("Could not delete snapshot file: %s", snapshot.c_str());
        return CLI_UNEXPECTED_ERROR;
    }

    return CLI_SUCCESS;
}

static int
GetCommentMain(int argc, const char** argv)
{
    if (argc > 2) {
        return CLI_INVALID_ARGS;
    }

    std::string snapshot;
    int rc = SelectSnapshot(snapshot, argc == 1 ? NULL : argv[1]);
    if (rc != CLI_SUCCESS)
        return rc;

    AutoSignalHandlerMgt autoSignalHandlerMgt(UnInterruptibleHdr);
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0,
                      CONFIG_CMD, "snapshot_get_comment",
                      snapshot.c_str(), NULL) != 0)
        return CLI_UNEXPECTED_ERROR;

    return CLI_SUCCESS;
}

static int
SetCommentMain(int argc, const char** argv)
{
    std::string snapshot;
    std::string comment;

    int rc = SelectSnapshot(snapshot, argc == 1 ? NULL : argv[1]);
    if (rc != CLI_SUCCESS)
        return rc;

    if (argc > 2) {
        comment = argv[2];
        for (int i = 3; i < argc; ++i) {
            comment += ' ';
            comment += argv[i];
        }
        comment += '\n';
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
    AutoSignalHandlerMgt autoSignalHandlerMgt(UnInterruptibleHdr);
    if (HexSystemF(0, "cat %s | " CONFIG_CMD " snapshot_set_comment %s", tmpfile.path(), snapshot.c_str()) == 0) {
        CliPrintf("Comment updated\n");
    }
    else {
        status = CLI_UNEXPECTED_ERROR;
    }

    return status;
}

static int
SetupNetwork(std::string *ifname)
{
    int index;
    std::string cmd;
    CliList list;

    HexSpawn(0, HEX_SDK, "-v", "DumpInterface", NULL);
    cmd = "/usr/sbin/ip link | awk -F'[: ]' '/eth.*mtu/{print $3}' | sort -V";
    if (CliPopulateList(list, cmd.c_str()) != 0) {
        return CLI_UNEXPECTED_ERROR;
    }

    if (list.size() == 0) {
        CliPrintf("no interface available\n");
        return CLI_UNEXPECTED_ERROR;
    }

    CliPrintf("Select an interface: ");
    index = CliReadListIndex(list);
    if (index < 0 || index >= (int)list.size()) {
        CliPrintf("invalid selection");
        return CLI_UNEXPECTED_ERROR;
    }

    *ifname = list[index];

    CliPrintf("Setting up network ...");
    if (HexSystemF(0, "/usr/sbin/ip link set %s up", ifname->c_str()) != 0) {
        CliPrintf("failed to bring up interface %s", ifname->c_str());
        return CLI_UNEXPECTED_ERROR;
    }
    if (HexSystemF(0, "/usr/sbin/dhclient %s -cf /etc/dhcp/dhclient.conf 2>/dev/null", ifname->c_str()) != 0) {
        CliPrintf("failed to get ip from dhcp server");
        return CLI_UNEXPECTED_ERROR;
    }

    return CLI_SUCCESS;
}

static int
CleanupNetwork(const std::string &ifname)
{
    HexSystemF(0, "killall dhclient");
    HexSystemF(0, "ip addr flush dev %s", ifname.c_str());

    return CLI_SUCCESS;
}

static int
PushToUsb(const std::string &snapshot)
{
    CliPrintf("Insert a USB drive into the USB port on the appliance.\n");
    if (!CliReadConfirmation())
        return CLI_SUCCESS;

    AutoSignalHandlerMgt autoSignalHandlerMgt(UnInterruptibleHdr);
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, CONFIG_CMD, "mount_usb", NULL) != 0) {
        CliPrintf("Could not write to the USB drive. Please check the USB drive and retry the command.\n");
        return CLI_SUCCESS;
    }

    CliPrintf("Copying...\n");
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, CONFIG_CMD, "download_usb_file", snapshot.c_str(), NULL) != 0)
        CliPrintf("Copy failed. Please check the USB drive and retry the command.\n");
    else
        CliPrintf("Copy complete. It is safe to remove the USB drive.\n");

    HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, CONFIG_CMD, "umount_usb", NULL);

    return CLI_SUCCESS;
}

static int
PushToNfs(const std::string &snapshot)
{
    std::string ifname = "";

    if (FirstTimeSetupRequired())
        SetupNetwork(&ifname);

    int ret = CLI_SUCCESS;
    std::string server, filename;

    if (!CliReadLine("Enter NFS location [server:/path]: ", server)) {
        ret = CLI_INVALID_ARGS;
        goto nfs_push_cleanup;
    }

    CliPrintf("Copying...\n");

    filename = "/var/support/" + snapshot;
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0,
            HEX_SDK, "snapshot_nfs_push", filename.c_str(), server.c_str(), NULL) != 0) {
        CliPrintf("Copy failed to %s. Please check NFS settings.\n", server.c_str());
        ret = CLI_INVALID_ARGS;
        goto nfs_push_cleanup;
    }

    CliPrintf("Copy complete.\n");

nfs_push_cleanup:
    if (FirstTimeSetupRequired())
        CleanupNetwork(ifname);

    return ret;
}

static int
PushMain(int argc, const char** argv)
{
    if (argc > 3 /* argv[0]: "Push", argv[1]: "usb|nfs", argv[2]: "index" */)
        return CLI_INVALID_ARGS;

    int index;
    std::string media, snapshot;

    if (CliMatchCmdHelper(argc, argv, 1, "echo 'usb\nnfs'", &index, &media, "Select a media: ") != CLI_SUCCESS) {
        CliPrintf("Unknown media");
        return false;
    }

    int rc = SelectSnapshot(snapshot, argc < 3 ? NULL : argv[2]);
    if (rc != CLI_SUCCESS)
        return rc;

    switch(index) {
        case 0: return PushToUsb(snapshot);
        case 1: return PushToNfs(snapshot);
    }

    return CLI_SUCCESS;
}

static int
PullFromUsb()
{
    int ret = CLI_SUCCESS;

    CliPrintf("Insert a USB drive into the USB port on the appliance.\n");
    if (!CliReadConfirmation())
        return ret;

    AutoSignalHandlerMgt autoSignalHandlerMgt(UnInterruptibleHdr);
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, CONFIG_CMD, "mount_usb", NULL) != 0) {
        CliPrintf("Could not read from the USB drive. Please check the USB drive and retry the command.\n");
        return ret;
    }

    CliList list;
    int index;
    std::string filename;
again:
    if (CliPopulateList(list, CONFIG_CMD " list_usb_files | grep '\\.snapshot$' | sort") != 0) {
        ret = CLI_UNEXPECTED_ERROR;
        goto cleanup;
    }

    if (list.size() == 0) {
        CliPrintf("No snapshot files available\n");
        goto cleanup;
    }

    index = CliReadListIndex(list);
    if (index < 0) {
        goto cleanup;
    }

    CliPrintf("Copying...\n");
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, CONFIG_CMD, "upload_usb_file", list[index].c_str(), NULL) != 0) {
        CliPrintf("Copy failed. Please check the USB drive and retry the command.\n");
        goto cleanup;
    }

    filename = "/var/support/" + list[index];
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, CONFIG_CMD, "snapshot_get_comment", filename.c_str(), NULL) != 0) {
        CliPrintf("Unable to get comment from snapshot file. The snapshot file is invalid.\n\nPlease select a different file.\n\n");
        unlink(filename.c_str());
        goto again;
    }

    char username[256];
    CliGetUserName(username, sizeof(username));
    //HexLogEvent("User [username] successfully copy snapshot [snapshot] from USB drive from [CLI]");
    CliPrintf("Copy complete. It is safe to remove the USB drive.\n");

cleanup:
    HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, CONFIG_CMD, "umount_usb", NULL);

    return ret;
}

static int
PullFromNfs()
{
    std::string ifname = "";

    if (FirstTimeSetupRequired())
        SetupNetwork(&ifname);

    int ret = CLI_SUCCESS;
    int index;
    std::string server, snapshot, filename, cmdOpts, cmdDesc;
    CliList options;
    CliList descriptions;

    if (!CliReadLine("Enter NFS location [server:/path]: ", server)) {
        ret = CLI_INVALID_ARGS;
        goto nfs_pull_cleanup;
    }

    cmdOpts = HEX_SDK " snapshot_nfs_list " + server;
    cmdDesc = HEX_SDK " -v snapshot_nfs_list " + server;
    if (CliPopulateList(options, cmdOpts.c_str()) != 0 ||
        CliPopulateList(descriptions, cmdDesc.c_str()) != 0) {
        ret = CLI_UNEXPECTED_ERROR;
        goto nfs_pull_cleanup;
    }

    if (options.size() == 0 || options.size() != descriptions.size()) {
        CliPrintf("no snapshot available\n");
        ret = CLI_UNEXPECTED_ERROR;
        goto nfs_pull_cleanup;
    }

    CliPrintf("Select a snapshot: ");
    index = CliReadListIndex(descriptions);
    if (index < 0 || index >= (int)descriptions.size()) {
        CliPrintf("invalid selection");
        ret = CLI_INVALID_ARGS;
        goto nfs_pull_cleanup;
    }

    snapshot = options[index];

    CliPrintf("Copying...\n");
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0,
            HEX_SDK, "snapshot_nfs_pull", server.c_str(), snapshot.c_str(), "/var/support/", NULL) != 0) {
        CliPrintf("Copy failed from %s. Please check NFS settings.\n", filename.c_str());
        ret = CLI_INVALID_ARGS;
        goto nfs_pull_cleanup;
    }

    filename = "/var/support/" + snapshot;
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, CONFIG_CMD, "snapshot_get_comment", filename.c_str(), NULL) != 0) {
        CliPrintf("Unable to get comment from snapshot file. The snapshot file is invalid.\n\nPlease select a different file.\n\n");
        unlink(filename.c_str());
        ret = CLI_INVALID_ARGS;
        goto nfs_pull_cleanup;
    }

    CliPrintf("Copy complete.\n");

nfs_pull_cleanup:
    if (FirstTimeSetupRequired())
        CleanupNetwork(ifname);

    return ret;
}

static int
PullFromUrl()
{
    std::string ifname = "";

    if (FirstTimeSetupRequired())
        SetupNetwork(&ifname);

    int ret = CLI_SUCCESS;
    std::string server, snapshot, filename;

    if (!CliReadLine("Enter URL or snapshot name of PXE server: ", server)) {
        ret = CLI_INVALID_ARGS;
        goto url_pull_cleanup;
    }

    if (server.length() == 0) {
        CliPrintf("No URL or name specified\n");
        ret = CLI_INVALID_ARGS;
        goto url_pull_cleanup;
    }

    if (server.rfind("http", 0) == std::string::npos) {
        server = "http://192.168.1.150/" + server + ".snapshot";
    }

    snapshot = fs::path(server).filename();

    CliPrintf("Downloading... %s\n", snapshot.c_str());
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0,
            HEX_SDK, "util_url_download", server.c_str(), "/var/support/", "10", NULL) != 0) {
        CliPrintf("Failed to download %s. Please check if URL or name is valid.\n", server.c_str());
        ret = CLI_INVALID_ARGS;
        goto url_pull_cleanup;
    }

    filename = "/var/support/" + snapshot;
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, CONFIG_CMD, "snapshot_get_comment", filename.c_str(), NULL) != 0) {
        CliPrintf("Unable to get comment from snapshot file. The snapshot file is invalid.\n\nPlease select a different file.\n\n");
        unlink(filename.c_str());
        ret = CLI_INVALID_ARGS;
        goto url_pull_cleanup;
    }

    CliPrintf("Download completed.\n");

url_pull_cleanup:
    if (FirstTimeSetupRequired())
        CleanupNetwork(ifname);

    return ret;
}


static int
PullMain(int argc, const char** argv)
{
    if (argc > 2 /* argv[0]: "pull", argv[1]: "usb|nfs|url" */)
        return CLI_INVALID_ARGS;

    int index;
    std::string media;

    if (CliMatchCmdHelper(argc, argv, 1, "echo 'usb\nnfs\nurl'", &index, &media, "Select a media: ") != CLI_SUCCESS) {
        CliPrintf("Unknown media");
        return false;
    }

    switch(index) {
        case 0: return PullFromUsb();
        case 1: return PullFromNfs();
        case 2: return PullFromUrl();
    }

    return CLI_SUCCESS;
}

// This mode is not available in strict error state
CLI_MODE(CLI_TOP_MODE, "snapshot",
         "Work with policy snapshot files and will be stored in /var/support.",
         !HexStrictIsErrorState());

CLI_MODE_COMMAND("snapshot", "create", CreateMain, 0,
    "Create a snapshot of current policy files.",
    "create [<comment>]");

CLI_MODE_COMMAND("snapshot", "apply", ApplyMain, 0,
    "Apply a policy snapshot file to the system.",
    "apply [<index>]");

CLI_MODE_COMMAND("snapshot", "load_network", ApplyMain, 0,
    "Load only network policy for environment verification.",
    "load_network [<index>]");

CLI_MODE_COMMAND("snapshot", "list", ListMain, 0,
    "List the policy snapshot files.",
    "list");

CLI_MODE_COMMAND("snapshot", "delete", DeleteMain, 0,
    "Delete a policy snapshot file.",
    "delete [<index>]");

CLI_MODE_COMMAND("snapshot", "get_comment", GetCommentMain, 0,
    "View the comment associated with a policy snapshot file.",
    "get_comment [<index>]");

CLI_MODE_COMMAND("snapshot", "set_comment", SetCommentMain, 0,
    "Replace the comment associated with a policy snapshot file.",
    "set_comment [<index>] [<comment>]");

CLI_MODE_COMMAND("snapshot", "push", PushMain, 0,
    "Push a policy snapshot file to a storage.",
    "push [usb|nfs] [<index>]");

CLI_MODE_COMMAND("snapshot", "pull", PullMain, 0,
    "Pull a policy snapshot file from a storage.",
    "pull [usb|nfs|url]");

