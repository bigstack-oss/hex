// HEX SDK

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <string>
#include <vector>
#include <fstream>

#include <hex/process.h>
#include <hex/strict.h>

#include <hex/cli_module.h>
#include <hex/cli_util.h>

static int
ReGenerateKeys(int argc, const char** argv)
{
    if (argc != 1) {
        return CLI_INVALID_ARGS;
    }

    int rc = HexSystem(0, "/usr/sbin/hex_config", "regenerate_ssh_keys", NULL);
    if (rc != 0) {
        CliPrintf("Failed to regenerated SSH keys.\n");
        return rc;
    }

    CliPrintf("SSH keys successfully regenerated.\n");
    return CLI_SUCCESS;
}

// This mode is not available in STRICT error state
CLI_MODE(CLI_TOP_MODE, "ssh", "Work with SSH keys.", !HexStrictIsErrorState() && !FirstTimeSetupRequired());

CLI_MODE_COMMAND("ssh", "regen_ssh_keys", ReGenerateKeys, 0, "Regenerate SSH keys.", "regen_ssh_keys");

