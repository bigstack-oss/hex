// HEX SDK

#include <hex/config_module.h>
#include <hex/config_tuning.h>

CONFIG_TUNING_STR(SYS_VENDOR_NAME, "sys.vendor.name", TUNING_UNPUB, "Vendor defined product name", "", ValidateNone);
CONFIG_TUNING_STR(SYS_VENDOR_VERSION, "sys.vendor.version", TUNING_UNPUB, "Vendor defined product version", "", ValidateNone);

