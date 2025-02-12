// HEX SDK

#include <hex/process.h>
#include <hex/log.h>

#include <hex/cli_module.h>
#include <hex/cli_util.h>

static int
ShutdownMain(int argc, const char** argv)
{
    if (argc == 1) {
        if (!CliReadConfirmation())
            return CLI_SUCCESS;

        char username[256];
        CliGetUserName(username, sizeof(username));
        //TODO: HexLogEvent("system shutdown by [user] via [CLI]");
        HexSpawn(0, "/usr/sbin/hex_config", "shutdown", NULL);
        return CLI_SUCCESS;
    }

    return CLI_INVALID_ARGS;
}

static int
RebootMain(int argc, const char** argv)
{
    if (argc == 1) {
        if (!CliReadConfirmation())
            return CLI_SUCCESS;

        char username[256];
        CliGetUserName(username, sizeof(username));
        //TODO: HexLogEvent("system reboot by [user] via [CLI]");
        HexSpawn(0, "/usr/sbin/hex_config", "reboot", NULL);
        return CLI_SUCCESS;
    }

    return CLI_INVALID_ARGS;
}

CLI_GLOBAL_COMMAND("shutdown", ShutdownMain, 0,
    "End system operation and turn off the power.",
    "shutdown");

CLI_GLOBAL_COMMAND("reboot", RebootMain, 0,
    "Reboot the appliance.",
    "reboot");

