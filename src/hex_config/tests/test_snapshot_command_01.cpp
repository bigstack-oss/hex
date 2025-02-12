
#include <hex/test.h>
#include <hex/config_module.h>

// execute order should be: c a b


static int s_order = 0;

static int
Validate_A(const char* dir1, const char* dir2)
{
    HEX_TEST_FATAL(s_order++ == 1);
    return 0;
}

CONFIG_SNAPSHOT_COMMAND(a, 0, Validate_A, 0);

static int
Validate_B(const char* dir1, const char* dir2)
{
    HEX_TEST_FATAL(s_order++ == 2);
    return 0;
}

CONFIG_SNAPSHOT_COMMAND(b, 0, Validate_B, 0);
CONFIG_SNAPSHOT_COMMAND_LAST(b);

static int
Validate_C(const char* dir1, const char* dir2)
{
    HEX_TEST_FATAL(s_order++ == 0);
    return 0;
}

CONFIG_SNAPSHOT_COMMAND(c, 0, Validate_C, 0);
CONFIG_SNAPSHOT_COMMAND_FIRST(c);


