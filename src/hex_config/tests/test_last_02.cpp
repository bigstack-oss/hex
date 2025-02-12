
#include <hex/test.h>
#include <hex/config_module.h>

// Commit order should be: sys first b last c a
// Alphabetic order of module names should not match commit order

static int s_order = 0;

static bool
Validate_B()
{
    HEX_TEST_FATAL(s_order++ == 0);
    return true;
}

CONFIG_MODULE(b, 0, 0, Validate_B, 0, 0);

static bool
Validate_C()
{
    HEX_TEST_FATAL(s_order++ == 1);
    return true;
}

CONFIG_MODULE(c, 0, 0, Validate_C, 0, 0);
CONFIG_LAST(c);

static bool
Validate_A()
{
    HEX_TEST_FATAL(s_order++ == 2);
    return true;
}

CONFIG_MODULE(a, 0, 0, Validate_A, 0, 0);
CONFIG_LAST(a);
CONFIG_REQUIRES(a, c);

