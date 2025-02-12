// HEX SDK

#include <hex/process.h>
#include <hex/log.h>

#include <hex/cli_module.h>
#include <hex/cli_util.h>

static int
RestoreMain(int argc, const char** argv)
{
    if (argc != 1) {
        return CLI_INVALID_ARGS;
    }
    else {
        CliList list;

        if (CliPopulateList(list, "/usr/sbin/hex_install list") != 0)
            return CLI_UNEXPECTED_ERROR;

        if (list.size() != 1) {
            HexLogError("restore: expected 1 image, found %zd", list.size());
            return CLI_UNEXPECTED_ERROR;
        }

        CliList devices;
        CliList descriptions;
        std::string device;
        int index;

        std::string cmd = "/bin/lsblk -dn --sort name -o NAME,SIZE,MODEL,TYPE,TRAN | /bin/grep disk | /bin/egrep -v 'disk usb|disk fc' | /usr/bin/awk ";
        std::string optCmd = cmd + "'{print \"/dev/\"$1}'";
        std::string descCmd = cmd + "'{ printf \"%-8s %-8s %s\\n\", $1, $2, $3 }'";

        if(CliMatchCmdDescHelper(argc, argv, 1, optCmd, descCmd, &index, &device) != CLI_SUCCESS) {
            CliPrintf("device name is missing or not found");
            return CLI_INVALID_ARGS;
        }

        HexSystemF(0, "echo \"sys.install.hdd=%s\" > /etc/extra_settings.sys", device.c_str());

	CliPrintf("Restoring %s on %s", list[0].c_str(), device.c_str());

        if (!CliReadConfirmation())
            return CLI_SUCCESS;

        CliPrintf("Starting restore...");

        if (HexSpawn(0, "/usr/sbin/hex_install", "-v", "-w", "-t", "/etc/extra_settings.sys", "restore", list[0].c_str(), NULL) != 0)
            return CLI_UNEXPECTED_ERROR;
    }
    return CLI_SUCCESS;
}

CLI_MODE_COMMAND(CLI_TOP_MODE, "restore", RestoreMain, 0,
    "Restore a firmware image.",
    "restore [<device>]");

