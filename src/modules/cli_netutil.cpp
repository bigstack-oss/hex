// HEX SDK

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <vector>

#include <hex/process.h>
#include <hex/process_util.h>
#include <hex/parse.h>
#include <hex/strict.h>

#include <hex/cli_module.h>
#include <hex/cli_util.h>

static int
PingMain(int argc, const char **argv)
{
    std::vector<const char *> args;
    args.push_back("/bin/ping");

    // Must set to 0 to reset getopt!
    optind = 0;

    int opt;
    while ((opt = getopt(argc, (char *const *)argv, "6c:s:")) != -1) {
        switch (opt) {
        case '6':
            args.push_back("-6");
            break;
        case 'c':
            if (HexValidateInt(optarg, 0, 65535)) {
                args.push_back("-c");
                args.push_back(optarg);
            }
            else {
                printf("Count must be between 0 and 65535.\n");
                return CLI_INVALID_ARGS;
            }
            break;
        case 's':
            if (HexValidateInt(optarg, 0, 65507)) {
                args.push_back("-s");
                args.push_back(optarg);
            }
            else {
                printf("Size must be between 0 and 65507.\n");
                return CLI_INVALID_ARGS;
            }
            break;
        default:
            return CLI_INVALID_ARGS;
        }
    }

    if (optind == argc - 1) {
        // required host
        args.push_back(argv[optind]);
    }
    else {
        return CLI_INVALID_ARGS;
    }

    args.push_back(NULL);

    HexSpawnV(0, (char *const *)&args[0]);

    return CLI_SUCCESS;
}

static int
TracerouteMain(int argc, const char** argv)
{
    std::vector<const char *> args;
    args.push_back("/usr/bin/traceroute");

    // Must set to 0 to reset getopt!
    optind = 0;

    int opt;
    while ((opt = getopt(argc, (char *const *)argv, "6")) != -1) {
        switch (opt) {
        case '6':
            args.push_back("-6");
            break;
        default:
            return CLI_INVALID_ARGS;
        }
    }

    if (optind < argc) {
        // required host
        args.push_back(argv[optind]);
    }
    else {
        return CLI_INVALID_ARGS;
    }

    if (optind + 1 < argc) {
        // optional size
        if (HexValidateInt(argv[optind + 1], 46, 32768)) {
            args.push_back(argv[optind + 1]);
        }
        else {
            printf("Size must be between 46 and 32768.\n");
            return CLI_INVALID_ARGS;
        }
    }

    if (optind + 2 < argc)
        return CLI_INVALID_ARGS;

    args.push_back(NULL);

    HexSpawnV(0, (char *const *)&args[0]);
    return CLI_SUCCESS;
}

static int
NetstatMain(int argc, const char** argv)
{
    return HexUtilSystemWithPage("/bin/netstat -an") == 0 ? CLI_SUCCESS : CLI_FAILURE;
}

// This mode is not available in FIPS error state
CLI_MODE(CLI_TOP_MODE, "tools", "Work with diagnostic tools.", !HexStrictIsErrorState() && !FirstTimeSetupRequired());

CLI_MODE_COMMAND("tools", "ping", PingMain, 0,
    "Send an ICMP ECHO_REQUEST to network hosts.",
    "ping [ -6 ] [ -c <count> ] [ -s <size> ] <host>\n"
    "The count must be 0 - 65535. "
    "If the count is 0, then the system sends ICMP ECHO_REQUEST pings "
    "until it is interrupted by the user command CTRL+C. "
    "The default count is 0. "
    "The size must be 0 - 65507. The default size is 56 bytes.");

CLI_MODE_COMMAND("tools", "traceroute", TracerouteMain, 0,
    "Trace a packet from a computer to a remote destination, showing how many hops "
    "the packet required to reach the destination and how long each hop took.",
    "traceroute [ -6 ] <host> [ <size> ]\n"
    "Size must be 46-32768. Default size is 46 bytes.");

CLI_MODE_COMMAND("tools", "connections", NetstatMain, 0,
    "Display the network connections for the appliance.",
    "connections");

