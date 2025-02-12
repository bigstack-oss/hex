
#include <hex/config_module.h>

CONFIG_MODULE(foo, 0, 0, 0, 0, 0);
// Invalid: module not found
CONFIG_REQUIRES(bar, foo);

