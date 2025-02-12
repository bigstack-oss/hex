
#include <hex/config_module.h>

CONFIG_MODULE(foo, 0, 0, 0, 0, 0);
CONFIG_LAST(foo);

// Invalid: already registered last
CONFIG_FIRST(foo);

