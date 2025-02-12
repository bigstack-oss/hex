
#include <hex/config_module.h>

CONFIG_MODULE(foo, 0, 0, 0, 0, 0);
CONFIG_MODULE(bar, 0, 0, 0, 0, 0);

// Invalid: functions cannot be null
CONFIG_OBSERVES(foo, bar, 0, 0);
