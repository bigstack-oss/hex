
#include <cstdlib>
#include <hex/config_module.h>

static int
Trigger(int argc, char **argv)
{
    system("/bin/touch test.foo_triggered");
    return 0;
}

CONFIG_MODULE(foo, 0, 0, 0, 0, 0);
CONFIG_TRIGGER(foo, "pam_updated", Trigger);

