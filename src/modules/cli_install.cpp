// HEX SDK

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <unistd.h>

#include <hex/cli_module.h>
#include <hex/cli_util.h>
#include <hex/log.h>
#include <hex/process.h>
#include <hex/string_util.h>

// The data disks restore prepares, chosen with set_data_disks. Without this
// file restore prepares every data disk. The installer runs from a ramdisk,
// so a choice lasts for this boot only.
static const char DATA_DISKS_SETTINGS[] = "/etc/data_disks.sys";

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

static int
SetDataDisksMain(int argc, const char** argv)
{
    if (argc > 2) {
        return CLI_INVALID_ARGS;
    }

    CliList disks;
    if (CliPopulateList(disks, "/usr/sbin/hex_install_disks --paths") != 0) {
        return CLI_UNEXPECTED_ERROR;
    }

    std::string value;
    if (argc == 2) {
        value = argv[1];
    } else {
        CliList descs;
        if (CliPopulateList(descs, "/usr/sbin/hex_install_disks") != 0) {
            return CLI_UNEXPECTED_ERROR;
        }
        CliPrint("Disks restore could prepare, except the one it installs to:");
        for (auto& d : descs) {
            CliPrintf("  %s", d.c_str());
        }
        if (!CliReadLine("Enter all, none, or the disks to prepare separated by commas: ", value)) {
            return CLI_SUCCESS;
        }
    }
    hex_string_util::strip(value);

    if (value == "all") {
        if (unlink(DATA_DISKS_SETTINGS) != 0 && errno != ENOENT) {
            CliPrintf("Failed to clear the data disk selection.");
            return CLI_UNEXPECTED_ERROR;
        }
        CliPrint("restore prepares every data disk.");
        return CLI_SUCCESS;
    }

    std::string selected;
    if (value == "none") {
        selected = value;
    } else {
        for (std::string d : hex_string_util::split(value, ',')) {
            hex_string_util::strip(d);
            if (d.empty()) {
                continue;
            }
            if (d.compare(0, 5, "/dev/") != 0) {
                d = "/dev/" + d;
            }
            if (std::find(disks.begin(), disks.end(), d) == disks.end()) {
                CliPrintf("%s is not a disk restore could prepare.", d.c_str());
                return CLI_INVALID_ARGS;
            }
            if (!selected.empty()) {
                selected += ",";
            }
            selected += d;
        }
    }
    if (selected.empty()) {
        CliPrint("No disk given.");
        return CLI_INVALID_ARGS;
    }

    FILE* f = fopen(DATA_DISKS_SETTINGS, "w");
    if (f == NULL) {
        CliPrintf("Failed to write %s.", DATA_DISKS_SETTINGS);
        return CLI_UNEXPECTED_ERROR;
    }
    fprintf(f, "sys.install.data.disks=%s\n", selected.c_str());
    fclose(f);

    if (selected == "none") {
        CliPrint("restore prepares no data disk.");
    } else {
        CliPrintf("restore prepares %s.", selected.c_str());
    }
    return CLI_SUCCESS;
}

static int
ShowDataDisksMain(int argc, const char** argv)
{
    if (argc != 1) {
        return CLI_INVALID_ARGS;
    }

    FILE* f = fopen(DATA_DISKS_SETTINGS, "r");
    if (f == NULL) {
        CliPrint("restore prepares every data disk.");
        return CLI_SUCCESS;
    }

    char line[1024] = "";
    if (fgets(line, sizeof(line), f) == NULL) {
        line[0] = '\0';
    }
    fclose(f);

    std::string selected(line);
    const std::size_t eq = selected.find('=');
    selected = (eq == std::string::npos) ? "" : selected.substr(eq + 1);
    hex_string_util::strip(selected, "\t \n");

    if (selected == "none") {
        CliPrint("restore prepares no data disk.");
    } else {
        CliPrintf("restore prepares %s.", selected.c_str());
    }
    return CLI_SUCCESS;
}

CLI_MODE_COMMAND(
    CLI_TOP_MODE,
    "set_data_disks",
    SetDataDisksMain,
    0,
    "Set the data disks restore prepares.",
    "set_data_disks [all|none|<device>[,<device>...]]");

CLI_MODE_COMMAND(
    CLI_TOP_MODE,
    "show_data_disks",
    ShowDataDisksMain,
    0,
    "Show the data disks restore prepares.",
    "show_data_disks");
