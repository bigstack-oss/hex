
#include <hex/test.h>
#include <hex/config_module.h>

static int s_order;

static int
Validate_A(const char* dir1, const char* dir2)
{
    HEX_TEST_FATAL(s_order++ == 1);
    return 0;
}

CONFIG_SNAPSHOT_COMMAND(a, 0, Validate_A, 0);

CONFIG_SNAPSHOT_COMMAND_FIRST(a);

CONFIG_SNAPSHOT_COMMAND_LAST(a);


