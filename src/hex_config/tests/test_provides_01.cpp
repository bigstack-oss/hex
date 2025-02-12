
#include <hex/config_module.h>

CONFIG_MODULE(foo, 0, 0, 0, 0, 0);

// Invalid: module 'bar' not found
CONFIG_PROVIDES(bar, EVENT);
