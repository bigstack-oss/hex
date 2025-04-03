
#include <hex/config_module.h>

CONFIG_TUNING_STR(SYS_VENDOR_NAME, "sys.vendor.name", TUNING_UNPUB, "Vendor defined product name", "(Read-only)", ValidateRegex, DFT_REGEX_STR);
CONFIG_TUNING_STR(SYS_VENDOR_VERSION, "sys.vendor.version", TUNING_UNPUB, "Vendor defined product version", "(Read-only)", ValidateRegex, DFT_REGEX_STR);

CONFIG_SUPPORT_FILE("/var/log/messages");
CONFIG_SUPPORT_FILE("/etc/policies/*.yml");
CONFIG_SUPPORT_COMMAND("cat /proc/cpuinfo");
