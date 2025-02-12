
#include <hex/config_module.h>

static bool 
Parse(const char *name, const char *value, bool isNew)
{
    return true;
}

CONFIG_MODULE(foo, 0, 0, 0, 0, 0);
CONFIG_OBSERVES(foo, bar, Parse, 0);
