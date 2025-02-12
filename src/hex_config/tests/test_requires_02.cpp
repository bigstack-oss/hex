
#include <hex/config_module.h>

CONFIG_MODULE(foo, 0, 0, 0, 0, 0);
CONFIG_REQUIRES(foo, bar);

CONFIG_MODULE(bar, 0, 0, 0, 0, 0);
// Invalid: circular dependency
CONFIG_REQUIRES(bar, foo);

