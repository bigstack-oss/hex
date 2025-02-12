// HEX SDK

#include <hex/log.h>
#include <hex/process.h>

#include <hex/config_module.h>

CONFIG_SUPPORT_FILE("/etc/version");

CONFIG_SUPPORT_FILE("/etc/settings.*");
CONFIG_SUPPORT_FILE("/etc/policies");
CONFIG_SUPPORT_FILE("/etc/passwd");
CONFIG_SUPPORT_FILE("/etc/default_policies");
CONFIG_SUPPORT_FILE("/etc/failed_policies");
CONFIG_SUPPORT_FILE("/etc/appliance/state");
CONFIG_SUPPORT_FILE("/etc/resolv.conf");
// inlcude whole /var/log dir could be very overwhelming,
// leave it for project for granularly control
//CONFIG_SUPPORT_FILE("/var/log");
CONFIG_SUPPORT_FILE("/var/run/*.pid");
CONFIG_SUPPORT_FILE("/var/run/*.cmdline");
CONFIG_SUPPORT_FILE("/var/run/*.lock");

CONFIG_SUPPORT_COMMAND("date");
CONFIG_SUPPORT_COMMAND("uptime");
CONFIG_SUPPORT_COMMAND("systemctl list-dependencies");
CONFIG_SUPPORT_COMMAND("systemctl list-units | egrep -e 'active exited'");
CONFIG_SUPPORT_COMMAND("systemctl list-units | egrep -e 'active running'");
CONFIG_SUPPORT_COMMAND("top -b -n 1");
CONFIG_SUPPORT_COMMAND("pstree");
CONFIG_SUPPORT_COMMAND("free");
CONFIG_SUPPORT_COMMAND("df");
CONFIG_SUPPORT_COMMAND_TO_FILE("dmesg", "/tmp/dmesg.txt");
CONFIG_SUPPORT_COMMAND_TO_FILE("journalctl | grep hex_config", "/tmp/hex_config.log");
CONFIG_SUPPORT_COMMAND_TO_FILE("netstat -na", "/tmp/netstat.txt");
CONFIG_SUPPORT_COMMAND_TO_FILE("ps aux", "/tmp/ps.txt");
CONFIG_SUPPORT_COMMAND_TO_FILE("systemctl status", "/tmp/systemd_status.txt");
CONFIG_SUPPORT_COMMAND("ifconfig -a");
CONFIG_SUPPORT_COMMAND("cat /proc/interrupts");
CONFIG_SUPPORT_COMMAND("dmidecode");

// Move (not copy) core/map files into support info bundle
CONFIG_SUPPORT_COMMAND("mkdir -p $HEX_SUPPORT_DIR/var/support; mv -f /var/support/vmcore /var/support/core_* /var/support/crash_* /var/support/crashmap_* $HEX_SUPPORT_DIR/var/support >/dev/null 2>&1");

// Preserve log and support files across firmware update
CONFIG_MODULE(supportbase, 0, 0, 0, 0, 0);
CONFIG_MIGRATE(supportbase, "/var/log");

static void
RootShellEnabledCheckUsage(void)
{
    fprintf(stderr, "Usage: %s root_shell_check\n", HexLogProgramName());
}

static int
RootShellEnabledCheckMain(int argc, char* argv[])
{
    if (argc != 1) {
        RootShellEnabledCheckUsage();
        return 1;
    }

    // Root shell is disabled when first character of password field in /etc/shadow is an exclamation mark (!)
    printf("root shell enabled = %s", (HexSystemF(0, "grep '^root:!' /etc/shadow >/dev/null 2>&1") == 0) ? "no" : "yes");
    return EXIT_SUCCESS;
}

// Check for root shell enabled
CONFIG_SUPPORT_COMMAND("hex_config root_shell_check");
CONFIG_COMMAND(root_shell_check, RootShellEnabledCheckMain, RootShellEnabledCheckUsage);

