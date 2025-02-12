// HEX SDK

#include <netinet/in.h>
#include <arpa/inet.h>

#include <hex/parse.h>
#include <hex/process.h>
#include <hex/process_util.h>
#include <hex/log.h>

#include <hex/cli_module.h>
#include <hex/cli_util.h>

static int
PxeRestoreMain(int argc, const char** argv)
{
    if (argc > 4) {
        return CLI_INVALID_ARGS;
    }

    int index, argidx = 1;
    struct in_addr v4addr;
    std::string ifname = "", device = "", server = "";
    std::string cmd, optCmd, descCmd;

    std::string busid = HexUtilPOpen(HEX_SDK " GetKernelCmdLine pxe_net_busid");
    if (busid.length()) {
        ifname = HexUtilPOpen(HEX_SDK " FindIfnameById %s", busid.c_str());
    }

    if (!ifname.length()) {
        HexSpawn(0, HEX_SDK, "-v", "DumpInterface", NULL);
        cmd = "/usr/sbin/ip link | awk -F'[: ]' '/eth.*mtu/{print $3}' | sort -V";
        if(CliMatchCmdHelper(argc, argv, argidx++, cmd, &index, &ifname, "Select an interface: ") != CLI_SUCCESS) {
            CliPrintf("interface name is missing or not found");
            return CLI_INVALID_ARGS;
        }
    }

    CliPrintf("Setting up network ...");
    if (HexSystemF(0, "/usr/sbin/ip link set %s up", ifname.c_str()) != 0) {
        CliPrintf("failed to bring up interface %s", ifname.c_str());
        return CLI_UNEXPECTED_ERROR;
    }
    if (HexSystemF(0, "/usr/sbin/dhclient %s -cf /etc/dhclient.conf 2>/dev/null", ifname.c_str()) != 0) {
        CliPrintf("failed to get ip from dhcp server");
        return CLI_UNEXPECTED_ERROR;
    }

    std::string url = HexUtilPOpen(HEX_SDK " GetKernelCmdLine pxe_cfg_url");
    if (url.length()) {
        if (HexSystem(0, HEX_SDK, "FetchPxeCfg", url.c_str(), NULL) == 0) {
            device = HexUtilPOpen(HEX_SDK " ReadPxeCfg INSTALL_DRIVE");
            server = HexUtilPOpen(HEX_SDK " ReadPxeCfg PKG_SERVER");
        }
    }

    if (!device.length()) {
        cmd = "/bin/lsblk -dn --sort name -o NAME,SIZE,MODEL,TYPE,TRAN | /bin/grep disk | /bin/grep -v usb | /usr/bin/awk ";
        optCmd = cmd + "'{print \"/dev/\"$1}'";
        descCmd = cmd + "'{ printf \"%-8s %-8s %s\\n\", $1, $2, $3 }'";

        if(CliMatchCmdDescHelper(argc, argv, argidx++, optCmd, descCmd, &index, &device, "Select install drive: ") != CLI_SUCCESS) {
            CliPrintf("device name is missing or not found");
            return CLI_INVALID_ARGS;
        }
    }

    if (!server.length()) {
        if (!CliReadInputStr(argc, argv, argidx++, "Enter image server (http://): ", &server) ||
            !HexParseIP(server.c_str(), AF_INET, &v4addr)) {
            CliPrintf("invalid IP address %s\n", server.c_str());
            return CLI_INVALID_ARGS;
        }
    }

    if (HexSpawn(0, "/usr/sbin/hex_pxe_install", device.c_str(), server.c_str(), NULL) != 0)
        return CLI_UNEXPECTED_ERROR;

    return CLI_SUCCESS;
}

CLI_MODE_COMMAND(CLI_TOP_MODE, "restore", PxeRestoreMain, 0,
    "Restore from PXE server.",
    "restore [<interface>] [<device>] [<server>]");

