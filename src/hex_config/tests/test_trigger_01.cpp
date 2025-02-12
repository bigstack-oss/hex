
#include <hex/config_module.h>

static int
Trigger(int argc, char **argv)
{
    return 0;
}

// Invalid: module not found
CONFIG_TRIGGER(foo, "pam_updated", Trigger);

