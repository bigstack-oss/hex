
#include <cstdlib>
#include <hex/config_module.h>

static int
Trigger(int argc, char **argv)
{
    system("/bin/touch test.bar_triggered");
    return 0;
}

CONFIG_MODULE(bar, 0, 0, 0, 0, 0);
CONFIG_TRIGGER(bar, "pam_updated", Trigger);

