// HEX SDK

#include <unistd.h>

#include <hex/log.h>
#include <hex/process.h>

#include <hex/config_module.h>

static const char NEED_REBOOT[] = "/etc/appliance/state/appliance_need_reboot";

CONFIG_TUNING_STR(APPLIANCE_LOGIN_GREETING, "appliance.login.greeting", TUNING_PUB, "Set greeting message for login", "", ValidateRegex, DFT_REGEX_STR);

static void
ShutdownUsage(void)
{
    fprintf(stderr, "Usage: %s shutdown <delay_secs>\n", HexLogProgramName());
}

static int
ShutdownMain(int argc, char* argv[])
{
    int status = -1;
    if (argc > 2) {
        ShutdownUsage();
        return 1;
    }

    unlink(NEED_REBOOT);

    if (argc == 1) {
        HexLogNotice("System is halting");
        status = HexSystemF(0, HEX_SDK " appliance_shutdown");
    } else {
        HexLogNotice("System is halting in %s seconds", argv[1]);
        status = HexSystemF(0, HEX_SDK " appliance_shutdown %s", argv[1]);
    }

    // HexSystem() returns EXIT_SUCCESS, ...
    return status;
}

static void
RebootUsage(void)
{
    fprintf(stderr, "Usage: %s reboot [ <delay> ]\n", HexLogProgramName());
}

static int
RebootMain(int argc, char* argv[])
{
    int status = -1;
    if (argc > 2) {
        RebootUsage();
        return 1;
    }

    unlink(NEED_REBOOT);

    if (argc == 1) {
        HexLogNotice("System is rebooting");
        status = HexSystemF(0, HEX_SDK " appliance_reboot");
    } else {
        HexLogNotice("System is rebooting in %s seconds", argv[1]);
        status = HexSystemF(0, HEX_SDK " appliance_reboot %s", argv[1]);
    }

    // HexSystem() returns EXIT_SUCCESS, ...
    return status;
}

CONFIG_COMMAND(shutdown, ShutdownMain, ShutdownUsage);
CONFIG_COMMAND(reboot,   RebootMain,   RebootUsage);

CONFIG_MODULE(appliance, 0, 0, 0, 0, 0);
CONFIG_FIRST(appliance);

