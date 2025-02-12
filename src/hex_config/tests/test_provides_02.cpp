
#include <hex/config_module.h>

CONFIG_MODULE(foo, 0, 0, 0, 0, 0);

// Invalid: event name must be uppercase
CONFIG_PROVIDES(foo, event);
