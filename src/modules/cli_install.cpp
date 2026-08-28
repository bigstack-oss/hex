// HEX SDK

#include <hex/cli_module.h>
#include <hex/cli_util.h>
#include <hex/log.h>
#include <hex/process.h>

static int
RestoreMain(int argc, const char** argv)
{
    if (argc != 1) {
        return CLI_INVALID_ARGS;
    }

    CliList list;
    if (CliPopulateList(list, "/usr/sbin/hex_install list") != 0) {
        return CLI_UNEXPECTED_ERROR;
    }

    if (list.size() != 1) {
        HexLogError("restore: expected 1 image, found %zd", list.size());
        return CLI_UNEXPECTED_ERROR;
    }

    CliList devices;
    CliList descriptions;
    std::string device;
    int index;

    // Shared eligibility rule (hex_install_disks) so the disks an operator can
    // restore to are exactly the ones zero-touch autoinstall will accept.
    std::string optCmd = "/usr/sbin/hex_install_disks --paths";
    std::string descCmd = "/usr/sbin/hex_install_disks";

    if (CliMatchCmdDescHelper(
            argc,
            argv,
            1,
            optCmd,
            descCmd,
            &index,
            &device)
        != CLI_SUCCESS) {
        CliPrintf("device name is missing or not found");
        return CLI_INVALID_ARGS;
    }

    HexSystemF(
        0,
        "echo \"sys.install.hdd=%s\" > /etc/extra_settings.sys",
        device.c_str());

    CliPrintf("Restoring %s on %s", list[0].c_str(), device.c_str());

    if (!CliReadConfirmation()) {
        return CLI_SUCCESS;
    }

    CliPrintf("Starting restore...");

    if (HexSpawn(
            0,
            "/usr/sbin/hex_install",
            "-v",
            "-w",
            "-t",
            "/etc/extra_settings.sys",
            "restore",
            list[0].c_str(),
            NULL)
        != 0) {
        return CLI_UNEXPECTED_ERROR;
    }

    return CLI_SUCCESS;
}

CLI_MODE_COMMAND(
    CLI_TOP_MODE,
    "restore",
    RestoreMain,
    0,
    "Restore a firmware image.",
    "restore [<device>]");
