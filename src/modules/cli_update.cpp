// HEX SDK

#include <string>
#include <climits>   // PATH_MAX(255)

#include <hex/cli_module.h>
#include <hex/cli_util.h>
#include <hex/process.h>
#include <hex/parse.h>
#include <hex/log.h>
#include <hex/strict.h>

#define HEX_CONFIG "/usr/sbin/hex_config"
#define UPDATE_DIR "/var/update/"

static const char* MSG_INSERT_USB = "Insert a USB device into the USB port on the appliance.";
static const char* MSG_AVAIL_PKG = "Available Firmware Updates:";
static const char* ERR_NO_PKG = "no updates found in the inserted USB device.";
static const char* CMD_LIST_USB_PKG = "/usr/sbin/hex_config list_usb_files | grep \".*[.]pkg\" | sed -E \"s/_.[.]pkg//\" | sed -E \"s/[.]pkg//\" | sort | uniq";

static const char* CMD_LIST_LOCAL_PKG = "/usr/sbin/hex_install list";
static const char* ERR_NO_LOCAL_PKG = "no updates found in local folder %s";

static char*
ItemCompletionMatcher(int argc, const char** argv, int state)
{
    if (argc < 0 || argc > 2) {
        return NULL;
    }

    if (argc == 1) {
        // first arg is command itself, no need for tab completion
        return NULL;
    }
    else {
        static CliCompletionMatcher List;
        return List.match(argc, argv, state, "local", "usb", "server", 0);
    }
}

static int
ListLocal(void)
{
    CliList list;
    std::string cmd = CMD_LIST_LOCAL_PKG;

    AutoSignalHandlerMgt autoSignalHandlerMgt(UnInterruptibleHdr);

    if (CliPopulateList(list, cmd.c_str()) != 0 || list.size() == 0) {
        CliPrintf(ERR_NO_PKG);
        return CLI_SUCCESS;
    }

    CliPrintf(MSG_AVAIL_PKG);
    for (size_t i = 0 ; i < list.size() ; ++i) {
        CliPrintf("%zu: %s", i + 1, list[i].c_str());
    }

    return CLI_SUCCESS;
}

static int
ListUSB(void)
{
    CliList list;
    std::string cmd = CMD_LIST_USB_PKG;

    CliPrintf(MSG_INSERT_USB);
    if (!CliReadConfirmation())
        return CLI_SUCCESS;

    AutoSignalHandlerMgt autoSignalHandlerMgt(UnInterruptibleHdr);

    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, HEX_CONFIG, "mount_usb", NULL) != 0 ||
        CliPopulateList(list, cmd.c_str()) != 0 || list.size() == 0) {
        CliPrintf(ERR_NO_PKG);
        goto list_quit;
    }

    CliPrintf(MSG_AVAIL_PKG);
    for (size_t i = 0 ; i < list.size() ; ++i) {
        CliPrintf("%zu: %s", i + 1, list[i].c_str());
    }

list_quit:
    HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, HEX_CONFIG, "umount_usb", NULL);
    return CLI_SUCCESS;
}

static int
ListServer(void)
{
    CliPrint("Not available");
    return CLI_SUCCESS;
}

static int
ListMain(int argc, const char** argv)
{
    CliList list;
    int index = -1;
    std::string value = "";

    list.push_back("local");
    list.push_back("usb");
    list.push_back("server");

    if(CliMatchListHelper(argc, argv, 1, list, &index, &value) != CLI_SUCCESS) {
        CliPrintf("Unknown command");
        return CLI_INVALID_ARGS;
    }

    switch(index) {
        case 0: return ListLocal();
        case 1: return ListUSB();
        case 2: return ListServer();
    }

    return CLI_SUCCESS;
}

static int
InstallLocal(void)
{
    CliList list;
    std::string cmd = CMD_LIST_LOCAL_PKG;

    AutoSignalHandlerMgt autoSignalHandlerMgt(UnInterruptibleHdr);

    int index = -1;
    if (CliPopulateList(list, cmd.c_str()) != 0 || list.size() == 0 || CliUniqPkgList(list) == 0 || (index = CliReadListIndex(list)) < 0) {
        CliPrintf(ERR_NO_LOCAL_PKG, UPDATE_DIR);
        return CLI_SUCCESS;
    }

    CliPrintf("Firmware update will require an appliance reboot.");
    if (!CliReadConfirmation())
        return CLI_SUCCESS;

    std::string fpath = UPDATE_DIR + list[index];

    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0,
                      "/usr/sbin/hex_install", "-v", "update", fpath.c_str(), NULL) != 0) {
        CliPrintf("Install failed. %s is not a valid update.\n", list[index].c_str());
        HexLogError("Failed to install firmware update %s from %s.", list[index].c_str(), UPDATE_DIR);
        return CLI_SUCCESS;
    }

    HexLogInfo("Successfully started firmware update %s from %s.", list[index].c_str(), UPDATE_DIR);
    return CLI_SUCCESS;
}

static int
InstallUSB(void)
{
    CliList list, pkg_list;
    std::string cmd = CMD_LIST_USB_PKG;
    FILE *pipe = nullptr;

    CliPrintf(MSG_INSERT_USB);
    if (!CliReadConfirmation())
        return CLI_SUCCESS;

    AutoSignalHandlerMgt autoSignalHandlerMgt(UnInterruptibleHdr);

    int index = -1;
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, HEX_CONFIG, "mount_usb", NULL) != 0) {
        CliPrintf(ERR_NO_PKG);
        goto ins_quit;
    }

    CliPopulateList(list, cmd.c_str());

    pipe = popen((std::string("cd /mnt/usb && ls -1")).c_str(), "r");
    if (pipe) {
	char path[PATH_MAX];
	while (fgets(path, PATH_MAX, pipe) != NULL) {
	    size_t n = strlen(path);
            if (n > 0 && path[n-1] == '\n')
                path[n-1] = '\0';

	    pkg_list.push_back(path);
	}
	pclose(pipe);
    }

    if (list.size() == 0 || CliUniqPkgList(list) == 0 || (index = CliReadListIndex(list)) < 0) {
        CliPrintf(ERR_NO_PKG);
        goto ins_quit;
    }

    CliPrintf("Firmware update will require an appliance reboot.");
    if (!CliReadConfirmation())
        goto ins_quit;

    CliPrintf("Uploading %s", list[index].c_str());
    for (auto x : pkg_list) {
	if (x.find(list[index]) != std::string::npos) {
	    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, HEX_CONFIG,
			      "upload_usb_file", x.c_str(), UPDATE_DIR, NULL) != 0) {
		CliPrintf("Install failed. Unable to access file: %s",x.c_str());
		CliPrintf("Please check the USB device and retry the command.");
		goto ins_quit;
	    }
	}
    }

    CliPrintf("It is safe to remove the USB device now.");
    CliPrintf("Updating %s", list[index].c_str());
    if (HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0,
		      "/usr/sbin/hex_install", "-v", "update", (UPDATE_DIR + list[index]).c_str(), NULL) != 0) {
	CliPrintf("Install failed. %s is not a valid update.\n", list[index].c_str());
	HexLogError("Failed to install firmware update %s from USB device.", list[index].c_str());
	goto ins_quit;
    }

    HexLogInfo("Successfully started firmware update %s from USB device.", list[index].c_str());

ins_quit:
    HexSpawnNoSig(UnInterruptibleHdr, (int)true, 0, HEX_CONFIG, "umount_usb", NULL);
    return CLI_SUCCESS;
}

static int
InstallServer(void)
{
    CliPrint("Not available");
    return CLI_SUCCESS;
}

static int
InstallMain(int argc, const char** argv)
{
    CliList list;
    int index = -1;
    std::string value = "";

    list.push_back("local");
    list.push_back("usb");
    list.push_back("server");

    if(CliMatchListHelper(argc, argv, 1, list, &index, &value) != CLI_SUCCESS) {
        CliPrintf("Unknown command");
        return CLI_INVALID_ARGS;
    }

    switch(index) {
        case 0: return InstallLocal();
        case 1: return InstallUSB();
        case 2: return InstallServer();
    }

    return CLI_SUCCESS;
}

static int
ShowMain(int argc, const char** argv)
{
    if (argc > 1) {
        return CLI_INVALID_ARGS;
    }
    if (HexSpawn(0, HEX_CONFIG, "show_update_info", NULL) != 0) {
        CliPrintf("Could not retrieve update version information.");
        return CLI_UNEXPECTED_ERROR;
    }
    return CLI_SUCCESS;
}

static int
HistoryMain(int argc, const char** argv)
{
    if (argc  != 1) {
        return CLI_INVALID_ARGS;
    }
    if (HexSpawn(0, HEX_CONFIG, "get_update_history", NULL) != 0) {
        CliPrintf("Could not retrieve update history information.");
        return CLI_UNEXPECTED_ERROR;
    }
    return CLI_SUCCESS;
}

static int
SecurityListMain(int argc, const char** argv)
{
    if (argc  != 1) {
        return CLI_INVALID_ARGS;
    }
    if (HexSpawn(0, HEX_SDK, "update_security_list", NULL) != 0) {
        CliPrintf("Could not retrieve security update list.");
        return CLI_UNEXPECTED_ERROR;
    }
    return CLI_SUCCESS;
}

static int
SecurityUpdateMain(int argc, const char** argv)
{
    if (argc > 3 /* [0]="security_update", [1]=<[all|advisory|cve]>, [2]=<id>*/)
        return CLI_INVALID_ARGS;

    int index;
    std::string type = "all", id = "";

    if(CliMatchCmdHelper(argc, argv, 1, "echo 'all\nadvisory\ncve'", &index, &type) != CLI_SUCCESS) {
        CliPrintf("Unknown security update type. use 'all' by default");
        type = "all";
    }

    if (type != "all") {
        if (!CliReadInputStr(argc, argv, 2, "Specifiy security ID: ", &id) ||
            id.length() <= 0) {
            CliPrint("id is required");
            return CLI_INVALID_ARGS;
        }
    }

    HexSpawn(0, HEX_SDK, "update_security_update", type.c_str(), id.c_str(), NULL);

    return CLI_SUCCESS;
}

// This mode is not available in strict error state
CLI_MODE(CLI_TOP_MODE, "update", "Work with firmware.", !HexStrictIsErrorState() && !FirstTimeSetupRequired());

CLI_MODE_COMMAND("update", "list", ListMain, ItemCompletionMatcher,
    "List available updates on the inserted USB device or an update server.",
    "list [ USB | Server ]");

CLI_MODE_COMMAND("update", "update", InstallMain, ItemCompletionMatcher,
    "Install available updates on the inserted USB device or an update server.",
    "update [ USB | Server ]");

CLI_MODE_COMMAND("update", "show", ShowMain, 0,
    "Display version information for the installed update.",
    "show");

CLI_MODE_COMMAND("update", "view_history", HistoryMain, 0,
    "Display installation and rollback history for all updates.",
    "view_history");

CLI_MODE_COMMAND("update", "security_list", SecurityListMain, 0,
    "Display available security updates.",
    "security_list");

CLI_MODE_COMMAND("update", "security_update", SecurityUpdateMain, 0,
    "Install all or a specific advisory/cve security updates .",
    "security_update <[all|advisory|cve]> <id>");

