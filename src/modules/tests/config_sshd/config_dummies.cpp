// HEX SDK

#include <hex/config_module.h>

CONFIG_TUNING_STR(NET_DEFAULT_INTERFACE, "net.default_interface", false, "Set which interface's gateway settings should be used as the default gateway.", "", ValidateNone);

CONFIG_MODULE(net, NULL, NULL, NULL, NULL, NULL);
CONFIG_MODULE(net_static, NULL, NULL, NULL, NULL, NULL);
