
#include <hex/config_module.h>

CONFIG_MODULE(foo, 0, 0, 0, 0, 0);
CONFIG_FIRST(foo);

// Invalid: already registered first
CONFIG_LAST(foo);

