
#include <hex/config_module.h>

CONFIG_MODULE(foo, 0, 0, 0, 0, 0);
// Should be able to wait on a module (e.g. bar) before its declared
CONFIG_REQUIRES(foo, bar);

CONFIG_MODULE(bar, 0, 0, 0, 0, 0);
