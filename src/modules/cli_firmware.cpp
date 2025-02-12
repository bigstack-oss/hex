// HEX SDK

#include <hex/cli_module.h>
#include <hex/cli_util.h>
#include <hex/process.h>
#include <hex/process_util.h>
#include <hex/parse.h>
#include <hex/log.h>
#include <hex/tuning.h>
#include <hex/tempfile.h>

static int
GetList(CliList& list)
{
    if (CliPopulateList(list, HEX_SDK " firmware_list") != 0) {
        HexLogError("Could not list firmware");
        return CLI_UNEXPECTED_ERROR;
    }

    if (list.size() == 0)
        CliPrintf("No firmware information available");

    return CLI_SUCCESS;
}

static int
GetActive(int *index)
{
    std::string active = HexUtilPOpen(HEX_SDK " firmware_get_active");
    *index = std::stoi(active);
    return CLI_SUCCESS;
}

static int
GetCommentMain(int argc, const char** argv)
{
    if (argc > 2) {
        return CLI_INVALID_ARGS;
    }

    CliList list;
    int rc = GetList(list);
    if (rc != 0)
        return rc;

    int index;
    if (argc == 1) {
        index = CliReadListIndex(list);
        if (index < 0)
            return CLI_SUCCESS;
        ++index;
    }
    else {
        int64_t tmp;
        if (!HexParseInt(argv[1], 1, list.size(), &tmp)) {
            CliPrintf("Invalid index");
            return CLI_SUCCESS;
        }
        index = tmp;
    }

    if (HexSystemF(0, HEX_SDK " firmware_get_comment %d", index) != 0) {
        HexLogError("Could not get firmware comment");
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

    int index;
    std::string comment;
    if (argc == 1) {
        index = CliReadListIndex(list);
        if (index < 0)
            return CLI_SUCCESS;
        ++index;
    } else {
        // argc >= 2
        int64_t tmp;
        if (!HexParseInt(argv[1], 1, list.size(), &tmp)) {
            CliPrintf("Invalid index");
            return CLI_SUCCESS;
        }
        index = tmp;

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
    if (HexSystemF(0, HEX_SDK " firmware_set_comment %d %s", index, tmpfile.path()) == 0) {
        CliPrintf("Comment updated");
    } else {
        HexLogError("Could not set firmware comment");
        status = CLI_UNEXPECTED_ERROR;
    }

    return status;
}

static std::string
FormatTime(const char *value)
{
    // if value is not an integer string
    // just return the value, i.e. value "Never" returns "Never"
    if (!value || *value == '\0') {
        return value;
    }

    int saved_errno = errno;
    errno = 0;
    char *p = 0;
    time_t t = strtol(value, &p, 0);

    if (errno != 0 || *p != '\0') {
        return value;
    }
    // time should not be 0 (epoch)
    if (t < 1) {
        return value;
    }

    errno = saved_errno;
    struct tm *tm = localtime(&t);
    // Date/time in medium format
    char buf[sizeof("Mon DD, YYYY HH:MM:SS PM")+1];
    strftime(buf, sizeof(buf), "%b %e, %Y %r", tm);
    return std::string(buf);
}

static int
ParseInfo(int index, const std::string format = "pretty")
{
    char infopath[32];
    snprintf(infopath, sizeof(infopath), "/boot/grub2/info%d", index);

    FILE *fin = fopen(infopath, "r");
    if (!fin) {
        HexLogError("Could not read firmware info file");
        return CLI_UNEXPECTED_ERROR;
    }

    HexTuning_t tun = HexTuningAlloc(fin);
    if (!tun) {
        HexLogError("Could not allocate memory");
        return CLI_UNEXPECTED_ERROR;
    }

    int ret;
    const char *name, *value;
    std::string firmware_version;
    std::string install_date;
    std::string install_type;
    std::string backup_date;
    std::string last_boot;

    while ((ret = HexTuningParseLine(tun, &name, &value)) != HEX_TUNING_EOF) {
        if (ret != HEX_TUNING_SUCCESS) {
            // Malformed, exceeded buffer, etc.
            HexLogError("Could not read firmware info values");
            return CLI_UNEXPECTED_ERROR;
        }

        // display tuning params as text
        if (strcmp(name, "firmware_version") == 0) {
            firmware_version = value;
        }
        else if (strcmp(name, "install_date") == 0) {
            install_date = FormatTime(value);
        }
        else if (strcmp(name, "install_type") == 0) {
            if (strcmp(value, "PPU") == 0)
                install_type = "Upgrade";
            else if (strcmp(value, "ISO") == 0)
                install_type = "Re-install ISO";
            else if (strcmp(value, "USB") == 0)
                install_type = "Re-install USB";
            else //Factory, Backup, Disabled
                install_type = value;
        }
        else if (strcmp(name, "backup_date") == 0) {
            backup_date = FormatTime(value);
        }
        else if (strcmp(name, "last_boot") == 0) {
            last_boot = FormatTime(value);
        }
    }

    if (format == "pretty") {
        if (!firmware_version.empty())
            printf("Firmware Version: %s\n", firmware_version.c_str());
        if (!install_date.empty())
            printf("Installation Date: %s\n", install_date.c_str());
        if (!install_type.empty())
            printf("Installation Type: %s\n", install_type.c_str());
        if (!backup_date.empty())
            printf("Backup Date: %s\n", backup_date.c_str());
        if (!last_boot.empty())
            printf("Last Boot: %s\n", last_boot.c_str());
    }
    else {
        if (!firmware_version.empty())
            printf("firmware_version_%d=%s\n", index, firmware_version.c_str());
        if (!install_date.empty())
            printf("install_date_%d=%s\n", index, install_date.c_str());
        if (!install_type.empty())
            printf("install_type_%d=%s\n", index, install_type.c_str());
        if (!backup_date.empty())
            printf("backup_date_%d=%s\n", index, backup_date.c_str());
        if (!last_boot.empty())
            printf("last_boot_%d=%s\n", index, last_boot.c_str());
    }

    HexTuningRelease(tun);
    fclose(fin);

    return CLI_SUCCESS;
}

static int
GetInfoMain(int argc, const char** argv)
{
    if (argc > 2) {
        return CLI_INVALID_ARGS;
    }

    CliList list;
    int rc = GetList(list);
    if (rc != 0)
        return rc;

    int index;
    if (argc == 1) {
        index = CliReadListIndex(list);
        if (index < 0)
            return CLI_SUCCESS;
        ++index;
    } else {
        int64_t tmp;
        if (!HexParseInt(argv[1], 1, list.size(), &tmp)) {
            CliPrintf("Invalid index");
            return CLI_SUCCESS;
        }
        index = tmp;
    }

    return ParseInfo(index);
}

static int
ListMain(int argc, const char** argv)
{
    if (argc > 2) {
        return CLI_INVALID_ARGS;
    }

    int active = 1;
    int rc = GetActive(&active);
    if (rc != 0)
        return rc;

    CliList list;
    rc = GetList(list);
    if (rc != 0)
        return rc;

    if (argc == 1) {
        for (size_t i = 1; i <= list.size(); ++i) {
            CliPrintf("%zu: %s %s", i, list[i-1].c_str(), active == (int)i ? "[ACTIVE]":"");
            if (ParseInfo(i) != CLI_SUCCESS) {
                HexLogError("Could not parse firmware info");
                return CLI_UNEXPECTED_ERROR;
            }

            CliPrintf("Comment:");
            if (HexSystemF(0, HEX_SDK " firmware_get_comment %zu", i) != 0) {
                HexLogError("Could not get firmware comment");
                return CLI_UNEXPECTED_ERROR;
            }
        }
    }
    else if (argc == 2 && strcmp(argv[1], "shell") == 0) {
        CliPrintf("active=%d", active);
        for (size_t i = 1; i <= list.size(); ++i) {
            if (ParseInfo(i, "shell") != CLI_SUCCESS) {
                HexLogError("Could not parse firmware info");
                return CLI_UNEXPECTED_ERROR;
            }
        }
    }

    return CLI_SUCCESS;
}

static int
SwapActiveMain(int argc, const char** argv)
{
    if (argc > 1)
        return CLI_INVALID_ARGS;

    CliPrintf("Warning: This operation will require the appliance to reboot.");
    CliPrintf("Are you sure you want to swap the active firmware image?");

    int active = 0;
    GetActive(&active);
    if (active == 1)
        active = 2;
    else
        active = 1;

    if (!CliReadConfirmation())
        return CLI_SUCCESS;

    if (HexSpawn(0, HEX_SDK, "ceph_mon_store_renew", NULL) != 0) {
        HexLogError("Could not swap active firmware, failing to renew ceph mon store.db");
        return CLI_UNEXPECTED_ERROR;
    }

    if (HexSpawn(0, HEX_SDK, "firmware_swap_active", NULL) != 0) {
        HexLogError("Could not swap active firmware");
        return CLI_UNEXPECTED_ERROR;
    }

    char username[256];
    if (CliGetUserName(username, sizeof(username)) != 0)
        snprintf(username, sizeof(username), "Unknown");

    HexLogInfo("swap active partition to %d", active);
    // HexLogEvent("swap active partition to %d", active);
    return CLI_SUCCESS;
}

static int
BackupMain(int argc, const char** argv)
{
    if (argc > 1)
        return CLI_INVALID_ARGS;

    CliPrintf("Warning: This operation will require the appliance to reboot.");
    CliPrintf("Are you sure you want to back up the current partition to the other?");

    if (!CliReadConfirmation())
        return CLI_SUCCESS;

    if (HexSpawn(0, HEX_SDK, "firmware_backup", NULL) != 0) {
        HexLogError("Could not backup firmware");
        return CLI_UNEXPECTED_ERROR;
    }
    char username[256];
    if (CliGetUserName(username, sizeof(username)) != 0)
        snprintf(username, sizeof(username), "Unknown");

    // int active = 0;
    // GetActive(&active);
    // HexLogEvent("backup active partition to %d", active);
    return CLI_SUCCESS;
}

CLI_MODE(CLI_TOP_MODE, "firmware", "Work with firmware images.", !FirstTimeSetupRequired());

CLI_MODE_COMMAND("firmware", "list", ListMain, 0,
    "List information about installed firmware images. Firmware information includes the active firmware image, a description of the firmware, the date when firmware was installed, and optional backup information.",
    "list");

CLI_MODE_COMMAND("firmware", "get_comment", GetCommentMain, 0,
    "View the comment associated with a firmware image.",
    "get_comment <index>");

CLI_MODE_COMMAND("firmware", "set_comment", SetCommentMain, 0,
    "Replace the comment associated with a firmware image.",
    "set_comment <index> <comment>");

CLI_MODE_COMMAND("firmware", "get_info", GetInfoMain, 0,
    "View the version information associated with a firmware image.",
    "get_info <index>");

CLI_MODE_COMMAND("firmware", "swap_active", SwapActiveMain, 0,
    "Swap the active firmware image. The appliance restarts and boots with the inactive firmware image.",
    "swap_active");

CLI_MODE_COMMAND("firmware", "backup", BackupMain, 0,
    "Back up firmware on the active partition to the inactive partition.",
    "backup");

