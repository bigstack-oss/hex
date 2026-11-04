
#include <hex/config_module.h>

CONFIG_MODULE(test, 0, 0, 0, 0, 0);
CONFIG_MODULE(big, 0, 0, 0, 0, 0);
CONFIG_MIGRATE(test, "/etc/test5/serial");
CONFIG_MIGRATE(test, "/var/log");
CONFIG_MIGRATE_PARALLEL(big, "/etc/test5/big");
